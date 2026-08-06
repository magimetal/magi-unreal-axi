bool P11GraphBudgetCountsWithinBounds(int32 Graphs, int32 Nodes, int32 Pins, int32 Links)
{
    return Graphs >= 0 && Nodes >= 0 && Pins >= 0 && Links >= 0 &&
        Graphs <= P11MaxGraphs && Nodes <= P11MaxNodes && Pins <= P11MaxPins && Links <= P11MaxLinks;
}

bool P11PageBudgetCountsWithinBounds(int32 Pins, int32 Links)
{
    return Pins >= 0 && Links >= 0 && Pins <= P11MaxPagePins && Links <= P11MaxPageLinks;
}


bool P11GraphBudgetWithinBounds(UBlueprint& Blueprint)
{
    TArray<UEdGraph*> Graphs;
    Blueprint.GetAllGraphs(Graphs);
    if (!P11GraphBudgetCountsWithinBounds(Graphs.Num(), 0, 0, 0)) return false;
    int32 Nodes = 0;
    int32 Pins = 0;
    int32 Links = 0;
    for (const UEdGraph* Graph : Graphs)
    {
        if (!Graph) continue;
        Nodes += Graph->Nodes.Num();
        if (!P11GraphBudgetCountsWithinBounds(Graphs.Num(), Nodes, Pins, Links)) return false;
        for (const UEdGraphNode* Node : Graph->Nodes)
        {
            if (!Node) continue;
            if (Node->Pins.Num() > P11MaxPinsPerNode) return false;
            Pins += Node->Pins.Num();
            if (!P11GraphBudgetCountsWithinBounds(Graphs.Num(), Nodes, Pins, Links)) return false;
            for (const UEdGraphPin* Pin : Node->Pins)
            {
                if (Pin && Pin->LinkedTo.Num() > P11MaxLinksPerPin) return false;
                if (Pin) Links += Pin->LinkedTo.Num();
                if (!P11GraphBudgetCountsWithinBounds(Graphs.Num(), Nodes, Pins, Links)) return false;
            }
        }
    }
    return true;
}

bool IsP11BlueprintOperation(const FString& Operation)
{
    return Operation == TEXT("blueprint.create") || Operation == TEXT("blueprint.graph_view") ||
        Operation == TEXT("blueprint.event_ensure") || Operation == TEXT("blueprint.node_ensure") ||
        Operation == TEXT("blueprint.pin_default_set") || Operation == TEXT("blueprint.pin_connect");
}

FString P11DeterministicGuidSeed(const UBlueprint& Blueprint, const UEdGraph& Graph, const FString& AgentKey)
{
    return Sha256(CanonicalRow({Blueprint.GetPathName(), BlueprintGraphIdentity(Blueprint, Graph), AgentKey}));
}

FGuid P11DeterministicGuid(const UBlueprint& Blueprint, const UEdGraph& Graph, const FString& AgentKey)
{
    FGuid Guid;
    FGuid::ParseExact(P11DeterministicGuidSeed(Blueprint, Graph, AgentKey).Left(32), EGuidFormats::Digits, Guid);
    return Guid;
}

FString P11CallIntent(const UK2Node_CallFunction& Node)
{
    const UFunction* Function = Node.GetTargetFunction();
    if (!Function) return FString();
    const UClass* Owner = Function->GetOuterUClass();
    const FName Name = Function->GetFName();
    if (Owner == UGameplayStatics::StaticClass() && Name == GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerController)) return TEXT("game.get_player_controller");
    if (Owner == AActor::StaticClass() && Name == GET_FUNCTION_NAME_CHECKED(AActor, EnableInput)) return TEXT("actor.enable_input");
    if (Owner == UKismetMathLibrary::StaticClass() && Name == GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeVector)) return TEXT("math.make_vector");
    if (Owner == AActor::StaticClass() && Name == GET_FUNCTION_NAME_CHECKED(AActor, K2_AddActorWorldOffset)) return TEXT("actor.add_world_offset");
    return FString();
}

FString P11NodeIntent(const UEdGraphNode& Node)
{
    if (const UK2Node_InputKey* Input = Cast<UK2Node_InputKey>(&Node))
    {
        if (Input->InputKey == EKeys::E && Input->bConsumeInput && !Input->bExecuteWhenPaused && Input->bOverrideParentBinding &&
            !Input->bControl && !Input->bAlt && !Input->bShift && !Input->bCommand) return TEXT("input.key_e");
        return FString();
    }
    if (const UK2Node_Event* Event = Cast<UK2Node_Event>(&Node))
    {
        if (Event->bOverrideFunction && Event->EventReference.GetMemberName() == FName(TEXT("ReceiveBeginPlay")) &&
            Event->EventReference.GetMemberParentClass(Event->GetBlueprintClassFromNode()) == AActor::StaticClass()) return TEXT("actor.begin_play");
        return FString();
    }
    if (const UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(&Node)) return P11CallIntent(*Call);
    return FString();
}

UBlueprint* P11LoadBlueprint(const FString& BlueprintId)
{
    if (BlueprintId.IsEmpty()) return nullptr;
    if (UBlueprint* Blueprint = FindObject<UBlueprint>(nullptr, *BlueprintId))
        return Blueprint->GetPathName() == BlueprintId ? Blueprint : nullptr;
    if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(BlueprintId))) return nullptr;
    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintId);
    return Blueprint && Blueprint->GetPathName() == BlueprintId ? Blueprint : nullptr;
}

UEdGraph* P11FindGraph(UBlueprint& Blueprint, const FString& GraphId)
{
    TArray<UEdGraph*> Graphs;
    Blueprint.GetAllGraphs(Graphs);
    for (UEdGraph* Graph : Graphs)
        if (Graph && BlueprintGraphIdentity(Blueprint, *Graph) == GraphId) return Graph;
    return nullptr;
}

UEdGraphNode* P11FindNode(UBlueprint& Blueprint, const FString& NodeId, UEdGraph*& OutGraph)
{
    OutGraph = nullptr;
    TArray<UEdGraph*> Graphs;
    Blueprint.GetAllGraphs(Graphs);
    for (UEdGraph* Graph : Graphs)
    {
        if (!Graph) continue;
        const FString GraphId = BlueprintGraphIdentity(Blueprint, *Graph);
        for (UEdGraphNode* Node : Graph->Nodes)
            if (Node && BlueprintNodeIdentity(GraphId, *Node) == NodeId) { OutGraph = Graph; return Node; }
    }
    return nullptr;
}

UEdGraphPin* P11FindPin(UBlueprint& Blueprint, const FString& PinId, UEdGraph*& OutGraph, UEdGraphNode*& OutNode)
{
    OutGraph = nullptr;
    OutNode = nullptr;
    TArray<UEdGraph*> Graphs;
    Blueprint.GetAllGraphs(Graphs);
    for (UEdGraph* Graph : Graphs)
    {
        if (!Graph) continue;
        const FString GraphId = BlueprintGraphIdentity(Blueprint, *Graph);
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (!Node) continue;
            const FString NodeId = BlueprintNodeIdentity(GraphId, *Node);
            for (UEdGraphPin* Pin : Node->Pins)
                if (Pin && BlueprintPinIdentity(NodeId, *Pin) == PinId) { OutGraph = Graph; OutNode = Node; return Pin; }
        }
    }
    return nullptr;
}

TArray<TSharedPtr<FJsonValue>> P11DirtyPackages(const UBlueprint& Blueprint, bool)
{
    return Blueprint.GetOutermost() && Blueprint.GetOutermost()->IsDirty()
        ? TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueString>(Blueprint.GetOutermost()->GetName())}
        : TArray<TSharedPtr<FJsonValue>>{};
}

TSharedRef<FJsonObject> P11MutationResult(UBlueprint& Blueprint, const FString& GraphId, const FString& NodeId, const FString& PinId,
    const FString& SourcePinId, const FString& TargetPinId, bool Changed)
{
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("blueprintId"), Blueprint.GetPathName());
    if (!GraphId.IsEmpty()) Result->SetStringField(TEXT("graphId"), GraphId);
    if (!NodeId.IsEmpty()) Result->SetStringField(TEXT("nodeId"), NodeId);
    if (!PinId.IsEmpty()) Result->SetStringField(TEXT("pinId"), PinId);
    if (!SourcePinId.IsEmpty()) Result->SetStringField(TEXT("sourcePinId"), SourcePinId);
    if (!TargetPinId.IsEmpty()) Result->SetStringField(TEXT("targetPinId"), TargetPinId);
    Result->SetBoolField(TEXT("changed"), Changed);
    Result->SetArrayField(TEXT("dirtyPackages"), P11DirtyPackages(Blueprint, Changed));
    Result->SetArrayField(TEXT("savedPackages"), {});
    Result->SetStringField(TEXT("revision"), BlueprintContentRevision(Blueprint));
    return Result;
}

TSharedRef<FJsonObject> P11PageResult(const FString& BlueprintId, const FString& Scope, const FString& Revision,
    const FString& CursorRevision, const TArray<TSharedPtr<FJsonValue>>& Items, int32 Total, int32 Offset)
{
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("blueprintId"), BlueprintId);
    Result->SetNumberField(TEXT("count"), Items.Num());
    Result->SetNumberField(TEXT("total"), Total);
    Result->SetStringField(TEXT("scope"), Scope);
    Result->SetStringField(TEXT("revision"), Revision);
    Result->SetArrayField(TEXT("items"), Items);
    if (Offset + Items.Num() < Total) Result->SetStringField(TEXT("nextCursor"), FString::Printf(TEXT("v1.%s.%d"), *CursorRevision, Offset + Items.Num()));
    else Result->SetField(TEXT("nextCursor"), MakeShared<FJsonValueNull>());
    return Result;
}

bool P11ReadPage(const TSharedPtr<FJsonObject>& Args, int32& Limit, FString& Cursor)
{
    Limit = 100;
    int64 Number = 0;
    if (Args->HasField(TEXT("limit")) && (!IntegerField(Args, TEXT("limit"), Number) || Number < 1 || Number > 100)) return false;
    if (Args->HasField(TEXT("limit"))) Limit = static_cast<int32>(Number);
    return !Args->HasField(TEXT("cursor")) || (StringField(Args, TEXT("cursor"), Cursor) && Cursor.Len() <= 256);
}

