#include "MagiUnrealAXI.h"

#include "Common/TcpSocketBuilder.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/ARFilter.h"
#include "Dom/JsonObject.h"
#include "Engine/DataAsset.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputKeyEventArgs.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Kismet2/CompilerResultsLog.h"
#include "EdGraphToken.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_CustomEvent.h"
#include "EdGraph/EdGraphNode.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Logging/TokenizedMessage.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "FileHelpers.h"
#include "Containers/Ticker.h"
#include "EngineUtils.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IMainFrameModule.h"
#include "Editor.h"
#include "AssetCompilingManager.h"
#include "EditorBuildUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "Logging/LogMacros.h"
#include "Misc/EngineVersion.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/SavePackage.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ImageUtils.h"
#include "Engine/Engine.h"
#include "InputCoreTypes.h"
#include "Serialization/JsonWriter.h"
#include "Editor/UnrealEdEngine.h"
//
#include "UnrealEdGlobals.h"
#include "GameFramework/PlayerInput.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "UObject/Package.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectGlobals.h"
#include "MagiAxiCatalog.generated.h"
#include "Engine/Level.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#endif

#include <CommonCrypto/CommonDigest.h>
#include "Engine/BlueprintGeneratedClass.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <fcntl.h>
#include <libproc.h>
#include <mutex>
#include <limits.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>
bool IsGeneratedNativeCapability(const FString& Operation)
{
#define MAGI_AXI_MATCH_NATIVE(Name) if (Operation == Name) return true;
    MAGI_AXI_NATIVE_CAPABILITIES(MAGI_AXI_MATCH_NATIVE)
#undef MAGI_AXI_MATCH_NATIVE
    return false;
}

bool IsGeneratedPublicOperation(const FString& Operation)
{
#define MAGI_AXI_MATCH_PUBLIC(Name) if (Operation == Name) return true;
    MAGI_AXI_PUBLIC_OPERATIONS(MAGI_AXI_MATCH_PUBLIC)
#undef MAGI_AXI_MATCH_PUBLIC
    return false;
}

TArray<TSharedPtr<FJsonValue>> GeneratedPublicOperations()
{
    TArray<TSharedPtr<FJsonValue>> Operations;
#define MAGI_AXI_ADD_PUBLIC(Name) Operations.Add(MakeShared<FJsonValueString>(Name));
    MAGI_AXI_PUBLIC_OPERATIONS(MAGI_AXI_ADD_PUBLIC)
#undef MAGI_AXI_ADD_PUBLIC
    return Operations;
}

IMPLEMENT_MODULE(FMagiUnrealAXIModule, MagiUnrealAXI)

namespace
{
constexpr uint32 ProtocolVersion = 1;
constexpr uint32 RequestLimit = 8 * 1024 * 1024;
constexpr uint32 ResponseLimit = 16 * 1024 * 1024;
constexpr double HandshakeSeconds = 2.0;
constexpr double RequestSeconds = 30.0;

FSocket* Listener = nullptr;
TArray<FSocket*> ActiveClients;
FCriticalSection StateMutex;
std::thread Worker;
struct FConnectionWorker
{
    std::thread Thread;
    TSharedPtr<std::atomic<bool>> Done;
};
std::vector<FConnectionWorker> ConnectionWorkers;
std::mutex ConnectionMutex;
std::atomic<bool> Running{false};
enum class ELifecycle : uint8 { Starting, Ready, Stopping };
std::atomic<ELifecycle> Lifecycle{ELifecycle::Starting};
std::atomic<bool> CloseRequested{false};
FTSTicker::FDelegateHandle GameThreadTicker;
FDelegateHandle EditorInitializedHandle;
FString RuntimeDirectory;
FString ProjectPath;
FString ProjectId;
FString Token;
FString SessionNonce;
constexpr int32 MaxConnections = 72;
FString PlaySessionId;
uint64 PlaySessionCounter = 0;
enum class EPlayState : uint8 { Stopped, Starting, Running, Stopping };
EPlayState PlayState = EPlayState::Stopped;
constexpr int32 MaxFailedHandshakes = 64;
constexpr double FailedHandshakeWindowSeconds = 10.0;
std::mutex AuthenticationMutex;
double FailedHandshakeWindowStart = 0.0;
int32 FailedHandshakes = 0;

struct FGameThreadRequest
{
    FString Id;
    FString Operation;
    TSharedPtr<FJsonObject> Args;
    FString ExpectedRevision;
    FString IdempotencyKey;
    FString Fingerprint;

    FString Response;
    bool MayStop = false;
    bool Done = false;
    bool Cancelled = false;
    bool Dispatched = false;
    bool EverDispatched = false;
    bool DeferredCompletion = false;
    double Deadline = 0.0;
    std::mutex Mutex;
    std::condition_variable Condition;
};
TArray<TSharedRef<FGameThreadRequest>> GameThreadQueue;
constexpr int32 MaxGameThreadQueue = 64;

FString Sha256(const FString& Text);
bool ResponseStatusIsOk(const FString& Response);
FString LevelContentRevision(const UWorld& World);
FString LevelSettingsRevision(const UWorld& World);
UWorld* PieWorld();
TSharedRef<FJsonObject> PlayStatusResult();
TSharedRef<FJsonObject> ObservePlayResult();
TSharedRef<FJsonObject> LevelSettingsResult(const UWorld& World);

FString BlueprintStatus(const UBlueprint& Blueprint);
TSharedRef<FJsonObject> BuildReceiptMetadata(const FString& Id, const FString& Operation, const TSharedRef<FJsonObject>& Result, const TSharedRef<FJsonObject>& Verification, const FString& Target);
FString Serialize(const TSharedRef<FJsonObject>& Object);
struct FLedgerRecord
{
    FString OperationId;
    FString Operation;
    FString Fingerprint;
    FString IntentKey;
    FString Response;
    FString State = TEXT("queued");
    double CreatedAt = 0.0;
};
FCriticalSection LedgerMutex;
TArray<FLedgerRecord> Ledger;
constexpr int32 MaxLedgerRecords = 1024;
constexpr double LedgerTtlSeconds = 24.0 * 60.0 * 60.0;
bool IsMutationOperation(const FString& Operation)
{
    return Operation == TEXT("actor.spawn") || Operation == TEXT("actor.update_transform") || Operation == TEXT("actor.delete") || Operation == TEXT("component.add") || Operation == TEXT("component.update") || Operation == TEXT("component.remove") || Operation == TEXT("level.create") || Operation == TEXT("level.open") || Operation == TEXT("level.save") || Operation == TEXT("level.set_game_mode") || Operation == TEXT("asset.create_input_action") || Operation == TEXT("asset.create_input_mapping_context") || Operation == TEXT("asset.save") || Operation == TEXT("blueprint.compile") || Operation == TEXT("play.start") || Operation == TEXT("play.input") || Operation == TEXT("play.screenshot") || Operation == TEXT("play.stop");
}
bool IsPlayOperation(const FString& Operation)
{
    return Operation.StartsWith(TEXT("play."));
}

void PruneLedger()
{
    const double Now = FPlatformTime::Seconds();
    for (int32 Index = Ledger.Num() - 1; Index >= 0; --Index)
        if (Now - Ledger[Index].CreatedAt > LedgerTtlSeconds) Ledger.RemoveAt(Index);
    while (Ledger.Num() > MaxLedgerRecords) Ledger.RemoveAt(0);
}

FLedgerRecord* FindLedger(const FString& Id)
{
    for (FLedgerRecord& Record : Ledger) if (Record.OperationId == Id) return &Record;
    return nullptr;
}

void SetReceipt(const FString& Id, const FString& Operation, const FString& Response, const FString& State)
{
    FScopeLock Lock(&LedgerMutex);
    PruneLedger();
    FLedgerRecord* Record = FindLedger(Id);
    if (!Record)
    {
        FLedgerRecord NewRecord;
        NewRecord.OperationId = Id;
        NewRecord.Operation = Operation;
        NewRecord.CreatedAt = FPlatformTime::Seconds();
        Ledger.Add(MoveTemp(NewRecord));
        Record = &Ledger.Last();
    }
    Record->Response = Response;
    Record->State = State;
}

FString MakeQueuedReceipt(const FLedgerRecord& Record)
{
    const TSharedRef<FJsonObject> Receipt = MakeShared<FJsonObject>();
    Receipt->SetStringField(TEXT("operationId"), Record.OperationId);
    Receipt->SetStringField(TEXT("operation"), Record.Operation);
    Receipt->SetStringField(TEXT("state"), Record.State);
    Receipt->SetStringField(TEXT("projectId"), ProjectId);
    Receipt->SetNumberField(TEXT("editorPid"), FPlatformProcess::GetCurrentProcessId());
    Receipt->SetStringField(TEXT("target"), Record.Operation);
    Receipt->SetBoolField(TEXT("changed"), false);
    Receipt->SetStringField(TEXT("transaction"), TEXT("none"));
    Receipt->SetStringField(TEXT("reversibility"), TEXT("unknown"));
    Receipt->SetArrayField(TEXT("dirtyPackages"), {});
    Receipt->SetArrayField(TEXT("savedPackages"), {});
    Receipt->SetStringField(TEXT("revision"), Sha256(Record.OperationId));
    Receipt->SetStringField(TEXT("persistence"), TEXT("unknown"));
    const TSharedRef<FJsonObject> Verification = MakeShared<FJsonObject>();
    Verification->SetStringField(TEXT("readback"), TEXT("operation.view"));
    Verification->SetStringField(TEXT("target"), Record.Operation);
    Verification->SetBoolField(TEXT("matched"), false);
    Receipt->SetObjectField(TEXT("verification"), Verification);
    return Serialize(Receipt);
}

FString Utf8Path(const FString& Path)
{
    FString Result = FPaths::ConvertRelativePathToFull(Path);
    FPaths::NormalizeFilename(Result);
    return Result;
}

bool RandomHex(const int32 ByteCount, FString& Out)
{
    const int File = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (File < 0) return false;
    TArray<uint8> Bytes;
    Bytes.SetNumUninitialized(ByteCount);
    int32 Offset = 0;
    while (Offset < ByteCount)
    {
        const ssize_t Count = read(File, Bytes.GetData() + Offset, ByteCount - Offset);
        if (Count <= 0) { close(File); return false; }
        Offset += static_cast<int32>(Count);
    }
    close(File);
    Out.Reset(ByteCount * 2);
    static constexpr TCHAR Hex[] = TEXT("0123456789abcdef");
    for (const uint8 Byte : Bytes)
    {
        Out.AppendChar(Hex[Byte >> 4]);
        Out.AppendChar(Hex[Byte & 0x0f]);
    }
    return true;
}

FString Sha256(const FString& Text)
{
    FTCHARToUTF8 Bytes(*Text);
    uint8 Signature[CC_SHA256_DIGEST_LENGTH]{};
    CC_SHA256(Bytes.Get(), static_cast<CC_LONG>(Bytes.Length()), Signature);
    FString Result;
    static constexpr TCHAR Hex[] = TEXT("0123456789abcdef");
    for (const uint8 Byte : Signature)
    {
        Result.AppendChar(Hex[Byte >> 4]);
        Result.AppendChar(Hex[Byte & 0x0f]);
    }
    return Result;
}

FString ProcessStartIdentity(const uint32 Pid)
{
    proc_bsdinfo Info{};
    const int32 Size = sizeof(Info);
    if (proc_pidinfo(static_cast<int32>(Pid), PROC_PIDTBSDINFO, 0, &Info, Size) != Size || Info.pbi_pid != Pid) return FString();
    return FString::Printf(TEXT("%llu:%llu"), Info.pbi_start_tvsec, Info.pbi_start_tvusec);
}

bool SecureDirectory(const FString& Path)
{
    IFileManager::Get().MakeDirectory(*Path, true);
    const FTCHARToUTF8 Native(*Path);
    struct stat Info{};
    if (lstat(Native.Get(), &Info) != 0 || !S_ISDIR(Info.st_mode) || S_ISLNK(Info.st_mode) || Info.st_uid != geteuid()) return false;
    if (chmod(Native.Get(), 0700) != 0) return false;
    return lstat(Native.Get(), &Info) == 0 && (Info.st_mode & 0777) == 0700;
}

bool AtomicPrivateWrite(const FString& Path, const FString& Contents)
{
    FString Suffix;
    if (!RandomHex(8, Suffix)) return false;
    const FString Temporary = Path + TEXT(".") + Suffix + TEXT(".tmp");
    const FTCHARToUTF8 NativeTemporary(*Temporary);
    const FTCHARToUTF8 NativePath(*Path);
    const FTCHARToUTF8 Bytes(*Contents);
    const int File = open(NativeTemporary.Get(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
    if (File < 0) return false;
    int32 Offset = 0;
    while (Offset < Bytes.Length())
    {
        const ssize_t Count = write(File, Bytes.Get() + Offset, Bytes.Length() - Offset);
        if (Count <= 0) { close(File); unlink(NativeTemporary.Get()); return false; }
        Offset += static_cast<int32>(Count);
    }
    const bool Flushed = fsync(File) == 0;
    const bool Closed = close(File) == 0;
    if (!Flushed || !Closed || rename(NativeTemporary.Get(), NativePath.Get()) != 0)
    {
        unlink(NativeTemporary.Get());
        return false;
    }
    return chmod(NativePath.Get(), 0600) == 0;
}

FString Serialize(const TSharedRef<FJsonObject>& Object)
{
    FString Text;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Text);
    return FJsonSerializer::Serialize(Object, Writer) ? Text : FString();
}

bool ParseObject(const TArray<uint8>& Bytes, TSharedPtr<FJsonObject>& Out)
{
    if (Bytes.IsEmpty() || Bytes.Contains(0)) return false;
    FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Bytes.GetData()), Bytes.Num());
    const FString Text(Converted.Length(), Converted.Get());
    FTCHARToUTF8 RoundTrip(*Text);
    if (RoundTrip.Length() != Bytes.Num() || FMemory::Memcmp(RoundTrip.Get(), Bytes.GetData(), Bytes.Num()) != 0) return false;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
    return FJsonSerializer::Deserialize(Reader, Out) && Out.IsValid();
}
bool ResponseStatusIsOk(const FString& Response)
{
    TArray<uint8> Bytes;
    FTCHARToUTF8 Utf8(*Response);
    Bytes.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
    TSharedPtr<FJsonObject> Object;
    FString Status;
    return ParseObject(Bytes, Object) && Object->TryGetStringField(TEXT("status"), Status) && Status == TEXT("ok");
}

bool IntegerField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Name, int64& Out)
{
    double Number = 0;
    constexpr double MaxExactJsonInteger = 9007199254740991.0;
    if (!Object->TryGetNumberField(Name, Number) || !FMath::IsFinite(Number) || Number < -MaxExactJsonInteger || Number > MaxExactJsonInteger || FMath::FloorToDouble(Number) != Number) return false;
    Out = static_cast<int64>(Number);
    return true;
}

bool StringField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Name, FString& Out)
{
    return Object->TryGetStringField(Name, Out) && !Out.IsEmpty();
}

bool ConstantTimeEqual(const FString& Left, const FString& Right)
{
    const int32 Length = FMath::Max(Left.Len(), Right.Len());
    uint32 Difference = static_cast<uint32>(Left.Len() ^ Right.Len());
    for (int32 Index = 0; Index < Length; ++Index)
    {
        Difference |= static_cast<uint32>((Index < Left.Len() ? Left[Index] : 0) ^ (Index < Right.Len() ? Right[Index] : 0));
    }
    return Difference == 0;
}

bool DecodeFrameSize(const uint8 Header[4], uint32& OutSize)
{
    OutSize = uint32(Header[0]) | (uint32(Header[1]) << 8) | (uint32(Header[2]) << 16) | (uint32(Header[3]) << 24);
    return OutSize > 0 && OutSize <= RequestLimit;
}
bool ReceiveExact(FSocket* Socket, uint8* Data, const uint32 Size, const double Deadline)
{
    uint32 Offset = 0;
    while (Running && Offset < Size && FPlatformTime::Seconds() < Deadline)
    {
        if (!Socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromMilliseconds(50))) continue;
        int32 Count = 0;
        if (!Socket->Recv(Data + Offset, Size - Offset, Count, ESocketReceiveFlags::None) || Count <= 0) return false;
        Offset += static_cast<uint32>(Count);
    }
    return Offset == Size;
}

bool ReceiveFrame(FSocket* Socket, TArray<uint8>& Out, const double Timeout)
{
    const double Deadline = FPlatformTime::Seconds() + Timeout;
    uint8 Header[4]{};
    if (!ReceiveExact(Socket, Header, 4, Deadline)) return false;
    uint32 Size = 0;
    if (!DecodeFrameSize(Header, Size)) return false;
    Out.SetNumUninitialized(Size);
    return ReceiveExact(Socket, Out.GetData(), Size, Deadline);
}

bool SendExact(FSocket* Socket, const uint8* Data, const uint32 Size, const double Deadline)
{
    uint32 Offset = 0;
    while (Running && Offset < Size && FPlatformTime::Seconds() < Deadline)
    {
        if (!Socket->Wait(ESocketWaitConditions::WaitForWrite, FTimespan::FromMilliseconds(50))) continue;
        int32 Count = 0;
        if (!Socket->Send(Data + Offset, Size - Offset, Count) || Count <= 0) return false;
        Offset += static_cast<uint32>(Count);
    }
    return Offset == Size;
}

bool SendFrame(FSocket* Socket, const FString& Text, const double Deadline)
{
    FTCHARToUTF8 Bytes(*Text);
    const uint32 Size = Bytes.Length();
    if (Size == 0 || Size > ResponseLimit) return false;
    const uint8 Header[4] = {uint8(Size), uint8(Size >> 8), uint8(Size >> 16), uint8(Size >> 24)};
    return SendExact(Socket, Header, 4, Deadline) && SendExact(Socket, reinterpret_cast<const uint8*>(Bytes.Get()), Size, Deadline);
}

bool SendFrame(FSocket* Socket, const FString& Text)
{
    return SendFrame(Socket, Text, FPlatformTime::Seconds() + 2.0);
}

bool ConnectionOpen(FSocket* Socket)
{
    if (Socket->GetConnectionState() != ESocketConnectionState::SCS_Connected) return false;
    if (!Socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::Zero())) return true;
    uint8 Byte = 0;
    int32 Count = 0;
    return Socket->Recv(&Byte, 1, Count, ESocketReceiveFlags::Peek) && Count > 0;
}

FString ErrorResponse(const FString& Id, const TCHAR* Type, const TCHAR* Message, const bool Retryable = false)
{
    const TSharedRef<FJsonObject> Error = MakeShared<FJsonObject>();
    Error->SetStringField(TEXT("type"), Type); Error->SetStringField(TEXT("message"), Message); Error->SetBoolField(TEXT("retryable"), Retryable);
    const TSharedRef<FJsonObject> Response = MakeShared<FJsonObject>(); Response->SetNumberField(TEXT("protocol"), ProtocolVersion); Response->SetStringField(TEXT("id"), Id); Response->SetStringField(TEXT("status"), TEXT("error")); Response->SetObjectField(TEXT("error"), Error); return Serialize(Response);
}
bool ClosedArgs(const TSharedPtr<FJsonObject>& Args, const TSet<FString>& Allowed)
{
    if (!Args.IsValid()) return false;
    for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Args->Values)
    {
        if (!Allowed.Contains(Entry.Key)) return false;
    }
    return true;
}
bool ValidateLevelPath(const FString& Path, FString& OutFilename, FString& OutError)
{
    if (!Path.StartsWith(TEXT("/Game/")) || Path.Contains(TEXT("..")) || Path.Contains(TEXT("//")) || Path.EndsWith(TEXT("/")) || Path.Contains(TEXT(".")))
    {
        OutError = TEXT("level path must be an exact /Game/... long package name without extension or traversal");
        return false;
    }
    if (!FPackageName::IsValidLongPackageName(Path, false))
    {
        OutError = TEXT("level path is not a valid long package name");
        return false;
    }
    OutFilename = FPackageName::LongPackageNameToFilename(Path, FPackageName::GetMapPackageExtension());
    return true;
}


TSharedRef<FJsonObject> LevelMutationResult(const FString& Level, const bool Changed, const TArray<TSharedPtr<FJsonValue>>& Dirty, const TArray<TSharedPtr<FJsonValue>>& Saved)
{
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("level"), Level);
    Result->SetBoolField(TEXT("changed"), Changed);
    Result->SetArrayField(TEXT("dirtyPackages"), Dirty);
    Result->SetArrayField(TEXT("savedPackages"), Saved);
    Result->SetStringField(TEXT("revision"), LevelContentRevision(*GEditor->GetEditorWorldContext().World()));
    return Result;
}

TSharedRef<FJsonObject> AssetMutationResult(const FString& Id, const FString& Class, const FString& ValueType, const int32 MappingCount, const bool Changed, const TArray<TSharedPtr<FJsonValue>>& Dirty, const TArray<TSharedPtr<FJsonValue>>& Saved, const FString& Revision)
{
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("id"), Id); Result->SetStringField(TEXT("class"), Class); if (!ValueType.IsEmpty()) Result->SetStringField(TEXT("valueType"), ValueType);
    if (MappingCount >= 0) Result->SetNumberField(TEXT("mappingCount"), MappingCount);
    Result->SetBoolField(TEXT("changed"), Changed); Result->SetArrayField(TEXT("dirtyPackages"), Dirty); Result->SetArrayField(TEXT("savedPackages"), Saved); Result->SetStringField(TEXT("revision"), Revision);
    return Result;
}


bool ReadPageArgs(
    const TSharedPtr<FJsonObject>& Args,
    const TSet<FString>& KnownFields,
    int32& Limit,
    FString& Cursor,
    TSet<FString>& Fields)
{
    if (!ClosedArgs(Args, {TEXT("limit"), TEXT("cursor"), TEXT("fields")})) return false;
    Limit = 100;
    int64 Value = 0;
    if (Args->HasField(TEXT("limit")))
    {
        if (!IntegerField(Args, TEXT("limit"), Value) || Value < 1 || Value > 100) return false;
        Limit = static_cast<int32>(Value);
    }
    if (Args->HasField(TEXT("cursor"))
        && (!StringField(Args, TEXT("cursor"), Cursor) || Cursor.Len() > 256)) return false;
    if (Args->HasField(TEXT("fields")))
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Args->TryGetArrayField(TEXT("fields"), Values) || !Values || Values->IsEmpty() || Values->Num() > KnownFields.Num()) return false;
        for (const TSharedPtr<FJsonValue>& Field : *Values)
        {
            if (!Field.IsValid() || Field->Type != EJson::String || !KnownFields.Contains(Field->AsString()) || Fields.Contains(Field->AsString())) return false;
            Fields.Add(Field->AsString());
        }
    }
    return true;
}

bool ReadComponentListPageArgs(
    const TSharedPtr<FJsonObject>& Args,
    int32& Limit,
    FString& Cursor,
    TSet<FString>& Fields)
{
    TSharedRef<FJsonObject> PageArgs = MakeShared<FJsonObject>();
    PageArgs->Values = Args->Values;
    PageArgs->RemoveField(TEXT("actorId"));
    return ReadPageArgs(PageArgs, {TEXT("id"), TEXT("name"), TEXT("class"), TEXT("scene")}, Limit, Cursor, Fields);
}


FString CanonicalRow(const TArray<FString>& Fields)
{
    FString Result;
    for (const FString& Field : Fields)
    {
        Result += FString::Printf(TEXT("%d:"), Field.Len()) + Field;
    }
    return Result;
}
FString ExtendRevision(const FString& Revision, const TArray<FString>& Fields)
{
    return Sha256(CanonicalRow({Revision}) + CanonicalRow(Fields));
}

FString SnapshotRevision(const FString& Operation, const FString& Scope, const TSet<FString>& SelectedFields, const TArray<FString>& Rows)
{
    TArray<FString> Fields = SelectedFields.Array();
    Fields.Sort();
    FString Revision = Sha256(CanonicalRow({Operation, Scope}) + CanonicalRow(Fields));
    for (const FString& Row : Rows) Revision = ExtendRevision(Revision, {Row});
    return Revision;
}

