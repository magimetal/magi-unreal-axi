#pragma once

#define MAGI_AXI_CATALOG_HASH TEXT("b1888d3416a0873e31b1b600f6c84a7e01cdce982fedc8088ab42e8b76c3b506")
#define MAGI_AXI_CATALOG_COUNT 34
#define MAGI_AXI_NATIVE_CAPABILITY_COUNT 32
#define MAGI_AXI_PUBLIC_OPERATION_COUNT 35
#define MAGI_AXI_NATIVE_CAPABILITIES(X) \
    X(TEXT("actor.delete")) \
    X(TEXT("actor.list")) \
    X(TEXT("actor.spawn")) \
    X(TEXT("actor.update_transform")) \
    X(TEXT("actor.view")) \
    X(TEXT("asset.create_input_action")) \
    X(TEXT("asset.create_input_mapping_context")) \
    X(TEXT("asset.save")) \
    X(TEXT("blueprint.compile")) \
    X(TEXT("blueprint.view")) \
    X(TEXT("component.add")) \
    X(TEXT("component.list")) \
    X(TEXT("component.remove")) \
    X(TEXT("component.update")) \
    X(TEXT("component.view")) \
    X(TEXT("level.set_game_mode")) \
    X(TEXT("level.settings")) \
    X(TEXT("play.input")) \
    X(TEXT("play.observe")) \
    X(TEXT("play.screenshot")) \
    X(TEXT("play.start")) \
    X(TEXT("play.status")) \
    X(TEXT("play.stop")) \
    X(TEXT("asset.list")) \
    X(TEXT("asset.view")) \
    X(TEXT("editor.status")) \
    X(TEXT("level.create")) \
    X(TEXT("level.current")) \
    X(TEXT("level.list")) \
    X(TEXT("level.open")) \
    X(TEXT("level.save")) \
    X(TEXT("operation.view"))

#define MAGI_AXI_PUBLIC_OPERATIONS(X) \
    X(TEXT("bridge.health")) \
    X(TEXT("bridge.describe")) \
    X(TEXT("actor.delete")) \
    X(TEXT("actor.list")) \
    X(TEXT("actor.spawn")) \
    X(TEXT("actor.update_transform")) \
    X(TEXT("actor.view")) \
    X(TEXT("asset.create_input_action")) \
    X(TEXT("asset.create_input_mapping_context")) \
    X(TEXT("asset.save")) \
    X(TEXT("blueprint.compile")) \
    X(TEXT("blueprint.view")) \
    X(TEXT("component.add")) \
    X(TEXT("component.list")) \
    X(TEXT("component.remove")) \
    X(TEXT("component.update")) \
    X(TEXT("component.view")) \
    X(TEXT("level.set_game_mode")) \
    X(TEXT("level.settings")) \
    X(TEXT("play.input")) \
    X(TEXT("play.observe")) \
    X(TEXT("play.screenshot")) \
    X(TEXT("play.start")) \
    X(TEXT("play.status")) \
    X(TEXT("play.stop")) \
    X(TEXT("asset.list")) \
    X(TEXT("asset.view")) \
    X(TEXT("editor.status")) \
    X(TEXT("level.create")) \
    X(TEXT("level.current")) \
    X(TEXT("level.list")) \
    X(TEXT("level.open")) \
    X(TEXT("level.save")) \
    X(TEXT("operation.view")) \
    X(TEXT("editor.stop"))

#define MAGI_AXI_INVALID_OUTPUT_FIXTURES_JSON TEXT("[{\"operation\":\"actor.delete\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"actor.delete\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"changed\":false,\"dryRun\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"actor.list\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"actor.list\",\"case\":\"root-not-closed\",\"result\":{\"count\":0,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null,\"unknown\":true}},{\"operation\":\"actor.spawn\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"actor.spawn\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"actorGuid\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"actor.update_transform\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"actor.update_transform\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"actor.view\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"actor.view\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"actorGuid\":\"x\",\"levelId\":\"x\",\"label\":\"x\",\"class\":\"x\",\"objectPath\":\"x\",\"location\":[-1000000000.0,-1000000000.0,-1000000000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"asset.create_input_action\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"asset.create_input_action\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"class\":\"x\",\"valueType\":\"Boolean\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"asset.create_input_mapping_context\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"asset.create_input_mapping_context\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"class\":\"x\",\"mappingCount\":0,\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"asset.save\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"asset.save\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"blueprint.compile\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"blueprint.compile\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"parentClass\":\"x\",\"generatedClass\":\"x\",\"status\":\"unknown\",\"graphCount\":0,\"errorCount\":0,\"warningCount\":0,\"diagnostics\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"unknown\":true}},{\"operation\":\"blueprint.view\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"blueprint.view\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"parentClass\":\"x\",\"generatedClass\":\"x\",\"status\":\"unknown\",\"graphCount\":0,\"errorCount\":0,\"warningCount\":0,\"diagnostics\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"component.add\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"component.add\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"component.list\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"component.list\",\"case\":\"root-not-closed\",\"result\":{\"count\":0,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null,\"unknown\":true}},{\"operation\":\"component.remove\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"component.remove\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"changed\":false,\"dryRun\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"component.update\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"component.update\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"component.view\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"component.view\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"actorId\":\"x\",\"name\":\"x\",\"class\":\"x\",\"scene\":false,\"location\":[-1000000000.0,-1000000000.0,-1000000000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"level.set_game_mode\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"level.set_game_mode\",\"case\":\"root-not-closed\",\"result\":{\"levelId\":\"x\",\"gameModeClass\":\"x\",\"defaultPawnClass\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"level.settings\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"level.settings\",\"case\":\"root-not-closed\",\"result\":{\"levelId\":\"x\",\"gameModeClass\":\"\",\"defaultPawnClass\":\"\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"play.input\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"play.input\",\"case\":\"root-not-closed\",\"result\":{\"sessionId\":\"x\",\"key\":\"x\",\"event\":\"pressed\",\"accepted\":false,\"changed\":false,\"beforeRevision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"afterRevision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"play.observe\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"play.observe\",\"case\":\"root-not-closed\",\"result\":{\"sessionId\":\"x\",\"worldId\":null,\"levelId\":null,\"actors\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"play.screenshot\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"play.screenshot\",\"case\":\"root-not-closed\",\"result\":{\"sessionId\":\"x\",\"path\":\"x\",\"width\":1,\"height\":1,\"format\":\"png\",\"changed\":false,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"play.start\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"play.start\",\"case\":\"root-not-closed\",\"result\":{\"sessionId\":\"x\",\"state\":\"starting\",\"worldId\":null,\"levelId\":null,\"changed\":false,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"play.status\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"play.status\",\"case\":\"root-not-closed\",\"result\":{\"state\":\"stopped\",\"sessionId\":null,\"worldId\":null,\"levelId\":null,\"playerCount\":0,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"play.stop\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"play.stop\",\"case\":\"root-not-closed\",\"result\":{\"sessionId\":\"x\",\"state\":\"stopping\",\"changed\":false,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"asset.list\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"asset.list\",\"case\":\"root-not-closed\",\"result\":{\"count\":0,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null,\"unknown\":true}},{\"operation\":\"asset.view\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"asset.view\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"packagePath\":\"x\",\"objectPath\":\"x\",\"name\":\"x\",\"class\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"capability.describe\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"capability.describe\",\"case\":\"root-not-closed\",\"result\":{\"capability\":{\"id\":\"x\",\"version\":1,\"domain\":\"x\",\"summary\":\"x\",\"execution\":\"local\",\"mutates\":false,\"destructive\":false,\"idempotency\":\"x\",\"saveBehavior\":\"x\",\"transactionBehavior\":\"x\",\"reversibility\":\"x\",\"allowedEditorStates\":[],\"requiresModules\":[],\"inputSchema\":\"xx\",\"outputSchema\":\"xx\",\"verification\":\"xx\",\"engineSupport\":\"xx\"},\"runtime\":{\"available\":false,\"catalogHash\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"},\"unknown\":true}},{\"operation\":\"capability.search\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"capability.search\",\"case\":\"root-not-closed\",\"result\":{\"count\":0,\"total\":0,\"scope\":\"x\",\"items\":[],\"nextCursor\":null,\"unknown\":true}},{\"operation\":\"editor.status\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"editor.status\",\"case\":\"root-not-closed\",\"result\":{\"state\":\"starting\",\"projectId\":\"x\",\"editorPid\":0,\"levelId\":\"\",\"pie\":\"running\",\"dirtyPackageCount\":0,\"unknown\":true}},{\"operation\":\"level.create\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"level.create\",\"case\":\"root-not-closed\",\"result\":{\"level\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"level.current\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"level.current\",\"case\":\"root-not-closed\",\"result\":{\"level\":{\"id\":\"x\",\"name\":\"x\",\"worldType\":\"x\",\"persistent\":false,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"},\"scope\":\"x\",\"unknown\":true}},{\"operation\":\"level.list\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"level.list\",\"case\":\"root-not-closed\",\"result\":{\"count\":0,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null,\"unknown\":true}},{\"operation\":\"level.open\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"level.open\",\"case\":\"root-not-closed\",\"result\":{\"level\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"level.save\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"level.save\",\"case\":\"root-not-closed\",\"result\":{\"level\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"operation.view\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"operation.view\",\"case\":\"root-not-closed\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false},\"unknown\":true}},{\"operation\":\"actor.list\",\"case\":\"missing-list-id\",\"result\":{\"count\":1,\"total\":1,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[{\"label\":\"actor\"}],\"nextCursor\":null}},{\"operation\":\"level.current\",\"case\":\"nested-not-closed\",\"result\":{\"level\":{\"id\":\"x\",\"name\":\"x\",\"worldType\":\"x\",\"persistent\":false,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true},\"scope\":\"x\"}},{\"operation\":\"editor.status\",\"case\":\"wrong-type\",\"result\":{\"state\":\"starting\",\"projectId\":\"x\",\"editorPid\":\"not-an-integer\",\"levelId\":\"\",\"pie\":\"running\",\"dirtyPackageCount\":0}},{\"operation\":\"editor.status\",\"case\":\"enum\",\"result\":{\"state\":\"invalid\",\"projectId\":\"x\",\"editorPid\":0,\"levelId\":\"\",\"pie\":\"running\",\"dirtyPackageCount\":0}},{\"operation\":\"actor.view\",\"case\":\"string-bound\",\"result\":{\"id\":\"x\",\"actorGuid\":\"x\",\"levelId\":\"x\",\"label\":\"x\",\"class\":\"x\",\"objectPath\":\"x\",\"location\":[-1000000000.0,-1000000000.0,-1000000000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"revision\":\"short\"}},{\"operation\":\"asset.list\",\"case\":\"array-bound\",\"result\":{\"count\":0,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"}],\"nextCursor\":null}}]")

static int32 MagiAxiUnicodeScalarCount(const FString& Text)
{
    int32 Count = 0;
    for (int32 Index = 0; Index < Text.Len(); ++Index, ++Count)
    {
        const uint32 CodeUnit = static_cast<uint32>(Text[Index]);
        if (CodeUnit >= 0xD800 && CodeUnit <= 0xDBFF && Index + 1 < Text.Len())
        {
            const uint32 Next = static_cast<uint32>(Text[Index + 1]);
            if (Next >= 0xDC00 && Next <= 0xDFFF) ++Index;
        }
    }
    return Count;
}