TSharedRef<FJsonObject> P11GraphViewResult(UBlueprint& Blueprint, const TSharedPtr<FJsonObject>& Args, FString& ErrorType, FString& Error)
{
    const FString BlueprintId = Blueprint.GetPathName();
    if (!P11GraphBudgetWithinBounds(Blueprint)) { ErrorType = TEXT("unsupported"); Error = TEXT("Blueprint graph content exceeds cumulative native read limits"); return MakeShared<FJsonObject>(); }
    const FString Revision = BlueprintContentRevision(Blueprint);
    if (Revision.Len() != 64) { ErrorType = TEXT("operation_failed"); Error = TEXT("Blueprint contains invalid graph, node, pin, or SCS identity"); return MakeShared<FJsonObject>(); }
    int32 Limit = 100;
    FString Cursor;
    if (!P11ReadPage(Args, Limit, Cursor)) { ErrorType = TEXT("invalid_input"); Error = TEXT("invalid blueprint.graph_view pagination"); return MakeShared<FJsonObject>(); }
    FString RequestedGraph;
    Args->TryGetStringField(TEXT("graphId"), RequestedGraph);
    if (RequestedGraph.IsEmpty())
    {
        TArray<UEdGraph*> Graphs;
        Blueprint.GetAllGraphs(Graphs);
        Graphs.RemoveAll([](const UEdGraph* Graph) { return Graph == nullptr; });
        if (Graphs.Num() > P11MaxGraphs) { ErrorType = TEXT("unsupported"); Error = TEXT("Blueprint graph count exceeds bounded read limit"); return MakeShared<FJsonObject>(); }
        Graphs.Sort([&Blueprint](const UEdGraph& Left, const UEdGraph& Right) { return BlueprintGraphIdentity(Blueprint, Left) < BlueprintGraphIdentity(Blueprint, Right); });
        const FString Scope = BlueprintId;
        const FString CursorRevision = Sha256(CanonicalRow({Revision, Scope, TEXT("graphs")}));
        int32 Offset = 0;
        if (!CursorOffset(Cursor, CursorRevision, Graphs.Num(), Offset)) { ErrorType = TEXT("stale_cursor"); Error = TEXT("Blueprint graph cursor is invalid or stale"); return MakeShared<FJsonObject>(); }
        TArray<TSharedPtr<FJsonValue>> Items;
        for (int32 Index = Offset; Index < FMath::Min(Offset + Limit, Graphs.Num()); ++Index)
        {
            UEdGraph* Graph = Graphs[Index];
            const TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
            Item->SetStringField(TEXT("graphId"), BlueprintGraphIdentity(Blueprint, *Graph));
            Item->SetStringField(TEXT("kind"), BlueprintGraphKind(Blueprint, *Graph));
            Item->SetStringField(TEXT("name"), Graph->GetName());
            Item->SetNumberField(TEXT("nodeCount"), Graph->Nodes.Num());
            Items.Add(MakeShared<FJsonValueObject>(Item));
        }
        return P11PageResult(BlueprintId, Scope, Revision, CursorRevision, Items, Graphs.Num(), Offset);
    }
    UEdGraph* Graph = P11FindGraph(Blueprint, RequestedGraph);
    if (!Graph) { ErrorType = TEXT("not_found"); Error = TEXT("graphId does not identify graph in Blueprint"); return MakeShared<FJsonObject>(); }
    TArray<UEdGraphNode*> Nodes;
    for (UEdGraphNode* Node : Graph->Nodes) if (Node) Nodes.Add(Node);
    if (Nodes.Num() > P11MaxNodes) { ErrorType = TEXT("unsupported"); Error = TEXT("Blueprint node count exceeds bounded read limit"); return MakeShared<FJsonObject>(); }
    Nodes.Sort([&RequestedGraph](const UEdGraphNode& Left, const UEdGraphNode& Right) { return BlueprintNodeIdentity(RequestedGraph, Left) < BlueprintNodeIdentity(RequestedGraph, Right); });
    const FString Scope = RequestedGraph;
    const FString CursorRevision = Sha256(CanonicalRow({Revision, Scope, TEXT("nodes")}));
    int32 Offset = 0;
    if (!CursorOffset(Cursor, CursorRevision, Nodes.Num(), Offset)) { ErrorType = TEXT("stale_cursor"); Error = TEXT("Blueprint node cursor is invalid or stale"); return MakeShared<FJsonObject>(); }
    int32 PagePins = 0;
    int32 PageLinks = 0;
    for (int32 Index = Offset; Index < FMath::Min(Offset + Limit, Nodes.Num()); ++Index)
    {
        const UEdGraphNode* Node = Nodes[Index];
        if (Node->Pins.Num() > P11MaxPinsPerNode) { ErrorType = TEXT("unsupported"); Error = TEXT("Blueprint node pin count exceeds bounded read limit"); return MakeShared<FJsonObject>(); }
        PagePins += Node->Pins.Num();
        for (const UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin && Pin->LinkedTo.Num() > P11MaxLinksPerPin) { ErrorType = TEXT("unsupported"); Error = TEXT("Blueprint pin link count exceeds bounded read limit"); return MakeShared<FJsonObject>(); }
            if (Pin) PageLinks += Pin->LinkedTo.Num();
        }
        if (!P11PageBudgetCountsWithinBounds(PagePins, PageLinks)) { ErrorType = TEXT("unsupported"); Error = TEXT("Blueprint graph page exceeds bounded response limits"); return MakeShared<FJsonObject>(); }
    }
    TArray<TSharedPtr<FJsonValue>> Items;
    for (int32 Index = Offset; Index < FMath::Min(Offset + Limit, Nodes.Num()); ++Index)
    {
        UEdGraphNode* Node = Nodes[Index];
        if (Node->Pins.Num() > P11MaxPinsPerNode) { ErrorType = TEXT("unsupported"); Error = TEXT("Blueprint node pin count exceeds bounded read limit"); return MakeShared<FJsonObject>(); }
        const FString NodeId = BlueprintNodeIdentity(RequestedGraph, *Node);
        const TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
        Item->SetStringField(TEXT("nodeId"), NodeId);
        Item->SetStringField(TEXT("class"), Node->GetClass()->GetPathName());
        FString Title = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString().Left(512);
        Item->SetStringField(TEXT("title"), Title.IsEmpty() ? Node->GetClass()->GetName() : Title);
        Item->SetNumberField(TEXT("x"), Node->NodePosX);
        Item->SetNumberField(TEXT("y"), Node->NodePosY);
        TArray<UEdGraphPin*> Pins;
        for (UEdGraphPin* Pin : Node->Pins) if (Pin) Pins.Add(Pin);
        Pins.Sort([&NodeId](const UEdGraphPin& Left, const UEdGraphPin& Right) { return BlueprintPinIdentity(NodeId, Left).Compare(BlueprintPinIdentity(NodeId, Right), ESearchCase::CaseSensitive) < 0; });
        TArray<TSharedPtr<FJsonValue>> PinValues;
        for (UEdGraphPin* Pin : Pins)
        {
            if (Pin->LinkedTo.Num() > P11MaxLinksPerPin) { ErrorType = TEXT("unsupported"); Error = TEXT("Blueprint pin link count exceeds bounded read limit"); return MakeShared<FJsonObject>(); }
            const FString PinId = BlueprintPinIdentity(NodeId, *Pin);
            const TSharedRef<FJsonObject> PinObject = MakeShared<FJsonObject>();
            PinObject->SetStringField(TEXT("pinId"), PinId);
            PinObject->SetStringField(TEXT("name"), Pin->PinName.ToString());
            PinObject->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
            FString Type = Pin->PinType.PinCategory.ToString();
            if (!Pin->PinType.PinSubCategory.IsNone()) Type += TEXT(":") + Pin->PinType.PinSubCategory.ToString();
            if (const UObject* TypeObject = Pin->PinType.PinSubCategoryObject.Get()) Type += TEXT(":") + TypeObject->GetPathName();
            PinObject->SetStringField(TEXT("type"), Type.Left(128));
            PinObject->SetStringField(TEXT("defaultValue"), CanonicalBlueprintPinDefault(*Pin).Left(1024));
            TArray<FString> LinkIds;
            for (UEdGraphPin* Link : Pin->LinkedTo)
                if (Link && Link->GetOwningNode()) LinkIds.Add(BlueprintPinIdentity(BlueprintNodeIdentity(RequestedGraph, *Link->GetOwningNode()), *Link));
            LinkIds.Sort([](const FString& Left, const FString& Right) { return Left.Compare(Right, ESearchCase::CaseSensitive) < 0; });
            TArray<TSharedPtr<FJsonValue>> Links;
            for (const FString& Link : LinkIds) Links.Add(MakeShared<FJsonValueString>(Link));
            PinObject->SetArrayField(TEXT("links"), Links);
            PinValues.Add(MakeShared<FJsonValueObject>(PinObject));
        }
        Item->SetArrayField(TEXT("pins"), PinValues);
        Items.Add(MakeShared<FJsonValueObject>(Item));
    }
    return P11PageResult(BlueprintId, Scope, Revision, CursorRevision, Items, Nodes.Num(), Offset);
}

bool P11ExpectedRevision(UBlueprint& Blueprint, const FString& ExpectedRevision, FString& ErrorType, FString& Error)
{
    if (ExpectedRevision.IsEmpty()) { ErrorType = TEXT("invalid_input"); Error = TEXT("expectedRevision is required"); return false; }
    if (!P11GraphBudgetWithinBounds(Blueprint)) { ErrorType = TEXT("unsupported"); Error = TEXT("Blueprint graph content exceeds cumulative native limits"); return false; }
    const FString Current = BlueprintContentRevision(Blueprint);
    if (Current.IsEmpty()) { ErrorType = TEXT("unsupported"); Error = TEXT("Blueprint graph content exceeds cumulative native limits"); return false; }
    if (Current != ExpectedRevision) { ErrorType = TEXT("conflict"); Error = TEXT("Blueprint revision is stale; re-read blueprint.graph_view before retrying"); return false; }
    return true;
}

FString P11NodeOwner(UBlueprint& Blueprint, const UEdGraphNode& Node)
{
    return Blueprint.GetOutermost()->GetMetaData().GetValue(&Blueprint, P11OwnershipMetadataKey(Node.NodeGuid));
}

void P11SetNodeOwner(UBlueprint& Blueprint, const UEdGraphNode& Node, const FString& AgentKey)
{
    Blueprint.GetOutermost()->GetMetaData().SetValue(&Blueprint, P11OwnershipMetadataKey(Node.NodeGuid), *AgentKey);
}

void P11ClearNodeOwner(UBlueprint& Blueprint, const UEdGraphNode& Node)
{
    Blueprint.GetOutermost()->GetMetaData().RemoveValue(&Blueprint, P11OwnershipMetadataKey(Node.NodeGuid));
}

bool P11DiscardCreatedBlueprint(UPackage& Package, UBlueprint* Blueprint, bool AssetRegistered)
{
    bool Disposed = true;
    if (AssetRegistered && Blueprint) FAssetRegistryModule::AssetDeleted(Blueprint);
    if (Blueprint)
    {
        Blueprint->ClearFlags(RF_Public | RF_Standalone);
        Disposed = Blueprint->Rename(nullptr, GetTransientPackage(), REN_AllowPackageLinkerMismatch | REN_DoNotDirty | REN_DontCreateRedirectors | REN_NonTransactional | REN_SkipGeneratedClasses) && Disposed;
        Blueprint->MarkAsGarbage();
    }
    Package.SetDirtyFlag(false);
    const FName DiscardedName = MakeUniqueObjectName(nullptr, UPackage::StaticClass(), TEXT("P11DiscardedPackage"));
    Disposed = Package.Rename(*DiscardedName.ToString(), nullptr, REN_AllowPackageLinkerMismatch | REN_DoNotDirty | REN_DontCreateRedirectors | REN_NonTransactional | REN_SkipGeneratedClasses) && Disposed;
    Package.SetDirtyFlag(false);
    Package.MarkAsGarbage();
    return Disposed;
}