bool CursorOffset(const FString& Cursor, const FString& Revision, const int32 Total, int32& Offset)
{
    Offset = 0;
    if (Cursor.IsEmpty()) return true;
    TArray<FString> Parts;
    Cursor.ParseIntoArray(Parts, TEXT("."), false);
    if (Parts.Num() != 3 || Parts[0] != TEXT("v1") || Parts[1] != Revision || !Parts[2].IsNumeric()) return false;
    Offset = FCString::Atoi(*Parts[2]);
    return Offset >= 0 && Offset < Total;
}

FString ActorId(const AActor& Actor);
TArray<AActor*> StableEditorActors(const UWorld& World);
TSharedRef<FJsonObject> PageResult(
    const FString& Scope,
    const TArray<TSharedPtr<FJsonValue>>& Items,
    const int32 Total,
    const int32 Offset,
    const FString& Revision)
{
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetNumberField(TEXT("count"), Items.Num());
    Result->SetNumberField(TEXT("total"), Total);
    Result->SetStringField(TEXT("scope"), Scope);
    Result->SetStringField(TEXT("revision"), Revision);
    Result->SetArrayField(TEXT("items"), Items);
    if (Offset + Items.Num() < Total)
    {
        Result->SetStringField(TEXT("nextCursor"), FString::Printf(TEXT("v1.%s.%d"), *Revision, Offset + Items.Num()));
    }
    else
    {
        Result->SetField(TEXT("nextCursor"), MakeShared<FJsonValueNull>());
    }
    return Result;
}

FString CanonicalTransform(const FTransform& Transform)
{
    const FVector Location = Transform.GetLocation();
    const FRotator Rotation = Transform.Rotator();
    const FVector Scale = Transform.GetScale3D();
    return CanonicalRow({FString::SanitizeFloat(Location.X), FString::SanitizeFloat(Location.Y), FString::SanitizeFloat(Location.Z), FString::SanitizeFloat(Rotation.Roll), FString::SanitizeFloat(Rotation.Pitch), FString::SanitizeFloat(Rotation.Yaw), FString::SanitizeFloat(Scale.X), FString::SanitizeFloat(Scale.Y), FString::SanitizeFloat(Scale.Z)});
}
FString ActorRevision(const AActor& Actor)
{
    TArray<FString> Tags;
    for (const FName& Tag : Actor.Tags) Tags.Add(Tag.ToString());
    Tags.Sort();
    TArray<UActorComponent*> Components = Actor.GetComponents().Array();
    Components.RemoveAll([](const UActorComponent* Component) { return !IsValid(Component) || Component->HasAnyFlags(RF_Transient) || Component->IsTemplate(); });
    Components.Sort([](const UActorComponent& Left, const UActorComponent& Right) { return Left.GetName() == Right.GetName() ? Left.GetClass()->GetPathName() < Right.GetClass()->GetPathName() : Left.GetName() < Right.GetName(); });
    FString Revision = Sha256(CanonicalRow({ActorId(Actor), Actor.GetClass()->GetPathName(), Actor.GetPathName(), Actor.GetActorLabel(), CanonicalTransform(Actor.GetActorTransform())}));
    for (const FString& Tag : Tags) Revision = ExtendRevision(Revision, {TEXT("tag"), Tag});
    for (const UActorComponent* Component : Components)
    {
        const USceneComponent* Scene = Cast<USceneComponent>(Component);
        Revision = ExtendRevision(Revision, {TEXT("component"), Component->GetName(), Component->GetClass()->GetPathName(), Scene ? CanonicalTransform(Scene->GetRelativeTransform()) : FString()});
    }
    return Revision;
}

void AddTransformFields(const TSharedRef<FJsonObject>& Result, const AActor& Actor)
{
    const FVector Location = Actor.GetActorLocation();
    const FRotator Rotation = Actor.GetActorRotation();
    const FVector Scale = Actor.GetActorScale3D();
    Result->SetArrayField(TEXT("location"), {MakeShared<FJsonValueNumber>(Location.X), MakeShared<FJsonValueNumber>(Location.Y), MakeShared<FJsonValueNumber>(Location.Z)});
    Result->SetArrayField(TEXT("rotation"), {MakeShared<FJsonValueNumber>(Rotation.Roll), MakeShared<FJsonValueNumber>(Rotation.Pitch), MakeShared<FJsonValueNumber>(Rotation.Yaw)});
    Result->SetArrayField(TEXT("scale"), {MakeShared<FJsonValueNumber>(Scale.X), MakeShared<FJsonValueNumber>(Scale.Y), MakeShared<FJsonValueNumber>(Scale.Z)});
}

AActor* FindActorById(UWorld& World, const FString& Wanted)
{
    for (AActor* Actor : StableEditorActors(World)) if (ActorId(*Actor) == Wanted) return Actor;
    return nullptr;
}

struct FMutationSafetyState
{
    bool LifecycleReady = true;
    bool EditorAvailable = true;
    bool PlaySessionActive = false;
    bool ModalWindowActive = false;
    bool ShutdownRequested = false;
    bool SlowTaskActive = false;
    bool AsyncLoadingActive = false;
    bool GarbageCollecting = false;
    bool PackageSaveActive = false;
    bool AssetCompilationActive = false;
    bool EditorBuildActive = false;
};

bool ValidateMutationSafetyState(const FMutationSafetyState& State, FString& Message)
{
    if (!State.LifecycleReady) { Message = TEXT("editor lifecycle is not ready"); return false; }
    if (!State.EditorAvailable) { Message = TEXT("editor is unavailable"); return false; }
    if (State.PlaySessionActive) { Message = TEXT("PIE or SIE is active"); return false; }
    if (State.ModalWindowActive) { Message = TEXT("editor modal dialog is active"); return false; }
    if (State.ShutdownRequested) { Message = TEXT("editor shutdown is in progress"); return false; }
    if (State.SlowTaskActive) { Message = TEXT("editor slow task is active"); return false; }
    if (State.AsyncLoadingActive) { Message = TEXT("async package loading is active"); return false; }
    if (State.GarbageCollecting) { Message = TEXT("garbage collection is active"); return false; }
    if (State.PackageSaveActive) { Message = TEXT("package save is active"); return false; }
    if (State.AssetCompilationActive) { Message = TEXT("asset compilation is active"); return false; }
    if (State.EditorBuildActive) { Message = TEXT("editor build is active"); return false; }
    return true;
}

bool MutationGate(FString& Message)
{
    const FMutationSafetyState State{
        Lifecycle.load() == ELifecycle::Ready,
        GEditor != nullptr,
        GEditor && (GEditor->PlayWorld || GEditor->IsPlaySessionInProgress()),
        FSlateApplication::IsInitialized() && FSlateApplication::Get().GetActiveModalWindow().IsValid(),
        IsEngineExitRequested(),
        GIsSlowTask,
        IsAsyncLoading(),
        IsGarbageCollecting(),
        UE::IsSavingPackage(),
        FAssetCompilingManager::Get().GetNumRemainingAssets() > 0,
        GEditor && (GEditor->IsLightingBuildCurrentlyRunning() || FEditorBuildUtils::IsBuildCurrentlyRunning())
    };
    if (!ValidateMutationSafetyState(State, Message)) return false;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World || World->WorldType != EWorldType::Editor) { Message = TEXT("editor world is unavailable or not editable"); return false; }
    if (UPackage* Package = World->GetOutermost())
    {
        FString Filename;
        if (FPackageName::DoesPackageExist(Package->GetName(), &Filename) && IFileManager::Get().IsReadOnly(*Filename)) { Message = TEXT("current level package is read-only"); return false; }
    }
    return true;
}
TSharedRef<FJsonObject> BuildReceiptMetadata(const FString& Id, const FString& Operation, const TSharedRef<FJsonObject>& Result, const TSharedRef<FJsonObject>& Verification, const FString& Target)
{
    bool Changed = false;
    Result->TryGetBoolField(TEXT("changed"), Changed);
    const TArray<TSharedPtr<FJsonValue>>* Dirty = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Saved = nullptr;
    Result->TryGetArrayField(TEXT("dirtyPackages"), Dirty);
    Result->TryGetArrayField(TEXT("savedPackages"), Saved);
    const TSharedRef<FJsonObject> Receipt = MakeShared<FJsonObject>();
    Receipt->SetStringField(TEXT("operationId"), Id);
    Receipt->SetStringField(TEXT("operation"), Operation);
    Receipt->SetStringField(TEXT("state"), TEXT("completed"));
    Receipt->SetStringField(TEXT("projectId"), ProjectId);
    Receipt->SetNumberField(TEXT("editorPid"), FPlatformProcess::GetCurrentProcessId());
    Receipt->SetStringField(TEXT("target"), Target);
    Receipt->SetBoolField(TEXT("changed"), Changed);
    Receipt->SetStringField(TEXT("transaction"), (Operation == TEXT("play.start") || Operation == TEXT("play.stop")) ? TEXT("atomic") : Operation.StartsWith(TEXT("play.")) ? TEXT("none") : Changed ? TEXT("atomic") : TEXT("none"));
    Receipt->SetStringField(TEXT("reversibility"), Operation.StartsWith(TEXT("play.")) ? TEXT("none") : Operation == TEXT("actor.delete") || Operation == TEXT("component.remove") ? TEXT("destructive") : TEXT("source-control"));
    Receipt->SetArrayField(TEXT("dirtyPackages"), Dirty ? *Dirty : TArray<TSharedPtr<FJsonValue>>{});
    Receipt->SetArrayField(TEXT("savedPackages"), Saved ? *Saved : TArray<TSharedPtr<FJsonValue>>{});
    FString Revision;
    Result->TryGetStringField(TEXT("revision"), Revision);
    Receipt->SetStringField(TEXT("revision"), Revision.IsEmpty() ? Sha256(Id) : Revision);
    if (Operation == TEXT("actor.delete") || Operation == TEXT("component.remove")) Verification->SetBoolField(TEXT("exists"), !Changed);
    Receipt->SetStringField(TEXT("persistence"), Operation == TEXT("play.screenshot") ? TEXT("saved") : Saved && !Saved->IsEmpty() ? TEXT("saved") : Dirty && !Dirty->IsEmpty() ? TEXT("dirty") : TEXT("unchanged"));
    Receipt->SetObjectField(TEXT("verification"), Verification);
    return Receipt;
}



FString LevelSettingsRevision(const UWorld& World);
bool VerifyMutationPostcondition(const FString& Operation, const TSharedRef<FJsonObject>& Result, const FString& Target, const TSharedRef<FJsonObject>& Verification);
UActorComponent* FindComponentById(UWorld& World, const FString& Wanted, AActor*& OutActor);
FString ComponentRevision(const AActor& Actor, const UActorComponent& Component);
FString ObjectContentRevision(const UObject* Object);
FString SuccessResponse(const FString& Id, const TSharedRef<FJsonObject>& Result, const FString& Operation = FString())
{
    if (!Operation.IsEmpty() && !MagiAxiValidateOutput(Operation, Result)) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("native output failed generated schema validation"));
    const TSharedRef<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetNumberField(TEXT("protocol"), ProtocolVersion);
    Response->SetStringField(TEXT("id"), Id);
    Response->SetStringField(TEXT("status"), TEXT("ok"));
    Response->SetObjectField(TEXT("result"), Result);
    if (IsMutationOperation(Operation))
    {
        FString Target = Operation;
        if (!Result->TryGetStringField(TEXT("id"), Target))
        {
            if (!Result->TryGetStringField(TEXT("level"), Target)) Result->TryGetStringField(TEXT("levelId"), Target);
        }
        if (Operation.StartsWith(TEXT("play."))) Result->TryGetStringField(Operation == TEXT("play.screenshot") ? TEXT("path") : TEXT("sessionId"), Target);
        FString Readback = Operation.StartsWith(TEXT("actor.")) ? TEXT("actor.view") : Operation.StartsWith(TEXT("component.")) ? TEXT("component.view") : Operation.StartsWith(TEXT("asset.")) ? TEXT("asset.view") : Operation.StartsWith(TEXT("blueprint.")) ? TEXT("blueprint.view") : Operation == TEXT("level.set_game_mode") ? TEXT("level.settings") : Operation == TEXT("play.input") ? TEXT("play.observe") : Operation == TEXT("play.screenshot") ? TEXT("artifact") : Operation.StartsWith(TEXT("play.")) ? TEXT("play.status") : TEXT("level.current");
        const TSharedRef<FJsonObject> Verification = MakeShared<FJsonObject>();
        Verification->SetStringField(TEXT("readback"), Readback); Verification->SetStringField(TEXT("target"), Target);
        if (!VerifyMutationPostcondition(Operation, Result, Target, Verification)) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("mutation postcondition verification failed"));
        bool Matched = true;
        if (Operation == TEXT("play.input"))
        {
            FString Key, Event, Session; bool Accepted = false;
            Result->TryGetStringField(TEXT("key"), Key); Result->TryGetStringField(TEXT("event"), Event); Result->TryGetStringField(TEXT("sessionId"), Session); Result->TryGetBoolField(TEXT("accepted"), Accepted);
            Target = Session + TEXT("#") + Key + TEXT("#") + Event; Matched = Accepted;
        }
        Verification->SetStringField(TEXT("target"), Target); Verification->SetBoolField(TEXT("matched"), Matched);
        Response->SetObjectField(TEXT("receipt"), BuildReceiptMetadata(Id, Operation, Result, Verification, Target));
    }
    const FString Wire = Serialize(Response);
    if (IsMutationOperation(Operation)) SetReceipt(Id, Operation, Wire, TEXT("completed"));
    return Wire;
}

FString ActorLevelId(const AActor& Actor)
{
    const ULevel* Level = Actor.GetLevel();
    return Level && Level->GetOutermost() ? Level->GetOutermost()->GetName() : FString();
}

FString ActorId(const AActor& Actor)
{
    return ActorLevelId(Actor) + TEXT("#") + Actor.GetActorGuid().ToString(EGuidFormats::DigitsWithHyphens);
}

TArray<AActor*> StableEditorActors(const UWorld& World)
{
    TArray<AActor*> Actors;
    for (const ULevel* Level : World.GetLevels())
    {
        if (!Level) continue;
        for (AActor* Actor : Level->Actors)
        {
            if (IsValid(Actor) && Actor->GetActorGuid().IsValid()) Actors.Add(Actor);
        }
    }
    Actors.Sort([](const AActor& Left, const AActor& Right) { return ActorId(Left) < ActorId(Right); });
    return Actors;
}
FString AssetRevision(const FAssetData& Asset)
{
    return Sha256(Asset.GetObjectPathString() + TEXT("\n") + Asset.AssetClassPath.ToString() + TEXT("\n") + FString::FromInt(Asset.PackageFlags));
}

FString AssetObjectId(const FString& PackagePath) { return PackagePath + TEXT(".") + FPackageName::GetShortName(PackagePath); }
bool ValidateAssetPath(const FString& Path, FString& Filename, FString& Error)
{
    if (!Path.StartsWith(TEXT("/Game/")) || Path.Contains(TEXT("..")) || Path.Contains(TEXT("//")) || Path.EndsWith(TEXT("/")) || Path.Contains(TEXT("."))) { Error = TEXT("asset path must be a package path under /Game without extension"); return false; }
    Filename = FPackageName::LongPackageNameToFilename(Path, FPackageName::GetAssetPackageExtension()); return true;
}
FString BlueprintStatus(const UBlueprint& Blueprint)
{
    if (Blueprint.Status == BS_UpToDate) return TEXT("up_to_date");
    if (Blueprint.Status == BS_Error) return TEXT("error");
    if (Blueprint.Status == BS_UpToDateWithWarnings) return TEXT("warning");
    return TEXT("dirty");
}
FString BlueprintContentRevision(const UBlueprint& Blueprint)
{
    TArray<UEdGraph*> Graphs;
    Blueprint.GetAllGraphs(Graphs);
    Graphs.Sort([](const UEdGraph& Left, const UEdGraph& Right) { return Left.GetPathName() < Right.GetPathName(); });
    FString Revision = Sha256(CanonicalRow({Blueprint.GetPathName(), Blueprint.ParentClass ? Blueprint.ParentClass->GetPathName() : FString()}));
    for (const UEdGraph* Graph : Graphs)
    {
        if (!Graph) continue;
        Revision = ExtendRevision(Revision, {TEXT("graph"), Graph->GetPathName()});
        TArray<const UEdGraphNode*> Nodes;
        for (const UEdGraphNode* Node : Graph->Nodes) if (Node) Nodes.Add(Node);
        Nodes.Sort([](const UEdGraphNode& Left, const UEdGraphNode& Right) { return Left.NodeGuid.ToString() < Right.NodeGuid.ToString(); });
        for (const UEdGraphNode* Node : Nodes)
        {
            Revision = ExtendRevision(Revision, {TEXT("node"), Node->NodeGuid.ToString(), Node->GetClass()->GetPathName(), Node->GetNodeTitle(ENodeTitleType::ListView).ToString(), FString::FromInt(Node->NodePosX), FString::FromInt(Node->NodePosY)});
            TArray<const UEdGraphPin*> Pins;
            for (const UEdGraphPin* Pin : Node->Pins) if (Pin) Pins.Add(Pin);
            Pins.Sort([](const UEdGraphPin& Left, const UEdGraphPin& Right) { return Left.PinId.ToString() < Right.PinId.ToString(); });
            for (const UEdGraphPin* Pin : Pins)
            {
                const FEdGraphPinType& Type = Pin->PinType;
                const UObject* SubCategoryObject = Type.PinSubCategoryObject.Get();
                Revision = ExtendRevision(Revision, {TEXT("pin"), Pin->PinId.ToString(), Pin->PinName.ToString(), FString::FromInt(static_cast<int32>(Pin->Direction)), Type.PinCategory.ToString(), Type.PinSubCategory.ToString(), SubCategoryObject ? SubCategoryObject->GetPathName() : FString(), FString::FromInt(static_cast<int32>(Type.ContainerType)), Type.bIsReference ? TEXT("reference") : FString(), Type.bIsConst ? TEXT("const") : FString(), Pin->DefaultValue, Pin->DefaultObject ? Pin->DefaultObject->GetPathName() : FString(), Pin->DefaultTextValue.ToString()});
                TArray<const UEdGraphPin*> Links;
                for (const UEdGraphPin* Link : Pin->LinkedTo) if (Link && Link->GetOwningNode()) Links.Add(Link);
                Links.Sort([](const UEdGraphPin& Left, const UEdGraphPin& Right) { const FString LeftKey = Left.GetOwningNode()->NodeGuid.ToString() + Left.PinId.ToString(); const FString RightKey = Right.GetOwningNode()->NodeGuid.ToString() + Right.PinId.ToString(); return LeftKey < RightKey; });
                for (const UEdGraphPin* Link : Links) Revision = ExtendRevision(Revision, {TEXT("link"), Link->GetOwningNode()->NodeGuid.ToString(), Link->PinId.ToString()});
            }
        }
    }
    TArray<const FBPVariableDescription*> Variables;
    for (const FBPVariableDescription& Variable : Blueprint.NewVariables) Variables.Add(&Variable);
    Variables.Sort([](const FBPVariableDescription& Left, const FBPVariableDescription& Right) { return Left.VarGuid.ToString() < Right.VarGuid.ToString(); });
    for (const FBPVariableDescription* Variable : Variables)
    {
        const UObject* SubCategoryObject = Variable->VarType.PinSubCategoryObject.Get();
        Revision = ExtendRevision(Revision, {TEXT("variable"), Variable->VarGuid.ToString(), Variable->VarName.ToString(), Variable->FriendlyName, Variable->Category.ToString(), Variable->VarType.PinCategory.ToString(), Variable->VarType.PinSubCategory.ToString(), SubCategoryObject ? SubCategoryObject->GetPathName() : FString(), FString::FromInt(static_cast<int32>(Variable->VarType.ContainerType)), Variable->VarType.bIsReference ? TEXT("reference") : FString(), Variable->VarType.bIsConst ? TEXT("const") : FString(), Variable->DefaultValue, FString::Printf(TEXT("%llu"), static_cast<unsigned long long>(Variable->PropertyFlags)), Variable->RepNotifyFunc.ToString(), FString::FromInt(static_cast<int32>(Variable->ReplicationCondition))});
        TArray<const FBPVariableMetaDataEntry*> Metadata;
        for (const FBPVariableMetaDataEntry& Entry : Variable->MetaDataArray) Metadata.Add(&Entry);
        Metadata.Sort([](const FBPVariableMetaDataEntry& Left, const FBPVariableMetaDataEntry& Right) { return Left.DataKey == Right.DataKey ? Left.DataValue < Right.DataValue : Left.DataKey.LexicalLess(Right.DataKey); });
        for (const FBPVariableMetaDataEntry* Entry : Metadata) Revision = ExtendRevision(Revision, {TEXT("metadata"), Entry->DataKey.ToString(), Entry->DataValue});
    }
    TArray<const FBPInterfaceDescription*> Interfaces;
    for (const FBPInterfaceDescription& Interface : Blueprint.ImplementedInterfaces) Interfaces.Add(&Interface);
    Interfaces.Sort([](const FBPInterfaceDescription& Left, const FBPInterfaceDescription& Right) { const FString LeftPath = Left.Interface ? Left.Interface->GetPathName() : FString(); const FString RightPath = Right.Interface ? Right.Interface->GetPathName() : FString(); return LeftPath < RightPath; });
    for (const FBPInterfaceDescription* Interface : Interfaces)
    {
        Revision = ExtendRevision(Revision, {TEXT("interface"), Interface->Interface ? Interface->Interface->GetPathName() : FString()});
        TArray<UEdGraph*> InterfaceGraphs = Interface->Graphs;
        InterfaceGraphs.Sort([](const UEdGraph& Left, const UEdGraph& Right) { return Left.GetPathName() < Right.GetPathName(); });
        for (const UEdGraph* Graph : InterfaceGraphs) if (Graph) Revision = ExtendRevision(Revision, {TEXT("interfaceGraph"), Graph->GetPathName()});
    }
    if (Blueprint.SimpleConstructionScript)
    {
        TArray<USCS_Node*> Components = Blueprint.SimpleConstructionScript->GetAllNodes();
        Components.RemoveAll([](const USCS_Node* Node) { return Node == nullptr; });
        Components.Sort([](const USCS_Node& Left, const USCS_Node& Right) { return Left.VariableGuid.ToString() < Right.VariableGuid.ToString(); });
        for (const USCS_Node* Node : Components)
        {
            const USceneComponent* Scene = Cast<USceneComponent>(Node->ComponentTemplate);
            Revision = ExtendRevision(Revision, {TEXT("component"), Node->VariableGuid.ToString(), Node->GetVariableName().ToString(), Node->ComponentClass ? Node->ComponentClass->GetPathName() : FString(), Node->ParentComponentOrVariableName.ToString(), Node->AttachToName.ToString(), Scene ? CanonicalTransform(Scene->GetRelativeTransform()) : FString()});
        }
    }
    return Revision;
}

FString ObjectContentRevision(const UObject* Object)
{
    if (!Object) return FString();
    if (const UBlueprint* Blueprint = Cast<UBlueprint>(Object)) return BlueprintContentRevision(*Blueprint);
    if (const UInputAction* Action = Cast<UInputAction>(Object)) return Sha256(Action->GetPathName() + TEXT("\n") + UEnum::GetValueAsString(Action->ValueType));
    if (const UInputMappingContext* Context = Cast<UInputMappingContext>(Object))
    {
        TArray<const FEnhancedActionKeyMapping*> Mappings;
        for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings()) Mappings.Add(&Mapping);
        Mappings.Sort([](const FEnhancedActionKeyMapping& Left, const FEnhancedActionKeyMapping& Right) { const FString LeftKey = (Left.Action ? Left.Action->GetPathName() : FString()) + Left.Key.ToString(); const FString RightKey = (Right.Action ? Right.Action->GetPathName() : FString()) + Right.Key.ToString(); return LeftKey < RightKey; });
        FString Revision = Sha256(Context->GetPathName());
        for (const FEnhancedActionKeyMapping* Mapping : Mappings) Revision = ExtendRevision(Revision, {Mapping->Action ? Mapping->Action->GetPathName() : FString(), Mapping->Key.ToString()});
        return Revision;
    }
    return AssetRevision(FAssetData(Object));
}