static bool MagiAxiValidateInput(const FString& Operation, const TSharedRef<FJsonObject>& Object)
{
    const TSharedPtr<FJsonValue> Root = MakeShared<FJsonValueObject>(Object);
    if (Operation == TEXT("actor.delete"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("force") && Entry.Key != TEXT("dryRun")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("force"));
        if (Value3.IsValid())
        {
            if (!Value3.IsValid() || Value3->Type != EJson::Boolean) return false;
        }
        const TSharedPtr<FJsonValue> Value4 = Object0->TryGetField(TEXT("dryRun"));
        if (Value4.IsValid())
        {
            if (!Value4.IsValid() || Value4->Type != EJson::Boolean) return false;
        }
        return true;
    }
    if (Operation == TEXT("actor.list"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("limit") && Entry.Key != TEXT("cursor") && Entry.Key != TEXT("fields")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("limit"));
        if (Value1.IsValid())
        {
            double Number2 = 0;
            if (!Value1.IsValid() || !Value1->TryGetNumber(Number2) || !FMath::IsFinite(Number2) || FMath::FloorToDouble(Number2) != Number2) return false;
            if (Number2 < 1) return false;
            if (Number2 > 100) return false;
        }
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("cursor"));
        if (Value3.IsValid())
        {
            if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
            const FString Text4 = Value3->AsString();
            if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text4) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("fields"));
        if (Value5.IsValid())
        {
            if (!Value5.IsValid() || Value5->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array6 = Value5->AsArray();
            if (Array6.Num() < 1) return false;
            if (Array6.Num() > 4) return false;
            for (const TSharedPtr<FJsonValue>& Value7 : Array6)
            {
                if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
                const FString Text8 = Value7->AsString();
                if (MagiAxiUnicodeScalarCount(Text8) > 32) return false;
                if (Text8 != TEXT("id") && Text8 != TEXT("label") && Text8 != TEXT("class") && Text8 != TEXT("levelId")) return false;
            }
        }
        return true;
    }
    if (Operation == TEXT("actor.spawn"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("levelId") && Entry.Key != TEXT("class") && Entry.Key != TEXT("agentKey") && Entry.Key != TEXT("label") && Entry.Key != TEXT("location") && Entry.Key != TEXT("rotation") && Entry.Key != TEXT("scale")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("levelId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("class"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("agentKey"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 256) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("label"));
        if (Value7.IsValid())
        {
            if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
            const FString Text8 = Value7->AsString();
            if (MagiAxiUnicodeScalarCount(Text8) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text8) > 512) return false;
        }
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("location"));
        if (Value9.IsValid())
        {
            if (!Value9.IsValid() || Value9->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array10 = Value9->AsArray();
            if (Array10.Num() < 3) return false;
            if (Array10.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value11 : Array10)
            {
                double Number12 = 0;
                if (!Value11.IsValid() || !Value11->TryGetNumber(Number12) || !FMath::IsFinite(Number12)) return false;
                if (Number12 < -1000000000) return false;
                if (Number12 > 1000000000) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value13 = Object0->TryGetField(TEXT("rotation"));
        if (Value13.IsValid())
        {
            if (!Value13.IsValid() || Value13->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array14 = Value13->AsArray();
            if (Array14.Num() < 3) return false;
            if (Array14.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value15 : Array14)
            {
                double Number16 = 0;
                if (!Value15.IsValid() || !Value15->TryGetNumber(Number16) || !FMath::IsFinite(Number16)) return false;
                if (Number16 < -360) return false;
                if (Number16 > 360) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value17 = Object0->TryGetField(TEXT("scale"));
        if (Value17.IsValid())
        {
            if (!Value17.IsValid() || Value17->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array18 = Value17->AsArray();
            if (Array18.Num() < 3) return false;
            if (Array18.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value19 : Array18)
            {
                double Number20 = 0;
                if (!Value19.IsValid() || !Value19->TryGetNumber(Number20) || !FMath::IsFinite(Number20)) return false;
                if (Number20 < 0) return false;
                if (Number20 > 1000000) return false;
            }
        }
        return true;
    }
    if (Operation == TEXT("actor.update_transform"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("location") && Entry.Key != TEXT("rotation") && Entry.Key != TEXT("scale")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("location"));
        if (Value3.IsValid())
        {
            if (!Value3.IsValid() || Value3->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array4 = Value3->AsArray();
            if (Array4.Num() < 3) return false;
            if (Array4.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value5 : Array4)
            {
                double Number6 = 0;
                if (!Value5.IsValid() || !Value5->TryGetNumber(Number6) || !FMath::IsFinite(Number6)) return false;
                if (Number6 < -1000000000) return false;
                if (Number6 > 1000000000) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("rotation"));
        if (Value7.IsValid())
        {
            if (!Value7.IsValid() || Value7->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array8 = Value7->AsArray();
            if (Array8.Num() < 3) return false;
            if (Array8.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value9 : Array8)
            {
                double Number10 = 0;
                if (!Value9.IsValid() || !Value9->TryGetNumber(Number10) || !FMath::IsFinite(Number10)) return false;
                if (Number10 < -360) return false;
                if (Number10 > 360) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("scale"));
        if (Value11.IsValid())
        {
            if (!Value11.IsValid() || Value11->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array12 = Value11->AsArray();
            if (Array12.Num() < 3) return false;
            if (Array12.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value13 : Array12)
            {
                double Number14 = 0;
                if (!Value13.IsValid() || !Value13->TryGetNumber(Number14) || !FMath::IsFinite(Number14)) return false;
                if (Number14 < 0) return false;
                if (Number14 > 1000000) return false;
            }
        }
        return true;
    }
    if (Operation == TEXT("actor.view"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        return true;
    }
    if (Operation == TEXT("asset.create_input_action"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("path") && Entry.Key != TEXT("valueType")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("path"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("valueType"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) > 8) return false;
        if (Text4 != TEXT("Boolean") && Text4 != TEXT("Axis1D")) return false;
        return true;
    }
    if (Operation == TEXT("asset.create_input_mapping_context"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("path") && Entry.Key != TEXT("mappings")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("path"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("mappings"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array4 = Value3->AsArray();
        if (Array4.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value5 : Array4)
        {
            if (!Value5.IsValid() || Value5->Type != EJson::Object) return false;
            const TSharedPtr<FJsonObject> Object6 = Value5->AsObject();
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object6->Values) if (Entry.Key != TEXT("actionId") && Entry.Key != TEXT("key")) return false;
            const TSharedPtr<FJsonValue> Value7 = Object6->TryGetField(TEXT("actionId"));
            if (!Value7.IsValid()) return false;
            if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
            const FString Text8 = Value7->AsString();
            if (MagiAxiUnicodeScalarCount(Text8) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text8) > 512) return false;
            const TSharedPtr<FJsonValue> Value9 = Object6->TryGetField(TEXT("key"));
            if (!Value9.IsValid()) return false;
            if (!Value9.IsValid() || Value9->Type != EJson::String) return false;
            const FString Text10 = Value9->AsString();
            if (MagiAxiUnicodeScalarCount(Text10) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text10) > 128) return false;
        }
        return true;
    }
    if (Operation == TEXT("asset.save"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        return true;
    }
    if (Operation == TEXT("blueprint.compile"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        return true;
    }
    if (Operation == TEXT("blueprint.view"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        return true;
    }
    if (Operation == TEXT("component.add"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("actorId") && Entry.Key != TEXT("class") && Entry.Key != TEXT("name") && Entry.Key != TEXT("location")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("actorId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("class"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("name"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 512) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("location"));
        if (Value7.IsValid())
        {
            if (!Value7.IsValid() || Value7->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array8 = Value7->AsArray();
            if (Array8.Num() < 3) return false;
            if (Array8.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value9 : Array8)
            {
                double Number10 = 0;
                if (!Value9.IsValid() || !Value9->TryGetNumber(Number10) || !FMath::IsFinite(Number10)) return false;
                if (Number10 < -1000000000) return false;
                if (Number10 > 1000000000) return false;
            }
        }
        return true;
    }
    if (Operation == TEXT("component.list"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("actorId") && Entry.Key != TEXT("limit") && Entry.Key != TEXT("cursor") && Entry.Key != TEXT("fields")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("actorId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("limit"));
        if (Value3.IsValid())
        {
            double Number4 = 0;
            if (!Value3.IsValid() || !Value3->TryGetNumber(Number4) || !FMath::IsFinite(Number4) || FMath::FloorToDouble(Number4) != Number4) return false;
            if (Number4 < 1) return false;
            if (Number4 > 100) return false;
        }
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("cursor"));
        if (Value5.IsValid())
        {
            if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
            const FString Text6 = Value5->AsString();
            if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text6) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("fields"));
        if (Value7.IsValid())
        {
            if (!Value7.IsValid() || Value7->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array8 = Value7->AsArray();
            if (Array8.Num() < 1) return false;
            if (Array8.Num() > 4) return false;
            for (const TSharedPtr<FJsonValue>& Value9 : Array8)
            {
                if (!Value9.IsValid() || Value9->Type != EJson::String) return false;
                const FString Text10 = Value9->AsString();
                if (MagiAxiUnicodeScalarCount(Text10) > 32) return false;
                if (Text10 != TEXT("id") && Text10 != TEXT("name") && Text10 != TEXT("class") && Text10 != TEXT("scene")) return false;
            }
        }
        return true;
    }
    if (Operation == TEXT("component.remove"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("force") && Entry.Key != TEXT("dryRun")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("force"));
        if (Value3.IsValid())
        {
            if (!Value3.IsValid() || Value3->Type != EJson::Boolean) return false;
        }
        const TSharedPtr<FJsonValue> Value4 = Object0->TryGetField(TEXT("dryRun"));
        if (Value4.IsValid())
        {
            if (!Value4.IsValid() || Value4->Type != EJson::Boolean) return false;
        }
        return true;
    }
    if (Operation == TEXT("component.update"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("location")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("location"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array4 = Value3->AsArray();
        if (Array4.Num() < 3) return false;
        if (Array4.Num() > 3) return false;
        for (const TSharedPtr<FJsonValue>& Value5 : Array4)
        {
            double Number6 = 0;
            if (!Value5.IsValid() || !Value5->TryGetNumber(Number6) || !FMath::IsFinite(Number6)) return false;
            if (Number6 < -1000000000) return false;
            if (Number6 > 1000000000) return false;
        }
        return true;
    }
    if (Operation == TEXT("component.view"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        return true;
    }
    if (Operation == TEXT("level.set_game_mode"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("levelId") && Entry.Key != TEXT("gameModeClass")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("levelId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("gameModeClass"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        return true;
    }
    if (Operation == TEXT("level.settings"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("levelId")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("levelId"));
        if (Value1.IsValid())
        {
            if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
            const FString Text2 = Value1->AsString();
            if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        }
        return true;
    }
    if (Operation == TEXT("play.input"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("sessionId") && Entry.Key != TEXT("key") && Entry.Key != TEXT("event")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("sessionId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 128) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("key"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 128) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("event"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) > 8) return false;
        if (Text6 != TEXT("pressed") && Text6 != TEXT("released")) return false;
        return true;
    }
    if (Operation == TEXT("play.observe"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("sessionId")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("sessionId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 128) return false;
        return true;
    }
    if (Operation == TEXT("play.screenshot"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("sessionId") && Entry.Key != TEXT("path")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("sessionId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 128) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("path"));
        if (Value3.IsValid())
        {
            if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
            const FString Text4 = Value3->AsString();
            if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        }
        return true;
    }
    if (Operation == TEXT("play.start"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        if (!Object0->Values.IsEmpty()) return false;
        return true;
    }
    if (Operation == TEXT("play.status"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("sessionId")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("sessionId"));
        if (Value1.IsValid())
        {
            if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
            const FString Text2 = Value1->AsString();
            if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text2) > 128) return false;
        }
        return true;
    }
    if (Operation == TEXT("play.stop"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("sessionId")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("sessionId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 128) return false;
        return true;
    }
    if (Operation == TEXT("asset.list"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("limit") && Entry.Key != TEXT("cursor") && Entry.Key != TEXT("fields")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("limit"));
        if (Value1.IsValid())
        {
            double Number2 = 0;
            if (!Value1.IsValid() || !Value1->TryGetNumber(Number2) || !FMath::IsFinite(Number2) || FMath::FloorToDouble(Number2) != Number2) return false;
            if (Number2 < 1) return false;
            if (Number2 > 100) return false;
        }
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("cursor"));
        if (Value3.IsValid())
        {
            if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
            const FString Text4 = Value3->AsString();
            if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text4) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("fields"));
        if (Value5.IsValid())
        {
            if (!Value5.IsValid() || Value5->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array6 = Value5->AsArray();
            if (Array6.Num() < 1) return false;
            if (Array6.Num() > 4) return false;
            for (const TSharedPtr<FJsonValue>& Value7 : Array6)
            {
                if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
                const FString Text8 = Value7->AsString();
                if (MagiAxiUnicodeScalarCount(Text8) > 32) return false;
                if (Text8 != TEXT("id") && Text8 != TEXT("name") && Text8 != TEXT("class") && Text8 != TEXT("packagePath")) return false;
            }
        }
        return true;
    }
    if (Operation == TEXT("asset.view"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        return true;
    }
    if (Operation == TEXT("capability.describe"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 128) return false;
        return true;
    }
    if (Operation == TEXT("capability.search"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("query") && Entry.Key != TEXT("limit")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("query"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 256) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("limit"));
        if (Value3.IsValid())
        {
            double Number4 = 0;
            if (!Value3.IsValid() || !Value3->TryGetNumber(Number4) || !FMath::IsFinite(Number4) || FMath::FloorToDouble(Number4) != Number4) return false;
            if (Number4 < 1) return false;
            if (Number4 > 50) return false;
        }
        return true;
    }
    if (Operation == TEXT("editor.status"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        if (!Object0->Values.IsEmpty()) return false;
        return true;
    }
    if (Operation == TEXT("level.create"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("path")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("path"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        return true;
    }
    if (Operation == TEXT("level.current"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        if (!Object0->Values.IsEmpty()) return false;
        return true;
    }
    if (Operation == TEXT("level.list"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("limit") && Entry.Key != TEXT("cursor") && Entry.Key != TEXT("fields")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("limit"));
        if (Value1.IsValid())
        {
            double Number2 = 0;
            if (!Value1.IsValid() || !Value1->TryGetNumber(Number2) || !FMath::IsFinite(Number2) || FMath::FloorToDouble(Number2) != Number2) return false;
            if (Number2 < 1) return false;
            if (Number2 > 100) return false;
        }
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("cursor"));
        if (Value3.IsValid())
        {
            if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
            const FString Text4 = Value3->AsString();
            if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text4) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("fields"));
        if (Value5.IsValid())
        {
            if (!Value5.IsValid() || Value5->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array6 = Value5->AsArray();
            if (Array6.Num() < 1) return false;
            if (Array6.Num() > 4) return false;
            for (const TSharedPtr<FJsonValue>& Value7 : Array6)
            {
                if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
                const FString Text8 = Value7->AsString();
                if (MagiAxiUnicodeScalarCount(Text8) > 32) return false;
                if (Text8 != TEXT("id") && Text8 != TEXT("name") && Text8 != TEXT("worldType") && Text8 != TEXT("persistent")) return false;
            }
        }
        return true;
    }
    if (Operation == TEXT("level.open"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("path")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("path"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        return true;
    }
    if (Operation == TEXT("level.save"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("path")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("path"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        return true;
    }
    if (Operation == TEXT("operation.view"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 128) return false;
        return true;
    }
    return false;
}

static bool MagiAxiValidateOutput(const FString& Operation, const TSharedRef<FJsonObject>& Object)
{
    const TSharedPtr<FJsonValue> Root = MakeShared<FJsonValueObject>(Object);
    if (Operation == TEXT("actor.delete"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dryRun") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("changed"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value4 = Object0->TryGetField(TEXT("dryRun"));
        if (!Value4.IsValid()) return false;
        if (!Value4.IsValid() || Value4->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("dirtyPackages"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array6 = Value5->AsArray();
        if (Array6.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value7 : Array6)
        {
            if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
            const FString Text8 = Value7->AsString();
            if (MagiAxiUnicodeScalarCount(Text8) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("savedPackages"));
        if (!Value9.IsValid()) return false;
        if (!Value9.IsValid() || Value9->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array10 = Value9->AsArray();
        if (Array10.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value11 : Array10)
        {
            if (!Value11.IsValid() || Value11->Type != EJson::String) return false;
            const FString Text12 = Value11->AsString();
            if (MagiAxiUnicodeScalarCount(Text12) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value13 = Object0->TryGetField(TEXT("revision"));
        if (!Value13.IsValid()) return false;
        if (!Value13.IsValid() || Value13->Type != EJson::String) return false;
        const FString Text14 = Value13->AsString();
        if (MagiAxiUnicodeScalarCount(Text14) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text14) > 64) return false;
        return true;
    }
    if (Operation == TEXT("actor.list"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("count") && Entry.Key != TEXT("total") && Entry.Key != TEXT("scope") && Entry.Key != TEXT("revision") && Entry.Key != TEXT("items") && Entry.Key != TEXT("nextCursor")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("count"));
        if (!Value1.IsValid()) return false;
        double Number2 = 0;
        if (!Value1.IsValid() || !Value1->TryGetNumber(Number2) || !FMath::IsFinite(Number2) || FMath::FloorToDouble(Number2) != Number2) return false;
        if (Number2 < 0) return false;
        if (Number2 > 100) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("total"));
        if (!Value3.IsValid()) return false;
        double Number4 = 0;
        if (!Value3.IsValid() || !Value3->TryGetNumber(Number4) || !FMath::IsFinite(Number4) || FMath::FloorToDouble(Number4) != Number4) return false;
        if (Number4 < 0) return false;
        if (Number4 > 9007199254740991) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("scope"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 512) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("revision"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text8) > 64) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("items"));
        if (!Value9.IsValid()) return false;
        if (!Value9.IsValid() || Value9->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array10 = Value9->AsArray();
        if (Array10.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value11 : Array10)
        {
            if (!Value11.IsValid() || Value11->Type != EJson::Object) return false;
            const TSharedPtr<FJsonObject> Object12 = Value11->AsObject();
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object12->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("label") && Entry.Key != TEXT("class") && Entry.Key != TEXT("levelId")) return false;
            const TSharedPtr<FJsonValue> Value13 = Object12->TryGetField(TEXT("id"));
            if (!Value13.IsValid()) return false;
            if (!Value13.IsValid() || Value13->Type != EJson::String) return false;
            const FString Text14 = Value13->AsString();
            if (MagiAxiUnicodeScalarCount(Text14) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text14) > 512) return false;
            const TSharedPtr<FJsonValue> Value15 = Object12->TryGetField(TEXT("label"));
            if (Value15.IsValid())
            {
                if (!Value15.IsValid() || Value15->Type != EJson::String) return false;
                const FString Text16 = Value15->AsString();
                if (MagiAxiUnicodeScalarCount(Text16) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text16) > 512) return false;
            }
            const TSharedPtr<FJsonValue> Value17 = Object12->TryGetField(TEXT("class"));
            if (Value17.IsValid())
            {
                if (!Value17.IsValid() || Value17->Type != EJson::String) return false;
                const FString Text18 = Value17->AsString();
                if (MagiAxiUnicodeScalarCount(Text18) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text18) > 512) return false;
            }
            const TSharedPtr<FJsonValue> Value19 = Object12->TryGetField(TEXT("levelId"));
            if (Value19.IsValid())
            {
                if (!Value19.IsValid() || Value19->Type != EJson::String) return false;
                const FString Text20 = Value19->AsString();
                if (MagiAxiUnicodeScalarCount(Text20) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text20) > 512) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value21 = Object0->TryGetField(TEXT("nextCursor"));
        if (!Value21.IsValid()) return false;
        if (Value21->Type != EJson::Null)
        {
            if (!Value21.IsValid() || Value21->Type != EJson::String) return false;
            const FString Text22 = Value21->AsString();
            if (MagiAxiUnicodeScalarCount(Text22) > 256) return false;
        }
        return true;
    }
    if (Operation == TEXT("actor.spawn"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("actorGuid") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("actorGuid"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 64) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("changed"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value6 = Object0->TryGetField(TEXT("dirtyPackages"));
        if (!Value6.IsValid()) return false;
        if (!Value6.IsValid() || Value6->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array7 = Value6->AsArray();
        if (Array7.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value8 : Array7)
        {
            if (!Value8.IsValid() || Value8->Type != EJson::String) return false;
            const FString Text9 = Value8->AsString();
            if (MagiAxiUnicodeScalarCount(Text9) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value10 = Object0->TryGetField(TEXT("savedPackages"));
        if (!Value10.IsValid()) return false;
        if (!Value10.IsValid() || Value10->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array11 = Value10->AsArray();
        if (Array11.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value12 : Array11)
        {
            if (!Value12.IsValid() || Value12->Type != EJson::String) return false;
            const FString Text13 = Value12->AsString();
            if (MagiAxiUnicodeScalarCount(Text13) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value14 = Object0->TryGetField(TEXT("revision"));
        if (!Value14.IsValid()) return false;
        if (!Value14.IsValid() || Value14->Type != EJson::String) return false;
        const FString Text15 = Value14->AsString();
        if (MagiAxiUnicodeScalarCount(Text15) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text15) > 64) return false;
        return true;
    }
    if (Operation == TEXT("actor.update_transform"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("changed"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value4 = Object0->TryGetField(TEXT("dirtyPackages"));
        if (!Value4.IsValid()) return false;
        if (!Value4.IsValid() || Value4->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array5 = Value4->AsArray();
        if (Array5.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value6 : Array5)
        {
            if (!Value6.IsValid() || Value6->Type != EJson::String) return false;
            const FString Text7 = Value6->AsString();
            if (MagiAxiUnicodeScalarCount(Text7) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value8 = Object0->TryGetField(TEXT("savedPackages"));
        if (!Value8.IsValid()) return false;
        if (!Value8.IsValid() || Value8->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array9 = Value8->AsArray();
        if (Array9.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value10 : Array9)
        {
            if (!Value10.IsValid() || Value10->Type != EJson::String) return false;
            const FString Text11 = Value10->AsString();
            if (MagiAxiUnicodeScalarCount(Text11) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value12 = Object0->TryGetField(TEXT("revision"));
        if (!Value12.IsValid()) return false;
        if (!Value12.IsValid() || Value12->Type != EJson::String) return false;
        const FString Text13 = Value12->AsString();
        if (MagiAxiUnicodeScalarCount(Text13) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text13) > 64) return false;
        return true;
    }
    if (Operation == TEXT("actor.view"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("actorGuid") && Entry.Key != TEXT("levelId") && Entry.Key != TEXT("label") && Entry.Key != TEXT("class") && Entry.Key != TEXT("objectPath") && Entry.Key != TEXT("location") && Entry.Key != TEXT("rotation") && Entry.Key != TEXT("scale") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("actorGuid"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 64) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("levelId"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 512) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("label"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text8) > 512) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("class"));
        if (!Value9.IsValid()) return false;
        if (!Value9.IsValid() || Value9->Type != EJson::String) return false;
        const FString Text10 = Value9->AsString();
        if (MagiAxiUnicodeScalarCount(Text10) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text10) > 512) return false;
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("objectPath"));
        if (!Value11.IsValid()) return false;
        if (!Value11.IsValid() || Value11->Type != EJson::String) return false;
        const FString Text12 = Value11->AsString();
        if (MagiAxiUnicodeScalarCount(Text12) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text12) > 1024) return false;
        const TSharedPtr<FJsonValue> Value13 = Object0->TryGetField(TEXT("location"));
        if (!Value13.IsValid()) return false;
        if (!Value13.IsValid() || Value13->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array14 = Value13->AsArray();
        if (Array14.Num() < 3) return false;
        if (Array14.Num() > 3) return false;
        for (const TSharedPtr<FJsonValue>& Value15 : Array14)
        {
            double Number16 = 0;
            if (!Value15.IsValid() || !Value15->TryGetNumber(Number16) || !FMath::IsFinite(Number16)) return false;
            if (Number16 < -1000000000) return false;
            if (Number16 > 1000000000) return false;
        }
        const TSharedPtr<FJsonValue> Value17 = Object0->TryGetField(TEXT("rotation"));
        if (!Value17.IsValid()) return false;
        if (!Value17.IsValid() || Value17->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array18 = Value17->AsArray();
        if (Array18.Num() < 3) return false;
        if (Array18.Num() > 3) return false;
        for (const TSharedPtr<FJsonValue>& Value19 : Array18)
        {
            double Number20 = 0;
            if (!Value19.IsValid() || !Value19->TryGetNumber(Number20) || !FMath::IsFinite(Number20)) return false;
            if (Number20 < -360) return false;
            if (Number20 > 360) return false;
        }
        const TSharedPtr<FJsonValue> Value21 = Object0->TryGetField(TEXT("scale"));
        if (!Value21.IsValid()) return false;
        if (!Value21.IsValid() || Value21->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array22 = Value21->AsArray();
        if (Array22.Num() < 3) return false;
        if (Array22.Num() > 3) return false;
        for (const TSharedPtr<FJsonValue>& Value23 : Array22)
        {
            double Number24 = 0;
            if (!Value23.IsValid() || !Value23->TryGetNumber(Number24) || !FMath::IsFinite(Number24)) return false;
            if (Number24 < 0) return false;
            if (Number24 > 1000000) return false;
        }
        const TSharedPtr<FJsonValue> Value25 = Object0->TryGetField(TEXT("revision"));
        if (!Value25.IsValid()) return false;
        if (!Value25.IsValid() || Value25->Type != EJson::String) return false;
        const FString Text26 = Value25->AsString();
        if (MagiAxiUnicodeScalarCount(Text26) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text26) > 64) return false;
        return true;
    }
    if (Operation == TEXT("asset.create_input_action"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("class") && Entry.Key != TEXT("valueType") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("class"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("valueType"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 8) return false;
        if (Text6 != TEXT("Boolean") && Text6 != TEXT("Axis1D")) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("changed"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value8 = Object0->TryGetField(TEXT("dirtyPackages"));
        if (!Value8.IsValid()) return false;
        if (!Value8.IsValid() || Value8->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array9 = Value8->AsArray();
        if (Array9.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value10 : Array9)
        {
            if (!Value10.IsValid() || Value10->Type != EJson::String) return false;
            const FString Text11 = Value10->AsString();
            if (MagiAxiUnicodeScalarCount(Text11) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value12 = Object0->TryGetField(TEXT("savedPackages"));
        if (!Value12.IsValid()) return false;
        if (!Value12.IsValid() || Value12->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array13 = Value12->AsArray();
        if (Array13.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value14 : Array13)
        {
            if (!Value14.IsValid() || Value14->Type != EJson::String) return false;
            const FString Text15 = Value14->AsString();
            if (MagiAxiUnicodeScalarCount(Text15) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value16 = Object0->TryGetField(TEXT("revision"));
        if (!Value16.IsValid()) return false;
        if (!Value16.IsValid() || Value16->Type != EJson::String) return false;
        const FString Text17 = Value16->AsString();
        if (MagiAxiUnicodeScalarCount(Text17) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text17) > 64) return false;
        return true;
    }
    if (Operation == TEXT("asset.create_input_mapping_context"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("class") && Entry.Key != TEXT("mappingCount") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("class"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("mappingCount"));
        if (!Value5.IsValid()) return false;
        double Number6 = 0;
        if (!Value5.IsValid() || !Value5->TryGetNumber(Number6) || !FMath::IsFinite(Number6) || FMath::FloorToDouble(Number6) != Number6) return false;
        if (Number6 < 0) return false;
        if (Number6 > 100) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("changed"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value8 = Object0->TryGetField(TEXT("dirtyPackages"));
        if (!Value8.IsValid()) return false;
        if (!Value8.IsValid() || Value8->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array9 = Value8->AsArray();
        if (Array9.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value10 : Array9)
        {
            if (!Value10.IsValid() || Value10->Type != EJson::String) return false;
            const FString Text11 = Value10->AsString();
            if (MagiAxiUnicodeScalarCount(Text11) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value12 = Object0->TryGetField(TEXT("savedPackages"));
        if (!Value12.IsValid()) return false;
        if (!Value12.IsValid() || Value12->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array13 = Value12->AsArray();
        if (Array13.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value14 : Array13)
        {
            if (!Value14.IsValid() || Value14->Type != EJson::String) return false;
            const FString Text15 = Value14->AsString();
            if (MagiAxiUnicodeScalarCount(Text15) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value16 = Object0->TryGetField(TEXT("revision"));
        if (!Value16.IsValid()) return false;
        if (!Value16.IsValid() || Value16->Type != EJson::String) return false;
        const FString Text17 = Value16->AsString();
        if (MagiAxiUnicodeScalarCount(Text17) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text17) > 64) return false;
        return true;
    }
    if (Operation == TEXT("asset.save"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("class") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("class"));
        if (Value3.IsValid())
        {
            if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
            const FString Text4 = Value3->AsString();
            if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        }
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("changed"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value6 = Object0->TryGetField(TEXT("dirtyPackages"));
        if (!Value6.IsValid()) return false;
        if (!Value6.IsValid() || Value6->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array7 = Value6->AsArray();
        if (Array7.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value8 : Array7)
        {
            if (!Value8.IsValid() || Value8->Type != EJson::String) return false;
            const FString Text9 = Value8->AsString();
            if (MagiAxiUnicodeScalarCount(Text9) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value10 = Object0->TryGetField(TEXT("savedPackages"));
        if (!Value10.IsValid()) return false;
        if (!Value10.IsValid() || Value10->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array11 = Value10->AsArray();
        if (Array11.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value12 : Array11)
        {
            if (!Value12.IsValid() || Value12->Type != EJson::String) return false;
            const FString Text13 = Value12->AsString();
            if (MagiAxiUnicodeScalarCount(Text13) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value14 = Object0->TryGetField(TEXT("revision"));
        if (!Value14.IsValid()) return false;
        if (!Value14.IsValid() || Value14->Type != EJson::String) return false;
        const FString Text15 = Value14->AsString();
        if (MagiAxiUnicodeScalarCount(Text15) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text15) > 64) return false;
        return true;
    }
    if (Operation == TEXT("blueprint.compile"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("parentClass") && Entry.Key != TEXT("generatedClass") && Entry.Key != TEXT("status") && Entry.Key != TEXT("graphCount") && Entry.Key != TEXT("errorCount") && Entry.Key != TEXT("warningCount") && Entry.Key != TEXT("diagnostics") && Entry.Key != TEXT("revision") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("parentClass"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("generatedClass"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 512) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("status"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) > 16) return false;
        if (Text8 != TEXT("unknown") && Text8 != TEXT("dirty") && Text8 != TEXT("up_to_date") && Text8 != TEXT("warning") && Text8 != TEXT("error")) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("graphCount"));
        if (!Value9.IsValid()) return false;
        double Number10 = 0;
        if (!Value9.IsValid() || !Value9->TryGetNumber(Number10) || !FMath::IsFinite(Number10) || FMath::FloorToDouble(Number10) != Number10) return false;
        if (Number10 < 0) return false;
        if (Number10 > 10000) return false;
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("errorCount"));
        if (!Value11.IsValid()) return false;
        double Number12 = 0;
        if (!Value11.IsValid() || !Value11->TryGetNumber(Number12) || !FMath::IsFinite(Number12) || FMath::FloorToDouble(Number12) != Number12) return false;
        if (Number12 < 0) return false;
        if (Number12 > 100) return false;
        const TSharedPtr<FJsonValue> Value13 = Object0->TryGetField(TEXT("warningCount"));
        if (!Value13.IsValid()) return false;
        double Number14 = 0;
        if (!Value13.IsValid() || !Value13->TryGetNumber(Number14) || !FMath::IsFinite(Number14) || FMath::FloorToDouble(Number14) != Number14) return false;
        if (Number14 < 0) return false;
        if (Number14 > 100) return false;
        const TSharedPtr<FJsonValue> Value15 = Object0->TryGetField(TEXT("diagnostics"));
        if (!Value15.IsValid()) return false;
        if (!Value15.IsValid() || Value15->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array16 = Value15->AsArray();
        if (Array16.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value17 : Array16)
        {
            if (!Value17.IsValid() || Value17->Type != EJson::Object) return false;
            const TSharedPtr<FJsonObject> Object18 = Value17->AsObject();
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object18->Values) if (Entry.Key != TEXT("severity") && Entry.Key != TEXT("message") && Entry.Key != TEXT("graph") && Entry.Key != TEXT("nodeGuid") && Entry.Key != TEXT("nodeTitle")) return false;
            const TSharedPtr<FJsonValue> Value19 = Object18->TryGetField(TEXT("severity"));
            if (!Value19.IsValid()) return false;
            if (!Value19.IsValid() || Value19->Type != EJson::String) return false;
            const FString Text20 = Value19->AsString();
            if (MagiAxiUnicodeScalarCount(Text20) > 8) return false;
            if (Text20 != TEXT("error") && Text20 != TEXT("warning")) return false;
            const TSharedPtr<FJsonValue> Value21 = Object18->TryGetField(TEXT("message"));
            if (!Value21.IsValid()) return false;
            if (!Value21.IsValid() || Value21->Type != EJson::String) return false;
            const FString Text22 = Value21->AsString();
            if (MagiAxiUnicodeScalarCount(Text22) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text22) > 1024) return false;
            const TSharedPtr<FJsonValue> Value23 = Object18->TryGetField(TEXT("graph"));
            if (!Value23.IsValid()) return false;
            if (!Value23.IsValid() || Value23->Type != EJson::String) return false;
            const FString Text24 = Value23->AsString();
            if (MagiAxiUnicodeScalarCount(Text24) > 512) return false;
            const TSharedPtr<FJsonValue> Value25 = Object18->TryGetField(TEXT("nodeGuid"));
            if (!Value25.IsValid()) return false;
            if (!Value25.IsValid() || Value25->Type != EJson::String) return false;
            const FString Text26 = Value25->AsString();
            if (MagiAxiUnicodeScalarCount(Text26) > 128) return false;
            const TSharedPtr<FJsonValue> Value27 = Object18->TryGetField(TEXT("nodeTitle"));
            if (!Value27.IsValid()) return false;
            if (!Value27.IsValid() || Value27->Type != EJson::String) return false;
            const FString Text28 = Value27->AsString();
            if (MagiAxiUnicodeScalarCount(Text28) > 512) return false;
        }
        const TSharedPtr<FJsonValue> Value29 = Object0->TryGetField(TEXT("revision"));
        if (!Value29.IsValid()) return false;
        if (!Value29.IsValid() || Value29->Type != EJson::String) return false;
        const FString Text30 = Value29->AsString();
        if (MagiAxiUnicodeScalarCount(Text30) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text30) > 64) return false;
        const TSharedPtr<FJsonValue> Value31 = Object0->TryGetField(TEXT("changed"));
        if (!Value31.IsValid()) return false;
        if (!Value31.IsValid() || Value31->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value32 = Object0->TryGetField(TEXT("dirtyPackages"));
        if (!Value32.IsValid()) return false;
        if (!Value32.IsValid() || Value32->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array33 = Value32->AsArray();
        if (Array33.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value34 : Array33)
        {
            if (!Value34.IsValid() || Value34->Type != EJson::String) return false;
            const FString Text35 = Value34->AsString();
            if (MagiAxiUnicodeScalarCount(Text35) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value36 = Object0->TryGetField(TEXT("savedPackages"));
        if (!Value36.IsValid()) return false;
        if (!Value36.IsValid() || Value36->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array37 = Value36->AsArray();
        if (Array37.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value38 : Array37)
        {
            if (!Value38.IsValid() || Value38->Type != EJson::String) return false;
            const FString Text39 = Value38->AsString();
            if (MagiAxiUnicodeScalarCount(Text39) > 256) return false;
        }
        return true;
    }
    if (Operation == TEXT("blueprint.view"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("parentClass") && Entry.Key != TEXT("generatedClass") && Entry.Key != TEXT("status") && Entry.Key != TEXT("graphCount") && Entry.Key != TEXT("errorCount") && Entry.Key != TEXT("warningCount") && Entry.Key != TEXT("diagnostics") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("parentClass"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("generatedClass"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 512) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("status"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) > 16) return false;
        if (Text8 != TEXT("unknown") && Text8 != TEXT("dirty") && Text8 != TEXT("up_to_date") && Text8 != TEXT("warning") && Text8 != TEXT("error")) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("graphCount"));
        if (!Value9.IsValid()) return false;
        double Number10 = 0;
        if (!Value9.IsValid() || !Value9->TryGetNumber(Number10) || !FMath::IsFinite(Number10) || FMath::FloorToDouble(Number10) != Number10) return false;
        if (Number10 < 0) return false;
        if (Number10 > 10000) return false;
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("errorCount"));
        if (!Value11.IsValid()) return false;
        double Number12 = 0;
        if (!Value11.IsValid() || !Value11->TryGetNumber(Number12) || !FMath::IsFinite(Number12) || FMath::FloorToDouble(Number12) != Number12) return false;
        if (Number12 < 0) return false;
        if (Number12 > 100) return false;
        const TSharedPtr<FJsonValue> Value13 = Object0->TryGetField(TEXT("warningCount"));
        if (!Value13.IsValid()) return false;
        double Number14 = 0;
        if (!Value13.IsValid() || !Value13->TryGetNumber(Number14) || !FMath::IsFinite(Number14) || FMath::FloorToDouble(Number14) != Number14) return false;
        if (Number14 < 0) return false;
        if (Number14 > 100) return false;
        const TSharedPtr<FJsonValue> Value15 = Object0->TryGetField(TEXT("diagnostics"));
        if (!Value15.IsValid()) return false;
        if (!Value15.IsValid() || Value15->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array16 = Value15->AsArray();
        if (Array16.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value17 : Array16)
        {
            if (!Value17.IsValid() || Value17->Type != EJson::Object) return false;
            const TSharedPtr<FJsonObject> Object18 = Value17->AsObject();
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object18->Values) if (Entry.Key != TEXT("severity") && Entry.Key != TEXT("message") && Entry.Key != TEXT("graph") && Entry.Key != TEXT("nodeGuid") && Entry.Key != TEXT("nodeTitle")) return false;
            const TSharedPtr<FJsonValue> Value19 = Object18->TryGetField(TEXT("severity"));
            if (!Value19.IsValid()) return false;
            if (!Value19.IsValid() || Value19->Type != EJson::String) return false;
            const FString Text20 = Value19->AsString();
            if (MagiAxiUnicodeScalarCount(Text20) > 8) return false;
            if (Text20 != TEXT("error") && Text20 != TEXT("warning")) return false;
            const TSharedPtr<FJsonValue> Value21 = Object18->TryGetField(TEXT("message"));
            if (!Value21.IsValid()) return false;
            if (!Value21.IsValid() || Value21->Type != EJson::String) return false;
            const FString Text22 = Value21->AsString();
            if (MagiAxiUnicodeScalarCount(Text22) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text22) > 1024) return false;
            const TSharedPtr<FJsonValue> Value23 = Object18->TryGetField(TEXT("graph"));
            if (!Value23.IsValid()) return false;
            if (!Value23.IsValid() || Value23->Type != EJson::String) return false;
            const FString Text24 = Value23->AsString();
            if (MagiAxiUnicodeScalarCount(Text24) > 512) return false;
            const TSharedPtr<FJsonValue> Value25 = Object18->TryGetField(TEXT("nodeGuid"));
            if (!Value25.IsValid()) return false;
            if (!Value25.IsValid() || Value25->Type != EJson::String) return false;
            const FString Text26 = Value25->AsString();
            if (MagiAxiUnicodeScalarCount(Text26) > 128) return false;
            const TSharedPtr<FJsonValue> Value27 = Object18->TryGetField(TEXT("nodeTitle"));
            if (!Value27.IsValid()) return false;
            if (!Value27.IsValid() || Value27->Type != EJson::String) return false;
            const FString Text28 = Value27->AsString();
            if (MagiAxiUnicodeScalarCount(Text28) > 512) return false;
        }
        const TSharedPtr<FJsonValue> Value29 = Object0->TryGetField(TEXT("revision"));
        if (!Value29.IsValid()) return false;
        if (!Value29.IsValid() || Value29->Type != EJson::String) return false;
        const FString Text30 = Value29->AsString();
        if (MagiAxiUnicodeScalarCount(Text30) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text30) > 64) return false;
        return true;
    }
    if (Operation == TEXT("component.add"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("changed"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value4 = Object0->TryGetField(TEXT("dirtyPackages"));
        if (!Value4.IsValid()) return false;
        if (!Value4.IsValid() || Value4->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array5 = Value4->AsArray();
        if (Array5.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value6 : Array5)
        {
            if (!Value6.IsValid() || Value6->Type != EJson::String) return false;
            const FString Text7 = Value6->AsString();
            if (MagiAxiUnicodeScalarCount(Text7) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value8 = Object0->TryGetField(TEXT("savedPackages"));
        if (!Value8.IsValid()) return false;
        if (!Value8.IsValid() || Value8->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array9 = Value8->AsArray();
        if (Array9.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value10 : Array9)
        {
            if (!Value10.IsValid() || Value10->Type != EJson::String) return false;
            const FString Text11 = Value10->AsString();
            if (MagiAxiUnicodeScalarCount(Text11) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value12 = Object0->TryGetField(TEXT("revision"));
        if (!Value12.IsValid()) return false;
        if (!Value12.IsValid() || Value12->Type != EJson::String) return false;
        const FString Text13 = Value12->AsString();
        if (MagiAxiUnicodeScalarCount(Text13) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text13) > 64) return false;
        return true;
    }
    if (Operation == TEXT("component.list"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("count") && Entry.Key != TEXT("total") && Entry.Key != TEXT("scope") && Entry.Key != TEXT("revision") && Entry.Key != TEXT("items") && Entry.Key != TEXT("nextCursor")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("count"));
        if (!Value1.IsValid()) return false;
        double Number2 = 0;
        if (!Value1.IsValid() || !Value1->TryGetNumber(Number2) || !FMath::IsFinite(Number2) || FMath::FloorToDouble(Number2) != Number2) return false;
        if (Number2 < 0) return false;
        if (Number2 > 100) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("total"));
        if (!Value3.IsValid()) return false;
        double Number4 = 0;
        if (!Value3.IsValid() || !Value3->TryGetNumber(Number4) || !FMath::IsFinite(Number4) || FMath::FloorToDouble(Number4) != Number4) return false;
        if (Number4 < 0) return false;
        if (Number4 > 9007199254740991) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("scope"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 512) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("revision"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text8) > 64) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("items"));
        if (!Value9.IsValid()) return false;
        if (!Value9.IsValid() || Value9->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array10 = Value9->AsArray();
        if (Array10.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value11 : Array10)
        {
            if (!Value11.IsValid() || Value11->Type != EJson::Object) return false;
            const TSharedPtr<FJsonObject> Object12 = Value11->AsObject();
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object12->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("name") && Entry.Key != TEXT("class") && Entry.Key != TEXT("scene")) return false;
            const TSharedPtr<FJsonValue> Value13 = Object12->TryGetField(TEXT("id"));
            if (!Value13.IsValid()) return false;
            if (!Value13.IsValid() || Value13->Type != EJson::String) return false;
            const FString Text14 = Value13->AsString();
            if (MagiAxiUnicodeScalarCount(Text14) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text14) > 512) return false;
            const TSharedPtr<FJsonValue> Value15 = Object12->TryGetField(TEXT("name"));
            if (Value15.IsValid())
            {
                if (!Value15.IsValid() || Value15->Type != EJson::String) return false;
                const FString Text16 = Value15->AsString();
                if (MagiAxiUnicodeScalarCount(Text16) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text16) > 512) return false;
            }
            const TSharedPtr<FJsonValue> Value17 = Object12->TryGetField(TEXT("class"));
            if (Value17.IsValid())
            {
                if (!Value17.IsValid() || Value17->Type != EJson::String) return false;
                const FString Text18 = Value17->AsString();
                if (MagiAxiUnicodeScalarCount(Text18) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text18) > 512) return false;
            }
            const TSharedPtr<FJsonValue> Value19 = Object12->TryGetField(TEXT("scene"));
            if (Value19.IsValid())
            {
                if (!Value19.IsValid() || Value19->Type != EJson::Boolean) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value20 = Object0->TryGetField(TEXT("nextCursor"));
        if (!Value20.IsValid()) return false;
        if (Value20->Type != EJson::Null)
        {
            if (!Value20.IsValid() || Value20->Type != EJson::String) return false;
            const FString Text21 = Value20->AsString();
            if (MagiAxiUnicodeScalarCount(Text21) > 256) return false;
        }
        return true;
    }
    if (Operation == TEXT("component.remove"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dryRun") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("changed"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value4 = Object0->TryGetField(TEXT("dryRun"));
        if (!Value4.IsValid()) return false;
        if (!Value4.IsValid() || Value4->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("dirtyPackages"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array6 = Value5->AsArray();
        if (Array6.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value7 : Array6)
        {
            if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
            const FString Text8 = Value7->AsString();
            if (MagiAxiUnicodeScalarCount(Text8) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("savedPackages"));
        if (!Value9.IsValid()) return false;
        if (!Value9.IsValid() || Value9->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array10 = Value9->AsArray();
        if (Array10.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value11 : Array10)
        {
            if (!Value11.IsValid() || Value11->Type != EJson::String) return false;
            const FString Text12 = Value11->AsString();
            if (MagiAxiUnicodeScalarCount(Text12) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value13 = Object0->TryGetField(TEXT("revision"));
        if (!Value13.IsValid()) return false;
        if (!Value13.IsValid() || Value13->Type != EJson::String) return false;
        const FString Text14 = Value13->AsString();
        if (MagiAxiUnicodeScalarCount(Text14) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text14) > 64) return false;
        return true;
    }
    if (Operation == TEXT("component.update"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("changed"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value4 = Object0->TryGetField(TEXT("dirtyPackages"));
        if (!Value4.IsValid()) return false;
        if (!Value4.IsValid() || Value4->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array5 = Value4->AsArray();
        if (Array5.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value6 : Array5)
        {
            if (!Value6.IsValid() || Value6->Type != EJson::String) return false;
            const FString Text7 = Value6->AsString();
            if (MagiAxiUnicodeScalarCount(Text7) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value8 = Object0->TryGetField(TEXT("savedPackages"));
        if (!Value8.IsValid()) return false;
        if (!Value8.IsValid() || Value8->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array9 = Value8->AsArray();
        if (Array9.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value10 : Array9)
        {
            if (!Value10.IsValid() || Value10->Type != EJson::String) return false;
            const FString Text11 = Value10->AsString();
            if (MagiAxiUnicodeScalarCount(Text11) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value12 = Object0->TryGetField(TEXT("revision"));
        if (!Value12.IsValid()) return false;
        if (!Value12.IsValid() || Value12->Type != EJson::String) return false;
        const FString Text13 = Value12->AsString();
        if (MagiAxiUnicodeScalarCount(Text13) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text13) > 64) return false;
        return true;
    }
    if (Operation == TEXT("component.view"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("actorId") && Entry.Key != TEXT("name") && Entry.Key != TEXT("class") && Entry.Key != TEXT("scene") && Entry.Key != TEXT("location") && Entry.Key != TEXT("rotation") && Entry.Key != TEXT("scale") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("actorId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("name"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 512) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("class"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text8) > 512) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("scene"));
        if (!Value9.IsValid()) return false;
        if (!Value9.IsValid() || Value9->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value10 = Object0->TryGetField(TEXT("location"));
        if (!Value10.IsValid()) return false;
        if (!Value10.IsValid() || Value10->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array11 = Value10->AsArray();
        if (Array11.Num() < 3) return false;
        if (Array11.Num() > 3) return false;
        for (const TSharedPtr<FJsonValue>& Value12 : Array11)
        {
            double Number13 = 0;
            if (!Value12.IsValid() || !Value12->TryGetNumber(Number13) || !FMath::IsFinite(Number13)) return false;
            if (Number13 < -1000000000) return false;
            if (Number13 > 1000000000) return false;
        }
        const TSharedPtr<FJsonValue> Value14 = Object0->TryGetField(TEXT("rotation"));
        if (!Value14.IsValid()) return false;
        if (!Value14.IsValid() || Value14->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array15 = Value14->AsArray();
        if (Array15.Num() < 3) return false;
        if (Array15.Num() > 3) return false;
        for (const TSharedPtr<FJsonValue>& Value16 : Array15)
        {
            double Number17 = 0;
            if (!Value16.IsValid() || !Value16->TryGetNumber(Number17) || !FMath::IsFinite(Number17)) return false;
            if (Number17 < -360) return false;
            if (Number17 > 360) return false;
        }
        const TSharedPtr<FJsonValue> Value18 = Object0->TryGetField(TEXT("scale"));
        if (!Value18.IsValid()) return false;
        if (!Value18.IsValid() || Value18->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array19 = Value18->AsArray();
        if (Array19.Num() < 3) return false;
        if (Array19.Num() > 3) return false;
        for (const TSharedPtr<FJsonValue>& Value20 : Array19)
        {
            double Number21 = 0;
            if (!Value20.IsValid() || !Value20->TryGetNumber(Number21) || !FMath::IsFinite(Number21)) return false;
            if (Number21 < 0) return false;
            if (Number21 > 1000000000) return false;
        }
        const TSharedPtr<FJsonValue> Value22 = Object0->TryGetField(TEXT("revision"));
        if (!Value22.IsValid()) return false;
        if (!Value22.IsValid() || Value22->Type != EJson::String) return false;
        const FString Text23 = Value22->AsString();
        if (MagiAxiUnicodeScalarCount(Text23) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text23) > 64) return false;
        return true;
    }
    if (Operation == TEXT("level.set_game_mode"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("levelId") && Entry.Key != TEXT("gameModeClass") && Entry.Key != TEXT("defaultPawnClass") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("levelId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("gameModeClass"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("defaultPawnClass"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 512) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("changed"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value8 = Object0->TryGetField(TEXT("dirtyPackages"));
        if (!Value8.IsValid()) return false;
        if (!Value8.IsValid() || Value8->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array9 = Value8->AsArray();
        if (Array9.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value10 : Array9)
        {
            if (!Value10.IsValid() || Value10->Type != EJson::String) return false;
            const FString Text11 = Value10->AsString();
            if (MagiAxiUnicodeScalarCount(Text11) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value12 = Object0->TryGetField(TEXT("savedPackages"));
        if (!Value12.IsValid()) return false;
        if (!Value12.IsValid() || Value12->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array13 = Value12->AsArray();
        if (Array13.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value14 : Array13)
        {
            if (!Value14.IsValid() || Value14->Type != EJson::String) return false;
            const FString Text15 = Value14->AsString();
            if (MagiAxiUnicodeScalarCount(Text15) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value16 = Object0->TryGetField(TEXT("revision"));
        if (!Value16.IsValid()) return false;
        if (!Value16.IsValid() || Value16->Type != EJson::String) return false;
        const FString Text17 = Value16->AsString();
        if (MagiAxiUnicodeScalarCount(Text17) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text17) > 64) return false;
        return true;
    }
    if (Operation == TEXT("level.settings"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("levelId") && Entry.Key != TEXT("gameModeClass") && Entry.Key != TEXT("defaultPawnClass") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("levelId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("gameModeClass"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 0) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("defaultPawnClass"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 0) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 512) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("revision"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text8) > 64) return false;
        return true;
    }
    if (Operation == TEXT("play.input"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("sessionId") && Entry.Key != TEXT("key") && Entry.Key != TEXT("event") && Entry.Key != TEXT("accepted") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("beforeRevision") && Entry.Key != TEXT("afterRevision") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("sessionId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 128) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("key"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 128) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("event"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) > 8) return false;
        if (Text6 != TEXT("pressed") && Text6 != TEXT("released")) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("accepted"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value8 = Object0->TryGetField(TEXT("changed"));
        if (!Value8.IsValid()) return false;
        if (!Value8.IsValid() || Value8->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("beforeRevision"));
        if (!Value9.IsValid()) return false;
        if (!Value9.IsValid() || Value9->Type != EJson::String) return false;
        const FString Text10 = Value9->AsString();
        if (MagiAxiUnicodeScalarCount(Text10) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text10) > 64) return false;
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("afterRevision"));
        if (!Value11.IsValid()) return false;
        if (!Value11.IsValid() || Value11->Type != EJson::String) return false;
        const FString Text12 = Value11->AsString();
        if (MagiAxiUnicodeScalarCount(Text12) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text12) > 64) return false;
        const TSharedPtr<FJsonValue> Value13 = Object0->TryGetField(TEXT("revision"));
        if (!Value13.IsValid()) return false;
        if (!Value13.IsValid() || Value13->Type != EJson::String) return false;
        const FString Text14 = Value13->AsString();
        if (MagiAxiUnicodeScalarCount(Text14) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text14) > 64) return false;
        return true;
    }
    if (Operation == TEXT("play.observe"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("sessionId") && Entry.Key != TEXT("worldId") && Entry.Key != TEXT("levelId") && Entry.Key != TEXT("actors") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("sessionId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 128) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("worldId"));
        if (!Value3.IsValid()) return false;
        if (Value3->Type != EJson::Null)
        {
            if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
            const FString Text4 = Value3->AsString();
            if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        }
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("levelId"));
        if (!Value5.IsValid()) return false;
        if (Value5->Type != EJson::Null)
        {
            if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
            const FString Text6 = Value5->AsString();
            if (MagiAxiUnicodeScalarCount(Text6) > 512) return false;
        }
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("actors"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array8 = Value7->AsArray();
        if (Array8.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value9 : Array8)
        {
            if (!Value9.IsValid() || Value9->Type != EJson::Object) return false;
            const TSharedPtr<FJsonObject> Object10 = Value9->AsObject();
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object10->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("name") && Entry.Key != TEXT("class") && Entry.Key != TEXT("location") && Entry.Key != TEXT("tags")) return false;
            const TSharedPtr<FJsonValue> Value11 = Object10->TryGetField(TEXT("id"));
            if (!Value11.IsValid()) return false;
            if (!Value11.IsValid() || Value11->Type != EJson::String) return false;
            const FString Text12 = Value11->AsString();
            if (MagiAxiUnicodeScalarCount(Text12) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text12) > 256) return false;
            const TSharedPtr<FJsonValue> Value13 = Object10->TryGetField(TEXT("name"));
            if (!Value13.IsValid()) return false;
            if (!Value13.IsValid() || Value13->Type != EJson::String) return false;
            const FString Text14 = Value13->AsString();
            if (MagiAxiUnicodeScalarCount(Text14) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text14) > 256) return false;
            const TSharedPtr<FJsonValue> Value15 = Object10->TryGetField(TEXT("class"));
            if (!Value15.IsValid()) return false;
            if (!Value15.IsValid() || Value15->Type != EJson::String) return false;
            const FString Text16 = Value15->AsString();
            if (MagiAxiUnicodeScalarCount(Text16) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text16) > 512) return false;
            const TSharedPtr<FJsonValue> Value17 = Object10->TryGetField(TEXT("location"));
            if (!Value17.IsValid()) return false;
            if (!Value17.IsValid() || Value17->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array18 = Value17->AsArray();
            if (Array18.Num() < 3) return false;
            if (Array18.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value19 : Array18)
            {
                double Number20 = 0;
                if (!Value19.IsValid() || !Value19->TryGetNumber(Number20) || !FMath::IsFinite(Number20)) return false;
                if (Number20 < -1000000000) return false;
                if (Number20 > 1000000000) return false;
            }
            const TSharedPtr<FJsonValue> Value21 = Object10->TryGetField(TEXT("tags"));
            if (!Value21.IsValid()) return false;
            if (!Value21.IsValid() || Value21->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array22 = Value21->AsArray();
            if (Array22.Num() > 32) return false;
            for (const TSharedPtr<FJsonValue>& Value23 : Array22)
            {
                if (!Value23.IsValid() || Value23->Type != EJson::String) return false;
                const FString Text24 = Value23->AsString();
                if (MagiAxiUnicodeScalarCount(Text24) > 128) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value25 = Object0->TryGetField(TEXT("revision"));
        if (!Value25.IsValid()) return false;
        if (!Value25.IsValid() || Value25->Type != EJson::String) return false;
        const FString Text26 = Value25->AsString();
        if (MagiAxiUnicodeScalarCount(Text26) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text26) > 64) return false;
        return true;
    }
    if (Operation == TEXT("play.screenshot"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("sessionId") && Entry.Key != TEXT("path") && Entry.Key != TEXT("width") && Entry.Key != TEXT("height") && Entry.Key != TEXT("format") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("sessionId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 128) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("path"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 1024) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("width"));
        if (!Value5.IsValid()) return false;
        double Number6 = 0;
        if (!Value5.IsValid() || !Value5->TryGetNumber(Number6) || !FMath::IsFinite(Number6) || FMath::FloorToDouble(Number6) != Number6) return false;
        if (Number6 < 1) return false;
        if (Number6 > 16384) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("height"));
        if (!Value7.IsValid()) return false;
        double Number8 = 0;
        if (!Value7.IsValid() || !Value7->TryGetNumber(Number8) || !FMath::IsFinite(Number8) || FMath::FloorToDouble(Number8) != Number8) return false;
        if (Number8 < 1) return false;
        if (Number8 > 16384) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("format"));
        if (!Value9.IsValid()) return false;
        if (!Value9.IsValid() || Value9->Type != EJson::String) return false;
        const FString Text10 = Value9->AsString();
        if (MagiAxiUnicodeScalarCount(Text10) > 3) return false;
        if (Text10 != TEXT("png")) return false;
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("changed"));
        if (!Value11.IsValid()) return false;
        if (!Value11.IsValid() || Value11->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value12 = Object0->TryGetField(TEXT("revision"));
        if (!Value12.IsValid()) return false;
        if (!Value12.IsValid() || Value12->Type != EJson::String) return false;
        const FString Text13 = Value12->AsString();
        if (MagiAxiUnicodeScalarCount(Text13) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text13) > 64) return false;
        return true;
    }
    if (Operation == TEXT("play.start"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("sessionId") && Entry.Key != TEXT("state") && Entry.Key != TEXT("worldId") && Entry.Key != TEXT("levelId") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("sessionId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 128) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("state"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) > 8) return false;
        if (Text4 != TEXT("starting") && Text4 != TEXT("running")) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("worldId"));
        if (!Value5.IsValid()) return false;
        if (Value5->Type != EJson::Null)
        {
            if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
            const FString Text6 = Value5->AsString();
            if (MagiAxiUnicodeScalarCount(Text6) > 512) return false;
        }
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("levelId"));
        if (!Value7.IsValid()) return false;
        if (Value7->Type != EJson::Null)
        {
            if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
            const FString Text8 = Value7->AsString();
            if (MagiAxiUnicodeScalarCount(Text8) > 512) return false;
        }
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("changed"));
        if (!Value9.IsValid()) return false;
        if (!Value9.IsValid() || Value9->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value10 = Object0->TryGetField(TEXT("revision"));
        if (!Value10.IsValid()) return false;
        if (!Value10.IsValid() || Value10->Type != EJson::String) return false;
        const FString Text11 = Value10->AsString();
        if (MagiAxiUnicodeScalarCount(Text11) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text11) > 64) return false;
        return true;
    }
    if (Operation == TEXT("play.status"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("state") && Entry.Key != TEXT("sessionId") && Entry.Key != TEXT("worldId") && Entry.Key != TEXT("levelId") && Entry.Key != TEXT("playerCount") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("state"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) > 8) return false;
        if (Text2 != TEXT("stopped") && Text2 != TEXT("starting") && Text2 != TEXT("running") && Text2 != TEXT("stopping")) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("sessionId"));
        if (!Value3.IsValid()) return false;
        if (Value3->Type != EJson::Null)
        {
            if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
            const FString Text4 = Value3->AsString();
            if (MagiAxiUnicodeScalarCount(Text4) > 128) return false;
        }
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("worldId"));
        if (!Value5.IsValid()) return false;
        if (Value5->Type != EJson::Null)
        {
            if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
            const FString Text6 = Value5->AsString();
            if (MagiAxiUnicodeScalarCount(Text6) > 512) return false;
        }
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("levelId"));
        if (!Value7.IsValid()) return false;
        if (Value7->Type != EJson::Null)
        {
            if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
            const FString Text8 = Value7->AsString();
            if (MagiAxiUnicodeScalarCount(Text8) > 512) return false;
        }
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("playerCount"));
        if (!Value9.IsValid()) return false;
        double Number10 = 0;
        if (!Value9.IsValid() || !Value9->TryGetNumber(Number10) || !FMath::IsFinite(Number10) || FMath::FloorToDouble(Number10) != Number10) return false;
        if (Number10 < 0) return false;
        if (Number10 > 100) return false;
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("revision"));
        if (!Value11.IsValid()) return false;
        if (!Value11.IsValid() || Value11->Type != EJson::String) return false;
        const FString Text12 = Value11->AsString();
        if (MagiAxiUnicodeScalarCount(Text12) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text12) > 64) return false;
        return true;
    }
    if (Operation == TEXT("play.stop"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("sessionId") && Entry.Key != TEXT("state") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("sessionId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 128) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("state"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) > 8) return false;
        if (Text4 != TEXT("stopping") && Text4 != TEXT("stopped")) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("changed"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value6 = Object0->TryGetField(TEXT("revision"));
        if (!Value6.IsValid()) return false;
        if (!Value6.IsValid() || Value6->Type != EJson::String) return false;
        const FString Text7 = Value6->AsString();
        if (MagiAxiUnicodeScalarCount(Text7) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text7) > 64) return false;
        return true;
    }
    if (Operation == TEXT("asset.list"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("count") && Entry.Key != TEXT("total") && Entry.Key != TEXT("scope") && Entry.Key != TEXT("revision") && Entry.Key != TEXT("items") && Entry.Key != TEXT("nextCursor")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("count"));
        if (!Value1.IsValid()) return false;
        double Number2 = 0;
        if (!Value1.IsValid() || !Value1->TryGetNumber(Number2) || !FMath::IsFinite(Number2) || FMath::FloorToDouble(Number2) != Number2) return false;
        if (Number2 < 0) return false;
        if (Number2 > 100) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("total"));
        if (!Value3.IsValid()) return false;
        double Number4 = 0;
        if (!Value3.IsValid() || !Value3->TryGetNumber(Number4) || !FMath::IsFinite(Number4) || FMath::FloorToDouble(Number4) != Number4) return false;
        if (Number4 < 0) return false;
        if (Number4 > 9007199254740991) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("scope"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 512) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("revision"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text8) > 64) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("items"));
        if (!Value9.IsValid()) return false;
        if (!Value9.IsValid() || Value9->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array10 = Value9->AsArray();
        if (Array10.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value11 : Array10)
        {
            if (!Value11.IsValid() || Value11->Type != EJson::Object) return false;
            const TSharedPtr<FJsonObject> Object12 = Value11->AsObject();
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object12->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("name") && Entry.Key != TEXT("class") && Entry.Key != TEXT("packagePath")) return false;
            const TSharedPtr<FJsonValue> Value13 = Object12->TryGetField(TEXT("id"));
            if (!Value13.IsValid()) return false;
            if (!Value13.IsValid() || Value13->Type != EJson::String) return false;
            const FString Text14 = Value13->AsString();
            if (MagiAxiUnicodeScalarCount(Text14) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text14) > 512) return false;
            const TSharedPtr<FJsonValue> Value15 = Object12->TryGetField(TEXT("name"));
            if (Value15.IsValid())
            {
                if (!Value15.IsValid() || Value15->Type != EJson::String) return false;
                const FString Text16 = Value15->AsString();
                if (MagiAxiUnicodeScalarCount(Text16) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text16) > 512) return false;
            }
            const TSharedPtr<FJsonValue> Value17 = Object12->TryGetField(TEXT("class"));
            if (Value17.IsValid())
            {
                if (!Value17.IsValid() || Value17->Type != EJson::String) return false;
                const FString Text18 = Value17->AsString();
                if (MagiAxiUnicodeScalarCount(Text18) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text18) > 512) return false;
            }
            const TSharedPtr<FJsonValue> Value19 = Object12->TryGetField(TEXT("packagePath"));
            if (Value19.IsValid())
            {
                if (!Value19.IsValid() || Value19->Type != EJson::String) return false;
                const FString Text20 = Value19->AsString();
                if (MagiAxiUnicodeScalarCount(Text20) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text20) > 512) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value21 = Object0->TryGetField(TEXT("nextCursor"));
        if (!Value21.IsValid()) return false;
        if (Value21->Type != EJson::Null)
        {
            if (!Value21.IsValid() || Value21->Type != EJson::String) return false;
            const FString Text22 = Value21->AsString();
            if (MagiAxiUnicodeScalarCount(Text22) > 256) return false;
        }
        return true;
    }
    if (Operation == TEXT("asset.view"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("packagePath") && Entry.Key != TEXT("objectPath") && Entry.Key != TEXT("name") && Entry.Key != TEXT("class") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("packagePath"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("objectPath"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 1024) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("name"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text8) > 512) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("class"));
        if (!Value9.IsValid()) return false;
        if (!Value9.IsValid() || Value9->Type != EJson::String) return false;
        const FString Text10 = Value9->AsString();
        if (MagiAxiUnicodeScalarCount(Text10) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text10) > 512) return false;
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("revision"));
        if (!Value11.IsValid()) return false;
        if (!Value11.IsValid() || Value11->Type != EJson::String) return false;
        const FString Text12 = Value11->AsString();
        if (MagiAxiUnicodeScalarCount(Text12) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text12) > 64) return false;
        return true;
    }
    if (Operation == TEXT("capability.describe"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("capability") && Entry.Key != TEXT("runtime")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("capability"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object2 = Value1->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object2->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("version") && Entry.Key != TEXT("domain") && Entry.Key != TEXT("summary") && Entry.Key != TEXT("execution") && Entry.Key != TEXT("mutates") && Entry.Key != TEXT("destructive") && Entry.Key != TEXT("idempotency") && Entry.Key != TEXT("saveBehavior") && Entry.Key != TEXT("transactionBehavior") && Entry.Key != TEXT("reversibility") && Entry.Key != TEXT("allowedEditorStates") && Entry.Key != TEXT("requiresModules") && Entry.Key != TEXT("inputSchema") && Entry.Key != TEXT("outputSchema") && Entry.Key != TEXT("verification") && Entry.Key != TEXT("engineSupport")) return false;
        const TSharedPtr<FJsonValue> Value3 = Object2->TryGetField(TEXT("id"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 128) return false;
        const TSharedPtr<FJsonValue> Value5 = Object2->TryGetField(TEXT("version"));
        if (!Value5.IsValid()) return false;
        double Number6 = 0;
        if (!Value5.IsValid() || !Value5->TryGetNumber(Number6) || !FMath::IsFinite(Number6) || FMath::FloorToDouble(Number6) != Number6) return false;
        if (Number6 < 1) return false;
        if (Number6 > 1) return false;
        const TSharedPtr<FJsonValue> Value7 = Object2->TryGetField(TEXT("domain"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text8) > 64) return false;
        const TSharedPtr<FJsonValue> Value9 = Object2->TryGetField(TEXT("summary"));
        if (!Value9.IsValid()) return false;
        if (!Value9.IsValid() || Value9->Type != EJson::String) return false;
        const FString Text10 = Value9->AsString();
        if (MagiAxiUnicodeScalarCount(Text10) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text10) > 512) return false;
        const TSharedPtr<FJsonValue> Value11 = Object2->TryGetField(TEXT("execution"));
        if (!Value11.IsValid()) return false;
        if (!Value11.IsValid() || Value11->Type != EJson::String) return false;
        const FString Text12 = Value11->AsString();
        if (MagiAxiUnicodeScalarCount(Text12) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text12) > 8) return false;
        if (Text12 != TEXT("local") && Text12 != TEXT("native")) return false;
        const TSharedPtr<FJsonValue> Value13 = Object2->TryGetField(TEXT("mutates"));
        if (!Value13.IsValid()) return false;
        if (!Value13.IsValid() || Value13->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value14 = Object2->TryGetField(TEXT("destructive"));
        if (!Value14.IsValid()) return false;
        if (!Value14.IsValid() || Value14->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value15 = Object2->TryGetField(TEXT("idempotency"));
        if (!Value15.IsValid()) return false;
        if (!Value15.IsValid() || Value15->Type != EJson::String) return false;
        const FString Text16 = Value15->AsString();
        if (MagiAxiUnicodeScalarCount(Text16) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text16) > 32) return false;
        const TSharedPtr<FJsonValue> Value17 = Object2->TryGetField(TEXT("saveBehavior"));
        if (!Value17.IsValid()) return false;
        if (!Value17.IsValid() || Value17->Type != EJson::String) return false;
        const FString Text18 = Value17->AsString();
        if (MagiAxiUnicodeScalarCount(Text18) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text18) > 32) return false;
        const TSharedPtr<FJsonValue> Value19 = Object2->TryGetField(TEXT("transactionBehavior"));
        if (!Value19.IsValid()) return false;
        if (!Value19.IsValid() || Value19->Type != EJson::String) return false;
        const FString Text20 = Value19->AsString();
        if (MagiAxiUnicodeScalarCount(Text20) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text20) > 32) return false;
        const TSharedPtr<FJsonValue> Value21 = Object2->TryGetField(TEXT("reversibility"));
        if (!Value21.IsValid()) return false;
        if (!Value21.IsValid() || Value21->Type != EJson::String) return false;
        const FString Text22 = Value21->AsString();
        if (MagiAxiUnicodeScalarCount(Text22) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text22) > 32) return false;
        const TSharedPtr<FJsonValue> Value23 = Object2->TryGetField(TEXT("allowedEditorStates"));
        if (!Value23.IsValid()) return false;
        if (!Value23.IsValid() || Value23->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array24 = Value23->AsArray();
        if (Array24.Num() > 8) return false;
        for (const TSharedPtr<FJsonValue>& Value25 : Array24)
        {
            if (!Value25.IsValid() || Value25->Type != EJson::String) return false;
            const FString Text26 = Value25->AsString();
            if (MagiAxiUnicodeScalarCount(Text26) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text26) > 32) return false;
        }
        const TSharedPtr<FJsonValue> Value27 = Object2->TryGetField(TEXT("requiresModules"));
        if (!Value27.IsValid()) return false;
        if (!Value27.IsValid() || Value27->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array28 = Value27->AsArray();
        if (Array28.Num() > 16) return false;
        for (const TSharedPtr<FJsonValue>& Value29 : Array28)
        {
            if (!Value29.IsValid() || Value29->Type != EJson::String) return false;
            const FString Text30 = Value29->AsString();
            if (MagiAxiUnicodeScalarCount(Text30) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text30) > 64) return false;
        }
        const TSharedPtr<FJsonValue> Value31 = Object2->TryGetField(TEXT("inputSchema"));
        if (!Value31.IsValid()) return false;
        if (!Value31.IsValid() || Value31->Type != EJson::String) return false;
        const FString Text32 = Value31->AsString();
        if (MagiAxiUnicodeScalarCount(Text32) < 2) return false;
        if (MagiAxiUnicodeScalarCount(Text32) > 4096) return false;
        const TSharedPtr<FJsonValue> Value33 = Object2->TryGetField(TEXT("outputSchema"));
        if (!Value33.IsValid()) return false;
        if (!Value33.IsValid() || Value33->Type != EJson::String) return false;
        const FString Text34 = Value33->AsString();
        if (MagiAxiUnicodeScalarCount(Text34) < 2) return false;
        if (MagiAxiUnicodeScalarCount(Text34) > 4096) return false;
        const TSharedPtr<FJsonValue> Value35 = Object2->TryGetField(TEXT("verification"));
        if (!Value35.IsValid()) return false;
        if (!Value35.IsValid() || Value35->Type != EJson::String) return false;
        const FString Text36 = Value35->AsString();
        if (MagiAxiUnicodeScalarCount(Text36) < 2) return false;
        if (MagiAxiUnicodeScalarCount(Text36) > 1024) return false;
        const TSharedPtr<FJsonValue> Value37 = Object2->TryGetField(TEXT("engineSupport"));
        if (!Value37.IsValid()) return false;
        if (!Value37.IsValid() || Value37->Type != EJson::String) return false;
        const FString Text38 = Value37->AsString();
        if (MagiAxiUnicodeScalarCount(Text38) < 2) return false;
        if (MagiAxiUnicodeScalarCount(Text38) > 2048) return false;
        const TSharedPtr<FJsonValue> Value39 = Object0->TryGetField(TEXT("runtime"));
        if (!Value39.IsValid()) return false;
        if (!Value39.IsValid() || Value39->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object40 = Value39->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object40->Values) if (Entry.Key != TEXT("available") && Entry.Key != TEXT("catalogHash")) return false;
        const TSharedPtr<FJsonValue> Value41 = Object40->TryGetField(TEXT("available"));
        if (!Value41.IsValid()) return false;
        if (!Value41.IsValid() || Value41->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value42 = Object40->TryGetField(TEXT("catalogHash"));
        if (!Value42.IsValid()) return false;
        if (!Value42.IsValid() || Value42->Type != EJson::String) return false;
        const FString Text43 = Value42->AsString();
        if (MagiAxiUnicodeScalarCount(Text43) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text43) > 64) return false;
        return true;
    }
    if (Operation == TEXT("capability.search"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("count") && Entry.Key != TEXT("total") && Entry.Key != TEXT("scope") && Entry.Key != TEXT("items") && Entry.Key != TEXT("nextCursor")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("count"));
        if (!Value1.IsValid()) return false;
        double Number2 = 0;
        if (!Value1.IsValid() || !Value1->TryGetNumber(Number2) || !FMath::IsFinite(Number2) || FMath::FloorToDouble(Number2) != Number2) return false;
        if (Number2 < 0) return false;
        if (Number2 > 50) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("total"));
        if (!Value3.IsValid()) return false;
        double Number4 = 0;
        if (!Value3.IsValid() || !Value3->TryGetNumber(Number4) || !FMath::IsFinite(Number4) || FMath::FloorToDouble(Number4) != Number4) return false;
        if (Number4 < 0) return false;
        if (Number4 > 9007199254740991) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("scope"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 512) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("items"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array8 = Value7->AsArray();
        if (Array8.Num() > 50) return false;
        for (const TSharedPtr<FJsonValue>& Value9 : Array8)
        {
            if (!Value9.IsValid() || Value9->Type != EJson::Object) return false;
            const TSharedPtr<FJsonObject> Object10 = Value9->AsObject();
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object10->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("domain") && Entry.Key != TEXT("summary") && Entry.Key != TEXT("available")) return false;
            const TSharedPtr<FJsonValue> Value11 = Object10->TryGetField(TEXT("id"));
            if (!Value11.IsValid()) return false;
            if (!Value11.IsValid() || Value11->Type != EJson::String) return false;
            const FString Text12 = Value11->AsString();
            if (MagiAxiUnicodeScalarCount(Text12) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text12) > 128) return false;
            const TSharedPtr<FJsonValue> Value13 = Object10->TryGetField(TEXT("domain"));
            if (!Value13.IsValid()) return false;
            if (!Value13.IsValid() || Value13->Type != EJson::String) return false;
            const FString Text14 = Value13->AsString();
            if (MagiAxiUnicodeScalarCount(Text14) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text14) > 64) return false;
            const TSharedPtr<FJsonValue> Value15 = Object10->TryGetField(TEXT("summary"));
            if (!Value15.IsValid()) return false;
            if (!Value15.IsValid() || Value15->Type != EJson::String) return false;
            const FString Text16 = Value15->AsString();
            if (MagiAxiUnicodeScalarCount(Text16) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text16) > 512) return false;
            const TSharedPtr<FJsonValue> Value17 = Object10->TryGetField(TEXT("available"));
            if (!Value17.IsValid()) return false;
            if (!Value17.IsValid() || Value17->Type != EJson::Boolean) return false;
        }
        const TSharedPtr<FJsonValue> Value18 = Object0->TryGetField(TEXT("nextCursor"));
        if (!Value18.IsValid()) return false;
        if (Value18->Type != EJson::Null)
        {
            if (!Value18.IsValid() || Value18->Type != EJson::String) return false;
            const FString Text19 = Value18->AsString();
            if (MagiAxiUnicodeScalarCount(Text19) > 256) return false;
        }
        return true;
    }
    if (Operation == TEXT("editor.status"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("state") && Entry.Key != TEXT("projectId") && Entry.Key != TEXT("editorPid") && Entry.Key != TEXT("levelId") && Entry.Key != TEXT("pie") && Entry.Key != TEXT("dirtyPackageCount")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("state"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) > 16) return false;
        if (Text2 != TEXT("starting") && Text2 != TEXT("ready") && Text2 != TEXT("stopping")) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("projectId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("editorPid"));
        if (!Value5.IsValid()) return false;
        double Number6 = 0;
        if (!Value5.IsValid() || !Value5->TryGetNumber(Number6) || !FMath::IsFinite(Number6) || FMath::FloorToDouble(Number6) != Number6) return false;
        if (Number6 < 0) return false;
        if (Number6 > 4294967295) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("levelId"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) > 512) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("pie"));
        if (!Value9.IsValid()) return false;
        if (!Value9.IsValid() || Value9->Type != EJson::String) return false;
        const FString Text10 = Value9->AsString();
        if (MagiAxiUnicodeScalarCount(Text10) > 16) return false;
        if (Text10 != TEXT("running") && Text10 != TEXT("stopped")) return false;
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("dirtyPackageCount"));
        if (!Value11.IsValid()) return false;
        double Number12 = 0;
        if (!Value11.IsValid() || !Value11->TryGetNumber(Number12) || !FMath::IsFinite(Number12) || FMath::FloorToDouble(Number12) != Number12) return false;
        if (Number12 < 0) return false;
        if (Number12 > 9007199254740991) return false;
        return true;
    }
    if (Operation == TEXT("level.create"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("level") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("level"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("changed"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value4 = Object0->TryGetField(TEXT("dirtyPackages"));
        if (!Value4.IsValid()) return false;
        if (!Value4.IsValid() || Value4->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array5 = Value4->AsArray();
        if (Array5.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value6 : Array5)
        {
            if (!Value6.IsValid() || Value6->Type != EJson::String) return false;
            const FString Text7 = Value6->AsString();
            if (MagiAxiUnicodeScalarCount(Text7) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value8 = Object0->TryGetField(TEXT("savedPackages"));
        if (!Value8.IsValid()) return false;
        if (!Value8.IsValid() || Value8->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array9 = Value8->AsArray();
        if (Array9.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value10 : Array9)
        {
            if (!Value10.IsValid() || Value10->Type != EJson::String) return false;
            const FString Text11 = Value10->AsString();
            if (MagiAxiUnicodeScalarCount(Text11) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value12 = Object0->TryGetField(TEXT("revision"));
        if (!Value12.IsValid()) return false;
        if (!Value12.IsValid() || Value12->Type != EJson::String) return false;
        const FString Text13 = Value12->AsString();
        if (MagiAxiUnicodeScalarCount(Text13) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text13) > 64) return false;
        return true;
    }
    if (Operation == TEXT("level.current"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("level") && Entry.Key != TEXT("scope")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("level"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object2 = Value1->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object2->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("name") && Entry.Key != TEXT("worldType") && Entry.Key != TEXT("persistent") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value3 = Object2->TryGetField(TEXT("id"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        const TSharedPtr<FJsonValue> Value5 = Object2->TryGetField(TEXT("name"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 512) return false;
        const TSharedPtr<FJsonValue> Value7 = Object2->TryGetField(TEXT("worldType"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text8) > 64) return false;
        const TSharedPtr<FJsonValue> Value9 = Object2->TryGetField(TEXT("persistent"));
        if (!Value9.IsValid()) return false;
        if (!Value9.IsValid() || Value9->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value10 = Object2->TryGetField(TEXT("revision"));
        if (!Value10.IsValid()) return false;
        if (!Value10.IsValid() || Value10->Type != EJson::String) return false;
        const FString Text11 = Value10->AsString();
        if (MagiAxiUnicodeScalarCount(Text11) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text11) > 64) return false;
        const TSharedPtr<FJsonValue> Value12 = Object0->TryGetField(TEXT("scope"));
        if (!Value12.IsValid()) return false;
        if (!Value12.IsValid() || Value12->Type != EJson::String) return false;
        const FString Text13 = Value12->AsString();
        if (MagiAxiUnicodeScalarCount(Text13) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text13) > 512) return false;
        return true;
    }
    if (Operation == TEXT("level.list"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("count") && Entry.Key != TEXT("total") && Entry.Key != TEXT("scope") && Entry.Key != TEXT("revision") && Entry.Key != TEXT("items") && Entry.Key != TEXT("nextCursor")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("count"));
        if (!Value1.IsValid()) return false;
        double Number2 = 0;
        if (!Value1.IsValid() || !Value1->TryGetNumber(Number2) || !FMath::IsFinite(Number2) || FMath::FloorToDouble(Number2) != Number2) return false;
        if (Number2 < 0) return false;
        if (Number2 > 100) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("total"));
        if (!Value3.IsValid()) return false;
        double Number4 = 0;
        if (!Value3.IsValid() || !Value3->TryGetNumber(Number4) || !FMath::IsFinite(Number4) || FMath::FloorToDouble(Number4) != Number4) return false;
        if (Number4 < 0) return false;
        if (Number4 > 9007199254740991) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("scope"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 512) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("revision"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text8) > 64) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("items"));
        if (!Value9.IsValid()) return false;
        if (!Value9.IsValid() || Value9->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array10 = Value9->AsArray();
        if (Array10.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value11 : Array10)
        {
            if (!Value11.IsValid() || Value11->Type != EJson::Object) return false;
            const TSharedPtr<FJsonObject> Object12 = Value11->AsObject();
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object12->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("name") && Entry.Key != TEXT("worldType") && Entry.Key != TEXT("persistent")) return false;
            const TSharedPtr<FJsonValue> Value13 = Object12->TryGetField(TEXT("id"));
            if (!Value13.IsValid()) return false;
            if (!Value13.IsValid() || Value13->Type != EJson::String) return false;
            const FString Text14 = Value13->AsString();
            if (MagiAxiUnicodeScalarCount(Text14) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text14) > 512) return false;
            const TSharedPtr<FJsonValue> Value15 = Object12->TryGetField(TEXT("name"));
            if (Value15.IsValid())
            {
                if (!Value15.IsValid() || Value15->Type != EJson::String) return false;
                const FString Text16 = Value15->AsString();
                if (MagiAxiUnicodeScalarCount(Text16) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text16) > 512) return false;
            }
            const TSharedPtr<FJsonValue> Value17 = Object12->TryGetField(TEXT("worldType"));
            if (Value17.IsValid())
            {
                if (!Value17.IsValid() || Value17->Type != EJson::String) return false;
                const FString Text18 = Value17->AsString();
                if (MagiAxiUnicodeScalarCount(Text18) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text18) > 64) return false;
            }
            const TSharedPtr<FJsonValue> Value19 = Object12->TryGetField(TEXT("persistent"));
            if (Value19.IsValid())
            {
                if (!Value19.IsValid() || Value19->Type != EJson::Boolean) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value20 = Object0->TryGetField(TEXT("nextCursor"));
        if (!Value20.IsValid()) return false;
        if (Value20->Type != EJson::Null)
        {
            if (!Value20.IsValid() || Value20->Type != EJson::String) return false;
            const FString Text21 = Value20->AsString();
            if (MagiAxiUnicodeScalarCount(Text21) > 256) return false;
        }
        return true;
    }
    if (Operation == TEXT("level.open"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("level") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("level"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("changed"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value4 = Object0->TryGetField(TEXT("dirtyPackages"));
        if (!Value4.IsValid()) return false;
        if (!Value4.IsValid() || Value4->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array5 = Value4->AsArray();
        if (Array5.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value6 : Array5)
        {
            if (!Value6.IsValid() || Value6->Type != EJson::String) return false;
            const FString Text7 = Value6->AsString();
            if (MagiAxiUnicodeScalarCount(Text7) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value8 = Object0->TryGetField(TEXT("savedPackages"));
        if (!Value8.IsValid()) return false;
        if (!Value8.IsValid() || Value8->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array9 = Value8->AsArray();
        if (Array9.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value10 : Array9)
        {
            if (!Value10.IsValid() || Value10->Type != EJson::String) return false;
            const FString Text11 = Value10->AsString();
            if (MagiAxiUnicodeScalarCount(Text11) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value12 = Object0->TryGetField(TEXT("revision"));
        if (!Value12.IsValid()) return false;
        if (!Value12.IsValid() || Value12->Type != EJson::String) return false;
        const FString Text13 = Value12->AsString();
        if (MagiAxiUnicodeScalarCount(Text13) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text13) > 64) return false;
        return true;
    }
    if (Operation == TEXT("level.save"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("level") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("level"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("changed"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value4 = Object0->TryGetField(TEXT("dirtyPackages"));
        if (!Value4.IsValid()) return false;
        if (!Value4.IsValid() || Value4->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array5 = Value4->AsArray();
        if (Array5.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value6 : Array5)
        {
            if (!Value6.IsValid() || Value6->Type != EJson::String) return false;
            const FString Text7 = Value6->AsString();
            if (MagiAxiUnicodeScalarCount(Text7) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value8 = Object0->TryGetField(TEXT("savedPackages"));
        if (!Value8.IsValid()) return false;
        if (!Value8.IsValid() || Value8->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array9 = Value8->AsArray();
        if (Array9.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value10 : Array9)
        {
            if (!Value10.IsValid() || Value10->Type != EJson::String) return false;
            const FString Text11 = Value10->AsString();
            if (MagiAxiUnicodeScalarCount(Text11) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value12 = Object0->TryGetField(TEXT("revision"));
        if (!Value12.IsValid()) return false;
        if (!Value12.IsValid() || Value12->Type != EJson::String) return false;
        const FString Text13 = Value12->AsString();
        if (MagiAxiUnicodeScalarCount(Text13) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text13) > 64) return false;
        return true;
    }
    if (Operation == TEXT("operation.view"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("operationId") && Entry.Key != TEXT("operation") && Entry.Key != TEXT("state") && Entry.Key != TEXT("projectId") && Entry.Key != TEXT("editorPid") && Entry.Key != TEXT("target") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("transaction") && Entry.Key != TEXT("reversibility") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision") && Entry.Key != TEXT("persistence") && Entry.Key != TEXT("verification")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("operationId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 128) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("operation"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 128) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("state"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) > 32) return false;
        if (Text6 != TEXT("queued") && Text6 != TEXT("running") && Text6 != TEXT("completed") && Text6 != TEXT("failed") && Text6 != TEXT("outcome_unknown")) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("projectId"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text8) > 512) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("editorPid"));
        if (!Value9.IsValid()) return false;
        double Number10 = 0;
        if (!Value9.IsValid() || !Value9->TryGetNumber(Number10) || !FMath::IsFinite(Number10) || FMath::FloorToDouble(Number10) != Number10) return false;
        if (Number10 < 1) return false;
        if (Number10 > 4294967295) return false;
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("target"));
        if (!Value11.IsValid()) return false;
        if (!Value11.IsValid() || Value11->Type != EJson::String) return false;
        const FString Text12 = Value11->AsString();
        if (MagiAxiUnicodeScalarCount(Text12) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text12) > 1024) return false;
        const TSharedPtr<FJsonValue> Value13 = Object0->TryGetField(TEXT("changed"));
        if (!Value13.IsValid()) return false;
        if (!Value13.IsValid() || Value13->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value14 = Object0->TryGetField(TEXT("transaction"));
        if (!Value14.IsValid()) return false;
        if (!Value14.IsValid() || Value14->Type != EJson::String) return false;
        const FString Text15 = Value14->AsString();
        if (MagiAxiUnicodeScalarCount(Text15) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text15) > 32) return false;
        const TSharedPtr<FJsonValue> Value16 = Object0->TryGetField(TEXT("reversibility"));
        if (!Value16.IsValid()) return false;
        if (!Value16.IsValid() || Value16->Type != EJson::String) return false;
        const FString Text17 = Value16->AsString();
        if (MagiAxiUnicodeScalarCount(Text17) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text17) > 32) return false;
        const TSharedPtr<FJsonValue> Value18 = Object0->TryGetField(TEXT("dirtyPackages"));
        if (!Value18.IsValid()) return false;
        if (!Value18.IsValid() || Value18->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array19 = Value18->AsArray();
        if (Array19.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value20 : Array19)
        {
            if (!Value20.IsValid() || Value20->Type != EJson::String) return false;
            const FString Text21 = Value20->AsString();
            if (MagiAxiUnicodeScalarCount(Text21) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value22 = Object0->TryGetField(TEXT("savedPackages"));
        if (!Value22.IsValid()) return false;
        if (!Value22.IsValid() || Value22->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array23 = Value22->AsArray();
        if (Array23.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value24 : Array23)
        {
            if (!Value24.IsValid() || Value24->Type != EJson::String) return false;
            const FString Text25 = Value24->AsString();
            if (MagiAxiUnicodeScalarCount(Text25) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value26 = Object0->TryGetField(TEXT("revision"));
        if (!Value26.IsValid()) return false;
        if (!Value26.IsValid() || Value26->Type != EJson::String) return false;
        const FString Text27 = Value26->AsString();
        if (MagiAxiUnicodeScalarCount(Text27) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text27) > 64) return false;
        const TSharedPtr<FJsonValue> Value28 = Object0->TryGetField(TEXT("persistence"));
        if (!Value28.IsValid()) return false;
        if (!Value28.IsValid() || Value28->Type != EJson::String) return false;
        const FString Text29 = Value28->AsString();
        if (MagiAxiUnicodeScalarCount(Text29) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text29) > 32) return false;
        const TSharedPtr<FJsonValue> Value30 = Object0->TryGetField(TEXT("verification"));
        if (!Value30.IsValid()) return false;
        if (!Value30.IsValid() || Value30->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object31 = Value30->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object31->Values) if (Entry.Key != TEXT("readback") && Entry.Key != TEXT("target") && Entry.Key != TEXT("matched") && Entry.Key != TEXT("exists") && Entry.Key != TEXT("observedRevision") && Entry.Key != TEXT("accepted") && Entry.Key != TEXT("beforeRevision") && Entry.Key != TEXT("afterRevision")) return false;
        const TSharedPtr<FJsonValue> Value32 = Object31->TryGetField(TEXT("readback"));
        if (!Value32.IsValid()) return false;
        if (!Value32.IsValid() || Value32->Type != EJson::String) return false;
        const FString Text33 = Value32->AsString();
        if (MagiAxiUnicodeScalarCount(Text33) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text33) > 128) return false;
        const TSharedPtr<FJsonValue> Value34 = Object31->TryGetField(TEXT("target"));
        if (!Value34.IsValid()) return false;
        if (!Value34.IsValid() || Value34->Type != EJson::String) return false;
        const FString Text35 = Value34->AsString();
        if (MagiAxiUnicodeScalarCount(Text35) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text35) > 1024) return false;
        const TSharedPtr<FJsonValue> Value36 = Object31->TryGetField(TEXT("matched"));
        if (!Value36.IsValid()) return false;
        if (!Value36.IsValid() || Value36->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value37 = Object31->TryGetField(TEXT("exists"));
        if (Value37.IsValid())
        {
            if (!Value37.IsValid() || Value37->Type != EJson::Boolean) return false;
        }
        const TSharedPtr<FJsonValue> Value38 = Object31->TryGetField(TEXT("observedRevision"));
        if (Value38.IsValid())
        {
            if (!Value38.IsValid() || Value38->Type != EJson::String) return false;
            const FString Text39 = Value38->AsString();
            if (MagiAxiUnicodeScalarCount(Text39) < 64) return false;
            if (MagiAxiUnicodeScalarCount(Text39) > 64) return false;
        }
        const TSharedPtr<FJsonValue> Value40 = Object31->TryGetField(TEXT("accepted"));
        if (Value40.IsValid())
        {
            if (!Value40.IsValid() || Value40->Type != EJson::Boolean) return false;
        }
        const TSharedPtr<FJsonValue> Value41 = Object31->TryGetField(TEXT("beforeRevision"));
        if (Value41.IsValid())
        {
            if (!Value41.IsValid() || Value41->Type != EJson::String) return false;
            const FString Text42 = Value41->AsString();
            if (MagiAxiUnicodeScalarCount(Text42) < 64) return false;
            if (MagiAxiUnicodeScalarCount(Text42) > 64) return false;
        }
        const TSharedPtr<FJsonValue> Value43 = Object31->TryGetField(TEXT("afterRevision"));
        if (Value43.IsValid())
        {
            if (!Value43.IsValid() || Value43->Type != EJson::String) return false;
            const FString Text44 = Value43->AsString();
            if (MagiAxiUnicodeScalarCount(Text44) < 64) return false;
            if (MagiAxiUnicodeScalarCount(Text44) > 64) return false;
        }
        return true;
    }
    return false;
}