bool P11ResolveFunction(const FString& Intent, UFunction*& Function, int32& X, int32& Y)
{
    Function = nullptr;
    if (Intent == TEXT("game.get_player_controller")) { Function = UGameplayStatics::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerController)); X = -450; Y = -20; }
    else if (Intent == TEXT("actor.enable_input")) { Function = AActor::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(AActor, EnableInput)); X = -100; Y = -200; }
    else if (Intent == TEXT("math.make_vector")) { Function = UKismetMathLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeVector)); X = -350; Y = 360; }
    else if (Intent == TEXT("actor.add_world_offset")) { Function = AActor::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(AActor, K2_AddActorWorldOffset)); X = 0; Y = 220; }
    return Function != nullptr;
}

bool P11AllowedConnection(const UEdGraphNode& SourceNode, const UEdGraphPin& Source, const UEdGraphNode& TargetNode, const UEdGraphPin& Target)
{
    const FString SourceIntent = P11NodeIntent(SourceNode);
    const FString TargetIntent = P11NodeIntent(TargetNode);
    const FString SourceName = Source.PinName.ToString();
    const FString TargetName = Target.PinName.ToString();
    return (SourceIntent == TEXT("actor.begin_play") && SourceName == UEdGraphSchema_K2::PN_Then && TargetIntent == TEXT("actor.enable_input") && TargetName == UEdGraphSchema_K2::PN_Execute) ||
        (SourceIntent == TEXT("game.get_player_controller") && SourceName == UEdGraphSchema_K2::PN_ReturnValue && TargetIntent == TEXT("actor.enable_input") && TargetName == TEXT("PlayerController")) ||
        (SourceIntent == TEXT("input.key_e") && SourceName == TEXT("Pressed") && TargetIntent == TEXT("actor.add_world_offset") && TargetName == UEdGraphSchema_K2::PN_Execute) ||
        (SourceIntent == TEXT("math.make_vector") && SourceName == UEdGraphSchema_K2::PN_ReturnValue && TargetIntent == TEXT("actor.add_world_offset") && TargetName == TEXT("DeltaLocation"));
}

#if WITH_DEV_AUTOMATION_TESTS
bool GP11ForceAtomicFailure = false;
bool GP11ForceRollbackVerificationFailure = false;
#endif

bool P11RollbackVerificationResult(bool Verified)
{
#if WITH_DEV_AUTOMATION_TESTS
    return Verified && !GP11ForceRollbackVerificationFailure;
#else
    return Verified;
#endif
}

FString P11AtomicResponse(const FString& Id, const FString& Response)
{
#if WITH_DEV_AUTOMATION_TESTS
    if (GP11ForceAtomicFailure) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("injected P1.1 atomic postcondition failure"));
#endif
    return Response;
}

FString P11FailedAtomicResponse(const FString& Id, const FString& Operation, const TSharedPtr<FJsonObject>& Args, const FString& RequestedTarget, const FString& BeforeRevision, const FString& ObservedRevision, const bool RollbackVerified, const TArray<TSharedPtr<FJsonValue>>& DirtyPackages, const TCHAR* Message)
{
    const FMagiAxiCapabilityMetadata* Metadata = CapabilityMetadata(Operation);
    FString Target = RequestedTarget, BlueprintId, GraphId, AgentKey, PinId, SourcePinId, TargetPinId;
    if (Args.IsValid()) { Args->TryGetStringField(TEXT("blueprintId"), BlueprintId); Args->TryGetStringField(TEXT("graphId"), GraphId); Args->TryGetStringField(TEXT("agentKey"), AgentKey); Args->TryGetStringField(TEXT("pinId"), PinId); Args->TryGetStringField(TEXT("sourcePinId"), SourcePinId); Args->TryGetStringField(TEXT("targetPinId"), TargetPinId); }
    if (Operation == TEXT("blueprint.event_ensure") || Operation == TEXT("blueprint.node_ensure")) Target = BlueprintId + TEXT("#") + GraphId + TEXT("#") + AgentKey;
    else if (Operation == TEXT("blueprint.pin_default_set")) Target = BlueprintId + TEXT("#") + PinId;
    else if (Operation == TEXT("blueprint.pin_connect")) Target = BlueprintId + TEXT("#") + SourcePinId + TEXT("#") + TargetPinId;
    const bool Verified = P11RollbackVerificationResult(RollbackVerified);
    const FString State = Verified ? TEXT("failed") : TEXT("outcome_unknown");
    const FString AbsentRevision = Sha256(Target + TEXT("\nabsent"));
    const FString Before = BeforeRevision.Len() == 64 ? BeforeRevision : (Operation == TEXT("blueprint.create") ? AbsentRevision : Sha256(Target + TEXT("\nbefore")));
    const FString Observed = ObservedRevision.Len() == 64 ? ObservedRevision : Before;
    const bool Changed = Before != Observed;
    const TSharedRef<FJsonObject> Verification = MakeShared<FJsonObject>(); Verification->SetStringField(TEXT("readback"), Metadata ? Metadata->Readback : TEXT("operation.view")); Verification->SetStringField(TEXT("target"), Target); Verification->SetBoolField(TEXT("matched"), Verified); Verification->SetStringField(TEXT("beforeRevision"), Before); Verification->SetStringField(TEXT("observedRevision"), Observed); Verification->SetStringField(TEXT("observedStatus"), TEXT("error"));
    if (Operation == TEXT("blueprint.create")) { FString Path, ParentClass; Args->TryGetStringField(TEXT("path"), Path); Args->TryGetStringField(TEXT("parentClass"), ParentClass); Verification->SetStringField(TEXT("requestPath"), Path); Verification->SetStringField(TEXT("requestParentClass"), ParentClass); }
    else if (Operation == TEXT("blueprint.event_ensure") || Operation == TEXT("blueprint.node_ensure")) { FString Intent; Args->TryGetStringField(Operation == TEXT("blueprint.event_ensure") ? TEXT("event") : TEXT("node"), Intent); Verification->SetStringField(TEXT("requestBlueprintId"), BlueprintId); Verification->SetStringField(TEXT("requestGraphId"), GraphId); Verification->SetStringField(TEXT("requestAgentKey"), AgentKey); Verification->SetStringField(TEXT("requestIntent"), Intent); }
    else if (Operation == TEXT("blueprint.pin_default_set")) { const TSharedPtr<FJsonObject>* Value = nullptr; FString Type; double Number = 0; Args->TryGetObjectField(TEXT("value"), Value); if (Value && (*Value)->TryGetStringField(TEXT("type"), Type) && (*Value)->TryGetNumberField(TEXT("value"), Number)) { Verification->SetStringField(TEXT("requestValueType"), Type); Verification->SetNumberField(TEXT("requestValue"), Number); } Verification->SetStringField(TEXT("requestBlueprintId"), BlueprintId); Verification->SetStringField(TEXT("requestPinId"), PinId); }
    else if (Operation == TEXT("blueprint.pin_connect")) { Verification->SetStringField(TEXT("requestBlueprintId"), BlueprintId); Verification->SetStringField(TEXT("requestSourcePinId"), SourcePinId); Verification->SetStringField(TEXT("requestTargetPinId"), TargetPinId); }
    const TSharedRef<FJsonObject> Receipt = MakeShared<FJsonObject>(); Receipt->SetStringField(TEXT("operationId"), Id); Receipt->SetStringField(TEXT("operation"), Operation); Receipt->SetStringField(TEXT("state"), State); Receipt->SetStringField(TEXT("projectId"), ProjectId); Receipt->SetNumberField(TEXT("editorPid"), FPlatformProcess::GetCurrentProcessId()); Receipt->SetStringField(TEXT("target"), Target); Receipt->SetBoolField(TEXT("changed"), Changed); Receipt->SetStringField(TEXT("transaction"), Metadata ? Metadata->TransactionBehavior : TEXT("atomic")); Receipt->SetStringField(TEXT("reversibility"), Metadata ? Metadata->Reversibility : TEXT("source-control")); Receipt->SetArrayField(TEXT("dirtyPackages"), DirtyPackages); Receipt->SetArrayField(TEXT("savedPackages"), {}); Receipt->SetStringField(TEXT("revision"), Observed); Receipt->SetStringField(TEXT("persistence"), DirtyPackages.IsEmpty() ? TEXT("unchanged") : TEXT("dirty")); Receipt->SetObjectField(TEXT("verification"), Verification);
    if (!MagiAxiValidateOutput(TEXT("operation.view"), Receipt)) { const FString Fallback = ErrorResponse(Id, TEXT("outcome_unknown"), TEXT("failed atomic receipt did not satisfy operation.view schema")); SetReceipt(Id, Operation, Fallback, TEXT("outcome_unknown")); return Fallback; }
    const TSharedRef<FJsonObject> Error = MakeShared<FJsonObject>(); Error->SetStringField(TEXT("type"), State == TEXT("outcome_unknown") ? TEXT("outcome_unknown") : TEXT("operation_failed")); Error->SetStringField(TEXT("message"), Message); Error->SetBoolField(TEXT("retryable"), Verified); Error->SetNumberField(TEXT("dirtyPackageCount"), DirtyPackages.Num()); Error->SetArrayField(TEXT("dirtyPackages"), DirtyPackages);
    const TSharedRef<FJsonObject> Response = MakeShared<FJsonObject>(); Response->SetNumberField(TEXT("protocol"), ProtocolVersion); Response->SetStringField(TEXT("id"), Id); Response->SetStringField(TEXT("status"), TEXT("error")); Response->SetObjectField(TEXT("error"), Error); Response->SetObjectField(TEXT("receipt"), Receipt); const FString Wire = Serialize(Response); SetReceipt(Id, Operation, Wire, State); return Wire;
}