bool VerifyMutationPostcondition(const FString& Operation, const TSharedRef<FJsonObject>& Result, const FString& Target, const TSharedRef<FJsonObject>& Verification)
{
    FString Revision;
    if (!Result->TryGetStringField(TEXT("revision"), Revision) || Revision.Len() != 64) return false;
    auto Verified = [&]() { Verification->SetStringField(TEXT("observedRevision"), Revision); return true; };
    if (Operation == TEXT("play.input"))
    {
        bool Accepted = false; FString Before, After, Session, Event;
        Result->TryGetBoolField(TEXT("accepted"), Accepted); Result->TryGetStringField(TEXT("beforeRevision"), Before); Result->TryGetStringField(TEXT("afterRevision"), After); Result->TryGetStringField(TEXT("sessionId"), Session); Result->TryGetStringField(TEXT("event"), Event);
        const bool Changed = Before != After;
        if (!Accepted || Before.Len() != 64 || After.Len() != 64 || After != Revision || Result->GetBoolField(TEXT("changed")) != Changed || (Event == TEXT("pressed") && !Changed) || (Event == TEXT("released") && Changed) || !PieWorld()) return false;
        const TSharedRef<FJsonObject> Observation = ObservePlayResult();
        if (Observation->GetStringField(TEXT("sessionId")) != Session || Observation->GetStringField(TEXT("revision")) != After) return false;
        Verification->SetBoolField(TEXT("accepted"), true); Verification->SetStringField(TEXT("beforeRevision"), Before); Verification->SetStringField(TEXT("afterRevision"), After);
        return Verified();
    }
    if (Operation == TEXT("play.screenshot"))
    {
        TArray<uint8> Bytes;
        return FFileHelper::LoadFileToArray(Bytes, *Target) && Bytes.Num() >= 8 && Bytes[0] == 0x89 && Bytes[1] == 'P' && Bytes[2] == 'N' && Bytes[3] == 'G';
    }
    if (Operation == TEXT("play.start") || Operation == TEXT("play.stop"))
    {
        FString Session, StatusSession, State;
        Result->TryGetStringField(TEXT("sessionId"), Session);
        const TSharedRef<FJsonObject> Status = PlayStatusResult();
        Status->TryGetStringField(TEXT("sessionId"), StatusSession);
        Status->TryGetStringField(TEXT("state"), State);
        if (Session != StatusSession || Status->GetStringField(TEXT("revision")) != Revision) return false;
        if (Operation == TEXT("play.start")) return GUnrealEd && (GUnrealEd->IsPlaySessionRequestQueued() || PieWorld()) && (State == TEXT("starting") || State == TEXT("running")) && Verified();
        return !PieWorld() && (!GUnrealEd || !GUnrealEd->IsPlayingSessionInEditor()) && State == TEXT("stopped") && Verified();
    }
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (Operation == TEXT("actor.delete"))
    {
        AActor* Actor = World ? FindActorById(*World, Target) : nullptr;
        return (Result->GetBoolField(TEXT("changed")) ? Actor == nullptr : Actor && ActorRevision(*Actor) == Revision) && Verified();
    }
    if (Operation == TEXT("component.remove"))
    {
        AActor* Owner = nullptr; UActorComponent* Component = World ? FindComponentById(*World, Target, Owner) : nullptr;
        return (Result->GetBoolField(TEXT("changed")) ? Component == nullptr : Component && Owner && ComponentRevision(*Owner, *Component) == Revision) && Verified();
    }
    if (Operation.StartsWith(TEXT("actor.")))
    {
        AActor* Actor = World ? FindActorById(*World, Target) : nullptr;
        return Actor && ActorRevision(*Actor) == Revision && Verified();
    }
    if (Operation.StartsWith(TEXT("component.")))
    {
        AActor* Owner = nullptr; UActorComponent* Component = World ? FindComponentById(*World, Target, Owner) : nullptr;
        return Component && Owner && ComponentRevision(*Owner, *Component) == Revision && Verified();
    }
    if (Operation == TEXT("level.set_game_mode"))
    {
        if (!World || !World->GetOutermost() || Target != World->GetOutermost()->GetName() || LevelSettingsRevision(*World) != Revision) return false;
        FString ExpectedClass; Result->TryGetStringField(TEXT("gameModeClass"), ExpectedClass); const AWorldSettings* Settings = World->GetWorldSettings(); const UClass* ActualClass = Settings ? Settings->DefaultGameMode.Get() : nullptr;
        return ActualClass && ActualClass->GetPathName() == ExpectedClass && Verified();
    }
    if (Operation == TEXT("level.create") || Operation == TEXT("level.open") || Operation == TEXT("level.save"))
    {
        if (!World || !World->GetOutermost() || Target != World->GetOutermost()->GetName() || Revision != LevelContentRevision(*World)) return false;
        FString Filename; const bool HasFile = FPackageName::DoesPackageExist(Target, &Filename);
        const TArray<TSharedPtr<FJsonValue>>* Dirty = nullptr; const TArray<TSharedPtr<FJsonValue>>* Saved = nullptr; Result->TryGetArrayField(TEXT("dirtyPackages"), Dirty); Result->TryGetArrayField(TEXT("savedPackages"), Saved);
        if (Operation == TEXT("level.create") && Dirty && !Dirty->IsEmpty() && (!World->GetOutermost()->IsDirty() || HasFile)) return false;
        if (Operation == TEXT("level.save") && Saved && !Saved->IsEmpty() && (!HasFile || World->GetOutermost()->IsDirty())) return false;
        return Verified();
    }
    if (Operation.StartsWith(TEXT("asset.")))
    {
        UObject* Asset = LoadObject<UObject>(nullptr, *Target);
        if (!Asset || ObjectContentRevision(Asset) != Revision) return false;
        if (Operation == TEXT("asset.save") && (!Asset->GetOutermost() || Asset->GetOutermost()->IsDirty())) return false;
        return Verified();
    }
    if (Operation == TEXT("blueprint.compile"))
    {
        UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *Target); FString Status; Result->TryGetStringField(TEXT("status"), Status);
        return Blueprint && BlueprintContentRevision(*Blueprint) == Revision && BlueprintStatus(*Blueprint) == Status && Blueprint->Status != BS_Error && Verified();
    }
    return false;
}
TSharedRef<FJsonObject> BlueprintResult(UBlueprint& Blueprint, bool Mutation = false)
{
    const FString Id = Blueprint.GetPathName();
    const int32 GraphCount = Blueprint.FunctionGraphs.Num() + Blueprint.UbergraphPages.Num();
    const FString Revision = BlueprintContentRevision(Blueprint);
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("id"), Id); Result->SetStringField(TEXT("parentClass"), Blueprint.ParentClass ? Blueprint.ParentClass->GetPathName() : TEXT("unknown")); Result->SetStringField(TEXT("generatedClass"), Blueprint.GeneratedClass ? Blueprint.GeneratedClass->GetPathName() : TEXT("unknown")); Result->SetStringField(TEXT("status"), BlueprintStatus(Blueprint)); Result->SetNumberField(TEXT("graphCount"), GraphCount); Result->SetNumberField(TEXT("errorCount"), Blueprint.Status == BS_Error ? 1 : 0); Result->SetNumberField(TEXT("warningCount"), Blueprint.Status == BS_UpToDateWithWarnings ? 1 : 0); Result->SetArrayField(TEXT("diagnostics"), {}); Result->SetStringField(TEXT("revision"), Revision);
    if (Mutation) { Result->SetBoolField(TEXT("changed"), false); Result->SetArrayField(TEXT("dirtyPackages"), Blueprint.GetOutermost() && Blueprint.GetOutermost()->IsDirty() ? TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueString>(Blueprint.GetOutermost()->GetName())} : TArray<TSharedPtr<FJsonValue>>{}); Result->SetArrayField(TEXT("savedPackages"), {}); }
    return Result;
}
TArray<TSharedPtr<FJsonValue>> BlueprintDiagnostics(const UBlueprint& Blueprint, const FCompilerResultsLog& Results)
{
    TArray<TSharedPtr<FJsonValue>> Diagnostics;
    for (const TSharedRef<FTokenizedMessage>& Message : Results.Messages)
    {
        if (Message->GetSeverity() != EMessageSeverity::Error && Message->GetSeverity() != EMessageSeverity::Warning) continue;
        const UEdGraphNode* Node = nullptr;
        for (const TSharedRef<IMessageToken>& MessageToken : Message->GetMessageTokens())
        {
            if (MessageToken->GetType() != EMessageToken::EdGraph) continue;
            const TSharedRef<FEdGraphToken> GraphToken = StaticCastSharedRef<FEdGraphToken>(MessageToken);
            Node = Cast<UEdGraphNode>(GraphToken->GetGraphObject());
            if (!Node && GraphToken->GetPin()) Node = GraphToken->GetPin()->GetOwningNodeUnchecked();
            if (Node) break;
        }
        FString Text = Message->ToText().ToString().Left(1024); if (Text.IsEmpty()) Text = TEXT("Blueprint compiler diagnostic");
        const TSharedRef<FJsonObject> Diagnostic = MakeShared<FJsonObject>();
        Diagnostic->SetStringField(TEXT("severity"), Message->GetSeverity() == EMessageSeverity::Warning ? TEXT("warning") : TEXT("error")); Diagnostic->SetStringField(TEXT("message"), Text);
        Diagnostic->SetStringField(TEXT("graph"), Node && Node->GetGraph() ? Node->GetGraph()->GetPathName() : Blueprint.GetPathName()); Diagnostic->SetStringField(TEXT("nodeGuid"), Node ? Node->NodeGuid.ToString(EGuidFormats::DigitsWithHyphens) : TEXT("")); Diagnostic->SetStringField(TEXT("nodeTitle"), Node ? Node->GetNodeTitle(ENodeTitleType::ListView).ToString().Left(512) : TEXT(""));
        Diagnostics.Add(MakeShared<FJsonValueObject>(Diagnostic)); if (Diagnostics.Num() == 100) break;
    }
    return Diagnostics;
}

FString BlueprintCompileErrorResponse(const FString& Id, const UBlueprint& Blueprint, const FCompilerResultsLog& Results)
{
    TArray<TSharedPtr<FJsonValue>> Diagnostics = BlueprintDiagnostics(Blueprint, Results);
    if (Diagnostics.IsEmpty()) { const TSharedRef<FJsonObject> Diagnostic = MakeShared<FJsonObject>(); Diagnostic->SetStringField(TEXT("severity"), TEXT("error")); Diagnostic->SetStringField(TEXT("message"), TEXT("Blueprint compile failed")); Diagnostic->SetStringField(TEXT("graph"), Blueprint.GetPathName()); Diagnostic->SetStringField(TEXT("nodeGuid"), TEXT("")); Diagnostic->SetStringField(TEXT("nodeTitle"), TEXT("")); Diagnostics.Add(MakeShared<FJsonValueObject>(Diagnostic)); }
    const TSharedRef<FJsonObject> Error = MakeShared<FJsonObject>(); Error->SetStringField(TEXT("type"), TEXT("blueprint_compile_failed")); Error->SetStringField(TEXT("message"), TEXT("Blueprint compile failed")); Error->SetBoolField(TEXT("retryable"), false); Error->SetNumberField(TEXT("errorCount"), FMath::Clamp(Results.NumErrors, 1, 100)); Error->SetNumberField(TEXT("warningCount"), FMath::Clamp(Results.NumWarnings, 0, 100)); Error->SetArrayField(TEXT("diagnostics"), Diagnostics);
    const TSharedRef<FJsonObject> Response = MakeShared<FJsonObject>(); Response->SetNumberField(TEXT("protocol"), ProtocolVersion); Response->SetStringField(TEXT("id"), Id); Response->SetStringField(TEXT("status"), TEXT("error")); Response->SetObjectField(TEXT("error"), Error); return Serialize(Response);
}

void SetIfSelected(const TSharedRef<FJsonObject>& Object, const TSet<FString>& Fields, const TCHAR* Name, const FString& Value)
{
    if (FCString::Strcmp(Name, TEXT("id")) == 0 || Fields.IsEmpty() || Fields.Contains(Name)) Object->SetStringField(Name, Value);
}

FString ComponentId(const AActor& Actor, const UActorComponent& Component)
{
    return ActorId(Actor) + TEXT("#component:") + Component.GetName();
}

UActorComponent* FindComponentById(UWorld& World, const FString& Wanted, AActor*& OutActor)
{
    OutActor = nullptr;
    for (AActor* Actor : StableEditorActors(World))
    {
        const FString Prefix = ActorId(*Actor) + TEXT("#component:");
        if (!Wanted.StartsWith(Prefix)) continue;
        for (UActorComponent* Component : Actor->GetComponents().Array())
            if (IsValid(Component) && ComponentId(*Actor, *Component) == Wanted) { OutActor = Actor; return Component; }
    }
    return nullptr;
}

FString LevelContentRevision(const UWorld& World)
{
    FString Revision = Sha256(CanonicalRow({World.GetOutermost() ? World.GetOutermost()->GetName() : FString(), LevelSettingsRevision(World)}));
    for (const AActor* Actor : StableEditorActors(World))
    {
        if (Actor->HasAnyFlags(RF_Transient) || Actor->IsTemplate()) continue;
        Revision = ExtendRevision(Revision, {TEXT("actor"), ActorId(*Actor), Actor->GetClass()->GetPathName(), Actor->GetActorLabel(), CanonicalTransform(Actor->GetActorTransform()), ActorRevision(*Actor)});
    }
    return Revision;
}

FString ComponentRevision(const AActor& Actor, const UActorComponent& Component)
{
    const USceneComponent* Scene = Cast<USceneComponent>(&Component);
    return Sha256(CanonicalRow({ComponentId(Actor, Component), Component.GetClass()->GetPathName(), Scene ? CanonicalTransform(Scene->GetRelativeTransform()) : FString()}));
}

void AddComponentTransformFields(const TSharedRef<FJsonObject>& Result, const USceneComponent& Scene)
{
    const FVector Location = Scene.GetRelativeLocation();
    const FRotator Rotation = Scene.GetRelativeRotation();
    const FVector Scale = Scene.GetRelativeScale3D();
    Result->SetArrayField(TEXT("location"), {MakeShared<FJsonValueNumber>(Location.X), MakeShared<FJsonValueNumber>(Location.Y), MakeShared<FJsonValueNumber>(Location.Z)});
    Result->SetArrayField(TEXT("rotation"), {MakeShared<FJsonValueNumber>(Rotation.Roll), MakeShared<FJsonValueNumber>(Rotation.Pitch), MakeShared<FJsonValueNumber>(Rotation.Yaw)});
    Result->SetArrayField(TEXT("scale"), {MakeShared<FJsonValueNumber>(Scale.X), MakeShared<FJsonValueNumber>(Scale.Y), MakeShared<FJsonValueNumber>(Scale.Z)});
}

FString LevelSettingsRevision(const UWorld& World)
{
    const AWorldSettings* Settings = World.GetWorldSettings();
    const UClass* GameMode = Settings ? Settings->DefaultGameMode.Get() : nullptr;
    const UClass* Pawn = GameMode && GameMode->GetDefaultObject<AGameModeBase>() ? GameMode->GetDefaultObject<AGameModeBase>()->DefaultPawnClass : nullptr;
    return Sha256(World.GetOutermost()->GetName() + TEXT("\n") + (GameMode ? GameMode->GetPathName() : FString()) + TEXT("\n") + (Pawn ? Pawn->GetPathName() : FString()));
}
UWorld* PieWorld()
{
    if (!GEngine) return nullptr;
    for (const FWorldContext& Context : GEngine->GetWorldContexts())
        if (Context.WorldType == EWorldType::PIE && Context.World()) return Context.World();
    return nullptr;
}
bool ValidPlaySession(const TSharedPtr<FJsonObject>& Args, FString& Error)
{
    FString Requested;
    Args->TryGetStringField(TEXT("sessionId"), Requested);
    if (Requested.IsEmpty() && Args->HasField(TEXT("sessionId"))) { Error = TEXT("sessionId is invalid"); return false; }
    if (!Requested.IsEmpty() && Requested != PlaySessionId) { Error = TEXT("sessionId does not identify active PIE session"); return false; }
    if (PlaySessionId.IsEmpty()) { Error = TEXT("no PIE session is active"); return false; }
    return true;
}
TSharedRef<FJsonObject> PlayStatusResult()
{
    UWorld* World = PieWorld();
    const TCHAR* State = PlayState == EPlayState::Starting ? TEXT("starting") : PlayState == EPlayState::Stopping ? TEXT("stopping") : World ? TEXT("running") : TEXT("stopped");
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("state"), State);
    if (PlaySessionId.IsEmpty()) Result->SetField(TEXT("sessionId"), MakeShared<FJsonValueNull>()); else Result->SetStringField(TEXT("sessionId"), PlaySessionId);
    if (World) { Result->SetStringField(TEXT("worldId"), World->GetPathName()); Result->SetStringField(TEXT("levelId"), World->GetOutermost()->GetName()); } else { Result->SetField(TEXT("worldId"), MakeShared<FJsonValueNull>()); Result->SetField(TEXT("levelId"), MakeShared<FJsonValueNull>()); }
    Result->SetNumberField(TEXT("playerCount"), World ? World->GetNumPlayerControllers() : 0); Result->SetStringField(TEXT("revision"), Sha256(PlaySessionId + State)); return Result;
}
TSharedRef<FJsonObject> ObservePlayResult()
{
    UWorld* World = PieWorld(); check(World);
    const FString WorldId = World->GetPathName();
    const FString LevelId = World->GetOutermost()->GetName();
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("sessionId"), PlaySessionId); Result->SetStringField(TEXT("worldId"), WorldId); Result->SetStringField(TEXT("levelId"), LevelId);
    TArray<AActor*> Actors;
    for (TActorIterator<AActor> It(World); It; ++It) if (IsValid(*It)) Actors.Add(*It);
    Actors.Sort([](const AActor& Left, const AActor& Right) { return Left.GetPathName() < Right.GetPathName(); });
    TArray<TSharedPtr<FJsonValue>> Rows;
    FString Revision = Sha256(CanonicalRow({PlaySessionId, WorldId, LevelId}));
    for (int32 Index = 0; Index < Actors.Num(); ++Index)
    {
        AActor* Actor = Actors[Index];
        const FString ActorIdValue = Actor->GetPathName();
        const FString ActorName = Actor->GetName();
        const FString ActorClass = Actor->GetClass()->GetPathName();
        const FVector Location = Actor->GetActorLocation();
        TArray<FString> TagStrings;
        for (const FName& TagName : Actor->Tags) TagStrings.Add(TagName.ToString());
        TagStrings.Sort();
        Revision = ExtendRevision(Revision, {TEXT("actor"), ActorIdValue, ActorName, ActorClass, FString::SanitizeFloat(Location.X), FString::SanitizeFloat(Location.Y), FString::SanitizeFloat(Location.Z)});
        for (const FString& Tag : TagStrings) Revision = ExtendRevision(Revision, {TEXT("tag"), Tag});
        if (Index >= 100) continue;
        const TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("id"), ActorIdValue); Row->SetStringField(TEXT("name"), ActorName); Row->SetStringField(TEXT("class"), ActorClass);
        Row->SetArrayField(TEXT("location"), {MakeShared<FJsonValueNumber>(Location.X), MakeShared<FJsonValueNumber>(Location.Y), MakeShared<FJsonValueNumber>(Location.Z)});
        TArray<TSharedPtr<FJsonValue>> Tags;
        for (int32 TagIndex = 0; TagIndex < FMath::Min(TagStrings.Num(), 32); ++TagIndex) Tags.Add(MakeShared<FJsonValueString>(TagStrings[TagIndex]));
        Row->SetArrayField(TEXT("tags"), Tags); Rows.Add(MakeShared<FJsonValueObject>(Row));
    }
    Result->SetArrayField(TEXT("actors"), Rows); Result->SetStringField(TEXT("revision"), Revision); return Result;
}
FString PendingPlayInputResponse(const FString& Id, const TSharedRef<FJsonObject>& Result)
{
    const TSharedRef<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetNumberField(TEXT("protocol"), ProtocolVersion);
    Response->SetStringField(TEXT("id"), Id);
    Response->SetStringField(TEXT("status"), TEXT("ok"));
    Response->SetObjectField(TEXT("result"), Result);
    return Serialize(Response);
}
FString PlayResponse(const FString& Id, const TSharedRef<FJsonObject>& Result, const FString& Operation) { return SuccessResponse(Id, Result, Operation); }
FString ReadPlayResponse(const FString& Id, const FString& Operation, const TSharedPtr<FJsonObject>& Args)
{
    if (Operation == TEXT("play.status")) { FString Requested; if (Args->HasField(TEXT("sessionId"))) { Args->TryGetStringField(TEXT("sessionId"), Requested); if (!Requested.IsEmpty() && Requested != PlaySessionId) return ErrorResponse(Id, TEXT("not_found"), TEXT("sessionId does not identify active PIE session")); } return PlayResponse(Id, PlayStatusResult(), Operation); }
    FString Error; if (!ValidPlaySession(Args, Error)) return ErrorResponse(Id, TEXT("unsafe_editor_state"), *Error);
    if (Operation == TEXT("play.start")) return ErrorResponse(Id, TEXT("conflict"), TEXT("PIE session is already active"));
    if (Operation == TEXT("play.observe")) { if (!PieWorld()) return ErrorResponse(Id, TEXT("unsafe_editor_state"), TEXT("PIE world is not running")); return PlayResponse(Id, ObservePlayResult(), Operation); }
    if (Operation == TEXT("play.input")) { UWorld* World = PieWorld(); if (!World) return ErrorResponse(Id, TEXT("unsafe_editor_state"), TEXT("PIE world is not running")); FString KeyName, Event; Args->TryGetStringField(TEXT("key"), KeyName); Args->TryGetStringField(TEXT("event"), Event); FKey Key{FName(*KeyName)}; APlayerController* Controller = World->GetFirstPlayerController(); const TSharedRef<FJsonObject> Before = ObservePlayResult(); FString BeforeRevision; Before->TryGetStringField(TEXT("revision"), BeforeRevision); const float Amount = Event == TEXT("pressed") ? 1.0f : 0.0f; bool Accepted = Controller && Key.IsValid() && Controller->InputKey(FInputKeyEventArgs::CreateSimulated(Key, Event == TEXT("pressed") ? IE_Pressed : IE_Released, Amount)); if (!Accepted) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("play input was not accepted by active session")); const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("sessionId"), PlaySessionId); Result->SetStringField(TEXT("key"), KeyName); Result->SetStringField(TEXT("event"), Event); Result->SetBoolField(TEXT("accepted"), true); Result->SetBoolField(TEXT("changed"), false); Result->SetStringField(TEXT("beforeRevision"), BeforeRevision); Result->SetStringField(TEXT("afterRevision"), BeforeRevision); Result->SetStringField(TEXT("revision"), BeforeRevision); return PendingPlayInputResponse(Id, Result); }
    if (Operation == TEXT("play.screenshot")) { if (!PieWorld()) return ErrorResponse(Id, TEXT("unsafe_editor_state"), TEXT("PIE world is not running")); FViewport* Viewport = GUnrealEd ? GUnrealEd->GetPIEViewport() : nullptr; if (!Viewport) return ErrorResponse(Id, TEXT("unsafe_editor_state"), TEXT("PIE viewport is unavailable")); FString Name; Args->TryGetStringField(TEXT("path"), Name); if (Name.IsEmpty()) Name = PlaySessionId + TEXT(".png"); FString Root = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("MagiUnrealAXI/Screenshots")); FString Path = FPaths::ConvertRelativePathToFull(Root / Name); FPaths::NormalizeFilename(Path); if (!Path.StartsWith(Root + TEXT("/")) || !Path.EndsWith(TEXT(".png"))) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("screenshot path must remain under Saved/MagiUnrealAXI/Screenshots and end in .png")); IFileManager::Get().MakeDirectory(*Root, true); char CanonicalRoot[PATH_MAX]{}, CanonicalParent[PATH_MAX]{}; const FTCHARToUTF8 NativeRoot(*Root); const FString Parent = FPaths::GetPath(Path); const FTCHARToUTF8 NativeParent(*Parent); if (!realpath(NativeRoot.Get(), CanonicalRoot) || !realpath(NativeParent.Get(), CanonicalParent)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("screenshot path resolves outside screenshot directory")); const FString CanonicalRootPath = UTF8_TO_TCHAR(CanonicalRoot); const FString CanonicalParentPath = UTF8_TO_TCHAR(CanonicalParent); if (!(CanonicalParentPath == CanonicalRootPath || CanonicalParentPath.StartsWith(CanonicalRootPath + TEXT("/")))) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("screenshot path resolves outside screenshot directory")); struct stat Existing{}; const FTCHARToUTF8 NativePath(*Path); if (lstat(NativePath.Get(), &Existing) == 0 && S_ISLNK(Existing.st_mode)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("screenshot path cannot be a symlink")); TArray<FColor> Pixels; FIntRect Rect(0, 0, Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y); if (!Viewport->ReadPixels(Pixels, FReadSurfaceDataFlags(), Rect) || Pixels.IsEmpty()) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("PIE viewport readback failed")); TArray64<uint8> Png; FImageUtils::PNGCompressImageArray(Rect.Width(), Rect.Height(), Pixels, Png); if (!FFileHelper::SaveArrayToFile(Png, *Path) || Png.Num() < 8 || Png[0] != 0x89 || Png[1] != 'P' || Png[2] != 'N' || Png[3] != 'G' || !FPaths::FileExists(Path)) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("PNG write or signature verification failed")); const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("sessionId"), PlaySessionId); Result->SetStringField(TEXT("path"), Path); Result->SetNumberField(TEXT("width"), Rect.Width()); Result->SetNumberField(TEXT("height"), Rect.Height()); Result->SetStringField(TEXT("format"), TEXT("png")); Result->SetBoolField(TEXT("changed"), true); Result->SetStringField(TEXT("revision"), Sha256(Path)); return PlayResponse(Id, Result, Operation); }
    return ErrorResponse(Id, TEXT("unsupported"), TEXT("unsupported play operation"));
}

TSharedRef<FJsonObject> LevelSettingsResult(const UWorld& World)
{
    const AWorldSettings* Settings = World.GetWorldSettings(); const UClass* GameMode = Settings ? Settings->DefaultGameMode.Get() : nullptr; const UClass* Pawn = GameMode && GameMode->GetDefaultObject<AGameModeBase>() ? GameMode->GetDefaultObject<AGameModeBase>()->DefaultPawnClass : nullptr;
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("levelId"), World.GetOutermost()->GetName()); Result->SetStringField(TEXT("gameModeClass"), GameMode ? GameMode->GetPathName() : FString()); Result->SetStringField(TEXT("defaultPawnClass"), Pawn ? Pawn->GetPathName() : FString()); Result->SetStringField(TEXT("revision"), LevelSettingsRevision(World)); return Result;
}
FString ReadResponseOnGameThread(const FString& Id, const FString& Operation, const TSharedPtr<FJsonObject>& Args, const FString& ExpectedRevision = FString())
{
    check(IsInGameThread());
    if (Operation == TEXT("editor.status"))
    {
        if (!ClosedArgs(Args, {})) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("editor.status accepts no arguments"));
        UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr; TArray<UPackage*> DirtyPackages; FEditorFileUtils::GetDirtyPackages(DirtyPackages);
        const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("state"), Lifecycle.load() == ELifecycle::Starting ? TEXT("starting") : Lifecycle.load() == ELifecycle::Stopping ? TEXT("stopping") : TEXT("ready")); Result->SetStringField(TEXT("projectId"), ProjectId); Result->SetNumberField(TEXT("editorPid"), FPlatformProcess::GetCurrentProcessId()); Result->SetStringField(TEXT("levelId"), World && World->GetOutermost() ? World->GetOutermost()->GetName() : FString()); Result->SetStringField(TEXT("pie"), GEditor && GEditor->PlayWorld ? TEXT("running") : TEXT("stopped")); Result->SetNumberField(TEXT("dirtyPackageCount"), DirtyPackages.Num()); return SuccessResponse(Id, Result, Operation);
    }

    if (Operation == TEXT("play.start"))

    {
        if (!Args->Values.IsEmpty()) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("play.start accepts no arguments"));
        if (!GUnrealEd) return ErrorResponse(Id, TEXT("unsafe_editor_state"), TEXT("UnrealEd is unavailable"));
        if (PlayState != EPlayState::Stopped || PieWorld() || GUnrealEd->IsPlaySessionRequestQueued()) return ErrorResponse(Id, TEXT("conflict"), TEXT("PIE session is already active or queued"));
        ++PlaySessionCounter; PlaySessionId = FString::Printf(TEXT("m6-pie-%llu"), static_cast<unsigned long long>(PlaySessionCounter)); PlayState = EPlayState::Starting; FRequestPlaySessionParams Params; GUnrealEd->RequestPlaySession(Params);
        if (!GUnrealEd->IsPlaySessionRequestQueued() && !PieWorld()) { PlayState = EPlayState::Stopped; PlaySessionId.Reset(); return ErrorResponse(Id, TEXT("operation_failed"), TEXT("PIE request was not queued or started")); }
        const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("sessionId"), PlaySessionId); Result->SetStringField(TEXT("state"), TEXT("starting")); Result->SetField(TEXT("worldId"), MakeShared<FJsonValueNull>()); Result->SetField(TEXT("levelId"), MakeShared<FJsonValueNull>()); Result->SetBoolField(TEXT("changed"), true); Result->SetStringField(TEXT("revision"), PlayStatusResult()->GetStringField(TEXT("revision"))); return PlayResponse(Id, Result, Operation);
    }
    if (Operation == TEXT("play.stop"))
    {
        FString Error;
        if (!ValidPlaySession(Args, Error)) return ErrorResponse(Id, TEXT("unsafe_editor_state"), *Error);
        if (!GUnrealEd || !PieWorld()) return ErrorResponse(Id, TEXT("unsafe_editor_state"), TEXT("PIE world is not running"));
        GUnrealEd->EndPlayMap();
        if (PieWorld() || GUnrealEd->IsPlayingSessionInEditor()) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("PIE session did not stop"));
        PlayState = EPlayState::Stopped;
        const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetStringField(TEXT("sessionId"), PlaySessionId); Result->SetStringField(TEXT("state"), TEXT("stopped")); Result->SetBoolField(TEXT("changed"), true); Result->SetStringField(TEXT("revision"), PlayStatusResult()->GetStringField(TEXT("revision")));
        return PlayResponse(Id, Result, Operation);
    }
    if (Operation.StartsWith(TEXT("play."))) { const FString Response = ReadPlayResponse(Id, Operation, Args); if (Operation == TEXT("play.status") && PieWorld()) PlayState = EPlayState::Running; return Response; }
    if (IsMutationOperation(Operation) && !IsPlayOperation(Operation))
    {
        FString GateMessage;
        if (!MutationGate(GateMessage)) return ErrorResponse(Id, TEXT("unsafe_editor_state"), *GateMessage);
    }
    UWorld* MutationWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (Operation == TEXT("level.create") || Operation == TEXT("level.open") || Operation == TEXT("level.save"))
    {
        FString Path, Filename, PathError;
        if (!Args->TryGetStringField(TEXT("path"), Path) || !ValidateLevelPath(Path, Filename, PathError)) return ErrorResponse(Id, TEXT("invalid_input"), *PathError);
        UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
        if (!World || !World->GetOutermost()) return ErrorResponse(Id, TEXT("unsafe_editor_state"), TEXT("editor world is unavailable"));
        TArray<UPackage*> DirtyPackages;
        FEditorFileUtils::GetDirtyPackages(DirtyPackages);
        const FString CurrentPath = World->GetOutermost()->GetName();
        if (Operation == TEXT("level.create"))
        {
            if (CurrentPath == Path)
            {
                const TArray<TSharedPtr<FJsonValue>> Dirty = World->GetOutermost()->IsDirty() ? TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueString>(Path)} : TArray<TSharedPtr<FJsonValue>>{};
                return SuccessResponse(Id, LevelMutationResult(Path, false, Dirty, {}), Operation);
            }
            if (FPaths::FileExists(Filename)) return ErrorResponse(Id, TEXT("conflict"), TEXT("level package already exists; create refuses overwrite"));
            if (!DirtyPackages.IsEmpty()) return ErrorResponse(Id, TEXT("unsafe_editor_state"), TEXT("level.create refuses to discard dirty packages"));
            World = FAutomationEditorCommonUtils::CreateNewMap();
            if (!World || !World->PersistentLevel || !World->GetOutermost()) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("new level creation failed"));
            UPackage* Package = World->GetOutermost();
            const FString WorldName = FPackageName::GetLongPackageAssetName(Path);
            if (!Package->Rename(*Path, nullptr, REN_Test) || !World->Rename(*WorldName, nullptr, REN_Test)) return ErrorResponse(Id, TEXT("conflict"), TEXT("level package or world name is already in use"));
            Package->Rename(*Path, nullptr, REN_NonTransactional | REN_DontCreateRedirectors | REN_AllowPackageLinkerMismatch);
            World->Rename(*WorldName, nullptr, REN_NonTransactional | REN_DontCreateRedirectors | REN_AllowPackageLinkerMismatch);
            Package->MarkPackageDirty();
            FAssetRegistryModule::AssetCreated(World);
            if (World->GetOutermost()->GetName() != Path || !World->GetOutermost()->IsDirty() || FPaths::FileExists(Filename)) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("created level identity or dirty unsaved readback failed"));
            return SuccessResponse(Id, LevelMutationResult(Path, true, {MakeShared<FJsonValueString>(Path)}, {}), Operation);

        }
        if (Operation == TEXT("level.open"))
        {
            if (!FPaths::FileExists(Filename)) return ErrorResponse(Id, TEXT("not_found"), TEXT("level package does not exist"));
            if (!DirtyPackages.IsEmpty()) return ErrorResponse(Id, TEXT("unsafe_editor_state"), TEXT("level.open refuses to discard dirty packages"));
            if (CurrentPath == Path) return SuccessResponse(Id, LevelMutationResult(Path, false, {}, {}), Operation);
            if (!FEditorFileUtils::LoadMap(Filename, false, false)) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("level open failed"));
            World = GEditor->GetEditorWorldContext().World();
            if (!World || !World->GetOutermost() || World->GetOutermost()->GetName() != Path) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("opened level identity readback failed"));
            return SuccessResponse(Id, LevelMutationResult(Path, true, {}, {}), Operation);
        }
        if (CurrentPath != Path) return ErrorResponse(Id, TEXT("conflict"), TEXT("level.save path must match current editor world exactly"));
        if (!FPaths::FileExists(Filename) && !World->GetOutermost()->IsDirty()) return ErrorResponse(Id, TEXT("not_found"), TEXT("current level file does not exist"));
        if (World->GetOutermost()->IsDirty())
        {
            if (!FEditorFileUtils::SaveMap(World, Filename) || World->GetOutermost()->IsDirty() || !FPaths::FileExists(Filename)) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("level save failed or package remained dirty"));
            return SuccessResponse(Id, LevelMutationResult(Path, true, {}, {MakeShared<FJsonValueString>(Path)}), Operation);
        }
        return SuccessResponse(Id, LevelMutationResult(Path, false, {}, {}), Operation);
    }
    if ((Operation == TEXT("actor.update_transform") || Operation == TEXT("actor.delete")) && MutationWorld)
    {
        FString Wanted;
        Args->TryGetStringField(TEXT("id"), Wanted);
        AActor* Actor = FindActorById(*MutationWorld, Wanted);
        if (!Actor) return ErrorResponse(Id, TEXT("not_found"), TEXT("actor was not found"));
        const FString CurrentRevision = ActorRevision(*Actor);
        if (ExpectedRevision.IsEmpty() || ExpectedRevision != CurrentRevision) return ErrorResponse(Id, ExpectedRevision.IsEmpty() ? TEXT("invalid_input") : TEXT("conflict"), ExpectedRevision.IsEmpty() ? TEXT("expectedRevision is required") : TEXT("actor revision is stale; re-read actor.view before retrying"));
        if (Operation == TEXT("actor.delete"))
        {
            bool DryRun = false; Args->TryGetBoolField(TEXT("dryRun"), DryRun);
            if (DryRun) { const TSharedRef<FJsonObject> R = MakeShared<FJsonObject>(); R->SetStringField(TEXT("id"), Wanted); R->SetBoolField(TEXT("changed"), false); R->SetBoolField(TEXT("dryRun"), true); R->SetArrayField(TEXT("dirtyPackages"), {}); R->SetArrayField(TEXT("savedPackages"), {}); R->SetStringField(TEXT("revision"), CurrentRevision); return SuccessResponse(Id, R, Operation); }
            bool Force = false; Args->TryGetBoolField(TEXT("force"), Force);
            if (!Force) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("delete requires force"));
            UPackage* OwningPackage = Actor->GetOutermost();
            const FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "DeleteActor", "Magi AXI Delete Actor"));
            Actor->Modify(); if (OwningPackage) OwningPackage->MarkPackageDirty();
            if (!Actor->Destroy() || FindActorById(*MutationWorld, Wanted)) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("actor deletion failed or readback still found actor"));
            const TArray<TSharedPtr<FJsonValue>> Dirty = OwningPackage ? TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueString>(OwningPackage->GetName())} : TArray<TSharedPtr<FJsonValue>>{};
            const TSharedRef<FJsonObject> R = MakeShared<FJsonObject>(); R->SetStringField(TEXT("id"), Wanted); R->SetBoolField(TEXT("changed"), true); R->SetBoolField(TEXT("dryRun"), false); R->SetArrayField(TEXT("dirtyPackages"), Dirty); R->SetArrayField(TEXT("savedPackages"), {}); R->SetStringField(TEXT("revision"), Sha256(Wanted + TEXT("deleted"))); return SuccessResponse(Id, R, Operation);
        }
        FVector Location; FRotator Rotation; FVector Scale; const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (Args->TryGetArrayField(TEXT("location"), Values) && Values->Num() == 3) Location = FVector((*Values)[0]->AsNumber(), (*Values)[1]->AsNumber(), (*Values)[2]->AsNumber()); else Location = Actor->GetActorLocation();
        if (Args->TryGetArrayField(TEXT("rotation"), Values) && Values->Num() == 3) Rotation = FRotator((*Values)[1]->AsNumber(), (*Values)[2]->AsNumber(), (*Values)[0]->AsNumber()); else Rotation = Actor->GetActorRotation();
        if (Args->TryGetArrayField(TEXT("scale"), Values) && Values->Num() == 3) Scale = FVector((*Values)[0]->AsNumber(), (*Values)[1]->AsNumber(), (*Values)[2]->AsNumber()); else Scale = Actor->GetActorScale3D();
        const bool Changed = !Actor->GetActorLocation().Equals(Location) || !Actor->GetActorRotation().Equals(Rotation) || !Actor->GetActorScale3D().Equals(Scale);
        if (Changed) { const FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "UpdateActorTransform", "Magi AXI Update Actor Transform")); Actor->Modify(); Actor->SetActorLocationAndRotation(Location, Rotation); Actor->SetActorScale3D(Scale); Actor->PostEditChange(); Actor->GetOutermost()->MarkPackageDirty(); }
        if (!Actor->GetActorLocation().Equals(Location) || !Actor->GetActorRotation().Equals(Rotation) || !Actor->GetActorScale3D().Equals(Scale)) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("actor transform readback failed"));
        const TArray<TSharedPtr<FJsonValue>> Dirty = Changed && Actor->GetOutermost() ? TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueString>(Actor->GetOutermost()->GetName())} : TArray<TSharedPtr<FJsonValue>>{};
        const TSharedRef<FJsonObject> R = MakeShared<FJsonObject>(); R->SetStringField(TEXT("id"), Wanted); R->SetBoolField(TEXT("changed"), Changed); R->SetArrayField(TEXT("dirtyPackages"), Dirty); R->SetArrayField(TEXT("savedPackages"), {}); R->SetStringField(TEXT("revision"), ActorRevision(*Actor)); return SuccessResponse(Id, R, Operation);
    }
    if (Operation == TEXT("component.list") || Operation == TEXT("component.view") || Operation == TEXT("component.add") || Operation == TEXT("component.update") || Operation == TEXT("component.remove"))
    {
        if (!MutationWorld) return ErrorResponse(Id, TEXT("unsafe_editor_state"), TEXT("editor world is unavailable"));
        FString Wanted; Args->TryGetStringField(TEXT("id"), Wanted);
        AActor* Actor = nullptr;
        UActorComponent* Component = nullptr;
        if (Operation == TEXT("component.list"))
        {
            FString ActorWanted; if (!Args->TryGetStringField(TEXT("actorId"), ActorWanted)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("component.list requires actorId"));
            Actor = FindActorById(*MutationWorld, ActorWanted); if (!Actor) return ErrorResponse(Id, TEXT("not_found"), TEXT("actor was not found"));
            int32 Limit = 100; FString Cursor; TSet<FString> Fields;
            if (!ReadComponentListPageArgs(Args, Limit, Cursor, Fields)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("invalid component.list arguments"));
            TArray<UActorComponent*> Components = Actor->GetComponents().Array();
            Components.RemoveAll([](UActorComponent* Value) { return !IsValid(Value); });
            Components.Sort([](const UActorComponent& Left, const UActorComponent& Right) { return Left.GetName() < Right.GetName(); });
            TArray<FString> Rows; for (UActorComponent* Value : Components) Rows.Add(CanonicalRow({ComponentId(*Actor, *Value), Value->GetName(), Value->GetClass()->GetPathName(), Cast<USceneComponent>(Value) ? TEXT("true") : TEXT("false")}));
            const FString Revision = SnapshotRevision(Operation, ActorWanted, Fields, Rows); int32 Offset = 0;
            if (!CursorOffset(Cursor, Revision, Components.Num(), Offset)) return ErrorResponse(Id, TEXT("stale_cursor"), TEXT("component list cursor is invalid or stale"));
            TArray<TSharedPtr<FJsonValue>> Items;
            for (int32 Index = Offset; Index < FMath::Min(Offset + Limit, Components.Num()); ++Index)
            {
                UActorComponent* Value = Components[Index]; const TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
                SetIfSelected(Item, Fields, TEXT("id"), ComponentId(*Actor, *Value)); SetIfSelected(Item, Fields, TEXT("name"), Value->GetName()); SetIfSelected(Item, Fields, TEXT("class"), Value->GetClass()->GetPathName());
                if (Fields.IsEmpty() || Fields.Contains(TEXT("scene"))) Item->SetBoolField(TEXT("scene"), Cast<USceneComponent>(Value) != nullptr); Items.Add(MakeShared<FJsonValueObject>(Item));
            }
            return SuccessResponse(Id, PageResult(ActorWanted, Items, Components.Num(), Offset, Revision), Operation);
        }
        if (Operation == TEXT("component.add"))
        {
            FString ActorWanted, ClassPath, Name; if (!Args->TryGetStringField(TEXT("actorId"), ActorWanted) || !Args->TryGetStringField(TEXT("class"), ClassPath) || !Args->TryGetStringField(TEXT("name"), Name)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("component.add requires actorId, class, and name"));
            Actor = FindActorById(*MutationWorld, ActorWanted); if (!Actor) return ErrorResponse(Id, TEXT("not_found"), TEXT("actor was not found")); if (ExpectedRevision.IsEmpty() || ExpectedRevision != ActorRevision(*Actor)) return ErrorResponse(Id, ExpectedRevision.IsEmpty() ? TEXT("invalid_input") : TEXT("conflict"), TEXT("actor revision is stale; re-read actor.view before retrying"));
            UClass* Class = LoadObject<UClass>(nullptr, *ClassPath); if (!Class || !Class->IsChildOf(UActorComponent::StaticClass()) || Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("class is not a concrete UActorComponent class"));
            FVector Location = FVector::ZeroVector; const TArray<TSharedPtr<FJsonValue>>* Values = nullptr; if (Args->TryGetArrayField(TEXT("location"), Values) && Values->Num() == 3) Location = FVector((*Values)[0]->AsNumber(), (*Values)[1]->AsNumber(), (*Values)[2]->AsNumber());
            for (UActorComponent* Existing : Actor->GetComponents().Array()) if (IsValid(Existing) && Existing->GetName() == Name) { if (Existing->GetClass() != Class || (Cast<USceneComponent>(Existing) && !Cast<USceneComponent>(Existing)->GetRelativeLocation().Equals(Location))) return ErrorResponse(Id, TEXT("conflict"), TEXT("component name already exists with different intent")); const TSharedRef<FJsonObject> R = MakeShared<FJsonObject>(); R->SetStringField(TEXT("id"), ComponentId(*Actor, *Existing)); R->SetBoolField(TEXT("changed"), false); R->SetArrayField(TEXT("dirtyPackages"), {}); R->SetArrayField(TEXT("savedPackages"), {}); R->SetStringField(TEXT("revision"), ComponentRevision(*Actor, *Existing)); return SuccessResponse(Id, R, Operation); }
            const FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "AddComponent", "Magi AXI Add Component")); Actor->Modify(); UActorComponent* NewComponent = NewObject<UActorComponent>(Actor, Class, FName(*Name), RF_Transactional); if (!NewComponent) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("component construction failed")); Actor->AddInstanceComponent(NewComponent); NewComponent->OnComponentCreated();
            if (USceneComponent* Scene = Cast<USceneComponent>(NewComponent)) { Scene->AttachToComponent(Actor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform); Scene->SetRelativeLocation(Location); }
            NewComponent->RegisterComponent(); Actor->GetOutermost()->MarkPackageDirty();
            const TSharedRef<FJsonObject> R = MakeShared<FJsonObject>(); R->SetStringField(TEXT("id"), ComponentId(*Actor, *NewComponent)); R->SetBoolField(TEXT("changed"), true); R->SetArrayField(TEXT("dirtyPackages"), {MakeShared<FJsonValueString>(Actor->GetOutermost()->GetName())}); R->SetArrayField(TEXT("savedPackages"), {}); R->SetStringField(TEXT("revision"), ComponentRevision(*Actor, *NewComponent)); return SuccessResponse(Id, R, Operation);
        }
        Component = FindComponentById(*MutationWorld, Wanted, Actor); if (!Component) return ErrorResponse(Id, TEXT("not_found"), TEXT("component was not found"));
        const FString CurrentRevision = ComponentRevision(*Actor, *Component);
        if (Operation == TEXT("component.view"))
        {
            const TSharedRef<FJsonObject> R = MakeShared<FJsonObject>(); R->SetStringField(TEXT("id"), Wanted); R->SetStringField(TEXT("actorId"), ActorId(*Actor)); R->SetStringField(TEXT("name"), Component->GetName()); R->SetStringField(TEXT("class"), Component->GetClass()->GetPathName()); R->SetBoolField(TEXT("scene"), Cast<USceneComponent>(Component) != nullptr); if (const USceneComponent* Scene = Cast<USceneComponent>(Component)) AddComponentTransformFields(R, *Scene); else { R->SetArrayField(TEXT("location"), {MakeShared<FJsonValueNumber>(0), MakeShared<FJsonValueNumber>(0), MakeShared<FJsonValueNumber>(0)}); R->SetArrayField(TEXT("rotation"), {MakeShared<FJsonValueNumber>(0), MakeShared<FJsonValueNumber>(0), MakeShared<FJsonValueNumber>(0)}); R->SetArrayField(TEXT("scale"), {MakeShared<FJsonValueNumber>(1), MakeShared<FJsonValueNumber>(1), MakeShared<FJsonValueNumber>(1)}); } R->SetStringField(TEXT("revision"), CurrentRevision); return SuccessResponse(Id, R, Operation);
        }
        bool DryRun = false; Args->TryGetBoolField(TEXT("dryRun"), DryRun); if (!DryRun && (ExpectedRevision.IsEmpty() || ExpectedRevision != CurrentRevision)) return ErrorResponse(Id, ExpectedRevision.IsEmpty() ? TEXT("invalid_input") : TEXT("conflict"), TEXT("component revision is stale; re-read component.view before retrying"));
        if (Operation == TEXT("component.remove")) { bool Force = false; Args->TryGetBoolField(TEXT("force"), Force); if (!DryRun && !Force) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("remove requires force")); if (Component->CreationMethod != EComponentCreationMethod::Instance || Component == Actor->GetRootComponent() || Component->IsDefaultSubobject() || Component->IsTemplate()) return ErrorResponse(Id, TEXT("conflict"), TEXT("native, default, root, inherited, and template components cannot be removed")); if (DryRun) { const TSharedRef<FJsonObject> R = MakeShared<FJsonObject>(); R->SetStringField(TEXT("id"), Wanted); R->SetBoolField(TEXT("changed"), false); R->SetBoolField(TEXT("dryRun"), true); R->SetArrayField(TEXT("dirtyPackages"), {}); R->SetArrayField(TEXT("savedPackages"), {}); R->SetStringField(TEXT("revision"), CurrentRevision); return SuccessResponse(Id, R, Operation); } AActor* OwningActor = Actor; const FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "RemoveComponent", "Magi AXI Remove Component")); Component->Modify(); Component->DestroyComponent(); OwningActor->GetOutermost()->MarkPackageDirty(); AActor* FoundOwner = nullptr; if (FindComponentById(*MutationWorld, Wanted, FoundOwner)) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("component removal readback still found component")); const TSharedRef<FJsonObject> R = MakeShared<FJsonObject>(); R->SetStringField(TEXT("id"), Wanted); R->SetBoolField(TEXT("changed"), true); R->SetBoolField(TEXT("dryRun"), false); R->SetArrayField(TEXT("dirtyPackages"), {MakeShared<FJsonValueString>(OwningActor->GetOutermost()->GetName())}); R->SetArrayField(TEXT("savedPackages"), {}); R->SetStringField(TEXT("revision"), Sha256(Wanted + TEXT("deleted"))); return SuccessResponse(Id, R, Operation); }
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr; if (!Cast<USceneComponent>(Component)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("component.update requires a scene component")); if (!Args->TryGetArrayField(TEXT("location"), Values) || Values->Num() != 3) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("location is required")); const FVector Location((*Values)[0]->AsNumber(), (*Values)[1]->AsNumber(), (*Values)[2]->AsNumber()); USceneComponent* Scene = CastChecked<USceneComponent>(Component); const bool Changed = !Scene->GetRelativeLocation().Equals(Location); if (Changed) { const FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "UpdateComponent", "Magi AXI Update Component")); Scene->Modify(); Scene->SetRelativeLocation(Location); Scene->PostEditComponentMove(true); Actor->GetOutermost()->MarkPackageDirty(); } if (!Scene->GetRelativeLocation().Equals(Location)) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("component transform readback failed")); const TSharedRef<FJsonObject> R = MakeShared<FJsonObject>(); R->SetStringField(TEXT("id"), Wanted); R->SetBoolField(TEXT("changed"), Changed); R->SetArrayField(TEXT("dirtyPackages"), Changed ? TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueString>(Actor->GetOutermost()->GetName())} : TArray<TSharedPtr<FJsonValue>>{}); R->SetArrayField(TEXT("savedPackages"), {}); R->SetStringField(TEXT("revision"), ComponentRevision(*Actor, *Component)); return SuccessResponse(Id, R, Operation);
    }
    if (Operation == TEXT("level.settings") || Operation == TEXT("level.set_game_mode"))
    {
        UWorld* World = MutationWorld; if (!World || !World->GetOutermost()) return ErrorResponse(Id, TEXT("unsafe_editor_state"), TEXT("editor world is unavailable")); FString LevelId; Args->TryGetStringField(TEXT("levelId"), LevelId); if (!LevelId.IsEmpty() && LevelId != World->GetOutermost()->GetName()) return ErrorResponse(Id, TEXT("conflict"), TEXT("levelId is not current editor level"));
        if (Operation == TEXT("level.settings")) return SuccessResponse(Id, LevelSettingsResult(*World), Operation);
        FString ClassPath; if (!Args->TryGetStringField(TEXT("gameModeClass"), ClassPath)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("gameModeClass is required")); UClass* GameMode = LoadObject<UClass>(nullptr, *ClassPath); if (!GameMode || !GameMode->IsChildOf(AGameModeBase::StaticClass()) || GameMode->HasAnyClassFlags(CLASS_Abstract)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("gameModeClass must be concrete AGameModeBase")); const FString CurrentRevision = LevelSettingsRevision(*World); if (ExpectedRevision.IsEmpty() || ExpectedRevision != CurrentRevision) return ErrorResponse(Id, ExpectedRevision.IsEmpty() ? TEXT("invalid_input") : TEXT("conflict"), TEXT("level settings revision is stale; re-read level.settings before retrying")); AWorldSettings* Settings = World->GetWorldSettings(); const bool Changed = Settings->DefaultGameMode != GameMode; if (Changed) { const FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "SetGameMode", "Magi AXI Set Game Mode")); Settings->Modify(); Settings->DefaultGameMode = GameMode; Settings->PostEditChange(); Settings->GetOutermost()->MarkPackageDirty(); } const TSharedRef<FJsonObject> R = LevelSettingsResult(*World); R->SetBoolField(TEXT("changed"), Changed); R->SetArrayField(TEXT("dirtyPackages"), Changed ? TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueString>(World->GetOutermost()->GetName())} : TArray<TSharedPtr<FJsonValue>>{}); R->SetArrayField(TEXT("savedPackages"), {}); return SuccessResponse(Id, R, Operation);
    }
    if (Operation == TEXT("level.current"))
    {
        if (!ClosedArgs(Args, {})) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("level.current accepts no arguments"));
        UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
        if (!World || !World->GetOutermost()) return ErrorResponse(Id, TEXT("unsafe_editor_state"), TEXT("editor world is unavailable"));
        const FString LevelId = World->GetOutermost()->GetName();
        const TSharedRef<FJsonObject> Level = MakeShared<FJsonObject>();
        Level->SetStringField(TEXT("id"), LevelId);
        Level->SetStringField(TEXT("name"), FPackageName::GetShortName(LevelId));
        Level->SetStringField(TEXT("worldType"), TEXT("editor"));
        Level->SetBoolField(TEXT("persistent"), true);
        Level->SetStringField(TEXT("revision"), LevelContentRevision(*World));
        const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetObjectField(TEXT("level"), Level);
        Result->SetStringField(TEXT("scope"), ProjectId);
        return SuccessResponse(Id, Result, Operation);
    }
    if (Operation == TEXT("operation.view"))
    {
        FString Wanted; if (!Args->TryGetStringField(TEXT("id"), Wanted)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("operation.view requires id"));
        FScopeLock Lock(&LedgerMutex); PruneLedger(); FLedgerRecord* Record = FindLedger(Wanted);
        if (!Record) return ErrorResponse(Id, TEXT("not_found"), TEXT("operation was not found"));
        TSharedPtr<FJsonObject> Stored;
        if (!Record->Response.IsEmpty()) { TArray<uint8> Bytes; FTCHARToUTF8 Utf8(*Record->Response); Bytes.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length()); ParseObject(Bytes, Stored); }
        if (Stored.IsValid()) { const TSharedPtr<FJsonObject>* Receipt = nullptr; if (Stored->TryGetObjectField(TEXT("receipt"), Receipt) && Receipt && Receipt->IsValid()) return SuccessResponse(Id, Receipt->ToSharedRef(), Operation); }
        TSharedPtr<FJsonObject> Pending;
        const FString PendingJson = MakeQueuedReceipt(*Record);
        TArray<uint8> Bytes; FTCHARToUTF8 Utf8(*PendingJson); Bytes.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
        if (!ParseObject(Bytes, Pending)) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("operation receipt is malformed"));
        return SuccessResponse(Id, Pending.ToSharedRef(), Operation);
    }
    if (Operation == TEXT("actor.list"))
    {
        int32 Limit = 0;
        FString Cursor;
        TSet<FString> Fields;
        if (!ReadPageArgs(Args, {TEXT("id"), TEXT("label"), TEXT("class"), TEXT("levelId")}, Limit, Cursor, Fields)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("invalid actor.list arguments"));
        UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
        if (!World || !World->GetOutermost()) return ErrorResponse(Id, TEXT("unsafe_editor_state"), TEXT("editor world is unavailable"));
        const FString LevelId = World->GetOutermost()->GetName();
        TArray<AActor*> Actors = StableEditorActors(*World);
        TArray<FString> Ids;
        TArray<FString> RevisionRows;
        for (const AActor* Actor : Actors)
        {
            const FString ActorIdentity = ActorId(*Actor);
            Ids.Add(ActorIdentity);
            RevisionRows.Add(CanonicalRow({ActorIdentity, Actor->GetActorLabel(), Actor->GetClass()->GetPathName(), ActorLevelId(*Actor)}));
        }
        const FString Revision = SnapshotRevision(Operation, LevelId, Fields, RevisionRows);
        int32 Offset = 0;
        if (!CursorOffset(Cursor, Revision, Actors.Num(), Offset)) return ErrorResponse(Id, TEXT("stale_cursor"), TEXT("actor list cursor is invalid or stale"));
        TArray<TSharedPtr<FJsonValue>> Items;
        for (int32 Index = Offset; Index < FMath::Min(Offset + Limit, Actors.Num()); ++Index)
        {
            const AActor* Actor = Actors[Index];
            const TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
            SetIfSelected(Item, Fields, TEXT("id"), Ids[Index]);
            SetIfSelected(Item, Fields, TEXT("label"), Actor->GetActorLabel());
            SetIfSelected(Item, Fields, TEXT("class"), Actor->GetClass()->GetPathName());
            SetIfSelected(Item, Fields, TEXT("levelId"), ActorLevelId(*Actor));
            Items.Add(MakeShared<FJsonValueObject>(Item));
        }
        return SuccessResponse(Id, PageResult(LevelId, Items, Actors.Num(), Offset, Revision), Operation);
    }
    if (Operation == TEXT("level.list"))
    {
        int32 Limit = 0;
        FString Cursor;
        TSet<FString> Fields;
        if (!ReadPageArgs(Args, {TEXT("id"), TEXT("name"), TEXT("worldType"), TEXT("persistent")}, Limit, Cursor, Fields)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("invalid level.list arguments"));
        FARFilter Filter;
        Filter.PackagePaths.Add(FName(TEXT("/Game")));
        Filter.ClassPaths.Add(UWorld::StaticClass()->GetClassPathName());
        Filter.bRecursivePaths = true;
        TArray<FAssetData> Worlds;
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get().GetAssets(Filter, Worlds);
        Worlds.Sort([](const FAssetData& Left, const FAssetData& Right) { return Left.PackageName.LexicalLess(Right.PackageName); });
        TArray<FString> Ids;
        TArray<FString> RevisionRows;
        for (const FAssetData& WorldAsset : Worlds)
        {
            const FString Identity = WorldAsset.PackageName.ToString();
            Ids.Add(Identity);
            RevisionRows.Add(CanonicalRow({Identity, WorldAsset.AssetName.ToString(), TEXT("editor"), TEXT("true")}));
        }
        const FString Scope = TEXT("/Game");
        const FString Revision = SnapshotRevision(Operation, Scope, Fields, RevisionRows);
        int32 Offset = 0;
        if (!CursorOffset(Cursor, Revision, Worlds.Num(), Offset)) return ErrorResponse(Id, TEXT("stale_cursor"), TEXT("level list cursor is invalid or stale"));
        TArray<TSharedPtr<FJsonValue>> Items;
        for (int32 Index = Offset; Index < FMath::Min(Offset + Limit, Worlds.Num()); ++Index)
        {
            const TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
            SetIfSelected(Item, Fields, TEXT("id"), Ids[Index]);
            SetIfSelected(Item, Fields, TEXT("name"), Worlds[Index].AssetName.ToString());
            SetIfSelected(Item, Fields, TEXT("worldType"), TEXT("editor"));
            if (Fields.IsEmpty() || Fields.Contains(TEXT("persistent"))) Item->SetBoolField(TEXT("persistent"), true);
            Items.Add(MakeShared<FJsonValueObject>(Item));
        }
        return SuccessResponse(Id, PageResult(Scope, Items, Worlds.Num(), Offset, Revision), Operation);
    }
    if (Operation == TEXT("asset.list"))
    {
        int32 Limit = 0;
        FString Cursor;
        TSet<FString> Fields;
        if (!ReadPageArgs(Args, {TEXT("id"), TEXT("name"), TEXT("class"), TEXT("packagePath")}, Limit, Cursor, Fields)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("invalid asset.list arguments"));
        FARFilter Filter;
        Filter.PackagePaths.Add(FName(TEXT("/Game")));
        Filter.bRecursivePaths = true;
        TArray<FAssetData> Assets;
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get().GetAssets(Filter, Assets);
        Assets.Sort([](const FAssetData& Left, const FAssetData& Right) { return Left.GetObjectPathString() < Right.GetObjectPathString(); });
        TArray<FString> Ids;
        TArray<FString> RevisionRows;
        for (const FAssetData& Asset : Assets)
        {
            const FString Identity = Asset.GetObjectPathString();
            Ids.Add(Identity);
            RevisionRows.Add(CanonicalRow({Identity, Asset.AssetName.ToString(), Asset.AssetClassPath.ToString(), Asset.PackageName.ToString()}));
        }
        const FString Scope = TEXT("/Game");
        const FString Revision = SnapshotRevision(Operation, Scope, Fields, RevisionRows);
        int32 Offset = 0;
        if (!CursorOffset(Cursor, Revision, Assets.Num(), Offset)) return ErrorResponse(Id, TEXT("stale_cursor"), TEXT("asset list cursor is invalid or stale"));
        TArray<TSharedPtr<FJsonValue>> Items;
        for (int32 Index = Offset; Index < FMath::Min(Offset + Limit, Assets.Num()); ++Index)
        {
            const FAssetData& Asset = Assets[Index];
            const TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
            SetIfSelected(Item, Fields, TEXT("id"), Ids[Index]);
            SetIfSelected(Item, Fields, TEXT("name"), Asset.AssetName.ToString());
            SetIfSelected(Item, Fields, TEXT("class"), Asset.AssetClassPath.ToString());
            SetIfSelected(Item, Fields, TEXT("packagePath"), Asset.PackageName.ToString());
            Items.Add(MakeShared<FJsonValueObject>(Item));
        }
        return SuccessResponse(Id, PageResult(Scope, Items, Assets.Num(), Offset, Revision), Operation);
    }
    if (Operation == TEXT("actor.spawn"))
    {
        FString LevelId, ClassPath, AgentKey;
        if (!Args->TryGetStringField(TEXT("levelId"), LevelId) || !Args->TryGetStringField(TEXT("class"), ClassPath) || !Args->TryGetStringField(TEXT("agentKey"), AgentKey)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("actor.spawn requires levelId, class, and agentKey"));
        UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
        if (!World || !World->GetOutermost() || World->GetOutermost()->GetName() != LevelId) return ErrorResponse(Id, TEXT("conflict"), TEXT("levelId is not current editor level"));
        ULevel* Level = World->PersistentLevel;
        if (!Level) return ErrorResponse(Id, TEXT("unsafe_editor_state"), TEXT("current level is unavailable"));
        const FName AgentTag(*FString::Printf(TEXT("MagiAxi.AgentKey.sha256:%s"), *Sha256(AgentKey)));
        for (AActor* Existing : Level->Actors)
        {
            if (!IsValid(Existing) || !Existing->Tags.Contains(AgentTag)) continue;
            if (Existing->GetClass()->GetPathName() != ClassPath) return ErrorResponse(Id, TEXT("conflict"), TEXT("agentKey already belongs to a different actor intent"));
            FString Label; if (Args->TryGetStringField(TEXT("label"), Label) && Existing->GetActorLabel() != Label) return ErrorResponse(Id, TEXT("conflict"), TEXT("agentKey already belongs to a different actor intent"));
            const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
            Result->SetStringField(TEXT("id"), ActorId(*Existing)); Result->SetStringField(TEXT("actorGuid"), Existing->GetActorGuid().ToString(EGuidFormats::DigitsWithHyphens)); Result->SetBoolField(TEXT("changed"), false);
            Result->SetArrayField(TEXT("dirtyPackages"), {}); Result->SetArrayField(TEXT("savedPackages"), {}); Result->SetStringField(TEXT("revision"), ActorRevision(*Existing));
            return SuccessResponse(Id, Result, Operation);
        }
        UClass* ActorClass = LoadObject<UClass>(nullptr, *ClassPath);
        if (!ActorClass || !ActorClass->IsChildOf(AActor::StaticClass())) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("class is not a concrete AActor class"));
        FVector Location = FVector::ZeroVector; FRotator Rotation = FRotator::ZeroRotator; FVector Scale = FVector::OneVector;
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (Args->TryGetArrayField(TEXT("location"), Values) && Values->Num() == 3) Location = FVector((*Values)[0]->AsNumber(), (*Values)[1]->AsNumber(), (*Values)[2]->AsNumber());
        if (Args->TryGetArrayField(TEXT("rotation"), Values) && Values->Num() == 3) Rotation = FRotator((*Values)[1]->AsNumber(), (*Values)[2]->AsNumber(), (*Values)[0]->AsNumber());
        if (Args->TryGetArrayField(TEXT("scale"), Values) && Values->Num() == 3) Scale = FVector((*Values)[0]->AsNumber(), (*Values)[1]->AsNumber(), (*Values)[2]->AsNumber());
        FActorSpawnParameters Parameters; Parameters.OverrideLevel = Level; Parameters.ObjectFlags |= RF_Transactional;
        const FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "SpawnActor", "Magi AXI Spawn Actor"));
        Level->Modify();
        AActor* Actor = World->SpawnActor<AActor>(ActorClass, Location, Rotation, Parameters);
        if (!Actor) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("actor spawn failed"));
        Actor->Modify(); Actor->SetActorScale3D(Scale); Actor->Tags.Add(AgentTag);
        FString Label; if (Args->TryGetStringField(TEXT("label"), Label)) Actor->SetActorLabel(Label);
        Actor->PostEditChange(); Level->MarkPackageDirty();
        const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetStringField(TEXT("id"), ActorId(*Actor)); Result->SetStringField(TEXT("actorGuid"), Actor->GetActorGuid().ToString(EGuidFormats::DigitsWithHyphens)); Result->SetBoolField(TEXT("changed"), true);
        Result->SetArrayField(TEXT("dirtyPackages"), {MakeShared<FJsonValueString>(Level->GetOutermost()->GetName())}); Result->SetArrayField(TEXT("savedPackages"), {}); Result->SetStringField(TEXT("revision"), ActorRevision(*Actor));
        return SuccessResponse(Id, Result, Operation);
    }
    if (Operation == TEXT("actor.view"))
    {
        FString Wanted; if (!StringField(Args, TEXT("id"), Wanted)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("actor id is invalid")); UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr; if (!World) return ErrorResponse(Id, TEXT("unsafe_editor_state"), TEXT("editor world is unavailable"));
        for (AActor* Actor : StableEditorActors(*World)) if (Wanted == ActorId(*Actor)) { const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("id"), Wanted); Result->SetStringField(TEXT("actorGuid"), Actor->GetActorGuid().ToString(EGuidFormats::DigitsWithHyphens)); Result->SetStringField(TEXT("levelId"), ActorLevelId(*Actor)); Result->SetStringField(TEXT("label"), Actor->GetActorLabel()); Result->SetStringField(TEXT("class"), Actor->GetClass()->GetPathName()); Result->SetStringField(TEXT("objectPath"), Actor->GetPathName()); AddTransformFields(Result, *Actor); Result->SetStringField(TEXT("revision"), ActorRevision(*Actor)); return SuccessResponse(Id, Result, Operation); }
        return ErrorResponse(Id, TEXT("not_found"), TEXT("actor was not found"));
    }
    if (Operation == TEXT("asset.create_input_action") || Operation == TEXT("asset.create_input_mapping_context") || Operation == TEXT("asset.save"))
    {
        FString Wanted, Filename, PathError; Args->TryGetStringField(TEXT("path"), Wanted); if (Operation == TEXT("asset.save")) Args->TryGetStringField(TEXT("id"), Wanted);
        if (Operation != TEXT("asset.save") && !ValidateAssetPath(Wanted, Filename, PathError)) return ErrorResponse(Id, TEXT("invalid_input"), *PathError);
        if (Operation == TEXT("asset.save")) { FString PackagePath = FPackageName::ObjectPathToPackageName(Wanted); if (PackagePath.IsEmpty()) PackagePath = Wanted; if (!ValidateAssetPath(PackagePath, Filename, PathError)) return ErrorResponse(Id, TEXT("invalid_input"), *PathError); Wanted = AssetObjectId(PackagePath); }
        UObject* Existing = LoadObject<UObject>(nullptr, *Wanted);
        if (Operation == TEXT("asset.save"))
        {
            if (!Existing) return ErrorResponse(Id, TEXT("not_found"), TEXT("asset was not found"));
            const FString CurrentRevision = ObjectContentRevision(Existing);
            if (ExpectedRevision.IsEmpty() || ExpectedRevision != CurrentRevision) return ErrorResponse(Id, ExpectedRevision.IsEmpty() ? TEXT("invalid_input") : TEXT("conflict"), TEXT("asset revision is stale; re-read asset.view before retrying"));
        }
        if (Operation == TEXT("asset.create_input_action")) { FString ValueType; Args->TryGetStringField(TEXT("valueType"), ValueType); UInputAction* Action = Cast<UInputAction>(Existing); if (Existing && !Action) return ErrorResponse(Id, TEXT("conflict"), TEXT("asset path belongs to a different class")); if (!Action) { UPackage* Package = CreatePackage(*FPackageName::ObjectPathToPackageName(Wanted)); Action = NewObject<UInputAction>(Package, *FPackageName::GetShortName(Wanted), RF_Public | RF_Standalone); Action->ValueType = ValueType == TEXT("Boolean") ? EInputActionValueType::Boolean : EInputActionValueType::Axis1D; FAssetRegistryModule::AssetCreated(Action); Package->MarkPackageDirty(); } else if ((ValueType == TEXT("Boolean") && Action->ValueType != EInputActionValueType::Boolean) || (ValueType == TEXT("Axis1D") && Action->ValueType != EInputActionValueType::Axis1D)) return ErrorResponse(Id, TEXT("conflict"), TEXT("existing input action has different valueType")); const FString PackagePath = Action->GetOutermost()->GetName(); return SuccessResponse(Id, AssetMutationResult(Action->GetPathName(), TEXT("/Script/EnhancedInput.InputAction"), ValueType, -1, Existing == nullptr, Existing ? TArray<TSharedPtr<FJsonValue>>{} : TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueString>(PackagePath)}, {}, ObjectContentRevision(Action)), Operation); }
        if (Operation == TEXT("asset.create_input_mapping_context"))
        {
            UInputMappingContext* Context = Cast<UInputMappingContext>(Existing);
            if (Existing && !Context) return ErrorResponse(Id, TEXT("conflict"), TEXT("asset path belongs to a different class"));
            const TArray<TSharedPtr<FJsonValue>>* Mappings = nullptr;
            if (!Args->TryGetArrayField(TEXT("mappings"), Mappings)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("mappings are required"));
            TArray<FString> Requested;
            TArray<TPair<UInputAction*, FKey>> Resolved;
            for (const TSharedPtr<FJsonValue>& Value : *Mappings)
            {
                const TSharedPtr<FJsonObject> Mapping = Value->AsObject(); FString ActionId, Key;
                if (!Mapping.IsValid() || !Mapping->TryGetStringField(TEXT("actionId"), ActionId) || !Mapping->TryGetStringField(TEXT("key"), Key)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("mapping requires actionId and key"));
                UInputAction* Action = LoadObject<UInputAction>(nullptr, *ActionId); if (!Action) return ErrorResponse(Id, TEXT("not_found"), TEXT("mapping action was not found"));
                Requested.Add(Action->GetPathName() + TEXT("\n") + FKey(FName(*Key)).ToString()); Resolved.Add({Action, FKey(FName(*Key))});
            }
            Requested.Sort();
            TArray<FString> ExistingNormalized;
            if (Context) for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings()) if (Mapping.Action) ExistingNormalized.Add(Mapping.Action->GetPathName() + TEXT("\n") + Mapping.Key.ToString());
            ExistingNormalized.Sort();
            if (Context && ExistingNormalized != Requested)
            {
                if (!ExistingNormalized.IsEmpty()) return ErrorResponse(Id, TEXT("conflict"), TEXT("mapping context already contains different mappings"));
            }
            if (!Context)
            {
                UPackage* Package = CreatePackage(*FPackageName::ObjectPathToPackageName(Wanted)); Context = NewObject<UInputMappingContext>(Package, *FPackageName::GetShortName(Wanted), RF_Public | RF_Standalone); FAssetRegistryModule::AssetCreated(Context);
            }
            if (ExistingNormalized == Requested) return SuccessResponse(Id, AssetMutationResult(Context->GetPathName(), TEXT("/Script/EnhancedInput.InputMappingContext"), FString(), Context->GetMappings().Num(), false, {}, {}, ObjectContentRevision(Context)), Operation);
            for (const TPair<UInputAction*, FKey>& Mapping : Resolved) Context->MapKey(Mapping.Key, Mapping.Value);
            Context->MarkPackageDirty(); const FString PackagePath = Context->GetOutermost()->GetName();
            return SuccessResponse(Id, AssetMutationResult(Context->GetPathName(), TEXT("/Script/EnhancedInput.InputMappingContext"), FString(), Context->GetMappings().Num(), true, {MakeShared<FJsonValueString>(PackagePath)}, {}, ObjectContentRevision(Context)), Operation);
        }
        UObject* Asset = Existing; if (!Asset) return ErrorResponse(Id, TEXT("not_found"), TEXT("asset was not found")); UPackage* Package = Asset->GetOutermost(); if (!Package->IsDirty()) return SuccessResponse(Id, AssetMutationResult(Asset->GetPathName(), Asset->GetClass()->GetPathName(), FString(), -1, false, {}, {}, ObjectContentRevision(Asset)), Operation); FSavePackageArgs SaveArgs; SaveArgs.TopLevelFlags = RF_Public | RF_Standalone; if (!UPackage::SavePackage(Package, Asset, *Filename, SaveArgs) || !FPaths::FileExists(Filename) || Package->IsDirty()) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("asset save failed or package remained dirty")); return SuccessResponse(Id, AssetMutationResult(Asset->GetPathName(), Asset->GetClass()->GetPathName(), FString(), -1, true, {}, {MakeShared<FJsonValueString>(Package->GetName())}, ObjectContentRevision(Asset)), Operation);
    }
    if (Operation == TEXT("blueprint.view") || Operation == TEXT("blueprint.compile"))
    {
        FString Wanted; if (!Args->TryGetStringField(TEXT("id"), Wanted) || Wanted.IsEmpty()) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("Blueprint id is required")); UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *Wanted); if (!Blueprint) return ErrorResponse(Id, TEXT("not_found"), TEXT("Blueprint was not found"));
        if (Operation == TEXT("blueprint.view")) return SuccessResponse(Id, BlueprintResult(*Blueprint), Operation);
        const FString BeforeRevision = BlueprintContentRevision(*Blueprint);
        if (ExpectedRevision.IsEmpty() || ExpectedRevision != BeforeRevision) return ErrorResponse(Id, ExpectedRevision.IsEmpty() ? TEXT("invalid_input") : TEXT("conflict"), TEXT("Blueprint revision is stale; re-read blueprint.view before retrying"));
        FCompilerResultsLog Results; Results.bSilentMode = true; FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave, &Results);
        if (Blueprint->Status == BS_Error || Results.NumErrors > 0) return BlueprintCompileErrorResponse(Id, *Blueprint, Results);
        const FString AfterRevision = BlueprintContentRevision(*Blueprint);
        const TSharedRef<FJsonObject> Result = BlueprintResult(*Blueprint, true); Result->SetBoolField(TEXT("changed"), BeforeRevision != AfterRevision); Result->SetArrayField(TEXT("dirtyPackages"), Blueprint->GetOutermost() && Blueprint->GetOutermost()->IsDirty() ? TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueString>(Blueprint->GetOutermost()->GetName())} : TArray<TSharedPtr<FJsonValue>>{}); Result->SetArrayField(TEXT("diagnostics"), BlueprintDiagnostics(*Blueprint, Results)); Result->SetNumberField(TEXT("errorCount"), FMath::Clamp(Results.NumErrors, 0, 100)); Result->SetNumberField(TEXT("warningCount"), FMath::Clamp(Results.NumWarnings, 0, 100)); return SuccessResponse(Id, Result, Operation);
    }
    if (Operation == TEXT("asset.view"))
    {
        if (!ClosedArgs(Args, {TEXT("id")})) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("asset.view requires only id"));
        FString Wanted; if (!StringField(Args, TEXT("id"), Wanted) || Wanted.Len() > 512 || !Wanted.StartsWith(TEXT("/Game/"))) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("asset id is invalid"));
        UObject* Object = LoadObject<UObject>(nullptr, *Wanted); if (!Object) return ErrorResponse(Id, TEXT("not_found"), TEXT("asset was not found")); const FAssetData Asset(Object);
        const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("id"), Object->GetPathName()); Result->SetStringField(TEXT("packagePath"), Object->GetOutermost()->GetName()); Result->SetStringField(TEXT("objectPath"), Object->GetPathName()); Result->SetStringField(TEXT("name"), Object->GetName()); Result->SetStringField(TEXT("class"), Object->GetClass()->GetPathName()); Result->SetStringField(TEXT("revision"), ObjectContentRevision(Object));
        return SuccessResponse(Id, Result, Operation);
    }
    return ErrorResponse(Id, TEXT("unsupported"), TEXT("operation is not supported"));
}