FString HandleP11BlueprintOperation(const FString& Id, const FString& Operation, const TSharedPtr<FJsonObject>& Args, const FString& ExpectedRevision)
{

    if (Operation == TEXT("blueprint.create"))
    {
        FString Path, ParentClass;
        Args->TryGetStringField(TEXT("path"), Path);
        Args->TryGetStringField(TEXT("parentClass"), ParentClass);
        if (ParentClass != TEXT("/Script/Engine.StaticMeshActor") || !Path.StartsWith(TEXT("/Game/")) || !FPackageName::IsValidLongPackageName(Path) || Path.Contains(TEXT(".")))
            return ErrorResponse(Id, TEXT("invalid_input"), TEXT("path must be a /Game long package path and parentClass must be /Script/Engine.StaticMeshActor"));
        const FString Name = FPackageName::GetLongPackageAssetName(Path);
        if (Name.IsEmpty()) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("Blueprint path requires asset name"));
        const FString BlueprintId = Path + TEXT(".") + Name;
        const FString AbsentRevision = Sha256(BlueprintId + TEXT("\nabsent"));
        auto CreatedAssetAbsent = [&]()
        {
            if (FPackageName::DoesPackageExist(Path)) return false;
            P11LoadBlueprint(BlueprintId);
            return !FPackageName::DoesPackageExist(Path) && FindPackage(nullptr, *Path) == nullptr &&
                FindObject<UObject>(nullptr, *BlueprintId) == nullptr &&
                !FAssetRegistryModule::GetRegistry().GetAssetByObjectPath(FSoftObjectPath(BlueprintId)).IsValid();
        };
        auto FailedCreate = [&](const TCHAR* Message) -> FString
        {
            const bool Absent = CreatedAssetAbsent();
            FString ObservedRevision = AbsentRevision;
            if (!Absent)
            {
                if (UBlueprint* ObservedBlueprint = P11LoadBlueprint(BlueprintId)) ObservedRevision = BlueprintContentRevision(*ObservedBlueprint);
                else ObservedRevision = Sha256(BlueprintId + (FindPackage(nullptr, *Path) ? TEXT("\npackage-present") : TEXT("\nregistry-present")));
            }
            TArray<TSharedPtr<FJsonValue>> DirtyPackages;
            if (UPackage* ObservedPackage = FindPackage(nullptr, *Path); ObservedPackage && ObservedPackage->IsDirty()) DirtyPackages.Add(MakeShared<FJsonValueString>(Path));
            return P11FailedAtomicResponse(Id, Operation, Args, BlueprintId, AbsentRevision, ObservedRevision, Absent, DirtyPackages, Absent ? Message : TEXT("Blueprint creation rollback verification failed"));
        };
        UPackage* PreexistingPackage = FindPackage(nullptr, *Path);
        const bool PackageExistedOnDisk = FPackageName::DoesPackageExist(Path);
        if (PreexistingPackage || PackageExistedOnDisk)
        {
            UObject* Existing = StaticLoadObject(UObject::StaticClass(), nullptr, *BlueprintId, nullptr, LOAD_NoWarn);
            UBlueprint* Blueprint = Cast<UBlueprint>(Existing);
            if (!Blueprint || Blueprint->ParentClass != AStaticMeshActor::StaticClass()) return ErrorResponse(Id, TEXT("conflict"), TEXT("asset path exists with incompatible Blueprint intent"));
            const TSharedRef<FJsonObject> Result = P11MutationResult(*Blueprint, FString(), FString(), FString(), FString(), FString(), false);
            Result->SetStringField(TEXT("parentClass"), Blueprint->ParentClass->GetPathName());
            Result->SetStringField(TEXT("generatedClass"), Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetPathName() : BlueprintId + TEXT("_C"));
            return SuccessResponse(Id, Result, Operation, Args);
        }
        UPackage* Package = CreatePackage(*Path);
        if (!Package || Package->GetName() != Path || FindObject<UObject>(Package, *Name)) return ErrorResponse(Id, TEXT("conflict"), TEXT("Blueprint package or object already exists"));
        FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P11CreateBlueprint", "Magi AXI Create Blueprint"));
        UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(AStaticMeshActor::StaticClass(), Package, FName(*Name), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), TEXT("MagiUnrealAXI"));
        bool AssetRegistered = false;
        if (!Blueprint) { Transaction.Cancel(); P11DiscardCreatedBlueprint(*Package, nullptr, false); return FailedCreate(TEXT("Blueprint creation failed")); }
        auto CreateFailure = [&](const TCHAR* Message) -> FString
        {
            Transaction.Cancel();
            P11DiscardCreatedBlueprint(*Package, Blueprint, AssetRegistered);
            return FailedCreate(Message);
        };
        AStaticMeshActor* Defaults = Blueprint->GeneratedClass ? Cast<AStaticMeshActor>(Blueprint->GeneratedClass->GetDefaultObject()) : nullptr;
        if (!Defaults || !Defaults->GetStaticMeshComponent()) return CreateFailure(TEXT("created Blueprint StaticMeshActor defaults are unavailable"));
        Defaults->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
        FAssetRegistryModule::AssetCreated(Blueprint);
        AssetRegistered = true;
        Package->MarkPackageDirty();
        if (Blueprint->GetPathName() != BlueprintId || BlueprintContentRevision(*Blueprint).Len() != 64) return CreateFailure(TEXT("created Blueprint identity or revision readback failed"));
        const TSharedRef<FJsonObject> Result = P11MutationResult(*Blueprint, FString(), FString(), FString(), FString(), FString(), true);
        Result->SetStringField(TEXT("parentClass"), Blueprint->ParentClass->GetPathName());
        Result->SetStringField(TEXT("generatedClass"), Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetPathName() : BlueprintId + TEXT("_C"));
        const FString Response = P11AtomicResponse(Id, SuccessResponse(Id, Result, Operation, Args));
        if (!ResponseStatusIsOk(Response))
        {
            Transaction.Cancel();
            P11DiscardCreatedBlueprint(*Package, Blueprint, AssetRegistered);
            return FailedCreate(TEXT("injected P1.1 atomic postcondition failure"));
        }
        return Response;
    }

    FString BlueprintId;
    Args->TryGetStringField(TEXT("blueprintId"), BlueprintId);
    UBlueprint* Blueprint = P11LoadBlueprint(BlueprintId);
    if (!Blueprint) return ErrorResponse(Id, TEXT("not_found"), TEXT("Blueprint was not found by exact object path"));
    if (Blueprint->ParentClass != AStaticMeshActor::StaticClass()) return ErrorResponse(Id, TEXT("conflict"), TEXT("P1.1 authoring requires exact StaticMeshActor parent"));

    if (Operation == TEXT("blueprint.graph_view"))
    {
        FString ErrorType, Error;
        const TSharedRef<FJsonObject> Result = P11GraphViewResult(*Blueprint, Args, ErrorType, Error);
        return Error.IsEmpty() ? SuccessResponse(Id, Result, Operation, Args) : ErrorResponse(Id, *ErrorType, *Error);
    }

    FString ErrorType, Error;
    if (!P11ExpectedRevision(*Blueprint, ExpectedRevision, ErrorType, Error)) return ErrorResponse(Id, *ErrorType, *Error);
    UPackage* BlueprintPackage = Blueprint->GetOutermost();
    const bool WasPackageDirty = BlueprintPackage && BlueprintPackage->IsDirty();
    const EBlueprintStatus BeforeBlueprintStatus = Blueprint->Status;
    auto VerifyRollback = [&](FString& ObservedRevision)
    {
        ObservedRevision = BlueprintContentRevision(*Blueprint);
        if (ObservedRevision != ExpectedRevision)
        {
            if (BlueprintPackage) BlueprintPackage->SetDirtyFlag(true);
            return false;
        }
        Blueprint->Status = BeforeBlueprintStatus;
        if (BlueprintPackage) BlueprintPackage->SetDirtyFlag(WasPackageDirty);
        ObservedRevision = BlueprintContentRevision(*Blueprint);
        return ObservedRevision == ExpectedRevision && Blueprint->Status == BeforeBlueprintStatus &&
            (!BlueprintPackage || BlueprintPackage->IsDirty() == WasPackageDirty);
    };

    if (Operation == TEXT("blueprint.event_ensure") || Operation == TEXT("blueprint.node_ensure"))
    {
        FString GraphId, AgentKey, Intent;
        Args->TryGetStringField(TEXT("graphId"), GraphId);
        Args->TryGetStringField(TEXT("agentKey"), AgentKey);
        Args->TryGetStringField(Operation == TEXT("blueprint.event_ensure") ? TEXT("event") : TEXT("node"), Intent);
        UEdGraph* Graph = P11FindGraph(*Blueprint, GraphId);
        if (!Graph || BlueprintGraphKind(*Blueprint, *Graph) != TEXT("ubergraph")) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("graphId must identify Blueprint ubergraph"));
        const FGuid WantedGuid = P11DeterministicGuid(*Blueprint, *Graph, AgentKey);
        if (!WantedGuid.IsValid()) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("natural-key GUID derivation failed"));
        for (UEdGraph* CandidateGraph : Blueprint->UbergraphPages)
            for (UEdGraphNode* Existing : CandidateGraph ? CandidateGraph->Nodes : TArray<TObjectPtr<UEdGraphNode>>{})
                if (Existing && Existing->NodeGuid == WantedGuid)
                {
                    if (CandidateGraph != Graph || P11NodeIntent(*Existing) != Intent || (!P11NodeOwner(*Blueprint, *Existing).IsEmpty() && P11NodeOwner(*Blueprint, *Existing) != AgentKey)) return ErrorResponse(Id, TEXT("conflict"), TEXT("agentKey already exists with different graph, intent, or durable owner"));
                    if (P11NodeOwner(*Blueprint, *Existing).IsEmpty())
                    {
                        FScopedTransaction OwnershipTransaction(NSLOCTEXT("MagiUnrealAXI", "P11AdoptNode", "Magi AXI Record Blueprint Node Owner"));
                        Blueprint->Modify(); CandidateGraph->Modify(); Existing->Modify();
                        P11SetNodeOwner(*Blueprint, *Existing, AgentKey);
                        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
                        auto RollbackOwnership = [&](const FString& Failure)
                        {
                            P11ClearNodeOwner(*Blueprint, *Existing);
                            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
                            OwnershipTransaction.Cancel();
                            FString After; const bool Verified = VerifyRollback(After); return P11FailedAtomicResponse(Id, Operation, Args, Blueprint->GetPathName(), ExpectedRevision, After, Verified, P11DirtyPackages(*Blueprint, BlueprintPackage && BlueprintPackage->IsDirty()), Verified ? TEXT("Blueprint node ownership readback failed") : TEXT("Blueprint node ownership rollback verification failed"));
                        };
                        if (P11NodeOwner(*Blueprint, *Existing) != AgentKey || BlueprintContentRevision(*Blueprint) == ExpectedRevision) return RollbackOwnership(ErrorResponse(Id, TEXT("operation_failed"), TEXT("Blueprint node ownership readback failed")));
                        const FString Response = P11AtomicResponse(Id, SuccessResponse(Id, P11MutationResult(*Blueprint, GraphId, BlueprintNodeIdentity(GraphId, *Existing), FString(), FString(), FString(), true), Operation, Args));
                        return ResponseStatusIsOk(Response) ? Response : RollbackOwnership(Response);
                    }
                    return SuccessResponse(Id, P11MutationResult(*Blueprint, GraphId, BlueprintNodeIdentity(GraphId, *Existing), FString(), FString(), FString(), false), Operation, Args);
                }
        if (Operation == TEXT("blueprint.event_ensure"))
            for (UEdGraphNode* Existing : Graph->Nodes)
                if (Existing && P11NodeIntent(*Existing) == Intent)
                {
                    for (const UEdGraphPin* Pin : Existing->Pins)
                        if (Pin && !Pin->LinkedTo.IsEmpty()) return ErrorResponse(Id, TEXT("conflict"), TEXT("allowlisted singleton event already has authored links under different natural key"));
                    const FString Owner = P11NodeOwner(*Blueprint, *Existing);
                    if (!Owner.IsEmpty() && Owner != AgentKey) return ErrorResponse(Id, TEXT("conflict"), TEXT("allowlisted singleton event is durably owned by different natural key"));
                    FScopedTransaction ClaimTransaction(NSLOCTEXT("MagiUnrealAXI", "P11ClaimEvent", "Magi AXI Claim Blueprint Event"));
                    const FGuid OldGuid = Existing->NodeGuid; const FString OldComment = Existing->NodeComment; const int32 OldX = Existing->NodePosX; const int32 OldY = Existing->NodePosY;
                    Blueprint->Modify(); Graph->Modify(); Existing->Modify();
                    Existing->NodeGuid = WantedGuid;
                    P11SetNodeOwner(*Blueprint, *Existing, AgentKey);
                    Existing->NodePosX = -700;
                    Existing->NodePosY = Intent == TEXT("actor.begin_play") ? -200 : 220;
                    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
                    auto RollbackClaim = [&](const FString& Failure)
                    {
                        P11ClearNodeOwner(*Blueprint, *Existing); Existing->NodeGuid = OldGuid; Existing->NodeComment = OldComment; Existing->NodePosX = OldX; Existing->NodePosY = OldY;
                        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
                        ClaimTransaction.Cancel();
                        FString After; const bool Verified = VerifyRollback(After); return P11FailedAtomicResponse(Id, Operation, Args, Blueprint->GetPathName(), ExpectedRevision, After, Verified, P11DirtyPackages(*Blueprint, BlueprintPackage && BlueprintPackage->IsDirty()), Verified ? TEXT("Blueprint event ownership readback failed") : TEXT("Blueprint event ownership rollback verification failed"));
                    };
                    if (P11NodeOwner(*Blueprint, *Existing) != AgentKey || BlueprintContentRevision(*Blueprint) == ExpectedRevision) return RollbackClaim(ErrorResponse(Id, TEXT("operation_failed"), TEXT("Blueprint event ownership readback failed")));
                    const FString Response = P11AtomicResponse(Id, SuccessResponse(Id, P11MutationResult(*Blueprint, GraphId, BlueprintNodeIdentity(GraphId, *Existing), FString(), FString(), FString(), true), Operation, Args));
                    return ResponseStatusIsOk(Response) ? Response : RollbackClaim(Response);
                }

        FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P11EnsureNode", "Magi AXI Ensure Blueprint Node"));
        Blueprint->Modify();
        Graph->Modify();
        UEdGraphNode* NewNode = nullptr;
        if (Operation == TEXT("blueprint.event_ensure") && Intent == TEXT("actor.begin_play"))
        {
            UK2Node_Event* Event = NewObject<UK2Node_Event>(Graph, NAME_None, RF_Transactional);
            Event->EventReference.SetExternalMember(FName(TEXT("ReceiveBeginPlay")), AActor::StaticClass());
            Event->bOverrideFunction = true;
            Event->NodePosX = -700; Event->NodePosY = -200;
            NewNode = Event;
        }
        else if (Operation == TEXT("blueprint.event_ensure") && Intent == TEXT("input.key_e"))
        {
            UK2Node_InputKey* Event = NewObject<UK2Node_InputKey>(Graph, NAME_None, RF_Transactional);
            Event->InputKey = EKeys::E;
            Event->bConsumeInput = true; Event->bExecuteWhenPaused = false; Event->bOverrideParentBinding = true;
            Event->bControl = false; Event->bAlt = false; Event->bShift = false; Event->bCommand = false;
            Event->NodePosX = -700; Event->NodePosY = 220;
            NewNode = Event;
        }
        else if (Operation == TEXT("blueprint.node_ensure"))
        {
            UFunction* Function = nullptr; int32 X = 0; int32 Y = 0;
            if (!P11ResolveFunction(Intent, Function, X, Y)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("node intent is not allowlisted or function is unavailable"));
            UK2Node_CallFunction* Call = NewObject<UK2Node_CallFunction>(Graph, NAME_None, RF_Transactional);
            Call->SetFromFunction(Function); Call->NodePosX = X; Call->NodePosY = Y; NewNode = Call;
        }
        if (!NewNode) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("event or node intent is not allowlisted"));
        NewNode->NodeGuid = WantedGuid;
        P11SetNodeOwner(*Blueprint, *NewNode, AgentKey);
        Graph->AddNode(NewNode, true, false);
        NewNode->PostPlacedNewNode();
        NewNode->AllocateDefaultPins();
        if (Intent == TEXT("actor.add_world_offset"))
        {
            const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
            if (UEdGraphPin* Sweep = NewNode->FindPin(TEXT("bSweep"), EGPD_Input)) Schema->TrySetDefaultValue(*Sweep, TEXT("false"));
            if (UEdGraphPin* Teleport = NewNode->FindPin(TEXT("bTeleport"), EGPD_Input)) Schema->TrySetDefaultValue(*Teleport, TEXT("false"));
        }
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
        const FString NodeId = BlueprintNodeIdentity(GraphId, *NewNode);
        auto RollbackNode = [&](const FString& Failure)
        {
            P11ClearNodeOwner(*Blueprint, *NewNode); NewNode->BreakAllNodeLinks(); Graph->RemoveNode(NewNode); NewNode->MarkAsGarbage();
            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
            Transaction.Cancel();
            FString After; const bool Verified = VerifyRollback(After); return P11FailedAtomicResponse(Id, Operation, Args, Blueprint->GetPathName(), ExpectedRevision, After, Verified, P11DirtyPackages(*Blueprint, BlueprintPackage && BlueprintPackage->IsDirty()), Verified ? TEXT("Blueprint node readback failed") : TEXT("Blueprint node rollback verification failed"));
        };
        if (NodeId.IsEmpty() || P11NodeIntent(*NewNode) != Intent || P11NodeOwner(*Blueprint, *NewNode) != AgentKey || BlueprintContentRevision(*Blueprint) == ExpectedRevision)
            return RollbackNode(ErrorResponse(Id, TEXT("operation_failed"), TEXT("Blueprint node readback failed")));
        const FString Response = P11AtomicResponse(Id, SuccessResponse(Id, P11MutationResult(*Blueprint, GraphId, NodeId, FString(), FString(), FString(), true), Operation, Args));
        return ResponseStatusIsOk(Response) ? Response : RollbackNode(Response);
    }
    if (Operation == TEXT("blueprint.pin_default_set"))
    {
        FString PinId; Args->TryGetStringField(TEXT("pinId"), PinId);
        UEdGraph* Graph = nullptr; UEdGraphNode* Node = nullptr;
        UEdGraphPin* Pin = P11FindPin(*Blueprint, PinId, Graph, Node);
        if (!Pin || !Graph || !Node || Pin->Direction != EGPD_Input || !Pin->LinkedTo.IsEmpty()) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("pinId must identify unconnected writable input pin"));
        const TSharedPtr<FJsonObject>* ValueObject = nullptr; FString ValueType; double Value = 0;
        if (!Args->TryGetObjectField(TEXT("value"), ValueObject) || !ValueObject || !ValueObject->IsValid() || !(*ValueObject)->TryGetStringField(TEXT("type"), ValueType) || !(*ValueObject)->TryGetNumberField(TEXT("value"), Value) || !FMath::IsFinite(Value)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("typed finite pin value is required"));
        const FString Intent = P11NodeIntent(*Node); const FString PinName = Pin->PinName.ToString(); FString Default;
        if (Intent == TEXT("game.get_player_controller") && PinName == TEXT("PlayerIndex") && ValueType == TEXT("integer") && Value == 0.0) Default = TEXT("0");
        else if (Intent == TEXT("math.make_vector") && (PinName == TEXT("X") || PinName == TEXT("Y") || PinName == TEXT("Z")) && ValueType == TEXT("real") && Value >= -1000000.0 && Value <= 1000000.0) Default = FString::SanitizeFloat(Value, 0);
        else return ErrorResponse(Id, TEXT("invalid_input"), TEXT("pin/type/value combination is outside P1.1 allowlist"));
        const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
        const FString Validation = Schema->IsPinDefaultValid(Pin, Default, nullptr, FText::GetEmpty());
        if (!Validation.IsEmpty()) return ErrorResponse(Id, TEXT("invalid_input"), *Validation);
        const bool Changed = Pin->DefaultValue != Default;
        if (!Changed) return SuccessResponse(Id, P11MutationResult(*Blueprint, FString(), FString(), PinId, FString(), FString(), false), Operation, Args);
        FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P11SetPinDefault", "Magi AXI Set Blueprint Pin Default"));
        const FString OldDefault = Pin->DefaultValue;
        Blueprint->Modify(); Graph->Modify(); Node->Modify(); Pin->Modify(); Schema->TrySetDefaultValue(*Pin, Default); FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
        auto RollbackDefault = [&](const FString& Failure)
        {
            Schema->TrySetDefaultValue(*Pin, OldDefault); FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint); Transaction.Cancel();
            FString After; const bool Verified = VerifyRollback(After); return P11FailedAtomicResponse(Id, Operation, Args, Blueprint->GetPathName(), ExpectedRevision, After, Verified, P11DirtyPackages(*Blueprint, BlueprintPackage && BlueprintPackage->IsDirty()), Verified ? TEXT("pin default readback failed") : TEXT("pin default rollback verification failed"));
        };
        if (Pin->DefaultValue != Default) return RollbackDefault(ErrorResponse(Id, TEXT("operation_failed"), TEXT("pin default readback failed")));
        const FString Response = P11AtomicResponse(Id, SuccessResponse(Id, P11MutationResult(*Blueprint, FString(), FString(), PinId, FString(), FString(), true), Operation, Args));
        return ResponseStatusIsOk(Response) ? Response : RollbackDefault(Response);
    }
    if (Operation == TEXT("blueprint.pin_connect"))
    {
        FString SourcePinId, TargetPinId; Args->TryGetStringField(TEXT("sourcePinId"), SourcePinId); Args->TryGetStringField(TEXT("targetPinId"), TargetPinId);
        UEdGraph* SourceGraph = nullptr; UEdGraphNode* SourceNode = nullptr; UEdGraph* TargetGraph = nullptr; UEdGraphNode* TargetNode = nullptr;
        UEdGraphPin* Source = P11FindPin(*Blueprint, SourcePinId, SourceGraph, SourceNode); UEdGraphPin* Target = P11FindPin(*Blueprint, TargetPinId, TargetGraph, TargetNode);
        if (!Source || !Target || !SourceNode || !TargetNode || SourceGraph != TargetGraph || Source->Direction != EGPD_Output || Target->Direction != EGPD_Input || !P11AllowedConnection(*SourceNode, *Source, *TargetNode, *Target)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("pin connection is outside P1.1 allowlist"));
        if (Source->LinkedTo.Contains(Target) && Target->LinkedTo.Contains(Source)) return SuccessResponse(Id, P11MutationResult(*Blueprint, FString(), FString(), FString(), SourcePinId, TargetPinId, false), Operation, Args);
        if (!Source->LinkedTo.IsEmpty() || !Target->LinkedTo.IsEmpty()) return ErrorResponse(Id, TEXT("conflict"), TEXT("pin already has conflicting connection"));
        const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>(); const FPinConnectionResponse Response = Schema->CanCreateConnection(Source, Target);
        if (Response.Response != CONNECT_RESPONSE_MAKE) return ErrorResponse(Id, TEXT("invalid_input"), *Response.Message.ToString());
        FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P11ConnectPins", "Magi AXI Connect Blueprint Pins"));
        const FString OldSourceDefault = Source->DefaultValue; const FString OldTargetDefault = Target->DefaultValue;
        UObject* OldSourceObject = Source->DefaultObject; UObject* OldTargetObject = Target->DefaultObject;
        const FText OldSourceText = Source->DefaultTextValue; const FText OldTargetText = Target->DefaultTextValue;
        const ENodeEnabledState OldSourceEnabled = SourceNode->GetDesiredEnabledState(); const ENodeEnabledState OldTargetEnabled = TargetNode->GetDesiredEnabledState();
        const bool OldSourceUserEnabled = SourceNode->HasUserSetTheEnabledState(); const bool OldTargetUserEnabled = TargetNode->HasUserSetTheEnabledState();
        const FString OldSourceComment = SourceNode->NodeComment; const FString OldTargetComment = TargetNode->NodeComment;
        const bool OldSourceCommentVisible = SourceNode->bCommentBubbleVisible; const bool OldTargetCommentVisible = TargetNode->bCommentBubbleVisible;
        Blueprint->Modify(); SourceGraph->Modify(); SourceNode->Modify(); TargetNode->Modify(); Source->Modify(); Target->Modify();
        auto RollbackConnection = [&](const FString& Failure)
        {
            Schema->BreakSinglePinLink(Source, Target);
            Schema->TrySetDefaultValue(*Source, OldSourceDefault); Schema->TrySetDefaultValue(*Target, OldTargetDefault); Schema->TrySetDefaultObject(*Source, OldSourceObject); Schema->TrySetDefaultObject(*Target, OldTargetObject); Source->DefaultTextValue = OldSourceText; Target->DefaultTextValue = OldTargetText;
            SourceNode->SetEnabledState(OldSourceEnabled, OldSourceUserEnabled); TargetNode->SetEnabledState(OldTargetEnabled, OldTargetUserEnabled); SourceNode->NodeComment = OldSourceComment; TargetNode->NodeComment = OldTargetComment;
            SourceNode->bCommentBubbleVisible = OldSourceCommentVisible; TargetNode->bCommentBubbleVisible = OldTargetCommentVisible;
            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint); Transaction.Cancel();
            FString After; const bool Verified = VerifyRollback(After); return P11FailedAtomicResponse(Id, Operation, Args, Blueprint->GetPathName(), ExpectedRevision, After, Verified, P11DirtyPackages(*Blueprint, BlueprintPackage && BlueprintPackage->IsDirty()), Verified ? TEXT("pin connection readback failed") : TEXT("pin connection rollback verification failed"));
        };
        if (!Schema->TryCreateConnection(Source, Target) || !Source->LinkedTo.Contains(Target) || !Target->LinkedTo.Contains(Source)) return RollbackConnection(ErrorResponse(Id, TEXT("operation_failed"), TEXT("pin connection readback failed")));
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
        const FString ResultResponse = P11AtomicResponse(Id, SuccessResponse(Id, P11MutationResult(*Blueprint, FString(), FString(), FString(), SourcePinId, TargetPinId, true), Operation, Args));
        return ResponseStatusIsOk(ResultResponse) ? ResultResponse : RollbackConnection(ResultResponse);
    }
    return ErrorResponse(Id, TEXT("unsupported"), TEXT("unsupported P1.1 Blueprint operation"));
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIP11BlueprintConstructionKernel, "MagiUnrealAXI.P11.BlueprintConstructionKernel", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIP11BlueprintConstructionKernel::RunTest(const FString&)
{
    auto ResultObject = [](const FString& Response) -> TSharedPtr<FJsonObject>
    {
        TSharedPtr<FJsonObject> Envelope;
        const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response);
        const TSharedPtr<FJsonObject>* Result = nullptr;
        return FJsonSerializer::Deserialize(Reader, Envelope) && Envelope.IsValid() && Envelope->TryGetObjectField(TEXT("result"), Result) && Result ? *Result : nullptr;
    };
    auto ErrorType = [](const FString& Response) -> FString
    {
        TSharedPtr<FJsonObject> Envelope;
        const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response);
        const TSharedPtr<FJsonObject>* Error = nullptr;
        return FJsonSerializer::Deserialize(Reader, Envelope) && Envelope.IsValid() && Envelope->TryGetObjectField(TEXT("error"), Error) && Error ? (*Error)->GetStringField(TEXT("type")) : FString();
    };
    auto Object = [](std::initializer_list<TPair<FString, FString>> Fields)
    {
        const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
        for (const TPair<FString, FString>& Field : Fields) Result->SetStringField(Field.Key, Field.Value);
        return Result;
    };

    auto QueuedP11 = [](const FString& Id, const FString& Operation, const TSharedPtr<FJsonObject>& Args, const FString& ExpectedRevision) -> FString
    {
        const TSharedRef<FGameThreadRequest> Request = MakeShared<FGameThreadRequest>(); Request->Id = Id; Request->Operation = Operation; Request->Args = Args; Request->ExpectedRevision = ExpectedRevision; Request->Deadline = FPlatformTime::Seconds() + 30.0;
        SetReceipt(Id, Operation, FString(), TEXT("queued"));
        if (!TryEnqueueGameThreadRequest(Request) || !DrainGameThreadQueue(0.0f)) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("P1.1 automation queue dispatch failed"));
        std::lock_guard Lock(Request->Mutex); return Request->Response;
    };
    auto LedgerReceipt = [&](const FString& OperationId) -> TSharedPtr<FJsonObject>
    {
        return ResultObject(ReadResponseOnGameThread(TEXT("p11-operation-view"), TEXT("operation.view"), Object({{TEXT("id"), OperationId}})));
    };
    auto TestFailedLedgerReceipt = [&](const FString& Label, const FString& OperationId, const FString& Operation, const FString& ExpectedState)
    {
        const TSharedPtr<FJsonObject> Receipt = LedgerReceipt(OperationId);
        TestTrue(*(Label + TEXT(" receipt is visible through operation.view")), Receipt.IsValid());
        if (Receipt.IsValid())
        {
            TestEqual(*(Label + TEXT(" operation is exact")), Receipt->GetStringField(TEXT("operation")), Operation);
            TestEqual(*(Label + TEXT(" state is exact")), Receipt->GetStringField(TEXT("state")), ExpectedState);
            TestEqual(*(Label + TEXT(" transaction binds catalog")), Receipt->GetStringField(TEXT("transaction")), FString(TEXT("atomic")));
            TestEqual(*(Label + TEXT(" reversibility binds catalog")), Receipt->GetStringField(TEXT("reversibility")), FString(TEXT("source-control")));
            const TSharedPtr<FJsonObject>* Verification = nullptr;
            TestTrue(*(Label + TEXT(" verification is present")), Receipt->TryGetObjectField(TEXT("verification"), Verification) && Verification && Verification->IsValid());
        }
    };
    const FString RollbackCreatePath = TEXT("/Game/MagiP11Automation/BP_CreateRollback");
    const TSharedRef<FJsonObject> RollbackCreate = Object({{TEXT("path"), RollbackCreatePath}, {TEXT("parentClass"), TEXT("/Script/Engine.StaticMeshActor")}});
    GP11ForceAtomicFailure = true;
    const FString RollbackCreateResponse = QueuedP11(TEXT("p11-create-rollback"), TEXT("blueprint.create"), RollbackCreate, FString());
    GP11ForceAtomicFailure = false;
    TestEqual(TEXT("injected create postcondition failure returns error"), ErrorType(RollbackCreateResponse), FString(TEXT("operation_failed")));
    TestFailedLedgerReceipt(TEXT("create rollback"), TEXT("p11-create-rollback"), TEXT("blueprint.create"), TEXT("failed"));
    TSharedPtr<FJsonObject> FailedEnvelope; const TSharedRef<TJsonReader<>> FailedReader = TJsonReaderFactory<>::Create(RollbackCreateResponse); const TSharedPtr<FJsonObject>* FailedReceipt = nullptr;
    TestTrue(TEXT("failed atomic create includes receipt"), FJsonSerializer::Deserialize(FailedReader, FailedEnvelope) && FailedEnvelope.IsValid() && FailedEnvelope->TryGetObjectField(TEXT("receipt"), FailedReceipt) && FailedReceipt && FailedReceipt->IsValid());
    const TSharedPtr<FJsonObject> ViewedCreateReceipt = LedgerReceipt(TEXT("p11-create-rollback"));
    TestEqual(TEXT("failed atomic receipt round-trips exactly through operation.view"), ViewedCreateReceipt.IsValid() && FailedReceipt && FailedReceipt->IsValid() ? Serialize(ViewedCreateReceipt.ToSharedRef()) : FString(), FailedReceipt && FailedReceipt->IsValid() ? Serialize((*FailedReceipt).ToSharedRef()) : FString(TEXT("missing")));
    TestNull(TEXT("failed create removes Blueprint asset"), P11LoadBlueprint(RollbackCreatePath + TEXT(".BP_CreateRollback")));
    TestNull(TEXT("failed create removes exact package"), FindPackage(nullptr, *RollbackCreatePath));
    TestFalse(TEXT("failed create removes registry asset"), FAssetRegistryModule::GetRegistry().GetAssetByObjectPath(FSoftObjectPath(RollbackCreatePath + TEXT(".BP_CreateRollback"))).IsValid());
    const FString RetryResponse = HandleP11BlueprintOperation(TEXT("p11-create-retry"), TEXT("blueprint.create"), RollbackCreate, FString());
    TestTrue(TEXT("same-path retry succeeds after failed create"), ResponseStatusIsOk(RetryResponse));
    UPackage* RetryPackage = FindPackage(nullptr, *RollbackCreatePath);
    TestNotNull(TEXT("same-path retry restores exact package"), RetryPackage);
    if (RetryPackage)
    {
        TestTrue(TEXT("retry package disposal succeeds"), P11DiscardCreatedBlueprint(*RetryPackage, P11LoadBlueprint(RollbackCreatePath + TEXT(".BP_CreateRollback")), true));
        TestNull(TEXT("retry package disposal removes exact package"), FindPackage(nullptr, *RollbackCreatePath));
    }
    const FString DirtyPackagePath = TEXT("/Game/MagiP11Automation/BP_PreexistingDirty");
    UPackage* DirtyPackage = CreatePackage(*DirtyPackagePath);
    DirtyPackage->MarkPackageDirty();
    const bool WasDirty = DirtyPackage->IsDirty();
    const FString DirtyResponse = HandleP11BlueprintOperation(TEXT("p11-create-dirty-package"), TEXT("blueprint.create"), Object({{TEXT("path"), DirtyPackagePath}, {TEXT("parentClass"), TEXT("/Script/Engine.StaticMeshActor")}}), FString());
    TestEqual(TEXT("pre-existing loaded package returns conflict"), ErrorType(DirtyResponse), FString(TEXT("conflict")));
    TestTrue(TEXT("pre-existing package pointer is unchanged"), FindPackage(nullptr, *DirtyPackagePath) == DirtyPackage);
    TestEqual(TEXT("pre-existing package dirty state is unchanged"), DirtyPackage->IsDirty(), WasDirty);
    TestTrue(TEXT("pre-existing package disposal succeeds"), P11DiscardCreatedBlueprint(*DirtyPackage, nullptr, false));
    TestNull(TEXT("pre-existing package disposal removes exact package"), FindPackage(nullptr, *DirtyPackagePath));

    const FString PackagePath = TEXT("/Game/MagiP11Automation/BP_Kernel");
    const FString BlueprintId = PackagePath + TEXT(".BP_Kernel");
    const TSharedRef<FJsonObject> Create = Object({{TEXT("path"), PackagePath}, {TEXT("parentClass"), TEXT("/Script/Engine.StaticMeshActor")}});
    const FString CreateResponse = HandleP11BlueprintOperation(TEXT("p11-create"), TEXT("blueprint.create"), Create, FString());
    TestTrue(TEXT("bounded Blueprint create succeeds"), ResponseStatusIsOk(CreateResponse));
    UBlueprint* Blueprint = P11LoadBlueprint(BlueprintId);
    TestNotNull(TEXT("created Blueprint has exact identity"), Blueprint);
    if (!Blueprint || Blueprint->UbergraphPages.IsEmpty()) return false;
    UEdGraph* Graph = Blueprint->UbergraphPages[0];
    const FString GraphId = BlueprintGraphIdentity(*Blueprint, *Graph);
    TestTrue(TEXT("ubergraph identity is stable"), !GraphId.IsEmpty());

    auto Ensure = [&](const FString& Operation, const FString& Key, const FString& Field, const FString& Intent)
    {
        const FString Before = BlueprintContentRevision(*Blueprint);
        const TSharedRef<FJsonObject> Args = Object({{TEXT("blueprintId"), BlueprintId}, {TEXT("graphId"), GraphId}, {TEXT("agentKey"), Key}, {Field, Intent}});
        const FString Response = HandleP11BlueprintOperation(Key, Operation, Args, Before);
        TestTrue(*(Operation + TEXT(" ") + Intent + TEXT(" succeeds")), ResponseStatusIsOk(Response));
        return ResultObject(Response);
    };
    const TSharedRef<FJsonObject> RollbackClaimArgs = Object({{TEXT("blueprintId"), BlueprintId}, {TEXT("graphId"), GraphId}, {TEXT("agentKey"), TEXT("p11.begin")}, {TEXT("event"), TEXT("actor.begin_play")}});
    Blueprint->GetOutermost()->SetDirtyFlag(false);
    const EBlueprintStatus BeforeClaimStatus = Blueprint->Status;
    const FString BeforeClaimRollback = BlueprintContentRevision(*Blueprint);
    GP11ForceAtomicFailure = true;
    const FString RollbackClaimResponse = QueuedP11(TEXT("p11-claim-rollback"), TEXT("blueprint.event_ensure"), RollbackClaimArgs, BeforeClaimRollback);
    GP11ForceAtomicFailure = false;
    TestEqual(TEXT("injected event claim postcondition failure returns error"), ErrorType(RollbackClaimResponse), FString(TEXT("operation_failed")));
    TestFailedLedgerReceipt(TEXT("event rollback"), TEXT("p11-claim-rollback"), TEXT("blueprint.event_ensure"), TEXT("failed"));
    TestEqual(TEXT("failed event claim restores exact revision"), BlueprintContentRevision(*Blueprint), BeforeClaimRollback);
    TestFalse(TEXT("failed event claim restores clean package state"), Blueprint->GetOutermost()->IsDirty());
    TestEqual(TEXT("failed event claim restores Blueprint status"), Blueprint->Status, BeforeClaimStatus);

    TSharedPtr<FJsonObject> Begin = Ensure(TEXT("blueprint.event_ensure"), TEXT("p11.begin"), TEXT("event"), TEXT("actor.begin_play"));
    TSharedPtr<FJsonObject> Input = Ensure(TEXT("blueprint.event_ensure"), TEXT("p11.input"), TEXT("event"), TEXT("input.key_e"));
    TSharedPtr<FJsonObject> Controller = Ensure(TEXT("blueprint.node_ensure"), TEXT("p11.controller"), TEXT("node"), TEXT("game.get_player_controller"));
    TSharedPtr<FJsonObject> Enable = Ensure(TEXT("blueprint.node_ensure"), TEXT("p11.enable"), TEXT("node"), TEXT("actor.enable_input"));
    TSharedPtr<FJsonObject> Vector = Ensure(TEXT("blueprint.node_ensure"), TEXT("p11.vector"), TEXT("node"), TEXT("math.make_vector"));
    TSharedPtr<FJsonObject> Offset = Ensure(TEXT("blueprint.node_ensure"), TEXT("p11.offset"), TEXT("node"), TEXT("actor.add_world_offset"));
    Blueprint->GetOutermost()->SetDirtyFlag(false);
    const EBlueprintStatus BeforeNodeStatus = Blueprint->Status;
    const FString BeforeNodeRollback = BlueprintContentRevision(*Blueprint);
    const TSharedRef<FJsonObject> RollbackNodeArgs = Object({{TEXT("blueprintId"), BlueprintId}, {TEXT("graphId"), GraphId}, {TEXT("agentKey"), TEXT("p11.rollback-node")}, {TEXT("node"), TEXT("math.make_vector")}});
    GP11ForceRollbackVerificationFailure = true;
    GP11ForceAtomicFailure = true;
    const FString RollbackNodeResponse = QueuedP11(TEXT("p11-node-rollback"), TEXT("blueprint.node_ensure"), RollbackNodeArgs, BeforeNodeRollback);
    GP11ForceAtomicFailure = false;
    GP11ForceRollbackVerificationFailure = false;
    TestEqual(TEXT("unverified node rollback returns outcome_unknown"), ErrorType(RollbackNodeResponse), FString(TEXT("outcome_unknown")));
    TestFailedLedgerReceipt(TEXT("node rollback"), TEXT("p11-node-rollback"), TEXT("blueprint.node_ensure"), TEXT("outcome_unknown"));
    TestEqual(TEXT("failed node insertion restores exact revision"), BlueprintContentRevision(*Blueprint), BeforeNodeRollback);
    TestFalse(TEXT("failed node insertion restores clean package state"), Blueprint->GetOutermost()->IsDirty());
    TestEqual(TEXT("failed node insertion restores Blueprint status"), Blueprint->Status, BeforeNodeStatus);

    const TSharedRef<FJsonObject> CrossKeyEventArgs = Object({{TEXT("blueprintId"), BlueprintId}, {TEXT("graphId"), GraphId}, {TEXT("agentKey"), TEXT("p11.other" )}, {TEXT("event"), TEXT("actor.begin_play")} });
    TestEqual(TEXT("singleton event owner cannot be reclaimed before links"), ErrorType(HandleP11BlueprintOperation(TEXT("p11-cross-key-before"), TEXT("blueprint.event_ensure"), CrossKeyEventArgs, BlueprintContentRevision(*Blueprint))), FString(TEXT("conflict")));
    if (!Begin || !Input || !Controller || !Enable || !Vector || !Offset) return false;

    const FString StableRevision = BlueprintContentRevision(*Blueprint);
    const TSharedRef<FJsonObject> RepeatArgs = Object({{TEXT("blueprintId"), BlueprintId}, {TEXT("graphId"), GraphId}, {TEXT("agentKey"), TEXT("p11.offset")}, {TEXT("node"), TEXT("actor.add_world_offset")}});
    const TSharedPtr<FJsonObject> Repeat = ResultObject(HandleP11BlueprintOperation(TEXT("p11-repeat"), TEXT("blueprint.node_ensure"), RepeatArgs, StableRevision));
    TestTrue(TEXT("natural-key repeat is safe no-op"), Repeat && !Repeat->GetBoolField(TEXT("changed")) && Repeat->GetStringField(TEXT("revision")) == StableRevision);
    const TSharedRef<FJsonObject> StaleArgs = Object({{TEXT("blueprintId"), BlueprintId}, {TEXT("graphId"), GraphId}, {TEXT("agentKey"), TEXT("p11.stale")}, {TEXT("node"), TEXT("math.make_vector")}});
    TestEqual(TEXT("stale revision returns conflict"), ErrorType(HandleP11BlueprintOperation(TEXT("p11-stale"), TEXT("blueprint.node_ensure"), StaleArgs, TEXT("0"))), FString(TEXT("conflict")));

    auto NodeByIntent = [&](const FString& Intent) -> UEdGraphNode*
    {
        for (UEdGraphNode* Node : Graph->Nodes) if (Node && P11NodeIntent(*Node) == Intent) return Node;
        return nullptr;
    };
    auto PinId = [&](const FString& Intent, const FName Name, EEdGraphPinDirection Direction) -> FString
    {
        UEdGraphNode* Node = NodeByIntent(Intent);
        UEdGraphPin* Pin = Node ? Node->FindPin(Name, Direction) : nullptr;
        return Pin ? BlueprintPinIdentity(BlueprintNodeIdentity(GraphId, *Node), *Pin) : FString();
    };
    auto SetDefault = [&](const FString& Pin, const FString& Type, double Value)
    {
        const TSharedRef<FJsonObject> Args = MakeShared<FJsonObject>();
        Args->SetStringField(TEXT("blueprintId"), BlueprintId); Args->SetStringField(TEXT("pinId"), Pin);
        const TSharedRef<FJsonObject> Typed = MakeShared<FJsonObject>(); Typed->SetStringField(TEXT("type"), Type); Typed->SetNumberField(TEXT("value"), Value); Args->SetObjectField(TEXT("value"), Typed);
        const FString Response = HandleP11BlueprintOperation(TEXT("p11-default"), TEXT("blueprint.pin_default_set"), Args, BlueprintContentRevision(*Blueprint));
        if (!ResponseStatusIsOk(Response)) AddInfo(Response);
        TestTrue(*(FString(TEXT("typed default succeeds for ")) + Pin), ResponseStatusIsOk(Response));
    };
    const FString RollbackDefaultPin = PinId(TEXT("math.make_vector"), TEXT("Z"), EGPD_Input);
    const TSharedRef<FJsonObject> RollbackDefaultArgs = MakeShared<FJsonObject>();
    RollbackDefaultArgs->SetStringField(TEXT("blueprintId"), BlueprintId); RollbackDefaultArgs->SetStringField(TEXT("pinId"), RollbackDefaultPin);
    const TSharedRef<FJsonObject> RollbackTypedDefault = MakeShared<FJsonObject>(); RollbackTypedDefault->SetStringField(TEXT("type"), TEXT("real")); RollbackTypedDefault->SetNumberField(TEXT("value"), 100); RollbackDefaultArgs->SetObjectField(TEXT("value"), RollbackTypedDefault);
    Blueprint->GetOutermost()->SetDirtyFlag(false);
    const EBlueprintStatus BeforeDefaultStatus = Blueprint->Status;
    const FString BeforeDefaultRollback = BlueprintContentRevision(*Blueprint);
    GP11ForceAtomicFailure = true;
    const FString RollbackDefaultResponse = QueuedP11(TEXT("p11-default-rollback"), TEXT("blueprint.pin_default_set"), RollbackDefaultArgs, BeforeDefaultRollback);
    GP11ForceAtomicFailure = false;
    TestEqual(TEXT("injected default postcondition failure returns error"), ErrorType(RollbackDefaultResponse), FString(TEXT("operation_failed")));
    TestFailedLedgerReceipt(TEXT("default rollback"), TEXT("p11-default-rollback"), TEXT("blueprint.pin_default_set"), TEXT("failed"));
    TestEqual(TEXT("failed default update restores exact revision"), BlueprintContentRevision(*Blueprint), BeforeDefaultRollback);
    TestFalse(TEXT("failed default update restores clean package state"), Blueprint->GetOutermost()->IsDirty());
    TestEqual(TEXT("failed default update restores Blueprint status"), Blueprint->Status, BeforeDefaultStatus);

    SetDefault(PinId(TEXT("game.get_player_controller"), TEXT("PlayerIndex"), EGPD_Input), TEXT("integer"), 0);
    SetDefault(PinId(TEXT("math.make_vector"), TEXT("X"), EGPD_Input), TEXT("real"), 0);
    SetDefault(PinId(TEXT("math.make_vector"), TEXT("Y"), EGPD_Input), TEXT("real"), 0);
    SetDefault(PinId(TEXT("math.make_vector"), TEXT("Z"), EGPD_Input), TEXT("real"), 100);

    auto Connect = [&](const FString& SourcePin, const FString& TargetPin)
    {
        const TSharedRef<FJsonObject> Args = Object({{TEXT("blueprintId"), BlueprintId}, {TEXT("sourcePinId"), SourcePin}, {TEXT("targetPinId"), TargetPin}});
        const FString Response = HandleP11BlueprintOperation(TEXT("p11-connect"), TEXT("blueprint.pin_connect"), Args, BlueprintContentRevision(*Blueprint));
        if (!ResponseStatusIsOk(Response)) AddInfo(Response);
        TestTrue(TEXT("allowlisted connection succeeds"), ResponseStatusIsOk(Response));
    };
    const FString RollbackSourcePin = PinId(TEXT("actor.begin_play"), UEdGraphSchema_K2::PN_Then, EGPD_Output);
    const FString RollbackTargetPin = PinId(TEXT("actor.enable_input"), UEdGraphSchema_K2::PN_Execute, EGPD_Input);
    const TSharedRef<FJsonObject> RollbackConnectArgs = Object({{TEXT("blueprintId"), BlueprintId}, {TEXT("sourcePinId"), RollbackSourcePin}, {TEXT("targetPinId"), RollbackTargetPin}});
    Blueprint->GetOutermost()->SetDirtyFlag(false);
    const EBlueprintStatus BeforeConnectionStatus = Blueprint->Status;
    FString BeforeViewErrorType, BeforeViewError;
    const FString BeforeConnectionView = Serialize(P11GraphViewResult(*Blueprint, Object({{TEXT("blueprintId"), BlueprintId}, {TEXT("graphId"), GraphId}}), BeforeViewErrorType, BeforeViewError));
    UEdGraphNode* RollbackSourceNode = NodeByIntent(TEXT("actor.begin_play")); UEdGraphNode* RollbackTargetNode = NodeByIntent(TEXT("actor.enable_input"));
    const bool BeforeSourceUserEnabled = RollbackSourceNode && RollbackSourceNode->HasUserSetTheEnabledState(); const bool BeforeTargetUserEnabled = RollbackTargetNode && RollbackTargetNode->HasUserSetTheEnabledState();
    const bool BeforeSourceCommentVisible = RollbackSourceNode && RollbackSourceNode->bCommentBubbleVisible; const bool BeforeTargetCommentVisible = RollbackTargetNode && RollbackTargetNode->bCommentBubbleVisible;
    const FString BeforeConnectionRollback = BlueprintContentRevision(*Blueprint);
    GP11ForceAtomicFailure = true;
    const FString RollbackConnectResponse = QueuedP11(TEXT("p11-connect-rollback"), TEXT("blueprint.pin_connect"), RollbackConnectArgs, BeforeConnectionRollback);
    GP11ForceAtomicFailure = false;
    TestEqual(TEXT("injected connection postcondition failure returns error"), ErrorType(RollbackConnectResponse), FString(TEXT("operation_failed")));
    TestFailedLedgerReceipt(TEXT("connection rollback"), TEXT("p11-connect-rollback"), TEXT("blueprint.pin_connect"), TEXT("failed"));
    TestEqual(TEXT("failed connection restores exact revision"), BlueprintContentRevision(*Blueprint), BeforeConnectionRollback);
    FString AfterViewErrorType, AfterViewError;
    TestEqual(TEXT("failed connection restores full public graph view"), Serialize(P11GraphViewResult(*Blueprint, Object({{TEXT("blueprintId"), BlueprintId}, {TEXT("graphId"), GraphId}}), AfterViewErrorType, AfterViewError)), BeforeConnectionView);
    TestFalse(TEXT("failed connection restores clean package state"), Blueprint->GetOutermost()->IsDirty());
    TestEqual(TEXT("failed connection restores Blueprint status"), Blueprint->Status, BeforeConnectionStatus);
    TestEqual(TEXT("failed connection restores source user-enabled flag"), RollbackSourceNode && RollbackSourceNode->HasUserSetTheEnabledState(), BeforeSourceUserEnabled);
    TestEqual(TEXT("failed connection restores target user-enabled flag"), RollbackTargetNode && RollbackTargetNode->HasUserSetTheEnabledState(), BeforeTargetUserEnabled);
    TestEqual(TEXT("failed connection restores source comment visibility"), RollbackSourceNode && RollbackSourceNode->bCommentBubbleVisible, BeforeSourceCommentVisible);
    TestEqual(TEXT("failed connection restores target comment visibility"), RollbackTargetNode && RollbackTargetNode->bCommentBubbleVisible, BeforeTargetCommentVisible);

    Connect(PinId(TEXT("actor.begin_play"), UEdGraphSchema_K2::PN_Then, EGPD_Output), PinId(TEXT("actor.enable_input"), UEdGraphSchema_K2::PN_Execute, EGPD_Input));
    Connect(PinId(TEXT("game.get_player_controller"), UEdGraphSchema_K2::PN_ReturnValue, EGPD_Output), PinId(TEXT("actor.enable_input"), TEXT("PlayerController"), EGPD_Input));
    Connect(PinId(TEXT("input.key_e"), TEXT("Pressed"), EGPD_Output), PinId(TEXT("actor.add_world_offset"), UEdGraphSchema_K2::PN_Execute, EGPD_Input));
    Connect(PinId(TEXT("math.make_vector"), UEdGraphSchema_K2::PN_ReturnValue, EGPD_Output), PinId(TEXT("actor.add_world_offset"), TEXT("DeltaLocation"), EGPD_Input));
    const TSharedRef<FJsonObject> CrossKeyAfterLinks = Object({{TEXT("blueprintId"), BlueprintId}, {TEXT("graphId"), GraphId}, {TEXT("agentKey"), TEXT("p11.other.after")}, {TEXT("event"), TEXT("actor.begin_play")} });
    TestEqual(TEXT("singleton event owner cannot be reclaimed after links"), ErrorType(HandleP11BlueprintOperation(TEXT("p11-cross-key-after"), TEXT("blueprint.event_ensure"), CrossKeyAfterLinks, BlueprintContentRevision(*Blueprint))), FString(TEXT("conflict")));

    const FString BeforeInvalid = BlueprintContentRevision(*Blueprint);
    const TSharedRef<FJsonObject> InvalidConnect = Object({{TEXT("blueprintId"), BlueprintId}, {TEXT("sourcePinId"), PinId(TEXT("math.make_vector"), UEdGraphSchema_K2::PN_ReturnValue, EGPD_Output)}, {TEXT("targetPinId"), PinId(TEXT("actor.enable_input"), TEXT("PlayerController"), EGPD_Input)}});
    TestEqual(TEXT("non-allowlisted connection is invalid"), ErrorType(HandleP11BlueprintOperation(TEXT("p11-invalid-connect"), TEXT("blueprint.pin_connect"), InvalidConnect, BeforeInvalid)), FString(TEXT("invalid_input")));
    TestEqual(TEXT("invalid connection preserves revision"), BlueprintContentRevision(*Blueprint), BeforeInvalid);

    const TSharedRef<FJsonObject> ViewArgs = Object({{TEXT("blueprintId"), BlueprintId}, {TEXT("graphId"), GraphId}});
    const TSharedPtr<FJsonObject> View = ResultObject(HandleP11BlueprintOperation(TEXT("p11-view"), TEXT("blueprint.graph_view"), ViewArgs, FString()));
    TestTrue(TEXT("bounded graph read returns authored nodes"), View && View->GetIntegerField(TEXT("total")) >= 6 && View->GetArrayField(TEXT("items")).Num() >= 6);
    FCompilerResultsLog Results; Results.bSilentMode = true;
    FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave, &Results);
    TestTrue(TEXT("authored Blueprint compiles without errors"), Results.NumErrors == 0 && Blueprint->Status != BS_Error);
    return true;
}
#endif