bool AuthenticationAllowedAt(const double Now)
{
    std::lock_guard Lock(AuthenticationMutex);
    if (FailedHandshakeWindowStart == 0.0 || Now - FailedHandshakeWindowStart >= FailedHandshakeWindowSeconds)
    {
        FailedHandshakeWindowStart = Now;
        FailedHandshakes = 0;
    }
    return FailedHandshakes < MaxFailedHandshakes;
}

void RecordAuthenticationFailureAt(const double Now)
{
    std::lock_guard Lock(AuthenticationMutex);
    if (FailedHandshakeWindowStart == 0.0 || Now - FailedHandshakeWindowStart >= FailedHandshakeWindowSeconds)
    {
        FailedHandshakeWindowStart = Now;
        FailedHandshakes = 0;
    }
    ++FailedHandshakes;
}

void ResetAuthenticationFailures()
{
    std::lock_guard Lock(AuthenticationMutex);
    FailedHandshakeWindowStart = 0.0;
    FailedHandshakes = 0;
}

bool Authenticate(const TSharedPtr<FJsonObject>& Object)
{
    int64 Protocol = 0, Pid = 0;
    FString Kind, SuppliedToken, SuppliedProject, SuppliedNonce, SuppliedProcessStart, ClientVersion;
    return IntegerField(Object, TEXT("protocol"), Protocol) && Protocol == ProtocolVersion
        && IntegerField(Object, TEXT("pid"), Pid) && Pid == FPlatformProcess::GetCurrentProcessId()
        && StringField(Object, TEXT("kind"), Kind) && Kind == TEXT("handshake")
        && StringField(Object, TEXT("token"), SuppliedToken) && ConstantTimeEqual(SuppliedToken, Token)
        && StringField(Object, TEXT("projectId"), SuppliedProject) && SuppliedProject == ProjectId
        && StringField(Object, TEXT("sessionNonce"), SuppliedNonce) && SuppliedNonce == SessionNonce
        && StringField(Object, TEXT("processStart"), SuppliedProcessStart) && SuppliedProcessStart == ProcessStartIdentity(FPlatformProcess::GetCurrentProcessId())
        && StringField(Object, TEXT("cliVersion"), ClientVersion) && ClientVersion == TEXT("0.1.0");
}

FString HandshakeResponse()
{
    const TSharedRef<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetNumberField(TEXT("protocol"), ProtocolVersion);
    Response->SetStringField(TEXT("status"), TEXT("ok"));
    Response->SetStringField(TEXT("pluginVersion"), TEXT("0.1.0"));
    Response->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
    Response->SetStringField(TEXT("processStart"), ProcessStartIdentity(FPlatformProcess::GetCurrentProcessId()));
    Response->SetStringField(TEXT("sessionNonce"), SessionNonce);
    Response->SetStringField(TEXT("catalogHash"), MAGI_AXI_CATALOG_HASH);
    return Serialize(Response);
}

FString StopResponseOnGameThread(const FString& Id, bool& OutMayStop)
{
    if (Lifecycle.load() != ELifecycle::Ready)
    {
        OutMayStop = false;
        return ErrorResponse(Id, TEXT("unsafe_editor_state"), TEXT("editor is not ready"));
    }
    TArray<UPackage*> DirtyPackages;
    FEditorFileUtils::GetDirtyPackages(DirtyPackages);
    if (!DirtyPackages.IsEmpty())
    {
        const TSharedRef<FJsonObject> Error = MakeShared<FJsonObject>();
        Error->SetStringField(TEXT("type"), TEXT("unsafe_editor_state"));
        Error->SetStringField(TEXT("message"), TEXT("editor has dirty packages; save before stop"));
        Error->SetBoolField(TEXT("retryable"), false);
        Error->SetNumberField(TEXT("dirtyPackageCount"), DirtyPackages.Num());
        TArray<TSharedPtr<FJsonValue>> Names;
        for (int32 Index = 0; Index < FMath::Min(DirtyPackages.Num(), 32); ++Index) Names.Add(MakeShared<FJsonValueString>(DirtyPackages[Index]->GetName()));
        Error->SetArrayField(TEXT("dirtyPackages"), Names);
        const TSharedRef<FJsonObject> Response = MakeShared<FJsonObject>();
        Response->SetNumberField(TEXT("protocol"), ProtocolVersion);
        Response->SetStringField(TEXT("id"), Id);
        Response->SetStringField(TEXT("status"), TEXT("error"));
        Response->SetObjectField(TEXT("error"), Error);
        OutMayStop = false;
        return Serialize(Response);
    }
    Lifecycle.store(ELifecycle::Stopping);
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("state"), TEXT("stopping"));
    const TSharedRef<FJsonObject> Response = MakeShared<FJsonObject>();
    Response->SetNumberField(TEXT("protocol"), ProtocolVersion);
    Response->SetStringField(TEXT("id"), Id);
    Response->SetStringField(TEXT("status"), TEXT("ok"));
    Response->SetObjectField(TEXT("result"), Result);
    OutMayStop = true;
    return Serialize(Response);
}

bool TryEnqueueGameThreadRequest(const TSharedRef<FGameThreadRequest>& Request)
{
    FScopeLock Lock(&StateMutex);
    if (!Running || GameThreadQueue.Num() >= MaxGameThreadQueue) return false;
    GameThreadQueue.Add(Request);
    return true;
}

bool CancelQueuedRequest(const TSharedRef<FGameThreadRequest>& Request)
{
    {
        FScopeLock StateLock(&StateMutex);
        std::lock_guard RequestLock(Request->Mutex);
        if (Request->EverDispatched || Request->Done) return false;
        GameThreadQueue.RemoveSingle(Request);
        Request->Response = ErrorResponse(Request->Id, TEXT("timeout"), TEXT("game-thread request timed out before dispatch"), true);
        Request->Cancelled = true;
        Request->Done = true;
    }
    if (IsMutationOperation(Request->Operation)) SetReceipt(Request->Id, Request->Operation, Request->Response, TEXT("failed"));
    Request->Condition.notify_one();
    return true;
}

bool DrainGameThreadQueue(float)
{
    TSharedPtr<FGameThreadRequest> Request;
    bool Execute = false;
    bool FailedBeforeDispatch = false;
    {
        FScopeLock StateLock(&StateMutex);
        if (GameThreadQueue.Num() > 0)
        {
            Request = GameThreadQueue[0];
            std::lock_guard RequestLock(Request->Mutex);
            GameThreadQueue.RemoveAt(0, 1, EAllowShrinking::No);
            if (!Request->Cancelled && Running && (FPlatformTime::Seconds() < Request->Deadline || Request->EverDispatched))
            {
                Request->Dispatched = true;
                Request->EverDispatched = true;
                Execute = true;
            }
            else
            {
                Request->Response = ErrorResponse(Request->Id, TEXT("timeout"), TEXT("game-thread request timed out before dispatch"), true);
                Request->Cancelled = true;
                Request->Done = true;
                FailedBeforeDispatch = true;
                Request->Condition.notify_one();
            }
        }
    }
    if (FailedBeforeDispatch && IsMutationOperation(Request->Operation)) SetReceipt(Request->Id, Request->Operation, Request->Response, TEXT("failed"));
    if (Execute)
    {
        if (IsMutationOperation(Request->Operation)) SetReceipt(Request->Id, Request->Operation, FString(), TEXT("running"));
        std::unique_lock RequestLock(Request->Mutex);
        bool MayStop = false;
        if (Request->DeferredCompletion)
        {
            TSharedPtr<FJsonObject> PendingResponse;
            const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Request->Response);
            const TSharedPtr<FJsonObject>* PendingResult = nullptr;
            if (FJsonSerializer::Deserialize(Reader, PendingResponse) && PendingResponse.IsValid() && PendingResponse->TryGetObjectField(TEXT("result"), PendingResult) && PendingResult && PendingResult->IsValid() && PieWorld())
            {
                const TSharedRef<FJsonObject> Observation = ObservePlayResult();
                const FString Before = (*PendingResult)->GetStringField(TEXT("beforeRevision"));
                const FString After = Observation->GetStringField(TEXT("revision"));
                (*PendingResult)->SetStringField(TEXT("afterRevision"), After);
                (*PendingResult)->SetStringField(TEXT("revision"), After);
                (*PendingResult)->SetBoolField(TEXT("changed"), Before != After);
                Request->Response = PlayResponse(Request->Id, (*PendingResult).ToSharedRef(), Request->Operation);
            }
            else
            {
                Request->Response = ErrorResponse(Request->Id, TEXT("operation_failed"), TEXT("deferred play input readback failed"));
            }
        }
        else
        {
            if (Request->Operation == TEXT("editor.stop")) Request->Response = StopResponseOnGameThread(Request->Id, MayStop);
            else Request->Response = ReadResponseOnGameThread(Request->Id, Request->Operation, Request->Args, Request->ExpectedRevision);
        }
        if (Request->Operation == TEXT("play.input") && !Request->DeferredCompletion && ResponseStatusIsOk(Request->Response))
        {
            Request->DeferredCompletion = true;
            Request->Dispatched = false;
            RequestLock.unlock();
            FScopeLock StateLock(&StateMutex);
            GameThreadQueue.Insert(Request.ToSharedRef(), 0);
            return Running.load();
        }
        Request->MayStop = MayStop;
        Request->Done = true;
        const FString CompletedResponse = Request->Response;
        RequestLock.unlock();
        if (IsMutationOperation(Request->Operation)) SetReceipt(Request->Id, Request->Operation, CompletedResponse, ResponseStatusIsOk(CompletedResponse) ? TEXT("completed") : TEXT("failed"));
        Request->Condition.notify_one();
    }
    if (CloseRequested.exchange(false) && Running) FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame")).RequestCloseEditor();
    return Running.load();
}

void Handle(FSocket* Socket)
{
    const double AuthenticationTime = FPlatformTime::Seconds();
    if (!AuthenticationAllowedAt(AuthenticationTime)) return;
    TArray<uint8> Bytes;
    if (!ReceiveFrame(Socket, Bytes, HandshakeSeconds)) { RecordAuthenticationFailureAt(AuthenticationTime); return; }
    TSharedPtr<FJsonObject> Handshake;
    if (!ParseObject(Bytes, Handshake) || !Authenticate(Handshake)) { RecordAuthenticationFailureAt(FPlatformTime::Seconds()); return; }
    if (!SendFrame(Socket, HandshakeResponse(), FPlatformTime::Seconds() + HandshakeSeconds)) return;
    if (!ReceiveFrame(Socket, Bytes, RequestSeconds)) return;
    TSharedPtr<FJsonObject> RequestObject;
    if (!ParseObject(Bytes, RequestObject)) return;
    int64 Protocol = 0, DeadlineMs = 30000;
    FString Id, Operation, ExpectedRevision, IdempotencyKey;
    if (!IntegerField(RequestObject, TEXT("protocol"), Protocol) || Protocol != ProtocolVersion || !StringField(RequestObject, TEXT("id"), Id) || !StringField(RequestObject, TEXT("operation"), Operation) || (RequestObject->HasField(TEXT("expectedRevision")) && (!StringField(RequestObject, TEXT("expectedRevision"), ExpectedRevision) || ExpectedRevision.Len() > 128)) || (RequestObject->HasField(TEXT("idempotencyKey")) && (!StringField(RequestObject, TEXT("idempotencyKey"), IdempotencyKey) || IdempotencyKey.Len() > 256)) || (RequestObject->HasField(TEXT("deadlineMs")) && (!IntegerField(RequestObject, TEXT("deadlineMs"), DeadlineMs) || DeadlineMs < 1)))
    {
        SendFrame(Socket, ErrorResponse(Id, TEXT("invalid_input"), TEXT("invalid request envelope")));
        return;
    }
    DeadlineMs = FMath::Clamp<int64>(DeadlineMs, 1, 30000);
    const double Deadline = FPlatformTime::Seconds() + (static_cast<double>(DeadlineMs) / 1000.0);
    auto SendResponse = [&](const FString& Response) { return SendFrame(Socket, Response, Deadline); };
    const TSharedPtr<FJsonValue> ArgsValue = RequestObject->TryGetField(TEXT("args"));
    const TSharedPtr<FJsonObject> Args = ArgsValue.IsValid() && ArgsValue->Type == EJson::Object ? ArgsValue->AsObject() : MakeShared<FJsonObject>();
    if (!ArgsValue.IsValid() || ArgsValue->Type != EJson::Object)
    {
        SendFrame(Socket, ErrorResponse(Id, TEXT("invalid_input"), TEXT("args must be an object")));
        return;
    }
    if (Operation == TEXT("editor.stop") && !ClosedArgs(Args, {})) { SendResponse(ErrorResponse(Id, TEXT("invalid_input"), TEXT("args must be closed"))); return; }
    if (Operation == TEXT("bridge.health") || Operation == TEXT("bridge.describe"))
    {
        const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
        if (Operation == TEXT("bridge.health"))
        {
            const ELifecycle State = Lifecycle.load();
            Result->SetStringField(TEXT("state"), State == ELifecycle::Starting ? TEXT("starting") : State == ELifecycle::Stopping ? TEXT("stopping") : TEXT("ready"));
        }
        else
        {
            Result->SetNumberField(TEXT("protocol"), ProtocolVersion);
            Result->SetStringField(TEXT("catalogHash"), MAGI_AXI_CATALOG_HASH);
            Result->SetArrayField(TEXT("operations"), GeneratedPublicOperations());
        }
        SendResponse(SuccessResponse(Id, Result));
        return;
    }
    if (!IsGeneratedPublicOperation(Operation))
    {
        SendResponse(ErrorResponse(Id, TEXT("unsupported"), TEXT("operation is not generated in public registry")));
        return;
    }
    if (Operation != TEXT("editor.stop") && !MagiAxiValidateInput(Operation, Args.ToSharedRef()))
    {
        SendResponse(ErrorResponse(Id, TEXT("invalid_input"), TEXT("arguments do not match generated capability schema")));
        return;
    }
    const TSharedRef<FGameThreadRequest> Request = MakeShared<FGameThreadRequest>();
    Request->Id = Id;
    Request->Operation = Operation;
    Request->Args = Args;
    Request->ExpectedRevision = ExpectedRevision;
    Request->IdempotencyKey = IdempotencyKey;
    Request->Fingerprint = Sha256(Operation + TEXT("\n") + Serialize(Args.ToSharedRef()) + TEXT("\n") + ExpectedRevision);
    if (IsMutationOperation(Operation))
    {
        FScopeLock Lock(&LedgerMutex); PruneLedger();
        const FString IntentKey = Sha256(Operation + TEXT("\n") + IdempotencyKey);
        for (const FLedgerRecord& Record : Ledger)
        {
            if (Record.OperationId != Id && (IdempotencyKey.IsEmpty() || Record.IntentKey != IntentKey)) continue;
            if (Record.Fingerprint != Request->Fingerprint) { SendResponse(ErrorResponse(Id, TEXT("conflict"), TEXT("operation identity conflicts with prior intent"))); return; }
            if (Record.State == TEXT("completed") || Record.State == TEXT("failed")) { SendResponse(Record.Response.IsEmpty() ? ErrorResponse(Id, TEXT("outcome_unknown"), TEXT("terminal receipt is unavailable")) : Record.Response); return; }
            SendResponse(ErrorResponse(Id, TEXT("busy"), TEXT("operation is already queued or running"), true)); return;
        }
        FLedgerRecord Record; Record.OperationId = Id; Record.Operation = Operation; Record.Fingerprint = Request->Fingerprint; Record.IntentKey = IntentKey; Record.CreatedAt = FPlatformTime::Seconds(); Ledger.Add(MoveTemp(Record));
    }
    Request->Deadline = Deadline;
    if (!TryEnqueueGameThreadRequest(Request))
    {
        if (IsMutationOperation(Operation)) SetReceipt(Id, Operation, FString(), TEXT("failed"));
        SendResponse(ErrorResponse(Id, TEXT("busy"), TEXT("game-thread request queue is full"), true));
        return;
    }
    bool Cancel = false;
    {
        std::unique_lock Lock(Request->Mutex);
        while (!Request->Done && Running && FPlatformTime::Seconds() < Deadline)
        {
            Request->Condition.wait_for(Lock, std::chrono::milliseconds(25));
            if (!Request->Done && !ConnectionOpen(Socket))
            {
                Cancel = true;
                break;
            }
        }
        Cancel |= !Request->Done || Request->Cancelled;
    }
    if (Cancel)
    {
        if (CancelQueuedRequest(Request))
        {
            if (ConnectionOpen(Socket)) SendResponse(Request->Response);
            return;
        }
        std::unique_lock DispatchedLock(Request->Mutex);
        Request->Condition.wait(DispatchedLock, [&] { return Request->Done || !Running.load(); });
        if (!Request->Done) return;
    }
    std::unique_lock Lock(Request->Mutex);
    const FString Response = Request->Response;
    const bool MayStop = Request->MayStop;
    Lock.unlock();
    if (MayStop)
    {
        if (SendResponse(Response)) CloseRequested.store(true);
        else Lifecycle.store(ELifecycle::Ready);
    }
    else SendResponse(Response);
}

void ServeConnection(FSocket* Client, const TSharedPtr<std::atomic<bool>>& Done)
{
    Handle(Client);
    {
        FScopeLock Lock(&StateMutex);
        ActiveClients.Remove(Client);
    }
    Client->Close();
    ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Client);
    Done->store(true);
}

void Serve()
{
    while (Running)
    {
        {
            std::lock_guard Lock(ConnectionMutex);
            for (auto It = ConnectionWorkers.begin(); It != ConnectionWorkers.end();)
            {
                if (!It->Done->load()) { ++It; continue; }
                if (It->Thread.joinable()) It->Thread.join();
                It = ConnectionWorkers.erase(It);
            }
        }
        bool Pending = false;
        if (!Listener || !Listener->HasPendingConnection(Pending) || !Pending)
        {
            FPlatformProcess::Sleep(0.01f);
            continue;
        }
        FSocket* Client = Listener->Accept(TEXT("MagiUnrealAXI"));
        if (!Client) continue;
        Client->SetNonBlocking(true);
        bool Accepted = false;
        {
            FScopeLock Lock(&StateMutex);
            if (Running && ActiveClients.Num() < MaxConnections)
            {
                ActiveClients.Add(Client);
                Accepted = true;
            }
        }
        if (!Accepted)
        {
            Client->Close();
            ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Client);
            continue;
        }
        const TSharedPtr<std::atomic<bool>> Done = MakeShared<std::atomic<bool>>(false);
        std::lock_guard Lock(ConnectionMutex);
        ConnectionWorkers.push_back({std::thread(ServeConnection, Client, Done), Done});
    }
}
void RemoveDiscovery()
{
    if (RuntimeDirectory.IsEmpty()) return;
    IFileManager::Get().Delete(*(RuntimeDirectory / TEXT("bridge-v1.json")));
    IFileManager::Get().Delete(*(RuntimeDirectory / TEXT("token")));
    IFileManager::Get().DeleteDirectory(*RuntimeDirectory, false, true);
}
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXILevelOperations, "MagiUnrealAXI.Mutation.LevelOperations", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXILevelOperations::RunTest(const FString&)
{
    FString Filename, Error;
    TestTrue(TEXT("exact Game package accepted"), ValidateLevelPath(TEXT("/Game/Temp/MagiM5Level"), Filename, Error));
    TestFalse(TEXT("extension rejected"), ValidateLevelPath(TEXT("/Game/Temp/MagiM5Level.umap"), Filename, Error));
    TestFalse(TEXT("traversal rejected"), ValidateLevelPath(TEXT("/Game/Temp/../MagiM5Level"), Filename, Error));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIActorMutationContracts, "MagiUnrealAXI.Mutation.ActorUpdateDelete", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIActorMutationContracts::RunTest(const FString&)
{
    TestTrue(TEXT("update is mutation"), IsMutationOperation(TEXT("actor.update_transform")));
    TestTrue(TEXT("delete is mutation"), IsMutationOperation(TEXT("actor.delete")));
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    TestNotNull(TEXT("editor world exists for receipt readback"), World);
    if (!World || !World->PersistentLevel) return false;
    FActorSpawnParameters Parameters; Parameters.OverrideLevel = World->PersistentLevel; Parameters.ObjectFlags |= RF_Transactional;
    AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Parameters);
    TestNotNull(TEXT("receipt fixture actor spawned"), Actor);
    if (!Actor) return false;
    const FString Target = ActorId(*Actor);
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("id"), Target); Result->SetBoolField(TEXT("changed"), false); Result->SetArrayField(TEXT("dirtyPackages"), {}); Result->SetArrayField(TEXT("savedPackages"), {}); Result->SetStringField(TEXT("revision"), ActorRevision(*Actor));
    TestTrue(TEXT("mutation result carries receipt fields"), MagiAxiValidateOutput(TEXT("actor.update_transform"), Result));
    TSharedPtr<FJsonObject> Response; const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(SuccessResponse(TEXT("mutation-receipt-test"), Result, TEXT("actor.update_transform")));
    TestTrue(TEXT("verified mutation receipt parses"), FJsonSerializer::Deserialize(Reader, Response) && Response.IsValid());
    const TSharedPtr<FJsonObject>* Receipt = nullptr; const TSharedPtr<FJsonObject>* Verification = nullptr;
    TestTrue(TEXT("verified mutation receipt exists"), Response.IsValid() && Response->TryGetObjectField(TEXT("receipt"), Receipt) && Receipt && Receipt->IsValid());
    TestTrue(TEXT("verified mutation receipt matches native actor readback"), Receipt && Receipt->IsValid() && (*Receipt)->GetStringField(TEXT("target")) == Target && (*Receipt)->TryGetObjectField(TEXT("verification"), Verification) && Verification && Verification->IsValid() && (*Verification)->GetBoolField(TEXT("matched")) && (*Verification)->GetStringField(TEXT("observedRevision")) == ActorRevision(*Actor));
    const TSharedRef<FJsonObject> DeleteResult = MakeShared<FJsonObject>(); DeleteResult->SetStringField(TEXT("id"), Target); DeleteResult->SetBoolField(TEXT("changed"), false); DeleteResult->SetBoolField(TEXT("dryRun"), true); DeleteResult->SetArrayField(TEXT("dirtyPackages"), {}); DeleteResult->SetArrayField(TEXT("savedPackages"), {}); DeleteResult->SetStringField(TEXT("revision"), ActorRevision(*Actor));
    TSharedPtr<FJsonObject> DeleteResponse; const TSharedRef<TJsonReader<>> DeleteReader = TJsonReaderFactory<>::Create(SuccessResponse(TEXT("delete-receipt-test"), DeleteResult, TEXT("actor.delete")));
    TestTrue(TEXT("delete dry-run receipt parses"), FJsonSerializer::Deserialize(DeleteReader, DeleteResponse) && DeleteResponse.IsValid());
    const TSharedPtr<FJsonObject>* DeleteReceipt = nullptr; const TSharedPtr<FJsonObject>* DeleteVerification = nullptr;
    TestTrue(TEXT("delete dry-run readback preserves target"), DeleteResponse.IsValid() && DeleteResponse->TryGetObjectField(TEXT("receipt"), DeleteReceipt) && DeleteReceipt && DeleteReceipt->IsValid() && (*DeleteReceipt)->TryGetObjectField(TEXT("verification"), DeleteVerification) && DeleteVerification && DeleteVerification->IsValid() && (*DeleteVerification)->GetBoolField(TEXT("exists")) && (*DeleteVerification)->GetBoolField(TEXT("matched")));
    Actor->Destroy();
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIMutationUnsafeStates, "MagiUnrealAXI.Mutation.UnsafeStates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIMutationUnsafeStates::RunTest(const FString&)
{
    struct FCase { void (*SetUnsafe)(FMutationSafetyState&); const TCHAR* Message; };
    const FCase Cases[] = {
        {[](FMutationSafetyState& S) { S.LifecycleReady = false; }, TEXT("editor lifecycle is not ready")},
        {[](FMutationSafetyState& S) { S.EditorAvailable = false; }, TEXT("editor is unavailable")},
        {[](FMutationSafetyState& S) { S.PlaySessionActive = true; }, TEXT("PIE or SIE is active")},
        {[](FMutationSafetyState& S) { S.ModalWindowActive = true; }, TEXT("editor modal dialog is active")},
        {[](FMutationSafetyState& S) { S.ShutdownRequested = true; }, TEXT("editor shutdown is in progress")},
        {[](FMutationSafetyState& S) { S.SlowTaskActive = true; }, TEXT("editor slow task is active")},
        {[](FMutationSafetyState& S) { S.AsyncLoadingActive = true; }, TEXT("async package loading is active")},
        {[](FMutationSafetyState& S) { S.GarbageCollecting = true; }, TEXT("garbage collection is active")},
        {[](FMutationSafetyState& S) { S.PackageSaveActive = true; }, TEXT("package save is active")},
        {[](FMutationSafetyState& S) { S.AssetCompilationActive = true; }, TEXT("asset compilation is active")},
        {[](FMutationSafetyState& S) { S.EditorBuildActive = true; }, TEXT("editor build is active")},
    };
    FString Message;
    TestTrue(TEXT("editing state permits mutation"), ValidateMutationSafetyState(FMutationSafetyState{}, Message));
    for (const FCase& Case : Cases)
    {
        FMutationSafetyState State;
        Case.SetUnsafe(State);
        Message.Reset();
        TestFalse(Case.Message, ValidateMutationSafetyState(State, Message));
        TestEqual(Case.Message, Message, FString(Case.Message));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIM6ComponentWorldSettingsContracts, "MagiUnrealAXI.M6.ComponentWorldSettingsContracts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIM6ComponentWorldSettingsContracts::RunTest(const FString&)
{
    TestTrue(TEXT("component add is mutation"), IsMutationOperation(TEXT("component.add")));
    TestTrue(TEXT("component update is mutation"), IsMutationOperation(TEXT("component.update")));
    TestTrue(TEXT("component remove is mutation"), IsMutationOperation(TEXT("component.remove")));
    TestTrue(TEXT("game mode is mutation"), IsMutationOperation(TEXT("level.set_game_mode")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIAdversarialFrames, "MagiUnrealAXI.Bridge.AdversarialFrames", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIAdversarialFrames::RunTest(const FString&)
{
    TSharedPtr<FJsonObject> Object;
    const TArray<uint8> Malformed = { '{', '"', 'x', '"', ':' };
    const TArray<uint8> NonUtf8 = { 0xff, 0xfe };
    TestFalse(TEXT("malformed JSON is rejected"), ParseObject(Malformed, Object));
    TestFalse(TEXT("non-UTF-8 payload is rejected"), ParseObject(NonUtf8, Object));

    uint32 Size = 0;
    const uint8 ZeroFrame[4] = { 0, 0, 0, 0 };
    const uint8 OversizedFrame[4] = { 0, 0, 0x00, 0x01 };
    const uint8 MaximumFrame[4] = { 0, 0, 0x80, 0 };
    TestFalse(TEXT("zero frame is rejected"), DecodeFrameSize(ZeroFrame, Size));
    TestFalse(TEXT("oversized frame is rejected"), DecodeFrameSize(OversizedFrame, Size));
    TestTrue(TEXT("request limit remains accepted"), DecodeFrameSize(MaximumFrame, Size));

    ISocketSubsystem* Sockets = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    FSocket* TestListener = FTcpSocketBuilder(TEXT("MagiUnrealAXIIncompleteTest")).BoundToAddress(FIPv4Address(127, 0, 0, 1)).BoundToPort(0).AsNonBlocking().Listening(1);
    TestNotNull(TEXT("incomplete-frame listener is created"), TestListener);
    if (TestListener)
    {
        TSharedRef<FInternetAddr> Address = Sockets->CreateInternetAddr();
        bool AddressValid = false;
        Address->SetIp(TEXT("127.0.0.1"), AddressValid);
        Address->SetPort(TestListener->GetPortNo());
        FSocket* Client = Sockets->CreateSocket(NAME_Stream, TEXT("MagiUnrealAXIIncompleteClient"), Address->GetProtocolType());
        TestTrue(TEXT("incomplete-frame client connects"), AddressValid && Client && Client->Connect(*Address));
        FSocket* Server = nullptr;
        for (int32 Attempt = 0; Client && !Server && Attempt < 100; ++Attempt)
        {
            Server = TestListener->Accept(TEXT("MagiUnrealAXIIncompleteServer"));
            if (!Server) FPlatformProcess::Sleep(0.001f);
        }
        TestNotNull(TEXT("incomplete-frame server accepts"), Server);
        if (Client && Server)
        {
            const uint8 Incomplete[] = {4, 0, 0, 0, '{', '}'};
            int32 Sent = 0;
            TestTrue(TEXT("incomplete frame prefix is sent"), Client->Send(Incomplete, sizeof(Incomplete), Sent) && Sent == sizeof(Incomplete));
            Client->Close();
            TArray<uint8> Received;
            TestFalse(TEXT("incomplete payload is rejected"), ReceiveFrame(Server, Received, 0.25));
        }
        if (Server) { Server->Close(); Sockets->DestroySocket(Server); }
        if (Client) { Client->Close(); Sockets->DestroySocket(Client); }
        TestListener->Close();
        Sockets->DestroySocket(TestListener);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIResponseIdentity, "MagiUnrealAXI.Bridge.ResponseIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIResponseIdentity::RunTest(const FString&)
{
    const ELifecycle Previous = Lifecycle.exchange(ELifecycle::Ready);
    bool MayStop = false;
    TSharedPtr<FJsonObject> Response;
    const FString Text = StopResponseOnGameThread(TEXT("response-test-id"), MayStop);
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
    TestTrue(TEXT("successful stop response parses"), FJsonSerializer::Deserialize(Reader, Response) && Response.IsValid());
    FString Id;
    TestTrue(TEXT("successful stop response preserves request id"), Response.IsValid() && Response->TryGetStringField(TEXT("id"), Id) && Id == TEXT("response-test-id"));
    Lifecycle.store(Previous);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIAuthentication, "MagiUnrealAXI.Bridge.Authentication", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIAuthentication::RunTest(const FString&)
{
    const TSharedRef<FJsonObject> Handshake = MakeShared<FJsonObject>();
    Handshake->SetNumberField(TEXT("protocol"), ProtocolVersion);
    Handshake->SetStringField(TEXT("kind"), TEXT("handshake"));
    Handshake->SetStringField(TEXT("token"), Token);
    Handshake->SetStringField(TEXT("projectId"), ProjectId);
    Handshake->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
    Handshake->SetStringField(TEXT("processStart"), ProcessStartIdentity(FPlatformProcess::GetCurrentProcessId()));
    Handshake->SetStringField(TEXT("sessionNonce"), SessionNonce);
    Handshake->SetStringField(TEXT("cliVersion"), TEXT("0.1.0"));
    TestTrue(TEXT("matching handshake authenticates"), Authenticate(Handshake));
    for (const TCHAR* Field : {TEXT("token"), TEXT("projectId"), TEXT("processStart"), TEXT("sessionNonce"), TEXT("cliVersion")})
    {
        const FString Original = Handshake->GetStringField(Field);
        Handshake->SetStringField(Field, TEXT("wrong"));
        TestFalse(FString::Printf(TEXT("wrong %s fails closed"), Field), Authenticate(Handshake));
        Handshake->SetStringField(Field, Original);
    }
    Handshake->SetNumberField(TEXT("protocol"), ProtocolVersion + 1);
    TestFalse(TEXT("wrong protocol version fails closed"), Authenticate(Handshake));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIQueueBounds, "MagiUnrealAXI.Bridge.QueueBounds", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIQueueBounds::RunTest(const FString&)
{
    {
        FScopeLock Lock(&StateMutex);
        GameThreadQueue.Reset();
        for (int32 Index = 0; Index < MaxGameThreadQueue; ++Index)
        {
            const TSharedRef<FGameThreadRequest> Request = MakeShared<FGameThreadRequest>();
            Request->Id = FString::FromInt(Index);
            Request->Deadline = FPlatformTime::Seconds() + 1.0;
            GameThreadQueue.Add(Request);
        }
    }
    const TSharedRef<FGameThreadRequest> Overflow = MakeShared<FGameThreadRequest>();
    TestFalse(TEXT("queue overflow is rejected as busy"), TryEnqueueGameThreadRequest(Overflow));
    {
        FScopeLock Lock(&StateMutex);
        GameThreadQueue.Reset();
    }

    const TSharedRef<FGameThreadRequest> Expired = MakeShared<FGameThreadRequest>();
    Expired->Id = TEXT("expired");
    Expired->Deadline = FPlatformTime::Seconds() - 1.0;
    TestTrue(TEXT("expired request is admitted before dispatch"), TryEnqueueGameThreadRequest(Expired));
    DrainGameThreadQueue(0.0f);
    {
        std::lock_guard ExpiredLock(Expired->Mutex);
        TestTrue(TEXT("expired request is cancelled before game-thread execution"), Expired->Cancelled && Expired->Done && !Expired->EverDispatched && !Expired->MayStop);
        TestTrue(TEXT("undispatched timeout has terminal error response"), !Expired->Response.IsEmpty() && !ResponseStatusIsOk(Expired->Response));
    }

    const TSharedRef<FGameThreadRequest> Deferred = MakeShared<FGameThreadRequest>();
    Deferred->Id = TEXT("deferred-expired");
    Deferred->Operation = TEXT("play.input");
    Deferred->Deadline = FPlatformTime::Seconds() - 1.0;
    Deferred->EverDispatched = true;
    Deferred->DeferredCompletion = true;
    const TSharedRef<FJsonObject> Pending = MakeShared<FJsonObject>();
    Pending->SetStringField(TEXT("beforeRevision"), Sha256(TEXT("before")));
    Deferred->Response = PendingPlayInputResponse(Deferred->Id, Pending);
    TestTrue(TEXT("post-dispatch deferred readback is admitted"), TryEnqueueGameThreadRequest(Deferred));
    DrainGameThreadQueue(0.0f);
    {
        std::lock_guard DeferredLock(Deferred->Mutex);
        TestTrue(TEXT("post-dispatch deadline completes instead of cancelling"), Deferred->Done && !Deferred->Cancelled && Deferred->EverDispatched);
        TestTrue(TEXT("failed deferred readback is terminal"), !ResponseStatusIsOk(Deferred->Response));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIAuthenticationLimit, "MagiUnrealAXI.Bridge.AuthenticationLimit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIAuthenticationLimit::RunTest(const FString&)
{
    ResetAuthenticationFailures();
    constexpr double Start = 100.0;
    for (int32 Attempt = 0; Attempt < MaxFailedHandshakes / 2; ++Attempt)
    {
        TestTrue(TEXT("failed handshake remains admitted below cap"), AuthenticationAllowedAt(Start));
        RecordAuthenticationFailureAt(Start);
    }
    const TSharedRef<FJsonObject> ValidHandshake = MakeShared<FJsonObject>();
    ValidHandshake->SetNumberField(TEXT("protocol"), ProtocolVersion);
    ValidHandshake->SetStringField(TEXT("kind"), TEXT("handshake"));
    ValidHandshake->SetStringField(TEXT("token"), Token);
    ValidHandshake->SetStringField(TEXT("projectId"), ProjectId);
    ValidHandshake->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
    ValidHandshake->SetStringField(TEXT("processStart"), ProcessStartIdentity(FPlatformProcess::GetCurrentProcessId()));
    ValidHandshake->SetStringField(TEXT("sessionNonce"), SessionNonce);
    ValidHandshake->SetStringField(TEXT("cliVersion"), TEXT("0.1.0"));
    TestTrue(TEXT("valid authentication succeeds during failure window"), Authenticate(ValidHandshake));
    for (int32 Attempt = MaxFailedHandshakes / 2; Attempt < MaxFailedHandshakes; ++Attempt)
    {
        TestTrue(TEXT("valid authentication does not reset failed-attempt count"), AuthenticationAllowedAt(Start));
        RecordAuthenticationFailureAt(Start);
    }
    TestFalse(TEXT("failed handshake cap closes admission"), AuthenticationAllowedAt(Start));
    TestTrue(TEXT("failed handshake window resets"), AuthenticationAllowedAt(Start + FailedHandshakeWindowSeconds));
    ResetAuthenticationFailures();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXICatalogContract, "MagiUnrealAXI.Read.CatalogContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXICatalogContract::RunTest(const FString&)
{
    TestTrue(TEXT("read automation runs on game thread"), IsInGameThread());
    TSharedPtr<FJsonObject> Handshake;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(HandshakeResponse());
    TestTrue(TEXT("handshake response parses"), FJsonSerializer::Deserialize(Reader, Handshake) && Handshake.IsValid());
    TestEqual(TEXT("handshake carries generated catalog hash"), Handshake->GetStringField(TEXT("catalogHash")), FString(MAGI_AXI_CATALOG_HASH));

    const TSharedRef<FJsonObject> Valid = MakeShared<FJsonObject>();
    Valid->SetNumberField(TEXT("limit"), 2);
    Valid->SetArrayField(TEXT("fields"), {MakeShared<FJsonValueString>(TEXT("id")), MakeShared<FJsonValueString>(TEXT("label"))});
    int32 Limit = 0;
    FString Cursor;
    TSet<FString> Fields;
    TestTrue(TEXT("bounded list args validate"), ReadPageArgs(Valid, {TEXT("id"), TEXT("label")}, Limit, Cursor, Fields));
    TestEqual(TEXT("list limit preserved"), Limit, 2);
    Valid->SetStringField(TEXT("unknown"), TEXT("rejected"));
    Fields.Reset();
    TestFalse(TEXT("unknown list arg rejected"), ReadPageArgs(Valid, {TEXT("id"), TEXT("label")}, Limit, Cursor, Fields));
    const TSharedRef<FJsonObject> ComponentArgs = MakeShared<FJsonObject>();
    ComponentArgs->SetStringField(TEXT("actorId"), TEXT("/Game/Map#guid"));
    ComponentArgs->SetNumberField(TEXT("limit"), 10);
    ComponentArgs->SetArrayField(TEXT("fields"), {MakeShared<FJsonValueString>(TEXT("id")), MakeShared<FJsonValueString>(TEXT("name"))});
    Fields.Reset();
    TestTrue(TEXT("component.list accepts actorId with pagination args"), ReadComponentListPageArgs(ComponentArgs, Limit, Cursor, Fields));
    ComponentArgs->SetStringField(TEXT("unknown"), TEXT("rejected"));
    Fields.Reset();
    TestFalse(TEXT("component.list rejects unknown args"), ReadComponentListPageArgs(ComponentArgs, Limit, Cursor, Fields));
    int32 NativeCount = 0;
#define MAGI_AXI_TEST_NATIVE_REGISTRY(Name) \
    { \
        ++NativeCount; \
        TestTrue(FString::Printf(TEXT("%s is generated"), Name), IsGeneratedNativeCapability(Name)); \
        TestFalse(FString::Printf(TEXT("%s rejects empty output"), Name), MagiAxiValidateOutput(Name, MakeShared<FJsonObject>())); \
        if (!IsMutationOperation(Name)) \
        { \
            TSharedPtr<FJsonObject> CoverageResponse; \
            const TSharedRef<TJsonReader<>> CoverageReader = TJsonReaderFactory<>::Create(ReadResponseOnGameThread(TEXT("coverage"), Name, MakeShared<FJsonObject>())); \
            TestTrue(FString::Printf(TEXT("%s handler returns JSON"), Name), FJsonSerializer::Deserialize(CoverageReader, CoverageResponse) && CoverageResponse.IsValid()); \
            const TSharedPtr<FJsonObject>* CoverageError = nullptr; \
            if (CoverageResponse->TryGetObjectField(TEXT("error"), CoverageError) && CoverageError && CoverageError->IsValid()) \
            { \
                TestNotEqual(FString::Printf(TEXT("%s has handwritten handler"), Name), (*CoverageError)->GetStringField(TEXT("type")), FString(TEXT("unsupported"))); \
            } \
        } \
    }
    MAGI_AXI_NATIVE_CAPABILITIES(MAGI_AXI_TEST_NATIVE_REGISTRY)
#undef MAGI_AXI_TEST_NATIVE_REGISTRY
    TestEqual(TEXT("generated native registry count"), NativeCount, MAGI_AXI_NATIVE_CAPABILITY_COUNT);
    const TArray<TSharedPtr<FJsonValue>> PublicOperations = GeneratedPublicOperations();
    TestEqual(TEXT("generated public registry count"), PublicOperations.Num(), MAGI_AXI_PUBLIC_OPERATION_COUNT);
    TSet<FString> UniquePublicOperations;
    for (const TSharedPtr<FJsonValue>& OperationValue : PublicOperations)
    {
        const FString OperationName = OperationValue->AsString();
        TestTrue(FString::Printf(TEXT("%s is in public dispatch registry"), *OperationName), IsGeneratedPublicOperation(OperationName));
        TestFalse(FString::Printf(TEXT("%s appears once"), *OperationName), UniquePublicOperations.Contains(OperationName));
        UniquePublicOperations.Add(OperationName);
    }
    TestFalse(TEXT("ungenerated public operation is rejected"), IsGeneratedPublicOperation(TEXT("actor.unregistered")));

    TArray<TSharedPtr<FJsonValue>> InvalidFixtures;
    const TSharedRef<TJsonReader<>> FixtureReader = TJsonReaderFactory<>::Create(FString(MAGI_AXI_INVALID_OUTPUT_FIXTURES_JSON));
    TestTrue(TEXT("shared invalid-output fixtures parse"), FJsonSerializer::Deserialize(FixtureReader, InvalidFixtures));
    for (const TSharedPtr<FJsonValue>& FixtureValue : InvalidFixtures)
    {
        const TSharedPtr<FJsonObject> Fixture = FixtureValue->AsObject();
        const TSharedPtr<FJsonObject>* Result = nullptr;
        TestTrue(TEXT("shared fixture result is object"), Fixture->TryGetObjectField(TEXT("result"), Result) && Result && Result->IsValid());
        if (Result && Result->IsValid())
        {
            TestFalse(FString::Printf(TEXT("shared %s fixture rejects"), *Fixture->GetStringField(TEXT("case"))), MagiAxiValidateOutput(Fixture->GetStringField(TEXT("operation")), Result->ToSharedRef()));
        }
    }

    AActor* PersistentActor = nullptr;
    if (UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr)
    {
        for (TActorIterator<AActor> It(EditorWorld); It; ++It)
        {
            if (*It && (*It)->GetLevel() && (*It)->GetLevel()->GetOutermost()) { PersistentActor = *It; break; }
        }
    }
    TestNotNull(TEXT("persistent-level actor fixture exists"), PersistentActor);
    if (PersistentActor)
    {
        TestEqual(TEXT("persistent actor identity uses owning level package"), ActorLevelId(*PersistentActor), PersistentActor->GetLevel()->GetOutermost()->GetName());
    }
    UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    TestNotNull(TEXT("streamed-level editor world exists"), EditorWorld);
    UPackage* StreamedPackage = CreatePackage(TEXT("/Temp/MagiUnrealAXIStreamedLevelFixture"));
    UWorld* StreamedWorld = NewObject<UWorld>(StreamedPackage, TEXT("StreamedWorld"), RF_Transient);
    ULevel* StreamedLevel = NewObject<ULevel>(StreamedWorld, TEXT("PersistentLevel"), RF_Transient);
    AActor* StreamedActor = nullptr;
    if (EditorWorld && EditorWorld->AddLevel(StreamedLevel))
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.OverrideLevel = StreamedLevel;
        SpawnParameters.ObjectFlags |= RF_Transient;
        StreamedActor = EditorWorld->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
    }
    TestNotNull(TEXT("streamed-sublevel actor fixture exists"), StreamedActor);
    if (StreamedActor)
    {
        TestTrue(TEXT("streamed actor has persistent guid"), StreamedActor->GetActorGuid().IsValid());
        TestEqual(TEXT("streamed actor identity uses streamed package"), ActorLevelId(*StreamedActor), StreamedPackage->GetName());
        const TSharedRef<FJsonObject> ViewArgs = MakeShared<FJsonObject>();
        ViewArgs->SetStringField(TEXT("id"), ActorId(*StreamedActor));
        TSharedPtr<FJsonObject> ViewResponse;
        const TSharedRef<TJsonReader<>> ViewReader = TJsonReaderFactory<>::Create(ReadResponseOnGameThread(TEXT("streamed-view"), TEXT("actor.view"), ViewArgs));
        TestTrue(TEXT("streamed actor view parses"), FJsonSerializer::Deserialize(ViewReader, ViewResponse) && ViewResponse.IsValid());
        TestEqual(TEXT("streamed actor view succeeds"), ViewResponse->GetStringField(TEXT("status")), FString(TEXT("ok")));
        const TSharedPtr<FJsonObject>* ViewResult = nullptr;
        if (ViewResponse->TryGetObjectField(TEXT("result"), ViewResult) && ViewResult && ViewResult->IsValid())
        {
            TestEqual(TEXT("streamed actor view returns owning package"), (*ViewResult)->GetStringField(TEXT("levelId")), StreamedPackage->GetName());
        }
    }
    if (EditorWorld) EditorWorld->RemoveLevel(StreamedLevel);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIPaginationContract, "MagiUnrealAXI.Read.PaginationContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIPaginationContract::RunTest(const FString&)
{
    const TArray<FString> Rows = {
        CanonicalRow({TEXT("/Game/A.A"), TEXT("A"), TEXT("ClassA"), TEXT("/Game/A")}),
        CanonicalRow({TEXT("/Game/B.B"), TEXT("B"), TEXT("ClassB"), TEXT("/Game/B")}),
    };
    const TSet<FString> SelectedFields = {TEXT("id"), TEXT("name")};
    const FString Revision = SnapshotRevision(TEXT("asset.list"), TEXT("/Game"), SelectedFields, Rows);
    TestEqual(TEXT("snapshot revision is SHA-256"), Revision.Len(), 64);
    TestEqual(TEXT("snapshot revision is deterministic"), SnapshotRevision(TEXT("asset.list"), TEXT("/Game"), SelectedFields, Rows), Revision);
    int32 Offset = -1;
    TestTrue(TEXT("empty cursor starts first page"), CursorOffset(FString(), Revision, Rows.Num(), Offset));
    TestEqual(TEXT("first page offset"), Offset, 0);
    TestTrue(TEXT("matching cursor is accepted"), CursorOffset(TEXT("v1.") + Revision + TEXT(".1"), Revision, Rows.Num(), Offset));
    TestEqual(TEXT("matching cursor offset"), Offset, 1);
    const TArray<FString> ChangedRows = {
        Rows[0],
        CanonicalRow({TEXT("/Game/B.B"), TEXT("renamed"), TEXT("ClassB"), TEXT("/Game/B")}),
    };
    const FString ChangedRevision = SnapshotRevision(TEXT("asset.list"), TEXT("/Game"), SelectedFields, ChangedRows);
    TestNotEqual(TEXT("metadata mutation changes snapshot revision"), ChangedRevision, Revision);
    TestFalse(TEXT("metadata mutation rejects stale cursor"), CursorOffset(TEXT("v1.") + Revision + TEXT(".1"), ChangedRevision, ChangedRows.Num(), Offset));
    const TSet<FString> DifferentFields = {TEXT("id"), TEXT("class")};
    TestNotEqual(TEXT("field projection changes snapshot revision"), SnapshotRevision(TEXT("asset.list"), TEXT("/Game"), DifferentFields, Rows), Revision);

    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    TestNotNull(TEXT("metadata mutation editor world exists"), World);
    AActor* MetadataActor = nullptr;
    AActor* PaginationActor = nullptr;
    if (World)
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.ObjectFlags |= RF_Transient;
        MetadataActor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
        PaginationActor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector(100.0, 0.0, 0.0), FRotator::ZeroRotator, SpawnParameters);
    }
    TestNotNull(TEXT("metadata actor spawned"), MetadataActor);
    TestNotNull(TEXT("pagination actor spawned"), PaginationActor);
    if (MetadataActor && PaginationActor)
    {
        MetadataActor->SetActorLabel(TEXT("Magi Metadata Before"));
        PaginationActor->SetActorLabel(TEXT("Magi Pagination"));
        const TSharedRef<FJsonObject> FirstArgs = MakeShared<FJsonObject>();
        FirstArgs->SetNumberField(TEXT("limit"), 1);
        FirstArgs->SetArrayField(TEXT("fields"), {MakeShared<FJsonValueString>(TEXT("id")), MakeShared<FJsonValueString>(TEXT("label"))});
        TSharedPtr<FJsonObject> FirstResponse;
        const TSharedRef<TJsonReader<>> FirstReader = TJsonReaderFactory<>::Create(ReadResponseOnGameThread(TEXT("metadata-page-1"), TEXT("actor.list"), FirstArgs));
        TestTrue(TEXT("real actor first page parses"), FJsonSerializer::Deserialize(FirstReader, FirstResponse) && FirstResponse.IsValid());
        if (FirstResponse.IsValid()) TestEqual(TEXT("real actor first page succeeds"), FirstResponse->GetStringField(TEXT("status")), FString(TEXT("ok")));
        const TSharedPtr<FJsonObject>* FirstResult = nullptr;
        FString Cursor;
        if (FirstResponse->TryGetObjectField(TEXT("result"), FirstResult) && FirstResult && FirstResult->IsValid())
        {
            TestTrue(TEXT("real actor first page has cursor"), (*FirstResult)->TryGetStringField(TEXT("nextCursor"), Cursor));
        }
        MetadataActor->SetActorLabel(TEXT("Magi Metadata After"));
        const TSharedRef<FJsonObject> SecondArgs = MakeShared<FJsonObject>();
        SecondArgs->SetNumberField(TEXT("limit"), 1);
        SecondArgs->SetStringField(TEXT("cursor"), Cursor);
        SecondArgs->SetArrayField(TEXT("fields"), {MakeShared<FJsonValueString>(TEXT("id")), MakeShared<FJsonValueString>(TEXT("label"))});
        TSharedPtr<FJsonObject> SecondResponse;
        const TSharedRef<TJsonReader<>> SecondReader = TJsonReaderFactory<>::Create(ReadResponseOnGameThread(TEXT("metadata-page-2"), TEXT("actor.list"), SecondArgs));
        TestTrue(TEXT("real actor stale page parses"), FJsonSerializer::Deserialize(SecondReader, SecondResponse) && SecondResponse.IsValid());
        const TSharedPtr<FJsonObject>* Error = nullptr;
        TestTrue(TEXT("real metadata mutation returns error"), SecondResponse->TryGetObjectField(TEXT("error"), Error) && Error && Error->IsValid());
        if (Error && Error->IsValid()) TestEqual(TEXT("real metadata mutation rejects cursor"), (*Error)->GetStringField(TEXT("type")), FString(TEXT("stale_cursor")));
    }

    const FString Status = ReadResponseOnGameThread(TEXT("read-test"), TEXT("editor.status"), MakeShared<FJsonObject>());
    TSharedPtr<FJsonObject> Response;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Status);
    TestTrue(TEXT("game-thread editor status parses"), FJsonSerializer::Deserialize(Reader, Response) && Response.IsValid());
    TestEqual(TEXT("game-thread editor status succeeds"), Response->GetStringField(TEXT("status")), FString(TEXT("ok")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIReadFixture, "MagiUnrealAXI.LiveFixture.ReadData", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIReadFixture::RunTest(const FString&)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    TestNotNull(TEXT("read fixture editor world exists"), World);
    if (!World) return false;
    int32 StableActorCount = 0;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (*It && (*It)->GetActorGuid().IsValid()) ++StableActorCount;
    }
    TestTrue(TEXT("read fixture world exposes stable actor identities"), StableActorCount > 0);

    UPackage* Package = CreatePackage(TEXT("/Game/MagiUnrealAXIReadFixture"));
    Package->AddToRoot();
    UInputAction* Asset = NewObject<UInputAction>(Package, TEXT("MagiUnrealAXIReadFixture"), RF_Public | RF_Standalone);
    FAssetRegistryModule::AssetCreated(Asset);
    TestTrue(TEXT("read fixture asset registered"), Asset != nullptr);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Package, Asset](float)
    {
        if (Asset) FAssetRegistryModule::AssetDeleted(Asset);
        Package->SetDirtyFlag(false);
        Package->RemoveFromRoot();
        return false;
    }), 20.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIDirtyPackageFixture, "MagiUnrealAXI.LiveFixture.DirtyPackage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIDirtyPackageFixture::RunTest(const FString&)
{
    UPackage* Package = CreatePackage(TEXT("/Game/MagiUnrealAXIDirtyFixture"));
    Package->AddToRoot();
    Package->MarkPackageDirty();
    TestTrue(TEXT("live fixture package is dirty"), Package->IsDirty());
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Package](float)
    {
        Package->SetDirtyFlag(false);
        Package->RemoveFromRoot();
        return false;
    }), 20.0f);
    return true;
}

#endif
}

void FMagiUnrealAXIModule::StartupModule()
{
    UE_LOG(LogTemp, Display, TEXT("MAGI_UNREAL_AXI_FIXTURE_STARTUP"));
    if (IsRunningCommandlet()) return;
    ProjectPath = Utf8Path(FPaths::GetProjectFilePath());
    const FString Hash = Sha256(ProjectPath);
    if (Hash.IsEmpty() || !RandomHex(32, Token) || !RandomHex(16, SessionNonce)) return;
    ProjectId = TEXT("sha256:") + Hash;
    const FString CurrentProcessStart = ProcessStartIdentity(FPlatformProcess::GetCurrentProcessId());
    if (CurrentProcessStart.IsEmpty()) return;

    const FString Root = FPaths::Combine(FPlatformProcess::UserHomeDir(), TEXT("Library/Caches/magi-unreal-axi"));
    const FString ProjectDirectory = Root / Hash;
    RuntimeDirectory = ProjectDirectory / FString::FromInt(FPlatformProcess::GetCurrentProcessId());
    if (!SecureDirectory(Root) || !SecureDirectory(ProjectDirectory) || !SecureDirectory(RuntimeDirectory)) { RuntimeDirectory.Reset(); return; }

    Listener = FTcpSocketBuilder(TEXT("MagiUnrealAXI")).BoundToAddress(FIPv4Address(127, 0, 0, 1)).BoundToPort(0).AsNonBlocking().Listening(8);
    if (!Listener) { RemoveDiscovery(); return; }
    const uint32 Port = Listener->GetPortNo();
    const FEngineVersion& Version = FEngineVersion::Current();
    const FString EngineVersion = FString::Printf(TEXT("%u.%u.%u"), Version.GetMajor(), Version.GetMinor(), Version.GetPatch());
    const TSharedRef<FJsonObject> Record = MakeShared<FJsonObject>();
    Record->SetNumberField(TEXT("protocol"), ProtocolVersion);
    Record->SetStringField(TEXT("pluginVersion"), TEXT("0.1.0"));
    Record->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
    Record->SetStringField(TEXT("projectPath"), ProjectPath);
    Record->SetStringField(TEXT("projectId"), ProjectId);
    Record->SetStringField(TEXT("engineVersion"), EngineVersion);
    Record->SetStringField(TEXT("host"), TEXT("127.0.0.1"));
    Record->SetNumberField(TEXT("port"), Port);
    Record->SetStringField(TEXT("sessionNonce"), SessionNonce);
    Record->SetStringField(TEXT("processStart"), CurrentProcessStart);
    Record->SetStringField(TEXT("startedAt"), FDateTime::UtcNow().ToIso8601());
    if (!AtomicPrivateWrite(RuntimeDirectory / TEXT("token"), Token) || !AtomicPrivateWrite(RuntimeDirectory / TEXT("bridge-v1.json"), Serialize(Record)))
    {
        Listener->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Listener);
        Listener = nullptr;
        RemoveDiscovery();
        return;
    }
    Running = true;
    Lifecycle.store(ELifecycle::Starting);
    EditorInitializedHandle = FEditorDelegates::OnEditorInitialized.AddLambda([](double)
    {
        if (Running) Lifecycle.store(ELifecycle::Ready);
    });
    GameThreadTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&DrainGameThreadQueue));
    Worker = std::thread(Serve);
}

void FMagiUnrealAXIModule::ShutdownModule()
{
    Running = false;
    CloseRequested = false;
    Lifecycle.store(ELifecycle::Stopping);
    FEditorDelegates::OnEditorInitialized.Remove(EditorInitializedHandle);
    FTSTicker::GetCoreTicker().RemoveTicker(GameThreadTicker);
    {
        FScopeLock Lock(&StateMutex);
        for (const TSharedRef<FGameThreadRequest>& Request : GameThreadQueue)
        {
            std::lock_guard RequestLock(Request->Mutex);
            Request->Done = true;
            Request->Response = ErrorResponse(Request->Id, TEXT("shutdown"), TEXT("plugin is shutting down"));
            Request->Condition.notify_one();
        }
        GameThreadQueue.Reset();
        for (FSocket* Client : ActiveClients) Client->Close();
    }
    if (Listener) Listener->Close();
    if (Worker.joinable()) Worker.join();
    {
        std::lock_guard Lock(ConnectionMutex);
        for (FConnectionWorker& Connection : ConnectionWorkers)
        {
            if (Connection.Thread.joinable()) Connection.Thread.join();
        }
        ConnectionWorkers.clear();
    }
    if (Listener)
    {
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Listener);
        Listener = nullptr;
    }
    RemoveDiscovery();
    UE_LOG(LogTemp, Display, TEXT("MAGI_UNREAL_AXI_FIXTURE_SHUTDOWN"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIM6PlayReceiptContracts, "MagiUnrealAXI.M6.PlayReceiptContracts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIM6PlayReceiptContracts::RunTest(const FString&)
{
    const FString Before = Sha256(TEXT("play-input-before"));
    const FString After = Sha256(TEXT("play-input-after"));
    const TSharedRef<FJsonObject> Input = MakeShared<FJsonObject>();
    Input->SetStringField(TEXT("sessionId"), TEXT("m6-pie-test")); Input->SetStringField(TEXT("key"), TEXT("SpaceBar")); Input->SetStringField(TEXT("event"), TEXT("pressed")); Input->SetBoolField(TEXT("accepted"), true); Input->SetBoolField(TEXT("changed"), true); Input->SetStringField(TEXT("beforeRevision"), Before); Input->SetStringField(TEXT("afterRevision"), After); Input->SetStringField(TEXT("revision"), After);
    const TSharedRef<FJsonObject> InputVerification = MakeShared<FJsonObject>();
    TestFalse(TEXT("synthetic play.input cannot earn matched readback without PIE"), VerifyMutationPostcondition(TEXT("play.input"), Input, TEXT("m6-pie-test#SpaceBar#pressed"), InputVerification));
    InputVerification->SetStringField(TEXT("readback"), TEXT("play.observe")); InputVerification->SetStringField(TEXT("target"), TEXT("m6-pie-test#SpaceBar#pressed")); InputVerification->SetBoolField(TEXT("matched"), false);
    const TSharedRef<FJsonObject> InputReceipt = BuildReceiptMetadata(TEXT("play-input-test"), TEXT("play.input"), Input, InputVerification, TEXT("m6-pie-test#SpaceBar#pressed"));
    TestEqual(TEXT("play.input exact target"), InputReceipt->GetStringField(TEXT("target")), FString(TEXT("m6-pie-test#SpaceBar#pressed"))); TestEqual(TEXT("play.input transaction"), InputReceipt->GetStringField(TEXT("transaction")), FString(TEXT("none"))); TestFalse(TEXT("unverified synthetic input remains unmatched"), InputReceipt->GetObjectField(TEXT("verification"))->GetBoolField(TEXT("matched")));
    for (const FString& Operation : {FString(TEXT("play.start")), FString(TEXT("play.stop"))})
    {
        const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("sessionId"), TEXT("m6-pie-test")); Result->SetBoolField(TEXT("changed"), true); Result->SetStringField(TEXT("revision"), Sha256(Operation));
        const TSharedRef<FJsonObject> Verification = MakeShared<FJsonObject>(); Verification->SetStringField(TEXT("readback"), TEXT("play.status")); Verification->SetStringField(TEXT("target"), TEXT("m6-pie-test")); Verification->SetBoolField(TEXT("matched"), false);
        const TSharedRef<FJsonObject> Receipt = BuildReceiptMetadata(Operation, Operation, Result, Verification, TEXT("m6-pie-test")); TestEqual(*(Operation + TEXT(" transaction")), Receipt->GetStringField(TEXT("transaction")), FString(TEXT("atomic")));
    }
    TestFalse(TEXT("invalid play.input key is rejected"), FKey(FName(TEXT(""))).IsValid()); return true;
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIM6BlueprintDiagnosticsContract, "MagiUnrealAXI.M6.BlueprintDiagnosticsContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIM6BlueprintDiagnosticsContract::RunTest(const FString&)
{
    const FString ValidPackagePath = TEXT("/Game/MagiM6/BP_ValidCompile"); UPackage* ValidPackage = CreatePackage(*ValidPackagePath); UBlueprint* ValidBlueprint = FindObject<UBlueprint>(ValidPackage, TEXT("BP_ValidCompile")); if (!ValidBlueprint) ValidBlueprint = FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(), ValidPackage, TEXT("BP_ValidCompile"), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), TEXT("MagiUnrealAXI")); TestNotNull(TEXT("valid Blueprint fixture created"), ValidBlueprint); if (ValidBlueprint) { FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(ValidBlueprint); const FString ValidId = ValidBlueprint->GetPathName(); const FString ValidRevision = BlueprintContentRevision(*ValidBlueprint); const TSharedRef<FJsonObject> CompileArgs = MakeShared<FJsonObject>(); CompileArgs->SetStringField(TEXT("id"), ValidId); const FString CompileResponse = ReadResponseOnGameThread(TEXT("m6-valid-compile"), TEXT("blueprint.compile"), CompileArgs, ValidRevision); TestTrue(TEXT("valid Blueprint production compile succeeds"), ResponseStatusIsOk(CompileResponse)); const FString RepeatResponse = ReadResponseOnGameThread(TEXT("m6-valid-compile-repeat"), TEXT("blueprint.compile"), CompileArgs, BlueprintContentRevision(*ValidBlueprint)); TestTrue(TEXT("valid Blueprint repeat compile succeeds"), ResponseStatusIsOk(RepeatResponse)); ValidPackage->MarkPackageDirty(); const FString ValidFilename = FPackageName::LongPackageNameToFilename(ValidPackagePath, FPackageName::GetAssetPackageExtension()); FSavePackageArgs ValidSaveArgs; ValidSaveArgs.TopLevelFlags = RF_Public | RF_Standalone; TestTrue(TEXT("valid Blueprint fixture saves for live CLI certification"), UPackage::SavePackage(ValidPackage, ValidBlueprint, *ValidFilename, ValidSaveArgs) && FPaths::FileExists(ValidFilename)); }
    const FString PackagePath = TEXT("/Game/MagiM6/BP_InvalidCompile"); UPackage* Package = CreatePackage(*PackagePath); UBlueprint* Blueprint = FindObject<UBlueprint>(Package, TEXT("BP_InvalidCompile"));
    if (!Blueprint) Blueprint = FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(), Package, TEXT("BP_InvalidCompile"), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), TEXT("MagiUnrealAXI"));
    TestNotNull(TEXT("invalid Blueprint fixture created"), Blueprint); if (!Blueprint) return false;
    UEdGraph* Graph = Blueprint->UbergraphPages.IsEmpty() ? nullptr : Blueprint->UbergraphPages[0]; TestNotNull(TEXT("invalid Blueprint event graph exists"), Graph); if (!Graph) return false;
    for (int32 Index = 0; Index < 2; ++Index) { UK2Node_CustomEvent* InvalidNode = NewObject<UK2Node_CustomEvent>(Graph); InvalidNode->CreateNewGuid(); InvalidNode->CustomFunctionName = TEXT("MagiDuplicateEvent"); Graph->AddNode(InvalidNode, true, false); InvalidNode->AllocateDefaultPins(); } FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FCompilerResultsLog Results; Results.bSilentMode = true; FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave, &Results); TestTrue(TEXT("invalid Blueprint compile produces errors"), Blueprint->Status == BS_Error || Results.NumErrors > 0);
    const TArray<TSharedPtr<FJsonValue>> Diagnostics = BlueprintDiagnostics(*Blueprint, Results); TestTrue(TEXT("compile diagnostics are retained"), !Diagnostics.IsEmpty()); bool HasNodeContext = false;
    for (const TSharedPtr<FJsonValue>& Value : Diagnostics) { const TSharedPtr<FJsonObject> Diagnostic = Value->AsObject(); if (Diagnostic.IsValid() && !Diagnostic->GetStringField(TEXT("graph")).IsEmpty() && !Diagnostic->GetStringField(TEXT("nodeGuid")).IsEmpty() && !Diagnostic->GetStringField(TEXT("nodeTitle")).IsEmpty()) { HasNodeContext = true; break; } }
    TestTrue(TEXT("compile diagnostic includes graph and node context when available"), HasNodeContext);
    Package->MarkPackageDirty(); const FString Filename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension()); FSavePackageArgs SaveArgs; SaveArgs.TopLevelFlags = RF_Public | RF_Standalone; TestTrue(TEXT("invalid Blueprint fixture saves for live CLI certification"), UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs) && FPaths::FileExists(Filename)); return true;
}
#endif
