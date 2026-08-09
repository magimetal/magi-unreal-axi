#pragma once

#define MAGI_AXI_VERSION TEXT("0.1.0")
#define MAGI_AXI_CATALOG_HASH TEXT("7cd513c54122e73b4c0b5faaf8f3669f89819584822e10602017e9f41f19e05b")
#define MAGI_AXI_CATALOG_COUNT 55
#define MAGI_AXI_NATIVE_CAPABILITY_COUNT 53
#define MAGI_AXI_PUBLIC_OPERATION_COUNT 56
#define MAGI_AXI_NATIVE_CAPABILITIES(X) \
    X(TEXT("actor.delete")) \
    X(TEXT("actor.list")) \
    X(TEXT("actor.spawn")) \
    X(TEXT("actor.update_transform")) \
    X(TEXT("actor.view")) \
    X(TEXT("asset.create_input_action")) \
    X(TEXT("asset.create_input_mapping_context")) \
    X(TEXT("asset.save")) \
    X(TEXT("blueprint.interface_create")) \
    X(TEXT("blueprint.interface_view")) \
    X(TEXT("blueprint.interface_ensure")) \
    X(TEXT("blueprint.scs_view")) \
    X(TEXT("blueprint.scs_component_ensure")) \
    X(TEXT("blueprint.scs_component_update")) \
    X(TEXT("blueprint.scs_component_remove")) \
    X(TEXT("play.component_observe")) \
    X(TEXT("blueprint.compile")) \
    X(TEXT("blueprint.view")) \
    X(TEXT("blueprint.create")) \
    X(TEXT("blueprint.graph_view")) \
    X(TEXT("blueprint.event_ensure")) \
    X(TEXT("blueprint.node_ensure")) \
    X(TEXT("blueprint.pin_default_set")) \
    X(TEXT("blueprint.pin_connect")) \
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
    X(TEXT("play.ui_observe")) \
    X(TEXT("widget.create")) \
    X(TEXT("widget.tree_view")) \
    X(TEXT("widget.child_ensure")) \
    X(TEXT("widget.property_set")) \
    X(TEXT("widget.event_ensure")) \
    X(TEXT("widget.viewport_ensure"))

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
    X(TEXT("blueprint.interface_create")) \
    X(TEXT("blueprint.interface_view")) \
    X(TEXT("blueprint.interface_ensure")) \
    X(TEXT("blueprint.scs_view")) \
    X(TEXT("blueprint.scs_component_ensure")) \
    X(TEXT("blueprint.scs_component_update")) \
    X(TEXT("blueprint.scs_component_remove")) \
    X(TEXT("play.component_observe")) \
    X(TEXT("blueprint.compile")) \
    X(TEXT("blueprint.view")) \
    X(TEXT("blueprint.create")) \
    X(TEXT("blueprint.graph_view")) \
    X(TEXT("blueprint.event_ensure")) \
    X(TEXT("blueprint.node_ensure")) \
    X(TEXT("blueprint.pin_default_set")) \
    X(TEXT("blueprint.pin_connect")) \
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
    X(TEXT("play.ui_observe")) \
    X(TEXT("widget.create")) \
    X(TEXT("widget.tree_view")) \
    X(TEXT("widget.child_ensure")) \
    X(TEXT("widget.property_set")) \
    X(TEXT("widget.event_ensure")) \
    X(TEXT("widget.viewport_ensure")) \
    X(TEXT("editor.stop"))

struct FMagiAxiCapabilityMetadata
{
const TCHAR* Id;
bool Mutates;
bool Destructive;
const TCHAR* Idempotency;
const TCHAR* SaveBehavior;
const TCHAR* TransactionBehavior;
const TCHAR* Reversibility;
const TCHAR* AllowedEditorStates;
const TCHAR* RequiredModules;
const TCHAR* Readback;
const TCHAR* TargetFields;
const TCHAR* FailureReceipt;
};

static const FMagiAxiCapabilityMetadata MagiAxiCapabilityMetadata[] =
{
    {TEXT("actor.delete"), true, true, TEXT("request-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("destructive"), TEXT("editing"), TEXT("UnrealEd"), TEXT("actor.view"), TEXT("id"), TEXT("")},
    {TEXT("actor.list"), false, false, TEXT("read-only"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("editing"), TEXT("UnrealEd"), TEXT("actor.view"), TEXT(""), TEXT("")},
    {TEXT("actor.spawn"), true, false, TEXT("natural-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd"), TEXT("actor.view"), TEXT("id"), TEXT("")},
    {TEXT("actor.update_transform"), true, false, TEXT("request-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd"), TEXT("actor.view"), TEXT("id"), TEXT("")},
    {TEXT("actor.view"), false, false, TEXT("read-only"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("editing"), TEXT("UnrealEd"), TEXT(""), TEXT(""), TEXT("")},
    {TEXT("asset.create_input_action"), true, false, TEXT("natural-key"), TEXT("explicit"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd"), TEXT("asset.view"), TEXT("id"), TEXT("")},
    {TEXT("asset.create_input_mapping_context"), true, false, TEXT("natural-key"), TEXT("explicit"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd"), TEXT("asset.view"), TEXT("id"), TEXT("")},
    {TEXT("asset.save"), true, false, TEXT("natural-key"), TEXT("explicit"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd"), TEXT("asset.view"), TEXT("id"), TEXT("")},
    {TEXT("blueprint.interface_create"), true, false, TEXT("natural-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd|BlueprintGraph"), TEXT("blueprint.interface_view"), TEXT("id"), TEXT("")},
    {TEXT("blueprint.interface_view"), false, false, TEXT("read-only"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("editing"), TEXT("UnrealEd|BlueprintGraph"), TEXT(""), TEXT(""), TEXT("")},
    {TEXT("blueprint.interface_ensure"), true, false, TEXT("natural-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd|BlueprintGraph"), TEXT("blueprint.view"), TEXT("blueprintId"), TEXT("")},
    {TEXT("blueprint.scs_view"), false, false, TEXT("read-only"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("editing"), TEXT("UnrealEd|BlueprintGraph"), TEXT(""), TEXT(""), TEXT("")},
    {TEXT("blueprint.scs_component_ensure"), true, false, TEXT("natural-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd|BlueprintGraph"), TEXT("blueprint.scs_view"), TEXT("blueprintId"), TEXT("")},
    {TEXT("blueprint.scs_component_update"), true, false, TEXT("natural-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd|BlueprintGraph"), TEXT("blueprint.scs_view"), TEXT("blueprintId|variableGuid"), TEXT("")},
    {TEXT("blueprint.scs_component_remove"), true, true, TEXT("request-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("destructive"), TEXT("editing"), TEXT("UnrealEd|BlueprintGraph"), TEXT("blueprint.scs_view"), TEXT("blueprintId|variableGuid"), TEXT("")},
    {TEXT("play.component_observe"), false, false, TEXT("read-only"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("playing"), TEXT("UnrealEd"), TEXT(""), TEXT(""), TEXT("")},
    {TEXT("blueprint.compile"), true, false, TEXT("request-key"), TEXT("dirty-only"), TEXT("non-atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd|KismetCompiler"), TEXT("blueprint.view"), TEXT("id"), TEXT("preserved-dirty")},
    {TEXT("blueprint.view"), false, false, TEXT("read-only"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("editing"), TEXT("UnrealEd|KismetCompiler"), TEXT(""), TEXT(""), TEXT("")},
    {TEXT("blueprint.create"), true, false, TEXT("natural-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd|KismetCompiler"), TEXT("blueprint.graph_view"), TEXT("blueprintId"), TEXT("")},
    {TEXT("blueprint.graph_view"), false, false, TEXT("read-only"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("editing"), TEXT("UnrealEd"), TEXT(""), TEXT(""), TEXT("")},
    {TEXT("blueprint.event_ensure"), true, false, TEXT("natural-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd|BlueprintGraph"), TEXT("blueprint.graph_view"), TEXT("blueprintId|graphId|nodeId"), TEXT("")},
    {TEXT("blueprint.node_ensure"), true, false, TEXT("natural-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd|BlueprintGraph"), TEXT("blueprint.graph_view"), TEXT("blueprintId|graphId|nodeId"), TEXT("")},
    {TEXT("blueprint.pin_default_set"), true, false, TEXT("natural-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd|BlueprintGraph"), TEXT("blueprint.graph_view"), TEXT("blueprintId|pinId"), TEXT("")},
    {TEXT("blueprint.pin_connect"), true, false, TEXT("natural-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd|BlueprintGraph"), TEXT("blueprint.graph_view"), TEXT("blueprintId|sourcePinId|targetPinId"), TEXT("")},
    {TEXT("component.add"), true, false, TEXT("natural-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd"), TEXT("component.view"), TEXT("id"), TEXT("")},
    {TEXT("component.list"), false, false, TEXT("read-only"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("editing"), TEXT("UnrealEd"), TEXT(""), TEXT(""), TEXT("")},
    {TEXT("component.remove"), true, true, TEXT("request-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("destructive"), TEXT("editing"), TEXT("UnrealEd"), TEXT("component.view"), TEXT("id"), TEXT("")},
    {TEXT("component.update"), true, false, TEXT("natural-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd"), TEXT("component.view"), TEXT("id"), TEXT("")},
    {TEXT("component.view"), false, false, TEXT("read-only"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("editing"), TEXT("UnrealEd"), TEXT(""), TEXT(""), TEXT("")},
    {TEXT("level.set_game_mode"), true, false, TEXT("natural-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd"), TEXT("level.settings"), TEXT("levelId"), TEXT("")},
    {TEXT("level.settings"), false, false, TEXT("read-only"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("editing"), TEXT("UnrealEd"), TEXT(""), TEXT(""), TEXT("")},
    {TEXT("play.input"), true, false, TEXT("request-key"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("playing"), TEXT("UnrealEd"), TEXT("play.observe"), TEXT("sessionId|key|event"), TEXT("")},
    {TEXT("play.observe"), false, false, TEXT("read-only"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("playing"), TEXT("UnrealEd"), TEXT(""), TEXT(""), TEXT("")},
    {TEXT("play.screenshot"), true, false, TEXT("request-key"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("playing"), TEXT("UnrealEd"), TEXT("artifact"), TEXT("path"), TEXT("")},
    {TEXT("play.start"), true, false, TEXT("natural-key"), TEXT("none"), TEXT("atomic"), TEXT("none"), TEXT("editing"), TEXT("UnrealEd"), TEXT("play.status"), TEXT("sessionId"), TEXT("")},
    {TEXT("play.status"), false, false, TEXT("read-only"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("editing|playing"), TEXT("UnrealEd"), TEXT(""), TEXT(""), TEXT("")},
    {TEXT("play.stop"), true, false, TEXT("natural-key"), TEXT("none"), TEXT("atomic"), TEXT("none"), TEXT("playing"), TEXT("UnrealEd"), TEXT("play.status"), TEXT("sessionId"), TEXT("")},
    {TEXT("asset.list"), false, false, TEXT("read-only"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("editing"), TEXT("AssetRegistry"), TEXT("asset.view"), TEXT(""), TEXT("")},
    {TEXT("asset.view"), false, false, TEXT("read-only"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("editing"), TEXT("AssetRegistry"), TEXT(""), TEXT(""), TEXT("")},
    {TEXT("editor.status"), false, false, TEXT("read-only"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("editing"), TEXT("UnrealEd"), TEXT(""), TEXT(""), TEXT("")},
    {TEXT("level.create"), true, false, TEXT("natural-key"), TEXT("explicit"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd"), TEXT("level.current"), TEXT("level"), TEXT("")},
    {TEXT("level.current"), false, false, TEXT("read-only"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("editing"), TEXT("UnrealEd"), TEXT(""), TEXT(""), TEXT("")},
    {TEXT("level.list"), false, false, TEXT("read-only"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("editing"), TEXT("AssetRegistry"), TEXT(""), TEXT(""), TEXT("")},
    {TEXT("level.open"), true, false, TEXT("natural-key"), TEXT("none"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd"), TEXT("level.current"), TEXT("level"), TEXT("")},
    {TEXT("level.save"), true, false, TEXT("natural-key"), TEXT("explicit"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UnrealEd"), TEXT("level.current"), TEXT("level"), TEXT("")},
    {TEXT("operation.view"), false, false, TEXT("read-only"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("editing|playing"), TEXT("UnrealEd"), TEXT(""), TEXT(""), TEXT("")},
    {TEXT("play.ui_observe"), false, false, TEXT("read-only"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("playing"), TEXT("UMG|UnrealEd"), TEXT(""), TEXT(""), TEXT("")},
    {TEXT("widget.create"), true, false, TEXT("natural-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UMG|UMGEditor|UnrealEd|KismetCompiler|BlueprintGraph"), TEXT("widget.tree_view"), TEXT("blueprintId"), TEXT("")},
    {TEXT("widget.tree_view"), false, false, TEXT("read-only"), TEXT("none"), TEXT("none"), TEXT("none"), TEXT("editing"), TEXT("UMG|UnrealEd"), TEXT(""), TEXT(""), TEXT("")},
    {TEXT("widget.child_ensure"), true, false, TEXT("natural-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UMG|UMGEditor|UnrealEd|KismetCompiler|BlueprintGraph"), TEXT("widget.tree_view"), TEXT("blueprintId|widgetId"), TEXT("")},
    {TEXT("widget.property_set"), true, false, TEXT("natural-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UMG|UMGEditor|UnrealEd|KismetCompiler|BlueprintGraph"), TEXT("widget.tree_view"), TEXT("blueprintId|widgetId|property"), TEXT("")},
    {TEXT("widget.event_ensure"), true, false, TEXT("natural-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UMG|UMGEditor|UnrealEd|KismetCompiler|BlueprintGraph"), TEXT("widget.tree_view"), TEXT("blueprintId|eventId"), TEXT("")},
    {TEXT("widget.viewport_ensure"), true, false, TEXT("natural-key"), TEXT("dirty-only"), TEXT("atomic"), TEXT("source-control"), TEXT("editing"), TEXT("UMG|UMGEditor|UnrealEd|KismetCompiler|BlueprintGraph"), TEXT("blueprint.graph_view"), TEXT("hostBlueprintId|viewportId"), TEXT("")},
};

static const FMagiAxiCapabilityMetadata* MagiAxiFindCapabilityMetadata(const FString& Operation)
{
for (const FMagiAxiCapabilityMetadata& Metadata : MagiAxiCapabilityMetadata)
if (Operation == Metadata.Id) return &Metadata;
return nullptr;
}

#define MAGI_AXI_INVALID_OUTPUT_FIXTURES_JSON TEXT("[{\"operation\":\"actor.delete\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"actor.delete\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"changed\":false,\"dryRun\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"actor.list\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"actor.list\",\"case\":\"root-not-closed\",\"result\":{\"count\":0,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null,\"unknown\":true}},{\"operation\":\"actor.list\",\"case\":\"numeric-below-minimum@count\",\"result\":{\"count\":-1,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"actor.list\",\"case\":\"numeric-above-maximum@count\",\"result\":{\"count\":101,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"actor.list\",\"case\":\"numeric-below-minimum@total\",\"result\":{\"count\":0,\"total\":-1,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"actor.list\",\"case\":\"numeric-above-maximum@total\",\"result\":{\"count\":0,\"total\":9007199254740992,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"actor.spawn\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"actor.spawn\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"actorGuid\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"actor.update_transform\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"actor.update_transform\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"actor.view\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"actor.view\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"actorGuid\":\"x\",\"levelId\":\"x\",\"label\":\"x\",\"class\":\"x\",\"objectPath\":\"x\",\"location\":[-1000000000.0,-1000000000.0,-1000000000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"actor.view\",\"case\":\"numeric-below-minimum@location.0\",\"result\":{\"id\":\"x\",\"actorGuid\":\"x\",\"levelId\":\"x\",\"label\":\"x\",\"class\":\"x\",\"objectPath\":\"x\",\"location\":[-1000000001.0,-1000000000.0,-1000000000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"actor.view\",\"case\":\"numeric-above-maximum@location.0\",\"result\":{\"id\":\"x\",\"actorGuid\":\"x\",\"levelId\":\"x\",\"label\":\"x\",\"class\":\"x\",\"objectPath\":\"x\",\"location\":[1000000001.0,-1000000000.0,-1000000000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"actor.view\",\"case\":\"numeric-below-minimum@rotation.0\",\"result\":{\"id\":\"x\",\"actorGuid\":\"x\",\"levelId\":\"x\",\"label\":\"x\",\"class\":\"x\",\"objectPath\":\"x\",\"location\":[-1000000000.0,-1000000000.0,-1000000000.0],\"rotation\":[-361.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"actor.view\",\"case\":\"numeric-above-maximum@rotation.0\",\"result\":{\"id\":\"x\",\"actorGuid\":\"x\",\"levelId\":\"x\",\"label\":\"x\",\"class\":\"x\",\"objectPath\":\"x\",\"location\":[-1000000000.0,-1000000000.0,-1000000000.0],\"rotation\":[361.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"actor.view\",\"case\":\"numeric-below-minimum@scale.0\",\"result\":{\"id\":\"x\",\"actorGuid\":\"x\",\"levelId\":\"x\",\"label\":\"x\",\"class\":\"x\",\"objectPath\":\"x\",\"location\":[-1000000000.0,-1000000000.0,-1000000000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[-1.0,0.0,0.0],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"actor.view\",\"case\":\"numeric-above-maximum@scale.0\",\"result\":{\"id\":\"x\",\"actorGuid\":\"x\",\"levelId\":\"x\",\"label\":\"x\",\"class\":\"x\",\"objectPath\":\"x\",\"location\":[-1000000000.0,-1000000000.0,-1000000000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[1000001.0,0.0,0.0],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"asset.create_input_action\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"asset.create_input_action\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"class\":\"x\",\"valueType\":\"Boolean\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"asset.create_input_mapping_context\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"asset.create_input_mapping_context\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"class\":\"x\",\"mappingCount\":0,\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"asset.create_input_mapping_context\",\"case\":\"numeric-below-minimum@mappingCount\",\"result\":{\"id\":\"x\",\"class\":\"x\",\"mappingCount\":-1,\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"asset.create_input_mapping_context\",\"case\":\"numeric-above-maximum@mappingCount\",\"result\":{\"id\":\"x\",\"class\":\"x\",\"mappingCount\":101,\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"asset.save\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"asset.save\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"blueprint.interface_create\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"blueprint.interface_create\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"function\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"blueprint.interface_view\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"blueprint.interface_view\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"function\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"blueprint.interface_ensure\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"blueprint.interface_ensure\",\"case\":\"root-not-closed\",\"result\":{\"blueprintId\":\"x\",\"interfaceId\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"blueprint.scs_view\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"blueprint.scs_view\",\"case\":\"root-not-closed\",\"result\":{\"blueprintId\":\"x\",\"components\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"blueprint.scs_view\",\"case\":\"numeric-below-minimum@components.0.location.0\",\"result\":{\"blueprintId\":\"x\",\"components\":[{\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"name\":\"x\",\"class\":\"x\",\"parent\":null,\"location\":[-100001.0,-100000.0,-100000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"boxExtent\":null,\"sphereRadius\":null}],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"blueprint.scs_view\",\"case\":\"numeric-above-maximum@components.0.location.0\",\"result\":{\"blueprintId\":\"x\",\"components\":[{\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"name\":\"x\",\"class\":\"x\",\"parent\":null,\"location\":[100001.0,-100000.0,-100000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"boxExtent\":null,\"sphereRadius\":null}],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"blueprint.scs_view\",\"case\":\"numeric-below-minimum@components.0.rotation.0\",\"result\":{\"blueprintId\":\"x\",\"components\":[{\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"name\":\"x\",\"class\":\"x\",\"parent\":null,\"location\":[-100000.0,-100000.0,-100000.0],\"rotation\":[-361.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"boxExtent\":null,\"sphereRadius\":null}],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"blueprint.scs_view\",\"case\":\"numeric-above-maximum@components.0.rotation.0\",\"result\":{\"blueprintId\":\"x\",\"components\":[{\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"name\":\"x\",\"class\":\"x\",\"parent\":null,\"location\":[-100000.0,-100000.0,-100000.0],\"rotation\":[361.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"boxExtent\":null,\"sphereRadius\":null}],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"blueprint.scs_view\",\"case\":\"numeric-below-minimum@components.0.scale.0\",\"result\":{\"blueprintId\":\"x\",\"components\":[{\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"name\":\"x\",\"class\":\"x\",\"parent\":null,\"location\":[-100000.0,-100000.0,-100000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[-1.0,0.0,0.0],\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"boxExtent\":null,\"sphereRadius\":null}],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"blueprint.scs_view\",\"case\":\"numeric-above-maximum@components.0.scale.0\",\"result\":{\"blueprintId\":\"x\",\"components\":[{\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"name\":\"x\",\"class\":\"x\",\"parent\":null,\"location\":[-100000.0,-100000.0,-100000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[1001.0,0.0,0.0],\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"boxExtent\":null,\"sphereRadius\":null}],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"blueprint.scs_view\",\"case\":\"numeric-below-minimum@components.0.massOverride\",\"result\":{\"blueprintId\":\"x\",\"components\":[{\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"name\":\"x\",\"class\":\"x\",\"parent\":null,\"location\":[-100000.0,-100000.0,-100000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":-1.0,\"boxExtent\":null,\"sphereRadius\":null}],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"blueprint.scs_view\",\"case\":\"numeric-above-maximum@components.0.massOverride\",\"result\":{\"blueprintId\":\"x\",\"components\":[{\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"name\":\"x\",\"class\":\"x\",\"parent\":null,\"location\":[-100000.0,-100000.0,-100000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":100001.0,\"boxExtent\":null,\"sphereRadius\":null}],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"blueprint.scs_view\",\"case\":\"numeric-below-minimum@components.0.boxExtent.0\",\"result\":{\"blueprintId\":\"x\",\"components\":[{\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"name\":\"x\",\"class\":\"x\",\"parent\":null,\"location\":[-100000.0,-100000.0,-100000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"boxExtent\":[-1.0],\"sphereRadius\":null}],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"blueprint.scs_view\",\"case\":\"numeric-above-maximum@components.0.boxExtent.0\",\"result\":{\"blueprintId\":\"x\",\"components\":[{\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"name\":\"x\",\"class\":\"x\",\"parent\":null,\"location\":[-100000.0,-100000.0,-100000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"boxExtent\":[100001.0],\"sphereRadius\":null}],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"blueprint.scs_view\",\"case\":\"numeric-below-minimum@components.0.sphereRadius\",\"result\":{\"blueprintId\":\"x\",\"components\":[{\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"name\":\"x\",\"class\":\"x\",\"parent\":null,\"location\":[-100000.0,-100000.0,-100000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"boxExtent\":null,\"sphereRadius\":-1.0}],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"blueprint.scs_view\",\"case\":\"numeric-above-maximum@components.0.sphereRadius\",\"result\":{\"blueprintId\":\"x\",\"components\":[{\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"name\":\"x\",\"class\":\"x\",\"parent\":null,\"location\":[-100000.0,-100000.0,-100000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"boxExtent\":null,\"sphereRadius\":100001.0}],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"blueprint.scs_component_ensure\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"blueprint.scs_component_ensure\",\"case\":\"root-not-closed\",\"result\":{\"blueprintId\":\"x\",\"variableGuid\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"blueprint.scs_component_update\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"blueprint.scs_component_update\",\"case\":\"root-not-closed\",\"result\":{\"blueprintId\":\"x\",\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"changed\":false,\"dirtyPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"blueprint.scs_component_remove\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"blueprint.scs_component_remove\",\"case\":\"root-not-closed\",\"result\":{\"blueprintId\":\"x\",\"variableGuid\":\"x\",\"changed\":false,\"dryRun\":false,\"dirtyPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"play.component_observe\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"play.component_observe\",\"case\":\"root-not-closed\",\"result\":{\"sessionId\":\"x\",\"actorId\":\"x\",\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"resolved\":false,\"reason\":null,\"componentName\":null,\"componentClass\":null,\"actorLocation\":null,\"location\":null,\"rotation\":null,\"scale\":null,\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"linearVelocity\":null,\"angularVelocity\":null,\"overlapCount\":null,\"overlappingActorIds\":[],\"interactionDisplacement\":null,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"play.component_observe\",\"case\":\"numeric-below-minimum@actorLocation.0\",\"result\":{\"sessionId\":\"x\",\"actorId\":\"x\",\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"resolved\":false,\"reason\":null,\"componentName\":null,\"componentClass\":null,\"actorLocation\":[-1000000001.0],\"location\":null,\"rotation\":null,\"scale\":null,\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"linearVelocity\":null,\"angularVelocity\":null,\"overlapCount\":null,\"overlappingActorIds\":[],\"interactionDisplacement\":null,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.component_observe\",\"case\":\"numeric-above-maximum@actorLocation.0\",\"result\":{\"sessionId\":\"x\",\"actorId\":\"x\",\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"resolved\":false,\"reason\":null,\"componentName\":null,\"componentClass\":null,\"actorLocation\":[1000000001.0],\"location\":null,\"rotation\":null,\"scale\":null,\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"linearVelocity\":null,\"angularVelocity\":null,\"overlapCount\":null,\"overlappingActorIds\":[],\"interactionDisplacement\":null,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.component_observe\",\"case\":\"numeric-below-minimum@location.0\",\"result\":{\"sessionId\":\"x\",\"actorId\":\"x\",\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"resolved\":false,\"reason\":null,\"componentName\":null,\"componentClass\":null,\"actorLocation\":null,\"location\":[-1000000001.0],\"rotation\":null,\"scale\":null,\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"linearVelocity\":null,\"angularVelocity\":null,\"overlapCount\":null,\"overlappingActorIds\":[],\"interactionDisplacement\":null,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.component_observe\",\"case\":\"numeric-above-maximum@location.0\",\"result\":{\"sessionId\":\"x\",\"actorId\":\"x\",\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"resolved\":false,\"reason\":null,\"componentName\":null,\"componentClass\":null,\"actorLocation\":null,\"location\":[1000000001.0],\"rotation\":null,\"scale\":null,\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"linearVelocity\":null,\"angularVelocity\":null,\"overlapCount\":null,\"overlappingActorIds\":[],\"interactionDisplacement\":null,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.component_observe\",\"case\":\"numeric-below-minimum@rotation.0\",\"result\":{\"sessionId\":\"x\",\"actorId\":\"x\",\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"resolved\":false,\"reason\":null,\"componentName\":null,\"componentClass\":null,\"actorLocation\":null,\"location\":null,\"rotation\":[-361.0],\"scale\":null,\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"linearVelocity\":null,\"angularVelocity\":null,\"overlapCount\":null,\"overlappingActorIds\":[],\"interactionDisplacement\":null,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.component_observe\",\"case\":\"numeric-above-maximum@rotation.0\",\"result\":{\"sessionId\":\"x\",\"actorId\":\"x\",\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"resolved\":false,\"reason\":null,\"componentName\":null,\"componentClass\":null,\"actorLocation\":null,\"location\":null,\"rotation\":[361.0],\"scale\":null,\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"linearVelocity\":null,\"angularVelocity\":null,\"overlapCount\":null,\"overlappingActorIds\":[],\"interactionDisplacement\":null,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.component_observe\",\"case\":\"numeric-below-minimum@scale.0\",\"result\":{\"sessionId\":\"x\",\"actorId\":\"x\",\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"resolved\":false,\"reason\":null,\"componentName\":null,\"componentClass\":null,\"actorLocation\":null,\"location\":null,\"rotation\":null,\"scale\":[-1.0],\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"linearVelocity\":null,\"angularVelocity\":null,\"overlapCount\":null,\"overlappingActorIds\":[],\"interactionDisplacement\":null,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.component_observe\",\"case\":\"numeric-above-maximum@scale.0\",\"result\":{\"sessionId\":\"x\",\"actorId\":\"x\",\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"resolved\":false,\"reason\":null,\"componentName\":null,\"componentClass\":null,\"actorLocation\":null,\"location\":null,\"rotation\":null,\"scale\":[1000000001.0],\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"linearVelocity\":null,\"angularVelocity\":null,\"overlapCount\":null,\"overlappingActorIds\":[],\"interactionDisplacement\":null,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.component_observe\",\"case\":\"numeric-below-minimum@massOverride\",\"result\":{\"sessionId\":\"x\",\"actorId\":\"x\",\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"resolved\":false,\"reason\":null,\"componentName\":null,\"componentClass\":null,\"actorLocation\":null,\"location\":null,\"rotation\":null,\"scale\":null,\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":-1.0,\"linearVelocity\":null,\"angularVelocity\":null,\"overlapCount\":null,\"overlappingActorIds\":[],\"interactionDisplacement\":null,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.component_observe\",\"case\":\"numeric-above-maximum@massOverride\",\"result\":{\"sessionId\":\"x\",\"actorId\":\"x\",\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"resolved\":false,\"reason\":null,\"componentName\":null,\"componentClass\":null,\"actorLocation\":null,\"location\":null,\"rotation\":null,\"scale\":null,\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":1000001.0,\"linearVelocity\":null,\"angularVelocity\":null,\"overlapCount\":null,\"overlappingActorIds\":[],\"interactionDisplacement\":null,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.component_observe\",\"case\":\"numeric-below-minimum@linearVelocity.0\",\"result\":{\"sessionId\":\"x\",\"actorId\":\"x\",\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"resolved\":false,\"reason\":null,\"componentName\":null,\"componentClass\":null,\"actorLocation\":null,\"location\":null,\"rotation\":null,\"scale\":null,\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"linearVelocity\":[-1000000001.0],\"angularVelocity\":null,\"overlapCount\":null,\"overlappingActorIds\":[],\"interactionDisplacement\":null,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.component_observe\",\"case\":\"numeric-above-maximum@linearVelocity.0\",\"result\":{\"sessionId\":\"x\",\"actorId\":\"x\",\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"resolved\":false,\"reason\":null,\"componentName\":null,\"componentClass\":null,\"actorLocation\":null,\"location\":null,\"rotation\":null,\"scale\":null,\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"linearVelocity\":[1000000001.0],\"angularVelocity\":null,\"overlapCount\":null,\"overlappingActorIds\":[],\"interactionDisplacement\":null,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.component_observe\",\"case\":\"numeric-below-minimum@angularVelocity.0\",\"result\":{\"sessionId\":\"x\",\"actorId\":\"x\",\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"resolved\":false,\"reason\":null,\"componentName\":null,\"componentClass\":null,\"actorLocation\":null,\"location\":null,\"rotation\":null,\"scale\":null,\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"linearVelocity\":null,\"angularVelocity\":[-1000000001.0],\"overlapCount\":null,\"overlappingActorIds\":[],\"interactionDisplacement\":null,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.component_observe\",\"case\":\"numeric-above-maximum@angularVelocity.0\",\"result\":{\"sessionId\":\"x\",\"actorId\":\"x\",\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"resolved\":false,\"reason\":null,\"componentName\":null,\"componentClass\":null,\"actorLocation\":null,\"location\":null,\"rotation\":null,\"scale\":null,\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"linearVelocity\":null,\"angularVelocity\":[1000000001.0],\"overlapCount\":null,\"overlappingActorIds\":[],\"interactionDisplacement\":null,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.component_observe\",\"case\":\"numeric-below-minimum@overlapCount\",\"result\":{\"sessionId\":\"x\",\"actorId\":\"x\",\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"resolved\":false,\"reason\":null,\"componentName\":null,\"componentClass\":null,\"actorLocation\":null,\"location\":null,\"rotation\":null,\"scale\":null,\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"linearVelocity\":null,\"angularVelocity\":null,\"overlapCount\":-1,\"overlappingActorIds\":[],\"interactionDisplacement\":null,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.component_observe\",\"case\":\"numeric-above-maximum@overlapCount\",\"result\":{\"sessionId\":\"x\",\"actorId\":\"x\",\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"resolved\":false,\"reason\":null,\"componentName\":null,\"componentClass\":null,\"actorLocation\":null,\"location\":null,\"rotation\":null,\"scale\":null,\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"linearVelocity\":null,\"angularVelocity\":null,\"overlapCount\":1001,\"overlappingActorIds\":[],\"interactionDisplacement\":null,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.component_observe\",\"case\":\"numeric-below-minimum@interactionDisplacement.0\",\"result\":{\"sessionId\":\"x\",\"actorId\":\"x\",\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"resolved\":false,\"reason\":null,\"componentName\":null,\"componentClass\":null,\"actorLocation\":null,\"location\":null,\"rotation\":null,\"scale\":null,\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"linearVelocity\":null,\"angularVelocity\":null,\"overlapCount\":null,\"overlappingActorIds\":[],\"interactionDisplacement\":[-1000000001.0],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.component_observe\",\"case\":\"numeric-above-maximum@interactionDisplacement.0\",\"result\":{\"sessionId\":\"x\",\"actorId\":\"x\",\"variableGuid\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"resolved\":false,\"reason\":null,\"componentName\":null,\"componentClass\":null,\"actorLocation\":null,\"location\":null,\"rotation\":null,\"scale\":null,\"collisionEnabled\":null,\"collisionProfile\":null,\"objectType\":null,\"generateOverlapEvents\":null,\"simulatePhysics\":null,\"gravityEnabled\":null,\"massOverride\":null,\"linearVelocity\":null,\"angularVelocity\":null,\"overlapCount\":null,\"overlappingActorIds\":[],\"interactionDisplacement\":[1000000001.0],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"blueprint.compile\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"blueprint.compile\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"parentClass\":\"x\",\"generatedClass\":\"x\",\"status\":\"unknown\",\"graphCount\":0,\"errorCount\":0,\"warningCount\":0,\"diagnostics\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"unknown\":true}},{\"operation\":\"blueprint.compile\",\"case\":\"numeric-below-minimum@graphCount\",\"result\":{\"id\":\"x\",\"parentClass\":\"x\",\"generatedClass\":\"x\",\"status\":\"unknown\",\"graphCount\":-1,\"errorCount\":0,\"warningCount\":0,\"diagnostics\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[]}},{\"operation\":\"blueprint.compile\",\"case\":\"numeric-above-maximum@graphCount\",\"result\":{\"id\":\"x\",\"parentClass\":\"x\",\"generatedClass\":\"x\",\"status\":\"unknown\",\"graphCount\":10001,\"errorCount\":0,\"warningCount\":0,\"diagnostics\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[]}},{\"operation\":\"blueprint.compile\",\"case\":\"numeric-below-minimum@errorCount\",\"result\":{\"id\":\"x\",\"parentClass\":\"x\",\"generatedClass\":\"x\",\"status\":\"unknown\",\"graphCount\":0,\"errorCount\":-1,\"warningCount\":0,\"diagnostics\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[]}},{\"operation\":\"blueprint.compile\",\"case\":\"numeric-above-maximum@errorCount\",\"result\":{\"id\":\"x\",\"parentClass\":\"x\",\"generatedClass\":\"x\",\"status\":\"unknown\",\"graphCount\":0,\"errorCount\":101,\"warningCount\":0,\"diagnostics\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[]}},{\"operation\":\"blueprint.compile\",\"case\":\"numeric-below-minimum@warningCount\",\"result\":{\"id\":\"x\",\"parentClass\":\"x\",\"generatedClass\":\"x\",\"status\":\"unknown\",\"graphCount\":0,\"errorCount\":0,\"warningCount\":-1,\"diagnostics\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[]}},{\"operation\":\"blueprint.compile\",\"case\":\"numeric-above-maximum@warningCount\",\"result\":{\"id\":\"x\",\"parentClass\":\"x\",\"generatedClass\":\"x\",\"status\":\"unknown\",\"graphCount\":0,\"errorCount\":0,\"warningCount\":101,\"diagnostics\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[]}},{\"operation\":\"blueprint.view\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"blueprint.view\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"parentClass\":\"x\",\"generatedClass\":\"x\",\"status\":\"unknown\",\"graphCount\":0,\"errorCount\":0,\"warningCount\":0,\"diagnostics\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"blueprint.view\",\"case\":\"numeric-below-minimum@graphCount\",\"result\":{\"id\":\"x\",\"parentClass\":\"x\",\"generatedClass\":\"x\",\"status\":\"unknown\",\"graphCount\":-1,\"errorCount\":0,\"warningCount\":0,\"diagnostics\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"blueprint.view\",\"case\":\"numeric-above-maximum@graphCount\",\"result\":{\"id\":\"x\",\"parentClass\":\"x\",\"generatedClass\":\"x\",\"status\":\"unknown\",\"graphCount\":10001,\"errorCount\":0,\"warningCount\":0,\"diagnostics\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"blueprint.view\",\"case\":\"numeric-below-minimum@errorCount\",\"result\":{\"id\":\"x\",\"parentClass\":\"x\",\"generatedClass\":\"x\",\"status\":\"unknown\",\"graphCount\":0,\"errorCount\":-1,\"warningCount\":0,\"diagnostics\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"blueprint.view\",\"case\":\"numeric-above-maximum@errorCount\",\"result\":{\"id\":\"x\",\"parentClass\":\"x\",\"generatedClass\":\"x\",\"status\":\"unknown\",\"graphCount\":0,\"errorCount\":101,\"warningCount\":0,\"diagnostics\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"blueprint.view\",\"case\":\"numeric-below-minimum@warningCount\",\"result\":{\"id\":\"x\",\"parentClass\":\"x\",\"generatedClass\":\"x\",\"status\":\"unknown\",\"graphCount\":0,\"errorCount\":0,\"warningCount\":-1,\"diagnostics\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"blueprint.view\",\"case\":\"numeric-above-maximum@warningCount\",\"result\":{\"id\":\"x\",\"parentClass\":\"x\",\"generatedClass\":\"x\",\"status\":\"unknown\",\"graphCount\":0,\"errorCount\":0,\"warningCount\":101,\"diagnostics\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"blueprint.create\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"blueprint.create\",\"case\":\"root-not-closed\",\"result\":{\"blueprintId\":\"x\",\"parentClass\":\"x\",\"generatedClass\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"blueprint.graph_view\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"blueprint.graph_view\",\"case\":\"root-not-closed\",\"result\":{\"blueprintId\":\"x\",\"count\":0,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null,\"unknown\":true}},{\"operation\":\"blueprint.graph_view\",\"case\":\"numeric-below-minimum@count\",\"result\":{\"blueprintId\":\"x\",\"count\":-1,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"blueprint.graph_view\",\"case\":\"numeric-above-maximum@count\",\"result\":{\"blueprintId\":\"x\",\"count\":101,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"blueprint.graph_view\",\"case\":\"numeric-below-minimum@total\",\"result\":{\"blueprintId\":\"x\",\"count\":0,\"total\":-1,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"blueprint.graph_view\",\"case\":\"numeric-above-maximum@total\",\"result\":{\"blueprintId\":\"x\",\"count\":0,\"total\":10001,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"blueprint.graph_view\",\"case\":\"numeric-below-minimum@items.0.@variant0.nodeCount\",\"result\":{\"blueprintId\":\"x\",\"count\":0,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[{\"graphId\":\"x\",\"kind\":\"x\",\"name\":\"x\",\"nodeCount\":-1}],\"nextCursor\":null}},{\"operation\":\"blueprint.graph_view\",\"case\":\"numeric-above-maximum@items.0.@variant0.nodeCount\",\"result\":{\"blueprintId\":\"x\",\"count\":0,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[{\"graphId\":\"x\",\"kind\":\"x\",\"name\":\"x\",\"nodeCount\":10001}],\"nextCursor\":null}},{\"operation\":\"blueprint.graph_view\",\"case\":\"numeric-below-minimum@items.0.@variant1.x\",\"result\":{\"blueprintId\":\"x\",\"count\":0,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[{\"nodeId\":\"x\",\"class\":\"x\",\"title\":\"x\",\"x\":-1000001,\"y\":-1000000,\"pins\":[]}],\"nextCursor\":null}},{\"operation\":\"blueprint.graph_view\",\"case\":\"numeric-above-maximum@items.0.@variant1.x\",\"result\":{\"blueprintId\":\"x\",\"count\":0,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[{\"nodeId\":\"x\",\"class\":\"x\",\"title\":\"x\",\"x\":1000001,\"y\":-1000000,\"pins\":[]}],\"nextCursor\":null}},{\"operation\":\"blueprint.graph_view\",\"case\":\"numeric-below-minimum@items.0.@variant1.y\",\"result\":{\"blueprintId\":\"x\",\"count\":0,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[{\"nodeId\":\"x\",\"class\":\"x\",\"title\":\"x\",\"x\":-1000000,\"y\":-1000001,\"pins\":[]}],\"nextCursor\":null}},{\"operation\":\"blueprint.graph_view\",\"case\":\"numeric-above-maximum@items.0.@variant1.y\",\"result\":{\"blueprintId\":\"x\",\"count\":0,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[{\"nodeId\":\"x\",\"class\":\"x\",\"title\":\"x\",\"x\":-1000000,\"y\":1000001,\"pins\":[]}],\"nextCursor\":null}},{\"operation\":\"blueprint.event_ensure\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"blueprint.event_ensure\",\"case\":\"root-not-closed\",\"result\":{\"blueprintId\":\"x\",\"graphId\":\"x\",\"nodeId\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"blueprint.node_ensure\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"blueprint.node_ensure\",\"case\":\"root-not-closed\",\"result\":{\"blueprintId\":\"x\",\"graphId\":\"x\",\"nodeId\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"blueprint.pin_default_set\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"blueprint.pin_default_set\",\"case\":\"root-not-closed\",\"result\":{\"blueprintId\":\"x\",\"pinId\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"blueprint.pin_connect\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"blueprint.pin_connect\",\"case\":\"root-not-closed\",\"result\":{\"blueprintId\":\"x\",\"sourcePinId\":\"x\",\"targetPinId\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"component.add\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"component.add\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"component.list\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"component.list\",\"case\":\"root-not-closed\",\"result\":{\"count\":0,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null,\"unknown\":true}},{\"operation\":\"component.list\",\"case\":\"numeric-below-minimum@count\",\"result\":{\"count\":-1,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"component.list\",\"case\":\"numeric-above-maximum@count\",\"result\":{\"count\":101,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"component.list\",\"case\":\"numeric-below-minimum@total\",\"result\":{\"count\":0,\"total\":-1,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"component.list\",\"case\":\"numeric-above-maximum@total\",\"result\":{\"count\":0,\"total\":9007199254740992,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"component.remove\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"component.remove\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"changed\":false,\"dryRun\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"component.update\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"component.update\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"component.view\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"component.view\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"actorId\":\"x\",\"name\":\"x\",\"class\":\"x\",\"scene\":false,\"location\":[-1000000000.0,-1000000000.0,-1000000000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"component.view\",\"case\":\"numeric-below-minimum@location.0\",\"result\":{\"id\":\"x\",\"actorId\":\"x\",\"name\":\"x\",\"class\":\"x\",\"scene\":false,\"location\":[-1000000001.0,-1000000000.0,-1000000000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"component.view\",\"case\":\"numeric-above-maximum@location.0\",\"result\":{\"id\":\"x\",\"actorId\":\"x\",\"name\":\"x\",\"class\":\"x\",\"scene\":false,\"location\":[1000000001.0,-1000000000.0,-1000000000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"component.view\",\"case\":\"numeric-below-minimum@rotation.0\",\"result\":{\"id\":\"x\",\"actorId\":\"x\",\"name\":\"x\",\"class\":\"x\",\"scene\":false,\"location\":[-1000000000.0,-1000000000.0,-1000000000.0],\"rotation\":[-361.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"component.view\",\"case\":\"numeric-above-maximum@rotation.0\",\"result\":{\"id\":\"x\",\"actorId\":\"x\",\"name\":\"x\",\"class\":\"x\",\"scene\":false,\"location\":[-1000000000.0,-1000000000.0,-1000000000.0],\"rotation\":[361.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"component.view\",\"case\":\"numeric-below-minimum@scale.0\",\"result\":{\"id\":\"x\",\"actorId\":\"x\",\"name\":\"x\",\"class\":\"x\",\"scene\":false,\"location\":[-1000000000.0,-1000000000.0,-1000000000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[-1.0,0.0,0.0],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"component.view\",\"case\":\"numeric-above-maximum@scale.0\",\"result\":{\"id\":\"x\",\"actorId\":\"x\",\"name\":\"x\",\"class\":\"x\",\"scene\":false,\"location\":[-1000000000.0,-1000000000.0,-1000000000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[1000000001.0,0.0,0.0],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"level.set_game_mode\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"level.set_game_mode\",\"case\":\"root-not-closed\",\"result\":{\"levelId\":\"x\",\"gameModeClass\":\"x\",\"defaultPawnClass\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"level.settings\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"level.settings\",\"case\":\"root-not-closed\",\"result\":{\"levelId\":\"x\",\"gameModeClass\":\"\",\"defaultPawnClass\":\"\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"play.input\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"play.input\",\"case\":\"root-not-closed\",\"result\":{\"sessionId\":\"x\",\"key\":\"x\",\"event\":\"pressed\",\"accepted\":false,\"changed\":false,\"beforeRevision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"afterRevision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"play.observe\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"play.observe\",\"case\":\"root-not-closed\",\"result\":{\"sessionId\":\"x\",\"worldId\":null,\"levelId\":null,\"actors\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"play.observe\",\"case\":\"numeric-below-minimum@actors.0.location.0\",\"result\":{\"sessionId\":\"x\",\"worldId\":null,\"levelId\":null,\"actors\":[{\"id\":\"x\",\"name\":\"x\",\"class\":\"x\",\"location\":[-1000000001.0,-1000000000.0,-1000000000.0],\"tags\":[]}],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.observe\",\"case\":\"numeric-above-maximum@actors.0.location.0\",\"result\":{\"sessionId\":\"x\",\"worldId\":null,\"levelId\":null,\"actors\":[{\"id\":\"x\",\"name\":\"x\",\"class\":\"x\",\"location\":[1000000001.0,-1000000000.0,-1000000000.0],\"tags\":[]}],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.screenshot\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"play.screenshot\",\"case\":\"root-not-closed\",\"result\":{\"sessionId\":\"x\",\"path\":\"x\",\"width\":1,\"height\":1,\"format\":\"png\",\"changed\":false,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"play.screenshot\",\"case\":\"numeric-below-minimum@width\",\"result\":{\"sessionId\":\"x\",\"path\":\"x\",\"width\":0,\"height\":1,\"format\":\"png\",\"changed\":false,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.screenshot\",\"case\":\"numeric-above-maximum@width\",\"result\":{\"sessionId\":\"x\",\"path\":\"x\",\"width\":16385,\"height\":1,\"format\":\"png\",\"changed\":false,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.screenshot\",\"case\":\"numeric-below-minimum@height\",\"result\":{\"sessionId\":\"x\",\"path\":\"x\",\"width\":1,\"height\":0,\"format\":\"png\",\"changed\":false,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.screenshot\",\"case\":\"numeric-above-maximum@height\",\"result\":{\"sessionId\":\"x\",\"path\":\"x\",\"width\":1,\"height\":16385,\"format\":\"png\",\"changed\":false,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.start\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"play.start\",\"case\":\"root-not-closed\",\"result\":{\"sessionId\":\"x\",\"state\":\"starting\",\"worldId\":null,\"levelId\":null,\"changed\":false,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"play.status\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"play.status\",\"case\":\"root-not-closed\",\"result\":{\"state\":\"stopped\",\"sessionId\":null,\"worldId\":null,\"levelId\":null,\"playerCount\":0,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"play.status\",\"case\":\"numeric-below-minimum@playerCount\",\"result\":{\"state\":\"stopped\",\"sessionId\":null,\"worldId\":null,\"levelId\":null,\"playerCount\":-1,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.status\",\"case\":\"numeric-above-maximum@playerCount\",\"result\":{\"state\":\"stopped\",\"sessionId\":null,\"worldId\":null,\"levelId\":null,\"playerCount\":101,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"play.stop\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"play.stop\",\"case\":\"root-not-closed\",\"result\":{\"sessionId\":\"x\",\"state\":\"stopping\",\"changed\":false,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"asset.list\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"asset.list\",\"case\":\"root-not-closed\",\"result\":{\"count\":0,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null,\"unknown\":true}},{\"operation\":\"asset.list\",\"case\":\"numeric-below-minimum@count\",\"result\":{\"count\":-1,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"asset.list\",\"case\":\"numeric-above-maximum@count\",\"result\":{\"count\":101,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"asset.list\",\"case\":\"numeric-below-minimum@total\",\"result\":{\"count\":0,\"total\":-1,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"asset.list\",\"case\":\"numeric-above-maximum@total\",\"result\":{\"count\":0,\"total\":9007199254740992,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"asset.view\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"asset.view\",\"case\":\"root-not-closed\",\"result\":{\"id\":\"x\",\"packagePath\":\"x\",\"objectPath\":\"x\",\"name\":\"x\",\"class\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"capability.describe\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"capability.describe\",\"case\":\"root-not-closed\",\"result\":{\"capability\":{\"id\":\"x\",\"version\":1,\"domain\":\"x\",\"summary\":\"x\",\"execution\":\"local\",\"mutates\":false,\"destructive\":false,\"idempotency\":\"x\",\"saveBehavior\":\"x\",\"transactionBehavior\":\"x\",\"reversibility\":\"x\",\"allowedEditorStates\":[],\"requiresModules\":[],\"inputSchema\":\"xx\",\"outputSchema\":\"xx\",\"verification\":\"xx\",\"engineSupport\":\"xx\"},\"runtime\":{\"availability\":\"available\",\"reasons\":[],\"catalogHash\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"},\"unknown\":true}},{\"operation\":\"capability.describe\",\"case\":\"numeric-below-minimum@capability.version\",\"result\":{\"capability\":{\"id\":\"x\",\"version\":0,\"domain\":\"x\",\"summary\":\"x\",\"execution\":\"local\",\"mutates\":false,\"destructive\":false,\"idempotency\":\"x\",\"saveBehavior\":\"x\",\"transactionBehavior\":\"x\",\"reversibility\":\"x\",\"allowedEditorStates\":[],\"requiresModules\":[],\"inputSchema\":\"xx\",\"outputSchema\":\"xx\",\"verification\":\"xx\",\"engineSupport\":\"xx\"},\"runtime\":{\"availability\":\"available\",\"reasons\":[],\"catalogHash\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}}},{\"operation\":\"capability.describe\",\"case\":\"numeric-above-maximum@capability.version\",\"result\":{\"capability\":{\"id\":\"x\",\"version\":2,\"domain\":\"x\",\"summary\":\"x\",\"execution\":\"local\",\"mutates\":false,\"destructive\":false,\"idempotency\":\"x\",\"saveBehavior\":\"x\",\"transactionBehavior\":\"x\",\"reversibility\":\"x\",\"allowedEditorStates\":[],\"requiresModules\":[],\"inputSchema\":\"xx\",\"outputSchema\":\"xx\",\"verification\":\"xx\",\"engineSupport\":\"xx\"},\"runtime\":{\"availability\":\"available\",\"reasons\":[],\"catalogHash\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}}},{\"operation\":\"capability.search\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"capability.search\",\"case\":\"root-not-closed\",\"result\":{\"count\":0,\"total\":0,\"scope\":\"x\",\"items\":[],\"nextCursor\":null,\"unknown\":true}},{\"operation\":\"capability.search\",\"case\":\"numeric-below-minimum@count\",\"result\":{\"count\":-1,\"total\":0,\"scope\":\"x\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"capability.search\",\"case\":\"numeric-above-maximum@count\",\"result\":{\"count\":51,\"total\":0,\"scope\":\"x\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"capability.search\",\"case\":\"numeric-below-minimum@total\",\"result\":{\"count\":0,\"total\":-1,\"scope\":\"x\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"capability.search\",\"case\":\"numeric-above-maximum@total\",\"result\":{\"count\":0,\"total\":9007199254740992,\"scope\":\"x\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"editor.status\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"editor.status\",\"case\":\"root-not-closed\",\"result\":{\"state\":\"starting\",\"projectId\":\"x\",\"editorPid\":0,\"levelId\":\"\",\"pie\":\"running\",\"dirtyPackageCount\":0,\"unknown\":true}},{\"operation\":\"editor.status\",\"case\":\"numeric-below-minimum@editorPid\",\"result\":{\"state\":\"starting\",\"projectId\":\"x\",\"editorPid\":-1,\"levelId\":\"\",\"pie\":\"running\",\"dirtyPackageCount\":0}},{\"operation\":\"editor.status\",\"case\":\"numeric-above-maximum@editorPid\",\"result\":{\"state\":\"starting\",\"projectId\":\"x\",\"editorPid\":4294967296,\"levelId\":\"\",\"pie\":\"running\",\"dirtyPackageCount\":0}},{\"operation\":\"editor.status\",\"case\":\"numeric-below-minimum@dirtyPackageCount\",\"result\":{\"state\":\"starting\",\"projectId\":\"x\",\"editorPid\":0,\"levelId\":\"\",\"pie\":\"running\",\"dirtyPackageCount\":-1}},{\"operation\":\"editor.status\",\"case\":\"numeric-above-maximum@dirtyPackageCount\",\"result\":{\"state\":\"starting\",\"projectId\":\"x\",\"editorPid\":0,\"levelId\":\"\",\"pie\":\"running\",\"dirtyPackageCount\":9007199254740992}},{\"operation\":\"level.create\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"level.create\",\"case\":\"root-not-closed\",\"result\":{\"level\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"level.current\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"level.current\",\"case\":\"root-not-closed\",\"result\":{\"level\":{\"id\":\"x\",\"name\":\"x\",\"worldType\":\"x\",\"persistent\":false,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"},\"scope\":\"x\",\"unknown\":true}},{\"operation\":\"level.list\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"level.list\",\"case\":\"root-not-closed\",\"result\":{\"count\":0,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null,\"unknown\":true}},{\"operation\":\"level.list\",\"case\":\"numeric-below-minimum@count\",\"result\":{\"count\":-1,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"level.list\",\"case\":\"numeric-above-maximum@count\",\"result\":{\"count\":101,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"level.list\",\"case\":\"numeric-below-minimum@total\",\"result\":{\"count\":0,\"total\":-1,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"level.list\",\"case\":\"numeric-above-maximum@total\",\"result\":{\"count\":0,\"total\":9007199254740992,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[],\"nextCursor\":null}},{\"operation\":\"level.open\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"level.open\",\"case\":\"root-not-closed\",\"result\":{\"level\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"level.save\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"level.save\",\"case\":\"root-not-closed\",\"result\":{\"level\":\"x\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"operation.view\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"operation.view\",\"case\":\"root-not-closed\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false},\"unknown\":true}},{\"operation\":\"operation.view\",\"case\":\"numeric-below-minimum@editorPid\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":0,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false}}},{\"operation\":\"operation.view\",\"case\":\"numeric-above-maximum@editorPid\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":4294967296,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false}}},{\"operation\":\"operation.view\",\"case\":\"numeric-below-minimum@verification.errorCount\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false,\"errorCount\":0}}},{\"operation\":\"operation.view\",\"case\":\"numeric-above-maximum@verification.errorCount\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false,\"errorCount\":101}}},{\"operation\":\"operation.view\",\"case\":\"numeric-below-minimum@verification.warningCount\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false,\"warningCount\":-1}}},{\"operation\":\"operation.view\",\"case\":\"numeric-above-maximum@verification.warningCount\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false,\"warningCount\":101}}},{\"operation\":\"operation.view\",\"case\":\"numeric-below-minimum@verification.requestValue\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false,\"requestValue\":-1000001.0}}},{\"operation\":\"operation.view\",\"case\":\"numeric-above-maximum@verification.requestValue\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false,\"requestValue\":1000001.0}}},{\"operation\":\"operation.view\",\"case\":\"numeric-below-minimum@verification.requestLocation.0\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false,\"requestLocation\":[-100001.0,-100000.0,-100000.0]}}},{\"operation\":\"operation.view\",\"case\":\"numeric-above-maximum@verification.requestLocation.0\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false,\"requestLocation\":[100001.0,-100000.0,-100000.0]}}},{\"operation\":\"operation.view\",\"case\":\"numeric-below-minimum@verification.requestRotation.0\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false,\"requestRotation\":[-361.0,-360.0,-360.0]}}},{\"operation\":\"operation.view\",\"case\":\"numeric-above-maximum@verification.requestRotation.0\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false,\"requestRotation\":[361.0,-360.0,-360.0]}}},{\"operation\":\"operation.view\",\"case\":\"numeric-below-minimum@verification.requestScale.0\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false,\"requestScale\":[-1.0,0.0,0.0]}}},{\"operation\":\"operation.view\",\"case\":\"numeric-above-maximum@verification.requestScale.0\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false,\"requestScale\":[1001.0,0.0,0.0]}}},{\"operation\":\"operation.view\",\"case\":\"numeric-below-minimum@verification.requestMassOverride\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false,\"requestMassOverride\":-0.999}}},{\"operation\":\"operation.view\",\"case\":\"numeric-above-maximum@verification.requestMassOverride\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false,\"requestMassOverride\":100001.0}}},{\"operation\":\"operation.view\",\"case\":\"numeric-below-minimum@verification.requestBoxExtent.0\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false,\"requestBoxExtent\":[-0.999,0.001,0.001]}}},{\"operation\":\"operation.view\",\"case\":\"numeric-above-maximum@verification.requestBoxExtent.0\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false,\"requestBoxExtent\":[100001.0,0.001,0.001]}}},{\"operation\":\"operation.view\",\"case\":\"numeric-below-minimum@verification.requestSphereRadius\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false,\"requestSphereRadius\":-0.999}}},{\"operation\":\"operation.view\",\"case\":\"numeric-above-maximum@verification.requestSphereRadius\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false,\"requestSphereRadius\":100001.0}}},{\"operation\":\"operation.view\",\"case\":\"numeric-below-minimum@verification.requestZOrder\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false,\"requestZOrder\":-1}}},{\"operation\":\"operation.view\",\"case\":\"numeric-above-maximum@verification.requestZOrder\",\"result\":{\"operationId\":\"x\",\"operation\":\"x\",\"state\":\"queued\",\"projectId\":\"x\",\"editorPid\":1,\"target\":\"x\",\"changed\":false,\"transaction\":\"x\",\"reversibility\":\"x\",\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"persistence\":\"x\",\"verification\":{\"readback\":\"x\",\"target\":\"x\",\"matched\":false,\"requestZOrder\":1}}},{\"operation\":\"play.ui_observe\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"play.ui_observe\",\"case\":\"root-not-closed\",\"result\":{\"sessionId\":\"x\",\"widgetBlueprintId\":\"x\",\"instanceId\":\"x\",\"inViewport\":false,\"widgets\":[{\"widgetId\":\"x\",\"name\":\"x\",\"class\":\"VerticalBox\",\"text\":null,\"visibility\":\"Visible\",\"enabled\":false}],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"widget.create\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"widget.create\",\"case\":\"root-not-closed\",\"result\":{\"blueprintId\":\"x\",\"generatedClass\":\"x\",\"rootWidgetId\":\"x\",\"rootName\":\"x\",\"rootClass\":\"VerticalBox\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"widget.tree_view\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"widget.tree_view\",\"case\":\"root-not-closed\",\"result\":{\"blueprintId\":\"x\",\"generatedClass\":\"x\",\"rootWidgetId\":\"x\",\"count\":1,\"total\":1,\"scope\":\"x\",\"widgets\":[{\"widgetId\":\"x\",\"name\":\"x\",\"class\":\"VerticalBox\",\"parentWidgetId\":null,\"index\":0,\"text\":null,\"visibility\":\"Visible\",\"enabled\":false}],\"events\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"widget.tree_view\",\"case\":\"numeric-below-minimum@count\",\"result\":{\"blueprintId\":\"x\",\"generatedClass\":\"x\",\"rootWidgetId\":\"x\",\"count\":0,\"total\":1,\"scope\":\"x\",\"widgets\":[{\"widgetId\":\"x\",\"name\":\"x\",\"class\":\"VerticalBox\",\"parentWidgetId\":null,\"index\":0,\"text\":null,\"visibility\":\"Visible\",\"enabled\":false}],\"events\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"widget.tree_view\",\"case\":\"numeric-above-maximum@count\",\"result\":{\"blueprintId\":\"x\",\"generatedClass\":\"x\",\"rootWidgetId\":\"x\",\"count\":101,\"total\":1,\"scope\":\"x\",\"widgets\":[{\"widgetId\":\"x\",\"name\":\"x\",\"class\":\"VerticalBox\",\"parentWidgetId\":null,\"index\":0,\"text\":null,\"visibility\":\"Visible\",\"enabled\":false}],\"events\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"widget.tree_view\",\"case\":\"numeric-below-minimum@total\",\"result\":{\"blueprintId\":\"x\",\"generatedClass\":\"x\",\"rootWidgetId\":\"x\",\"count\":1,\"total\":0,\"scope\":\"x\",\"widgets\":[{\"widgetId\":\"x\",\"name\":\"x\",\"class\":\"VerticalBox\",\"parentWidgetId\":null,\"index\":0,\"text\":null,\"visibility\":\"Visible\",\"enabled\":false}],\"events\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"widget.tree_view\",\"case\":\"numeric-above-maximum@total\",\"result\":{\"blueprintId\":\"x\",\"generatedClass\":\"x\",\"rootWidgetId\":\"x\",\"count\":1,\"total\":101,\"scope\":\"x\",\"widgets\":[{\"widgetId\":\"x\",\"name\":\"x\",\"class\":\"VerticalBox\",\"parentWidgetId\":null,\"index\":0,\"text\":null,\"visibility\":\"Visible\",\"enabled\":false}],\"events\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"widget.tree_view\",\"case\":\"numeric-below-minimum@widgets.0.index\",\"result\":{\"blueprintId\":\"x\",\"generatedClass\":\"x\",\"rootWidgetId\":\"x\",\"count\":1,\"total\":1,\"scope\":\"x\",\"widgets\":[{\"widgetId\":\"x\",\"name\":\"x\",\"class\":\"VerticalBox\",\"parentWidgetId\":null,\"index\":-1,\"text\":null,\"visibility\":\"Visible\",\"enabled\":false}],\"events\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"widget.tree_view\",\"case\":\"numeric-above-maximum@widgets.0.index\",\"result\":{\"blueprintId\":\"x\",\"generatedClass\":\"x\",\"rootWidgetId\":\"x\",\"count\":1,\"total\":1,\"scope\":\"x\",\"widgets\":[{\"widgetId\":\"x\",\"name\":\"x\",\"class\":\"VerticalBox\",\"parentWidgetId\":null,\"index\":100,\"text\":null,\"visibility\":\"Visible\",\"enabled\":false}],\"events\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"widget.child_ensure\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"widget.child_ensure\",\"case\":\"root-not-closed\",\"result\":{\"blueprintId\":\"x\",\"widgetId\":\"x\",\"parentWidgetId\":\"x\",\"name\":\"x\",\"class\":\"TextBlock\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"widget.property_set\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"widget.property_set\",\"case\":\"root-not-closed\",\"result\":{\"blueprintId\":\"x\",\"widgetId\":\"x\",\"property\":\"text\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"widget.event_ensure\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"widget.event_ensure\",\"case\":\"root-not-closed\",\"result\":{\"blueprintId\":\"x\",\"eventId\":\"x\",\"agentKey\":\"x\",\"event\":\"activate\",\"actions\":[{\"kind\":\"text.set\",\"targetWidgetId\":\"x\"}],\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"widget.viewport_ensure\",\"case\":\"missing-required\",\"result\":{}},{\"operation\":\"widget.viewport_ensure\",\"case\":\"root-not-closed\",\"result\":{\"hostBlueprintId\":\"x\",\"widgetBlueprintId\":\"x\",\"viewportId\":\"x\",\"graphId\":\"x\",\"inputKey\":\"E\",\"zOrder\":0,\"widgetRevision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true}},{\"operation\":\"widget.viewport_ensure\",\"case\":\"numeric-below-minimum@zOrder\",\"result\":{\"hostBlueprintId\":\"x\",\"widgetBlueprintId\":\"x\",\"viewportId\":\"x\",\"graphId\":\"x\",\"inputKey\":\"E\",\"zOrder\":-1,\"widgetRevision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"widget.viewport_ensure\",\"case\":\"numeric-above-maximum@zOrder\",\"result\":{\"hostBlueprintId\":\"x\",\"widgetBlueprintId\":\"x\",\"viewportId\":\"x\",\"graphId\":\"x\",\"inputKey\":\"E\",\"zOrder\":1,\"widgetRevision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"changed\":false,\"dirtyPackages\":[],\"savedPackages\":[],\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}},{\"operation\":\"actor.list\",\"case\":\"missing-list-id\",\"result\":{\"count\":1,\"total\":1,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[{\"label\":\"actor\"}],\"nextCursor\":null}},{\"operation\":\"level.current\",\"case\":\"nested-not-closed\",\"result\":{\"level\":{\"id\":\"x\",\"name\":\"x\",\"worldType\":\"x\",\"persistent\":false,\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"unknown\":true},\"scope\":\"x\"}},{\"operation\":\"editor.status\",\"case\":\"wrong-type\",\"result\":{\"state\":\"starting\",\"projectId\":\"x\",\"editorPid\":\"not-an-integer\",\"levelId\":\"\",\"pie\":\"running\",\"dirtyPackageCount\":0}},{\"operation\":\"editor.status\",\"case\":\"enum\",\"result\":{\"state\":\"invalid\",\"projectId\":\"x\",\"editorPid\":0,\"levelId\":\"\",\"pie\":\"running\",\"dirtyPackageCount\":0}},{\"operation\":\"actor.view\",\"case\":\"string-bound\",\"result\":{\"id\":\"x\",\"actorGuid\":\"x\",\"levelId\":\"x\",\"label\":\"x\",\"class\":\"x\",\"objectPath\":\"x\",\"location\":[-1000000000.0,-1000000000.0,-1000000000.0],\"rotation\":[-360.0,-360.0,-360.0],\"scale\":[0.0,0.0,0.0],\"revision\":\"short\"}},{\"operation\":\"blueprint.graph_view\",\"case\":\"partial-graph-row\",\"result\":{\"blueprintId\":\"x\",\"count\":0,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[{\"graphId\":\"/Game/BP.BP:graph\",\"kind\":\"ubergraph\",\"name\":\"EventGraph\"}],\"nextCursor\":null}},{\"operation\":\"blueprint.graph_view\",\"case\":\"hybrid-row\",\"result\":{\"blueprintId\":\"x\",\"count\":0,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[{\"graphId\":\"/Game/BP.BP:graph\",\"kind\":\"ubergraph\",\"name\":\"EventGraph\",\"nodeCount\":1,\"nodeId\":\"node\"}],\"nextCursor\":null}},{\"operation\":\"blueprint.graph_view\",\"case\":\"empty-row\",\"result\":{\"blueprintId\":\"x\",\"count\":0,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[{}],\"nextCursor\":null}},{\"operation\":\"blueprint.graph_view\",\"case\":\"primitive-row\",\"result\":{\"blueprintId\":\"x\",\"count\":1,\"total\":1,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[1],\"nextCursor\":null}},{\"operation\":\"blueprint.graph_view\",\"case\":\"null-row\",\"result\":{\"blueprintId\":\"x\",\"count\":1,\"total\":1,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[null],\"nextCursor\":null}},{\"operation\":\"blueprint.graph_view\",\"case\":\"array-row\",\"result\":{\"blueprintId\":\"x\",\"count\":1,\"total\":1,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[[]],\"nextCursor\":null}},{\"operation\":\"asset.list\",\"case\":\"array-bound\",\"result\":{\"count\":0,\"total\":0,\"scope\":\"x\",\"revision\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\",\"items\":[{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"},{\"id\":\"x\"}],\"nextCursor\":null}}]")

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
            if (Number2 < 1.0) return false;
            if (Number2 > 100.0) return false;
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
                if (Number12 < -1000000000.0) return false;
                if (Number12 > 1000000000.0) return false;
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
                if (Number16 < -360.0) return false;
                if (Number16 > 360.0) return false;
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
                if (Number20 < 0.0) return false;
                if (Number20 > 1000000.0) return false;
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
                if (Number6 < -1000000000.0) return false;
                if (Number6 > 1000000000.0) return false;
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
                if (Number10 < -360.0) return false;
                if (Number10 > 360.0) return false;
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
                if (Number14 < 0.0) return false;
                if (Number14 > 1000000.0) return false;
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
    if (Operation == TEXT("blueprint.interface_create"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("path") && Entry.Key != TEXT("function")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("path"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("function"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 128) return false;
        if (Text4 != TEXT("Interact")) return false;
        return true;
    }
    if (Operation == TEXT("blueprint.interface_view"))
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
    if (Operation == TEXT("blueprint.interface_ensure"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("interfaceId")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("interfaceId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        return true;
    }
    if (Operation == TEXT("blueprint.scs_view"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        return true;
    }
    if (Operation == TEXT("blueprint.scs_component_ensure"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("name") && Entry.Key != TEXT("class") && Entry.Key != TEXT("parent")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("name"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 128) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("class"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 128) return false;
        if (Text6 != TEXT("SceneComponent") && Text6 != TEXT("PointLightComponent") && Text6 != TEXT("StaticMeshComponent") && Text6 != TEXT("BoxComponent") && Text6 != TEXT("SphereComponent")) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("parent"));
        if (Value7.IsValid())
        {
            if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
            const FString Text8 = Value7->AsString();
            if (MagiAxiUnicodeScalarCount(Text8) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text8) > 128) return false;
        }
        return true;
    }
    if (Operation == TEXT("blueprint.scs_component_update"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("variableGuid") && Entry.Key != TEXT("location") && Entry.Key != TEXT("rotation") && Entry.Key != TEXT("scale") && Entry.Key != TEXT("collisionEnabled") && Entry.Key != TEXT("generateOverlapEvents") && Entry.Key != TEXT("simulatePhysics") && Entry.Key != TEXT("gravityEnabled") && Entry.Key != TEXT("massOverride") && Entry.Key != TEXT("boxExtent") && Entry.Key != TEXT("sphereRadius") && Entry.Key != TEXT("collisionProfile")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("variableGuid"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 36) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 36) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("location"));
        if (Value5.IsValid())
        {
            if (!Value5.IsValid() || Value5->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array6 = Value5->AsArray();
            if (Array6.Num() < 3) return false;
            if (Array6.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value7 : Array6)
            {
                double Number8 = 0;
                if (!Value7.IsValid() || !Value7->TryGetNumber(Number8) || !FMath::IsFinite(Number8)) return false;
                if (Number8 < -100000.0) return false;
                if (Number8 > 100000.0) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("rotation"));
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
                if (Number12 < -360.0) return false;
                if (Number12 > 360.0) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value13 = Object0->TryGetField(TEXT("scale"));
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
                if (Number16 < 0.0) return false;
                if (Number16 > 1000.0) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value17 = Object0->TryGetField(TEXT("collisionEnabled"));
        if (Value17.IsValid())
        {
            if (!Value17.IsValid() || Value17->Type != EJson::String) return false;
            const FString Text18 = Value17->AsString();
            if (MagiAxiUnicodeScalarCount(Text18) > 32) return false;
            if (Text18 != TEXT("NoCollision") && Text18 != TEXT("QueryOnly") && Text18 != TEXT("PhysicsOnly") && Text18 != TEXT("QueryAndPhysics")) return false;
        }
        const TSharedPtr<FJsonValue> Value19 = Object0->TryGetField(TEXT("generateOverlapEvents"));
        if (Value19.IsValid())
        {
            if (!Value19.IsValid() || Value19->Type != EJson::Boolean) return false;
        }
        const TSharedPtr<FJsonValue> Value20 = Object0->TryGetField(TEXT("simulatePhysics"));
        if (Value20.IsValid())
        {
            if (!Value20.IsValid() || Value20->Type != EJson::Boolean) return false;
        }
        const TSharedPtr<FJsonValue> Value21 = Object0->TryGetField(TEXT("gravityEnabled"));
        if (Value21.IsValid())
        {
            if (!Value21.IsValid() || Value21->Type != EJson::Boolean) return false;
        }
        const TSharedPtr<FJsonValue> Value22 = Object0->TryGetField(TEXT("massOverride"));
        if (Value22.IsValid())
        {
            double Number23 = 0;
            if (!Value22.IsValid() || !Value22->TryGetNumber(Number23) || !FMath::IsFinite(Number23)) return false;
            if (Number23 < 0.001) return false;
            if (Number23 > 100000.0) return false;
        }
        const TSharedPtr<FJsonValue> Value24 = Object0->TryGetField(TEXT("boxExtent"));
        if (Value24.IsValid())
        {
            if (!Value24.IsValid() || Value24->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array25 = Value24->AsArray();
            if (Array25.Num() < 3) return false;
            if (Array25.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value26 : Array25)
            {
                double Number27 = 0;
                if (!Value26.IsValid() || !Value26->TryGetNumber(Number27) || !FMath::IsFinite(Number27)) return false;
                if (Number27 < 0.001) return false;
                if (Number27 > 100000.0) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value28 = Object0->TryGetField(TEXT("sphereRadius"));
        if (Value28.IsValid())
        {
            double Number29 = 0;
            if (!Value28.IsValid() || !Value28->TryGetNumber(Number29) || !FMath::IsFinite(Number29)) return false;
            if (Number29 < 0.001) return false;
            if (Number29 > 100000.0) return false;
        }
        const TSharedPtr<FJsonValue> Value30 = Object0->TryGetField(TEXT("collisionProfile"));
        if (Value30.IsValid())
        {
            if (!Value30.IsValid() || Value30->Type != EJson::String) return false;
            const FString Text31 = Value30->AsString();
            if (MagiAxiUnicodeScalarCount(Text31) > 128) return false;
            if (Text31 != TEXT("OverlapAllDynamic")) return false;
        }
        return true;
    }
    if (Operation == TEXT("blueprint.scs_component_remove"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("variableGuid") && Entry.Key != TEXT("force") && Entry.Key != TEXT("dryRun")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("variableGuid"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 128) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("force"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value6 = Object0->TryGetField(TEXT("dryRun"));
        if (!Value6.IsValid()) return false;
        if (!Value6.IsValid() || Value6->Type != EJson::Boolean) return false;
        return true;
    }
    if (Operation == TEXT("play.component_observe"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("sessionId") && Entry.Key != TEXT("actorId") && Entry.Key != TEXT("variableGuid")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("sessionId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 128) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("actorId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("variableGuid"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 36) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 36) return false;
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
    if (Operation == TEXT("blueprint.create"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("path") && Entry.Key != TEXT("parentClass")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("path"));
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
        if (MagiAxiUnicodeScalarCount(Text4) > 64) return false;
        if (Text4 != TEXT("/Script/Engine.StaticMeshActor") && Text4 != TEXT("/Script/Engine.Actor")) return false;
        return true;
    }
    if (Operation == TEXT("blueprint.graph_view"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("graphId") && Entry.Key != TEXT("limit") && Entry.Key != TEXT("cursor")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("graphId"));
        if (Value3.IsValid())
        {
            if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
            const FString Text4 = Value3->AsString();
            if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text4) > 1024) return false;
        }
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("limit"));
        if (Value5.IsValid())
        {
            double Number6 = 0;
            if (!Value5.IsValid() || !Value5->TryGetNumber(Number6) || !FMath::IsFinite(Number6) || FMath::FloorToDouble(Number6) != Number6) return false;
            if (Number6 < 1.0) return false;
            if (Number6 > 100.0) return false;
        }
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("cursor"));
        if (Value7.IsValid())
        {
            if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
            const FString Text8 = Value7->AsString();
            if (MagiAxiUnicodeScalarCount(Text8) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text8) > 256) return false;
        }
        return true;
    }
    if (Operation == TEXT("blueprint.event_ensure"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("graphId") && Entry.Key != TEXT("agentKey") && Entry.Key != TEXT("event") && Entry.Key != TEXT("variableGuid") && Entry.Key != TEXT("interfaceId")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("graphId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 1024) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("agentKey"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 128) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("event"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) > 32) return false;
        if (Text8 != TEXT("actor.begin_play") && Text8 != TEXT("input.key_e") && Text8 != TEXT("component.begin_overlap") && Text8 != TEXT("interface.interact")) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("variableGuid"));
        if (Value9.IsValid())
        {
            if (!Value9.IsValid() || Value9->Type != EJson::String) return false;
            const FString Text10 = Value9->AsString();
            if (MagiAxiUnicodeScalarCount(Text10) < 36) return false;
            if (MagiAxiUnicodeScalarCount(Text10) > 36) return false;
        }
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("interfaceId"));
        if (Value11.IsValid())
        {
            if (!Value11.IsValid() || Value11->Type != EJson::String) return false;
            const FString Text12 = Value11->AsString();
            if (MagiAxiUnicodeScalarCount(Text12) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text12) > 512) return false;
        }
        return true;
    }
    if (Operation == TEXT("blueprint.node_ensure"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("graphId") && Entry.Key != TEXT("agentKey") && Entry.Key != TEXT("node") && Entry.Key != TEXT("interfaceId")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("graphId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 1024) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("agentKey"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 128) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("node"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) > 64) return false;
        if (Text8 != TEXT("game.get_player_controller") && Text8 != TEXT("actor.enable_input") && Text8 != TEXT("math.make_vector") && Text8 != TEXT("actor.add_world_offset") && Text8 != TEXT("interface.message_interact")) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("interfaceId"));
        if (Value9.IsValid())
        {
            if (!Value9.IsValid() || Value9->Type != EJson::String) return false;
            const FString Text10 = Value9->AsString();
            if (MagiAxiUnicodeScalarCount(Text10) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text10) > 512) return false;
        }
        return true;
    }
    if (Operation == TEXT("blueprint.pin_default_set"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("pinId") && Entry.Key != TEXT("value")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("pinId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 2048) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("value"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object6 = Value5->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object6->Values) if (Entry.Key != TEXT("type") && Entry.Key != TEXT("value")) return false;
        const TSharedPtr<FJsonValue> Value7 = Object6->TryGetField(TEXT("type"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) > 8) return false;
        if (Text8 != TEXT("integer") && Text8 != TEXT("real")) return false;
        const TSharedPtr<FJsonValue> Value9 = Object6->TryGetField(TEXT("value"));
        if (!Value9.IsValid()) return false;
        double Number10 = 0;
        if (!Value9.IsValid() || !Value9->TryGetNumber(Number10) || !FMath::IsFinite(Number10)) return false;
        if (Number10 < -1000000.0) return false;
        if (Number10 > 1000000.0) return false;
        return true;
    }
    if (Operation == TEXT("blueprint.pin_connect"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("sourcePinId") && Entry.Key != TEXT("targetPinId")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("sourcePinId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 2048) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("targetPinId"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 2048) return false;
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
                if (Number10 < -1000000000.0) return false;
                if (Number10 > 1000000000.0) return false;
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
            if (Number4 < 1.0) return false;
            if (Number4 > 100.0) return false;
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
            if (Number6 < -1000000000.0) return false;
            if (Number6 > 1000000000.0) return false;
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
            if (Number2 < 1.0) return false;
            if (Number2 > 100.0) return false;
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
            if (Number4 < 1.0) return false;
            if (Number4 > 50.0) return false;
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
            if (Number2 < 1.0) return false;
            if (Number2 > 100.0) return false;
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
    if (Operation == TEXT("play.ui_observe"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("sessionId") && Entry.Key != TEXT("widgetBlueprintId") && Entry.Key != TEXT("widgetIds")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("sessionId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 128) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("widgetBlueprintId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("widgetIds"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array6 = Value5->AsArray();
        if (Array6.Num() < 1) return false;
        if (Array6.Num() > 16) return false;
        for (const TSharedPtr<FJsonValue>& Value7 : Array6)
        {
            if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
            const FString Text8 = Value7->AsString();
            if (MagiAxiUnicodeScalarCount(Text8) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text8) > 1024) return false;
        }
        return true;
    }
    if (Operation == TEXT("widget.create"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("path") && Entry.Key != TEXT("rootName") && Entry.Key != TEXT("rootClass")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("path"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("rootName"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 64) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("rootClass"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) > 64) return false;
        if (Text6 != TEXT("VerticalBox")) return false;
        return true;
    }
    if (Operation == TEXT("widget.tree_view"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        return true;
    }
    if (Operation == TEXT("widget.child_ensure"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("parentWidgetId") && Entry.Key != TEXT("name") && Entry.Key != TEXT("class")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("parentWidgetId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 1024) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("name"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 64) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("class"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) > 64) return false;
        if (Text8 != TEXT("TextBlock")) return false;
        return true;
    }
    if (Operation == TEXT("widget.property_set"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("widgetId") && Entry.Key != TEXT("property") && Entry.Key != TEXT("text") && Entry.Key != TEXT("visibility") && Entry.Key != TEXT("enabled")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("widgetId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 1024) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("property"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) > 16) return false;
        if (Text6 != TEXT("text") && Text6 != TEXT("visibility") && Text6 != TEXT("enabled")) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("text"));
        if (Value7.IsValid())
        {
            if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
            const FString Text8 = Value7->AsString();
            if (MagiAxiUnicodeScalarCount(Text8) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("visibility"));
        if (Value9.IsValid())
        {
            if (!Value9.IsValid() || Value9->Type != EJson::String) return false;
            const FString Text10 = Value9->AsString();
            if (MagiAxiUnicodeScalarCount(Text10) > 9) return false;
            if (Text10 != TEXT("Visible") && Text10 != TEXT("Hidden") && Text10 != TEXT("Collapsed")) return false;
        }
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("enabled"));
        if (Value11.IsValid())
        {
            if (!Value11.IsValid() || Value11->Type != EJson::Boolean) return false;
        }
        return true;
    }
    if (Operation == TEXT("widget.event_ensure"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("agentKey") && Entry.Key != TEXT("event") && Entry.Key != TEXT("actions")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("agentKey"));
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
        if (Text6 != TEXT("activate")) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("actions"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array8 = Value7->AsArray();
        if (Array8.Num() < 1) return false;
        if (Array8.Num() > 3) return false;
        for (const TSharedPtr<FJsonValue>& Value9 : Array8)
        {
            if (!Value9.IsValid() || Value9->Type != EJson::Object) return false;
            const TSharedPtr<FJsonObject> Object10 = Value9->AsObject();
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object10->Values) if (Entry.Key != TEXT("kind") && Entry.Key != TEXT("targetWidgetId") && Entry.Key != TEXT("text") && Entry.Key != TEXT("visibility") && Entry.Key != TEXT("enabled")) return false;
            const TSharedPtr<FJsonValue> Value11 = Object10->TryGetField(TEXT("kind"));
            if (!Value11.IsValid()) return false;
            if (!Value11.IsValid() || Value11->Type != EJson::String) return false;
            const FString Text12 = Value11->AsString();
            if (MagiAxiUnicodeScalarCount(Text12) > 16) return false;
            if (Text12 != TEXT("text.set") && Text12 != TEXT("enabled.set") && Text12 != TEXT("visibility.set")) return false;
            const TSharedPtr<FJsonValue> Value13 = Object10->TryGetField(TEXT("targetWidgetId"));
            if (!Value13.IsValid()) return false;
            if (!Value13.IsValid() || Value13->Type != EJson::String) return false;
            const FString Text14 = Value13->AsString();
            if (MagiAxiUnicodeScalarCount(Text14) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text14) > 1024) return false;
            const TSharedPtr<FJsonValue> Value15 = Object10->TryGetField(TEXT("text"));
            if (Value15.IsValid())
            {
                if (!Value15.IsValid() || Value15->Type != EJson::String) return false;
                const FString Text16 = Value15->AsString();
                if (MagiAxiUnicodeScalarCount(Text16) > 256) return false;
            }
            const TSharedPtr<FJsonValue> Value17 = Object10->TryGetField(TEXT("visibility"));
            if (Value17.IsValid())
            {
                if (!Value17.IsValid() || Value17->Type != EJson::String) return false;
                const FString Text18 = Value17->AsString();
                if (MagiAxiUnicodeScalarCount(Text18) > 9) return false;
                if (Text18 != TEXT("Visible") && Text18 != TEXT("Hidden") && Text18 != TEXT("Collapsed")) return false;
            }
            const TSharedPtr<FJsonValue> Value19 = Object10->TryGetField(TEXT("enabled"));
            if (Value19.IsValid())
            {
                if (!Value19.IsValid() || Value19->Type != EJson::Boolean) return false;
            }
        }
        return true;
    }
    if (Operation == TEXT("widget.viewport_ensure"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("hostBlueprintId") && Entry.Key != TEXT("widgetBlueprintId") && Entry.Key != TEXT("agentKey") && Entry.Key != TEXT("inputKey") && Entry.Key != TEXT("zOrder")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("hostBlueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("widgetBlueprintId"));
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
        if (MagiAxiUnicodeScalarCount(Text6) > 128) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("inputKey"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) > 1) return false;
        if (Text8 != TEXT("E")) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("zOrder"));
        if (!Value9.IsValid()) return false;
        double Number10 = 0;
        if (!Value9.IsValid() || !Value9->TryGetNumber(Number10) || !FMath::IsFinite(Number10) || FMath::FloorToDouble(Number10) != Number10) return false;
        if (Number10 < 0.0) return false;
        if (Number10 > 0.0) return false;
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
        if (Number2 < 0.0) return false;
        if (Number2 > 100.0) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("total"));
        if (!Value3.IsValid()) return false;
        double Number4 = 0;
        if (!Value3.IsValid() || !Value3->TryGetNumber(Number4) || !FMath::IsFinite(Number4) || FMath::FloorToDouble(Number4) != Number4) return false;
        if (Number4 < 0.0) return false;
        if (Number4 > 9007199254740991.0) return false;
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
            if (Number16 < -1000000000.0) return false;
            if (Number16 > 1000000000.0) return false;
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
            if (Number20 < -360.0) return false;
            if (Number20 > 360.0) return false;
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
            if (Number24 < 0.0) return false;
            if (Number24 > 1000000.0) return false;
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
        if (Number6 < 0.0) return false;
        if (Number6 > 100.0) return false;
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
    if (Operation == TEXT("blueprint.interface_create"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("function") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("function"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 128) return false;
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
    if (Operation == TEXT("blueprint.interface_view"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("function") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("id"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("function"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 128) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("revision"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 64) return false;
        return true;
    }
    if (Operation == TEXT("blueprint.interface_ensure"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("interfaceId") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("interfaceId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
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
    if (Operation == TEXT("blueprint.scs_view"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("components") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("components"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array4 = Value3->AsArray();
        if (Array4.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value5 : Array4)
        {
            if (!Value5.IsValid() || Value5->Type != EJson::Object) return false;
            const TSharedPtr<FJsonObject> Object6 = Value5->AsObject();
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object6->Values) if (Entry.Key != TEXT("variableGuid") && Entry.Key != TEXT("name") && Entry.Key != TEXT("class") && Entry.Key != TEXT("parent") && Entry.Key != TEXT("location") && Entry.Key != TEXT("rotation") && Entry.Key != TEXT("scale") && Entry.Key != TEXT("collisionEnabled") && Entry.Key != TEXT("collisionProfile") && Entry.Key != TEXT("objectType") && Entry.Key != TEXT("generateOverlapEvents") && Entry.Key != TEXT("simulatePhysics") && Entry.Key != TEXT("gravityEnabled") && Entry.Key != TEXT("massOverride") && Entry.Key != TEXT("boxExtent") && Entry.Key != TEXT("sphereRadius")) return false;
            const TSharedPtr<FJsonValue> Value7 = Object6->TryGetField(TEXT("variableGuid"));
            if (!Value7.IsValid()) return false;
            if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
            const FString Text8 = Value7->AsString();
            if (MagiAxiUnicodeScalarCount(Text8) < 36) return false;
            if (MagiAxiUnicodeScalarCount(Text8) > 36) return false;
            const TSharedPtr<FJsonValue> Value9 = Object6->TryGetField(TEXT("name"));
            if (!Value9.IsValid()) return false;
            if (!Value9.IsValid() || Value9->Type != EJson::String) return false;
            const FString Text10 = Value9->AsString();
            if (MagiAxiUnicodeScalarCount(Text10) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text10) > 128) return false;
            const TSharedPtr<FJsonValue> Value11 = Object6->TryGetField(TEXT("class"));
            if (!Value11.IsValid()) return false;
            if (!Value11.IsValid() || Value11->Type != EJson::String) return false;
            const FString Text12 = Value11->AsString();
            if (MagiAxiUnicodeScalarCount(Text12) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text12) > 512) return false;
            const TSharedPtr<FJsonValue> Value13 = Object6->TryGetField(TEXT("parent"));
            if (!Value13.IsValid()) return false;
            if (Value13->Type != EJson::Null)
            {
                if (!Value13.IsValid() || Value13->Type != EJson::String) return false;
                const FString Text14 = Value13->AsString();
                if (MagiAxiUnicodeScalarCount(Text14) > 128) return false;
            }
            const TSharedPtr<FJsonValue> Value15 = Object6->TryGetField(TEXT("location"));
            if (!Value15.IsValid()) return false;
            if (!Value15.IsValid() || Value15->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array16 = Value15->AsArray();
            if (Array16.Num() < 3) return false;
            if (Array16.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value17 : Array16)
            {
                double Number18 = 0;
                if (!Value17.IsValid() || !Value17->TryGetNumber(Number18) || !FMath::IsFinite(Number18)) return false;
                if (Number18 < -100000.0) return false;
                if (Number18 > 100000.0) return false;
            }
            const TSharedPtr<FJsonValue> Value19 = Object6->TryGetField(TEXT("rotation"));
            if (!Value19.IsValid()) return false;
            if (!Value19.IsValid() || Value19->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array20 = Value19->AsArray();
            if (Array20.Num() < 3) return false;
            if (Array20.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value21 : Array20)
            {
                double Number22 = 0;
                if (!Value21.IsValid() || !Value21->TryGetNumber(Number22) || !FMath::IsFinite(Number22)) return false;
                if (Number22 < -360.0) return false;
                if (Number22 > 360.0) return false;
            }
            const TSharedPtr<FJsonValue> Value23 = Object6->TryGetField(TEXT("scale"));
            if (!Value23.IsValid()) return false;
            if (!Value23.IsValid() || Value23->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array24 = Value23->AsArray();
            if (Array24.Num() < 3) return false;
            if (Array24.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value25 : Array24)
            {
                double Number26 = 0;
                if (!Value25.IsValid() || !Value25->TryGetNumber(Number26) || !FMath::IsFinite(Number26)) return false;
                if (Number26 < 0.0) return false;
                if (Number26 > 1000.0) return false;
            }
            const TSharedPtr<FJsonValue> Value27 = Object6->TryGetField(TEXT("collisionEnabled"));
            if (!Value27.IsValid()) return false;
            if (Value27->Type != EJson::Null)
            {
                if (!Value27.IsValid() || Value27->Type != EJson::String) return false;
                const FString Text28 = Value27->AsString();
                if (MagiAxiUnicodeScalarCount(Text28) > 32) return false;
            }
            const TSharedPtr<FJsonValue> Value29 = Object6->TryGetField(TEXT("collisionProfile"));
            if (!Value29.IsValid()) return false;
            if (Value29->Type != EJson::Null)
            {
                if (!Value29.IsValid() || Value29->Type != EJson::String) return false;
                const FString Text30 = Value29->AsString();
                if (MagiAxiUnicodeScalarCount(Text30) > 128) return false;
            }
            const TSharedPtr<FJsonValue> Value31 = Object6->TryGetField(TEXT("objectType"));
            if (!Value31.IsValid()) return false;
            if (Value31->Type != EJson::Null)
            {
                if (!Value31.IsValid() || Value31->Type != EJson::String) return false;
                const FString Text32 = Value31->AsString();
                if (MagiAxiUnicodeScalarCount(Text32) > 128) return false;
            }
            const TSharedPtr<FJsonValue> Value33 = Object6->TryGetField(TEXT("generateOverlapEvents"));
            if (!Value33.IsValid()) return false;
            if (Value33->Type != EJson::Null)
            {
                if (!Value33.IsValid() || Value33->Type != EJson::Boolean) return false;
            }
            const TSharedPtr<FJsonValue> Value34 = Object6->TryGetField(TEXT("simulatePhysics"));
            if (!Value34.IsValid()) return false;
            if (Value34->Type != EJson::Null)
            {
                if (!Value34.IsValid() || Value34->Type != EJson::Boolean) return false;
            }
            const TSharedPtr<FJsonValue> Value35 = Object6->TryGetField(TEXT("gravityEnabled"));
            if (!Value35.IsValid()) return false;
            if (Value35->Type != EJson::Null)
            {
                if (!Value35.IsValid() || Value35->Type != EJson::Boolean) return false;
            }
            const TSharedPtr<FJsonValue> Value36 = Object6->TryGetField(TEXT("massOverride"));
            if (!Value36.IsValid()) return false;
            if (Value36->Type != EJson::Null)
            {
                double Number37 = 0;
                if (!Value36.IsValid() || !Value36->TryGetNumber(Number37) || !FMath::IsFinite(Number37)) return false;
                if (Number37 < 0.0) return false;
                if (Number37 > 100000.0) return false;
            }
            const TSharedPtr<FJsonValue> Value38 = Object6->TryGetField(TEXT("boxExtent"));
            if (!Value38.IsValid()) return false;
            if (Value38->Type != EJson::Null)
            {
                if (!Value38.IsValid() || Value38->Type != EJson::Array) return false;
                const TArray<TSharedPtr<FJsonValue>>& Array39 = Value38->AsArray();
                if (Array39.Num() < 3) return false;
                if (Array39.Num() > 3) return false;
                for (const TSharedPtr<FJsonValue>& Value40 : Array39)
                {
                    double Number41 = 0;
                    if (!Value40.IsValid() || !Value40->TryGetNumber(Number41) || !FMath::IsFinite(Number41)) return false;
                    if (Number41 < 0.0) return false;
                    if (Number41 > 100000.0) return false;
                }
            }
            const TSharedPtr<FJsonValue> Value42 = Object6->TryGetField(TEXT("sphereRadius"));
            if (!Value42.IsValid()) return false;
            if (Value42->Type != EJson::Null)
            {
                double Number43 = 0;
                if (!Value42.IsValid() || !Value42->TryGetNumber(Number43) || !FMath::IsFinite(Number43)) return false;
                if (Number43 < 0.0) return false;
                if (Number43 > 100000.0) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value44 = Object0->TryGetField(TEXT("revision"));
        if (!Value44.IsValid()) return false;
        if (!Value44.IsValid() || Value44->Type != EJson::String) return false;
        const FString Text45 = Value44->AsString();
        if (MagiAxiUnicodeScalarCount(Text45) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text45) > 64) return false;
        return true;
    }
    if (Operation == TEXT("blueprint.scs_component_ensure"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("variableGuid") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("variableGuid"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 128) return false;
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
        const TSharedPtr<FJsonValue> Value10 = Object0->TryGetField(TEXT("revision"));
        if (!Value10.IsValid()) return false;
        if (!Value10.IsValid() || Value10->Type != EJson::String) return false;
        const FString Text11 = Value10->AsString();
        if (MagiAxiUnicodeScalarCount(Text11) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text11) > 64) return false;
        return true;
    }
    if (Operation == TEXT("blueprint.scs_component_update"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("variableGuid") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("variableGuid"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 36) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 36) return false;
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
        if (Value10.IsValid())
        {
            if (!Value10.IsValid() || Value10->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array11 = Value10->AsArray();
            if (Array11.Num() > 100) return false;
            for (const TSharedPtr<FJsonValue>& Value12 : Array11)
            {
                if (!Value12.IsValid() || Value12->Type != EJson::String) return false;
                const FString Text13 = Value12->AsString();
                if (MagiAxiUnicodeScalarCount(Text13) > 256) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value14 = Object0->TryGetField(TEXT("revision"));
        if (!Value14.IsValid()) return false;
        if (!Value14.IsValid() || Value14->Type != EJson::String) return false;
        const FString Text15 = Value14->AsString();
        if (MagiAxiUnicodeScalarCount(Text15) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text15) > 64) return false;
        return true;
    }
    if (Operation == TEXT("blueprint.scs_component_remove"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("variableGuid") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dryRun") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("variableGuid"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 128) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("changed"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value6 = Object0->TryGetField(TEXT("dryRun"));
        if (!Value6.IsValid()) return false;
        if (!Value6.IsValid() || Value6->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("dirtyPackages"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array8 = Value7->AsArray();
        if (Array8.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value9 : Array8)
        {
            if (!Value9.IsValid() || Value9->Type != EJson::String) return false;
            const FString Text10 = Value9->AsString();
            if (MagiAxiUnicodeScalarCount(Text10) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("revision"));
        if (!Value11.IsValid()) return false;
        if (!Value11.IsValid() || Value11->Type != EJson::String) return false;
        const FString Text12 = Value11->AsString();
        if (MagiAxiUnicodeScalarCount(Text12) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text12) > 64) return false;
        return true;
    }
    if (Operation == TEXT("play.component_observe"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("sessionId") && Entry.Key != TEXT("actorId") && Entry.Key != TEXT("variableGuid") && Entry.Key != TEXT("resolved") && Entry.Key != TEXT("reason") && Entry.Key != TEXT("componentName") && Entry.Key != TEXT("componentClass") && Entry.Key != TEXT("actorLocation") && Entry.Key != TEXT("location") && Entry.Key != TEXT("rotation") && Entry.Key != TEXT("scale") && Entry.Key != TEXT("collisionEnabled") && Entry.Key != TEXT("collisionProfile") && Entry.Key != TEXT("objectType") && Entry.Key != TEXT("generateOverlapEvents") && Entry.Key != TEXT("simulatePhysics") && Entry.Key != TEXT("gravityEnabled") && Entry.Key != TEXT("massOverride") && Entry.Key != TEXT("linearVelocity") && Entry.Key != TEXT("angularVelocity") && Entry.Key != TEXT("overlapCount") && Entry.Key != TEXT("overlappingActorIds") && Entry.Key != TEXT("interactionDisplacement") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("sessionId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 128) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("actorId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("variableGuid"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 36) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 36) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("resolved"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value8 = Object0->TryGetField(TEXT("reason"));
        if (!Value8.IsValid()) return false;
        if (Value8->Type != EJson::Null)
        {
            if (!Value8.IsValid() || Value8->Type != EJson::String) return false;
            const FString Text9 = Value8->AsString();
            if (MagiAxiUnicodeScalarCount(Text9) > 128) return false;
        }
        const TSharedPtr<FJsonValue> Value10 = Object0->TryGetField(TEXT("componentName"));
        if (!Value10.IsValid()) return false;
        if (Value10->Type != EJson::Null)
        {
            if (!Value10.IsValid() || Value10->Type != EJson::String) return false;
            const FString Text11 = Value10->AsString();
            if (MagiAxiUnicodeScalarCount(Text11) > 128) return false;
        }
        const TSharedPtr<FJsonValue> Value12 = Object0->TryGetField(TEXT("componentClass"));
        if (!Value12.IsValid()) return false;
        if (Value12->Type != EJson::Null)
        {
            if (!Value12.IsValid() || Value12->Type != EJson::String) return false;
            const FString Text13 = Value12->AsString();
            if (MagiAxiUnicodeScalarCount(Text13) > 512) return false;
        }
        const TSharedPtr<FJsonValue> Value14 = Object0->TryGetField(TEXT("actorLocation"));
        if (!Value14.IsValid()) return false;
        if (Value14->Type != EJson::Null)
        {
            if (!Value14.IsValid() || Value14->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array15 = Value14->AsArray();
            if (Array15.Num() < 3) return false;
            if (Array15.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value16 : Array15)
            {
                double Number17 = 0;
                if (!Value16.IsValid() || !Value16->TryGetNumber(Number17) || !FMath::IsFinite(Number17)) return false;
                if (Number17 < -1000000000.0) return false;
                if (Number17 > 1000000000.0) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value18 = Object0->TryGetField(TEXT("location"));
        if (!Value18.IsValid()) return false;
        if (Value18->Type != EJson::Null)
        {
            if (!Value18.IsValid() || Value18->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array19 = Value18->AsArray();
            if (Array19.Num() < 3) return false;
            if (Array19.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value20 : Array19)
            {
                double Number21 = 0;
                if (!Value20.IsValid() || !Value20->TryGetNumber(Number21) || !FMath::IsFinite(Number21)) return false;
                if (Number21 < -1000000000.0) return false;
                if (Number21 > 1000000000.0) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value22 = Object0->TryGetField(TEXT("rotation"));
        if (!Value22.IsValid()) return false;
        if (Value22->Type != EJson::Null)
        {
            if (!Value22.IsValid() || Value22->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array23 = Value22->AsArray();
            if (Array23.Num() < 3) return false;
            if (Array23.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value24 : Array23)
            {
                double Number25 = 0;
                if (!Value24.IsValid() || !Value24->TryGetNumber(Number25) || !FMath::IsFinite(Number25)) return false;
                if (Number25 < -360.0) return false;
                if (Number25 > 360.0) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value26 = Object0->TryGetField(TEXT("scale"));
        if (!Value26.IsValid()) return false;
        if (Value26->Type != EJson::Null)
        {
            if (!Value26.IsValid() || Value26->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array27 = Value26->AsArray();
            if (Array27.Num() < 3) return false;
            if (Array27.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value28 : Array27)
            {
                double Number29 = 0;
                if (!Value28.IsValid() || !Value28->TryGetNumber(Number29) || !FMath::IsFinite(Number29)) return false;
                if (Number29 < 0.0) return false;
                if (Number29 > 1000000000.0) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value30 = Object0->TryGetField(TEXT("collisionEnabled"));
        if (!Value30.IsValid()) return false;
        if (Value30->Type != EJson::Null)
        {
            if (!Value30.IsValid() || Value30->Type != EJson::String) return false;
            const FString Text31 = Value30->AsString();
            if (MagiAxiUnicodeScalarCount(Text31) > 32) return false;
        }
        const TSharedPtr<FJsonValue> Value32 = Object0->TryGetField(TEXT("collisionProfile"));
        if (!Value32.IsValid()) return false;
        if (Value32->Type != EJson::Null)
        {
            if (!Value32.IsValid() || Value32->Type != EJson::String) return false;
            const FString Text33 = Value32->AsString();
            if (MagiAxiUnicodeScalarCount(Text33) > 128) return false;
        }
        const TSharedPtr<FJsonValue> Value34 = Object0->TryGetField(TEXT("objectType"));
        if (!Value34.IsValid()) return false;
        if (Value34->Type != EJson::Null)
        {
            if (!Value34.IsValid() || Value34->Type != EJson::String) return false;
            const FString Text35 = Value34->AsString();
            if (MagiAxiUnicodeScalarCount(Text35) > 128) return false;
        }
        const TSharedPtr<FJsonValue> Value36 = Object0->TryGetField(TEXT("generateOverlapEvents"));
        if (!Value36.IsValid()) return false;
        if (Value36->Type != EJson::Null)
        {
            if (!Value36.IsValid() || Value36->Type != EJson::Boolean) return false;
        }
        const TSharedPtr<FJsonValue> Value37 = Object0->TryGetField(TEXT("simulatePhysics"));
        if (!Value37.IsValid()) return false;
        if (Value37->Type != EJson::Null)
        {
            if (!Value37.IsValid() || Value37->Type != EJson::Boolean) return false;
        }
        const TSharedPtr<FJsonValue> Value38 = Object0->TryGetField(TEXT("gravityEnabled"));
        if (!Value38.IsValid()) return false;
        if (Value38->Type != EJson::Null)
        {
            if (!Value38.IsValid() || Value38->Type != EJson::Boolean) return false;
        }
        const TSharedPtr<FJsonValue> Value39 = Object0->TryGetField(TEXT("massOverride"));
        if (!Value39.IsValid()) return false;
        if (Value39->Type != EJson::Null)
        {
            double Number40 = 0;
            if (!Value39.IsValid() || !Value39->TryGetNumber(Number40) || !FMath::IsFinite(Number40)) return false;
            if (Number40 < 0.0) return false;
            if (Number40 > 1000000.0) return false;
        }
        const TSharedPtr<FJsonValue> Value41 = Object0->TryGetField(TEXT("linearVelocity"));
        if (!Value41.IsValid()) return false;
        if (Value41->Type != EJson::Null)
        {
            if (!Value41.IsValid() || Value41->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array42 = Value41->AsArray();
            if (Array42.Num() < 3) return false;
            if (Array42.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value43 : Array42)
            {
                double Number44 = 0;
                if (!Value43.IsValid() || !Value43->TryGetNumber(Number44) || !FMath::IsFinite(Number44)) return false;
                if (Number44 < -1000000000.0) return false;
                if (Number44 > 1000000000.0) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value45 = Object0->TryGetField(TEXT("angularVelocity"));
        if (!Value45.IsValid()) return false;
        if (Value45->Type != EJson::Null)
        {
            if (!Value45.IsValid() || Value45->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array46 = Value45->AsArray();
            if (Array46.Num() < 3) return false;
            if (Array46.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value47 : Array46)
            {
                double Number48 = 0;
                if (!Value47.IsValid() || !Value47->TryGetNumber(Number48) || !FMath::IsFinite(Number48)) return false;
                if (Number48 < -1000000000.0) return false;
                if (Number48 > 1000000000.0) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value49 = Object0->TryGetField(TEXT("overlapCount"));
        if (!Value49.IsValid()) return false;
        if (Value49->Type != EJson::Null)
        {
            double Number50 = 0;
            if (!Value49.IsValid() || !Value49->TryGetNumber(Number50) || !FMath::IsFinite(Number50) || FMath::FloorToDouble(Number50) != Number50) return false;
            if (Number50 < 0.0) return false;
            if (Number50 > 1000.0) return false;
        }
        const TSharedPtr<FJsonValue> Value51 = Object0->TryGetField(TEXT("overlappingActorIds"));
        if (!Value51.IsValid()) return false;
        if (!Value51.IsValid() || Value51->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array52 = Value51->AsArray();
        if (Array52.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value53 : Array52)
        {
            if (!Value53.IsValid() || Value53->Type != EJson::String) return false;
            const FString Text54 = Value53->AsString();
            if (MagiAxiUnicodeScalarCount(Text54) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text54) > 512) return false;
        }
        const TSharedPtr<FJsonValue> Value55 = Object0->TryGetField(TEXT("interactionDisplacement"));
        if (!Value55.IsValid()) return false;
        if (Value55->Type != EJson::Null)
        {
            if (!Value55.IsValid() || Value55->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array56 = Value55->AsArray();
            if (Array56.Num() < 3) return false;
            if (Array56.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value57 : Array56)
            {
                double Number58 = 0;
                if (!Value57.IsValid() || !Value57->TryGetNumber(Number58) || !FMath::IsFinite(Number58)) return false;
                if (Number58 < -1000000000.0) return false;
                if (Number58 > 1000000000.0) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value59 = Object0->TryGetField(TEXT("revision"));
        if (!Value59.IsValid()) return false;
        if (!Value59.IsValid() || Value59->Type != EJson::String) return false;
        const FString Text60 = Value59->AsString();
        if (MagiAxiUnicodeScalarCount(Text60) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text60) > 64) return false;
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
        if (Number10 < 0.0) return false;
        if (Number10 > 10000.0) return false;
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("errorCount"));
        if (!Value11.IsValid()) return false;
        double Number12 = 0;
        if (!Value11.IsValid() || !Value11->TryGetNumber(Number12) || !FMath::IsFinite(Number12) || FMath::FloorToDouble(Number12) != Number12) return false;
        if (Number12 < 0.0) return false;
        if (Number12 > 100.0) return false;
        const TSharedPtr<FJsonValue> Value13 = Object0->TryGetField(TEXT("warningCount"));
        if (!Value13.IsValid()) return false;
        double Number14 = 0;
        if (!Value13.IsValid() || !Value13->TryGetNumber(Number14) || !FMath::IsFinite(Number14) || FMath::FloorToDouble(Number14) != Number14) return false;
        if (Number14 < 0.0) return false;
        if (Number14 > 100.0) return false;
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
        if (Number10 < 0.0) return false;
        if (Number10 > 10000.0) return false;
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("errorCount"));
        if (!Value11.IsValid()) return false;
        double Number12 = 0;
        if (!Value11.IsValid() || !Value11->TryGetNumber(Number12) || !FMath::IsFinite(Number12) || FMath::FloorToDouble(Number12) != Number12) return false;
        if (Number12 < 0.0) return false;
        if (Number12 > 100.0) return false;
        const TSharedPtr<FJsonValue> Value13 = Object0->TryGetField(TEXT("warningCount"));
        if (!Value13.IsValid()) return false;
        double Number14 = 0;
        if (!Value13.IsValid() || !Value13->TryGetNumber(Number14) || !FMath::IsFinite(Number14) || FMath::FloorToDouble(Number14) != Number14) return false;
        if (Number14 < 0.0) return false;
        if (Number14 > 100.0) return false;
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
    if (Operation == TEXT("blueprint.create"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("parentClass") && Entry.Key != TEXT("generatedClass") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
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
    if (Operation == TEXT("blueprint.graph_view"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("count") && Entry.Key != TEXT("total") && Entry.Key != TEXT("scope") && Entry.Key != TEXT("revision") && Entry.Key != TEXT("items") && Entry.Key != TEXT("nextCursor")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("count"));
        if (!Value3.IsValid()) return false;
        double Number4 = 0;
        if (!Value3.IsValid() || !Value3->TryGetNumber(Number4) || !FMath::IsFinite(Number4) || FMath::FloorToDouble(Number4) != Number4) return false;
        if (Number4 < 0.0) return false;
        if (Number4 > 100.0) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("total"));
        if (!Value5.IsValid()) return false;
        double Number6 = 0;
        if (!Value5.IsValid() || !Value5->TryGetNumber(Number6) || !FMath::IsFinite(Number6) || FMath::FloorToDouble(Number6) != Number6) return false;
        if (Number6 < 0.0) return false;
        if (Number6 > 10000.0) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("scope"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text8) > 1024) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("revision"));
        if (!Value9.IsValid()) return false;
        if (!Value9.IsValid() || Value9->Type != EJson::String) return false;
        const FString Text10 = Value9->AsString();
        if (MagiAxiUnicodeScalarCount(Text10) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text10) > 64) return false;
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("items"));
        if (!Value11.IsValid()) return false;
        if (!Value11.IsValid() || Value11->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array12 = Value11->AsArray();
        if (Array12.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value13 : Array12)
        {
            if (!Value13.IsValid() || Value13->Type != EJson::Object) return false;
            if (Value13->AsObject()->HasField(TEXT("graphId")) == true && Value13->AsObject()->HasField(TEXT("kind")) == true && Value13->AsObject()->HasField(TEXT("name")) == true && Value13->AsObject()->HasField(TEXT("nodeCount")) == true && Value13->AsObject()->HasField(TEXT("nodeId")) == false && Value13->AsObject()->HasField(TEXT("class")) == false && Value13->AsObject()->HasField(TEXT("title")) == false && Value13->AsObject()->HasField(TEXT("x")) == false && Value13->AsObject()->HasField(TEXT("y")) == false && Value13->AsObject()->HasField(TEXT("pins")) == false)
            {
                if (!Value13.IsValid() || Value13->Type != EJson::Object) return false;
                const TSharedPtr<FJsonObject> Object14 = Value13->AsObject();
                for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object14->Values) if (Entry.Key != TEXT("graphId") && Entry.Key != TEXT("kind") && Entry.Key != TEXT("name") && Entry.Key != TEXT("nodeCount")) return false;
                const TSharedPtr<FJsonValue> Value15 = Object14->TryGetField(TEXT("graphId"));
                if (!Value15.IsValid()) return false;
                if (!Value15.IsValid() || Value15->Type != EJson::String) return false;
                const FString Text16 = Value15->AsString();
                if (MagiAxiUnicodeScalarCount(Text16) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text16) > 1024) return false;
                const TSharedPtr<FJsonValue> Value17 = Object14->TryGetField(TEXT("kind"));
                if (!Value17.IsValid()) return false;
                if (!Value17.IsValid() || Value17->Type != EJson::String) return false;
                const FString Text18 = Value17->AsString();
                if (MagiAxiUnicodeScalarCount(Text18) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text18) > 32) return false;
                const TSharedPtr<FJsonValue> Value19 = Object14->TryGetField(TEXT("name"));
                if (!Value19.IsValid()) return false;
                if (!Value19.IsValid() || Value19->Type != EJson::String) return false;
                const FString Text20 = Value19->AsString();
                if (MagiAxiUnicodeScalarCount(Text20) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text20) > 512) return false;
                const TSharedPtr<FJsonValue> Value21 = Object14->TryGetField(TEXT("nodeCount"));
                if (!Value21.IsValid()) return false;
                double Number22 = 0;
                if (!Value21.IsValid() || !Value21->TryGetNumber(Number22) || !FMath::IsFinite(Number22) || FMath::FloorToDouble(Number22) != Number22) return false;
                if (Number22 < 0.0) return false;
                if (Number22 > 10000.0) return false;
            }
            else if (Value13->AsObject()->HasField(TEXT("graphId")) == false && Value13->AsObject()->HasField(TEXT("kind")) == false && Value13->AsObject()->HasField(TEXT("name")) == false && Value13->AsObject()->HasField(TEXT("nodeCount")) == false && Value13->AsObject()->HasField(TEXT("nodeId")) == true && Value13->AsObject()->HasField(TEXT("class")) == true && Value13->AsObject()->HasField(TEXT("title")) == true && Value13->AsObject()->HasField(TEXT("x")) == true && Value13->AsObject()->HasField(TEXT("y")) == true && Value13->AsObject()->HasField(TEXT("pins")) == true)
            {
                if (!Value13.IsValid() || Value13->Type != EJson::Object) return false;
                const TSharedPtr<FJsonObject> Object23 = Value13->AsObject();
                for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object23->Values) if (Entry.Key != TEXT("nodeId") && Entry.Key != TEXT("class") && Entry.Key != TEXT("title") && Entry.Key != TEXT("x") && Entry.Key != TEXT("y") && Entry.Key != TEXT("pins")) return false;
                const TSharedPtr<FJsonValue> Value24 = Object23->TryGetField(TEXT("nodeId"));
                if (!Value24.IsValid()) return false;
                if (!Value24.IsValid() || Value24->Type != EJson::String) return false;
                const FString Text25 = Value24->AsString();
                if (MagiAxiUnicodeScalarCount(Text25) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text25) > 1024) return false;
                const TSharedPtr<FJsonValue> Value26 = Object23->TryGetField(TEXT("class"));
                if (!Value26.IsValid()) return false;
                if (!Value26.IsValid() || Value26->Type != EJson::String) return false;
                const FString Text27 = Value26->AsString();
                if (MagiAxiUnicodeScalarCount(Text27) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text27) > 512) return false;
                const TSharedPtr<FJsonValue> Value28 = Object23->TryGetField(TEXT("title"));
                if (!Value28.IsValid()) return false;
                if (!Value28.IsValid() || Value28->Type != EJson::String) return false;
                const FString Text29 = Value28->AsString();
                if (MagiAxiUnicodeScalarCount(Text29) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text29) > 512) return false;
                const TSharedPtr<FJsonValue> Value30 = Object23->TryGetField(TEXT("x"));
                if (!Value30.IsValid()) return false;
                double Number31 = 0;
                if (!Value30.IsValid() || !Value30->TryGetNumber(Number31) || !FMath::IsFinite(Number31) || FMath::FloorToDouble(Number31) != Number31) return false;
                if (Number31 < -1000000.0) return false;
                if (Number31 > 1000000.0) return false;
                const TSharedPtr<FJsonValue> Value32 = Object23->TryGetField(TEXT("y"));
                if (!Value32.IsValid()) return false;
                double Number33 = 0;
                if (!Value32.IsValid() || !Value32->TryGetNumber(Number33) || !FMath::IsFinite(Number33) || FMath::FloorToDouble(Number33) != Number33) return false;
                if (Number33 < -1000000.0) return false;
                if (Number33 > 1000000.0) return false;
                const TSharedPtr<FJsonValue> Value34 = Object23->TryGetField(TEXT("pins"));
                if (!Value34.IsValid()) return false;
                if (!Value34.IsValid() || Value34->Type != EJson::Array) return false;
                const TArray<TSharedPtr<FJsonValue>>& Array35 = Value34->AsArray();
                if (Array35.Num() > 64) return false;
                for (const TSharedPtr<FJsonValue>& Value36 : Array35)
                {
                    if (!Value36.IsValid() || Value36->Type != EJson::Object) return false;
                    const TSharedPtr<FJsonObject> Object37 = Value36->AsObject();
                    for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object37->Values) if (Entry.Key != TEXT("pinId") && Entry.Key != TEXT("name") && Entry.Key != TEXT("direction") && Entry.Key != TEXT("type") && Entry.Key != TEXT("defaultValue") && Entry.Key != TEXT("links")) return false;
                    const TSharedPtr<FJsonValue> Value38 = Object37->TryGetField(TEXT("pinId"));
                    if (!Value38.IsValid()) return false;
                    if (!Value38.IsValid() || Value38->Type != EJson::String) return false;
                    const FString Text39 = Value38->AsString();
                    if (MagiAxiUnicodeScalarCount(Text39) < 1) return false;
                    if (MagiAxiUnicodeScalarCount(Text39) > 2048) return false;
                    const TSharedPtr<FJsonValue> Value40 = Object37->TryGetField(TEXT("name"));
                    if (!Value40.IsValid()) return false;
                    if (!Value40.IsValid() || Value40->Type != EJson::String) return false;
                    const FString Text41 = Value40->AsString();
                    if (MagiAxiUnicodeScalarCount(Text41) < 1) return false;
                    if (MagiAxiUnicodeScalarCount(Text41) > 512) return false;
                    const TSharedPtr<FJsonValue> Value42 = Object37->TryGetField(TEXT("direction"));
                    if (!Value42.IsValid()) return false;
                    if (!Value42.IsValid() || Value42->Type != EJson::String) return false;
                    const FString Text43 = Value42->AsString();
                    if (MagiAxiUnicodeScalarCount(Text43) > 6) return false;
                    if (Text43 != TEXT("input") && Text43 != TEXT("output")) return false;
                    const TSharedPtr<FJsonValue> Value44 = Object37->TryGetField(TEXT("type"));
                    if (!Value44.IsValid()) return false;
                    if (!Value44.IsValid() || Value44->Type != EJson::String) return false;
                    const FString Text45 = Value44->AsString();
                    if (MagiAxiUnicodeScalarCount(Text45) < 1) return false;
                    if (MagiAxiUnicodeScalarCount(Text45) > 128) return false;
                    const TSharedPtr<FJsonValue> Value46 = Object37->TryGetField(TEXT("defaultValue"));
                    if (!Value46.IsValid()) return false;
                    if (!Value46.IsValid() || Value46->Type != EJson::String) return false;
                    const FString Text47 = Value46->AsString();
                    if (MagiAxiUnicodeScalarCount(Text47) > 1024) return false;
                    const TSharedPtr<FJsonValue> Value48 = Object37->TryGetField(TEXT("links"));
                    if (!Value48.IsValid()) return false;
                    if (!Value48.IsValid() || Value48->Type != EJson::Array) return false;
                    const TArray<TSharedPtr<FJsonValue>>& Array49 = Value48->AsArray();
                    if (Array49.Num() > 64) return false;
                    for (const TSharedPtr<FJsonValue>& Value50 : Array49)
                    {
                        if (!Value50.IsValid() || Value50->Type != EJson::String) return false;
                        const FString Text51 = Value50->AsString();
                        if (MagiAxiUnicodeScalarCount(Text51) < 1) return false;
                        if (MagiAxiUnicodeScalarCount(Text51) > 2048) return false;
                    }
                }
            }
            else return false;
        }
        const TSharedPtr<FJsonValue> Value52 = Object0->TryGetField(TEXT("nextCursor"));
        if (!Value52.IsValid()) return false;
        if (Value52->Type != EJson::Null)
        {
            if (!Value52.IsValid() || Value52->Type != EJson::String) return false;
            const FString Text53 = Value52->AsString();
            if (MagiAxiUnicodeScalarCount(Text53) > 256) return false;
        }
        return true;
    }
    if (Operation == TEXT("blueprint.event_ensure"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("graphId") && Entry.Key != TEXT("nodeId") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("graphId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 1024) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("nodeId"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 1024) return false;
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
    if (Operation == TEXT("blueprint.node_ensure"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("graphId") && Entry.Key != TEXT("nodeId") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("graphId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 1024) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("nodeId"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 1024) return false;
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
    if (Operation == TEXT("blueprint.pin_default_set"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("pinId") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("pinId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 2048) return false;
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
    if (Operation == TEXT("blueprint.pin_connect"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("sourcePinId") && Entry.Key != TEXT("targetPinId") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("sourcePinId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 2048) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("targetPinId"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 2048) return false;
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
        if (Number2 < 0.0) return false;
        if (Number2 > 100.0) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("total"));
        if (!Value3.IsValid()) return false;
        double Number4 = 0;
        if (!Value3.IsValid() || !Value3->TryGetNumber(Number4) || !FMath::IsFinite(Number4) || FMath::FloorToDouble(Number4) != Number4) return false;
        if (Number4 < 0.0) return false;
        if (Number4 > 9007199254740991.0) return false;
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
            if (Number13 < -1000000000.0) return false;
            if (Number13 > 1000000000.0) return false;
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
            if (Number17 < -360.0) return false;
            if (Number17 > 360.0) return false;
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
            if (Number21 < 0.0) return false;
            if (Number21 > 1000000000.0) return false;
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
                if (Number20 < -1000000000.0) return false;
                if (Number20 > 1000000000.0) return false;
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
        if (Number6 < 1.0) return false;
        if (Number6 > 16384.0) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("height"));
        if (!Value7.IsValid()) return false;
        double Number8 = 0;
        if (!Value7.IsValid() || !Value7->TryGetNumber(Number8) || !FMath::IsFinite(Number8) || FMath::FloorToDouble(Number8) != Number8) return false;
        if (Number8 < 1.0) return false;
        if (Number8 > 16384.0) return false;
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
        if (Number10 < 0.0) return false;
        if (Number10 > 100.0) return false;
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
        if (Number2 < 0.0) return false;
        if (Number2 > 100.0) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("total"));
        if (!Value3.IsValid()) return false;
        double Number4 = 0;
        if (!Value3.IsValid() || !Value3->TryGetNumber(Number4) || !FMath::IsFinite(Number4) || FMath::FloorToDouble(Number4) != Number4) return false;
        if (Number4 < 0.0) return false;
        if (Number4 > 9007199254740991.0) return false;
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
        if (Number6 < 1.0) return false;
        if (Number6 > 1.0) return false;
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
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object40->Values) if (Entry.Key != TEXT("availability") && Entry.Key != TEXT("reasons") && Entry.Key != TEXT("catalogHash")) return false;
        const TSharedPtr<FJsonValue> Value41 = Object40->TryGetField(TEXT("availability"));
        if (!Value41.IsValid()) return false;
        if (!Value41.IsValid() || Value41->Type != EJson::String) return false;
        const FString Text42 = Value41->AsString();
        if (MagiAxiUnicodeScalarCount(Text42) > 11) return false;
        if (Text42 != TEXT("available") && Text42 != TEXT("unavailable") && Text42 != TEXT("unknown")) return false;
        const TSharedPtr<FJsonValue> Value43 = Object40->TryGetField(TEXT("reasons"));
        if (!Value43.IsValid()) return false;
        if (!Value43.IsValid() || Value43->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array44 = Value43->AsArray();
        if (Array44.Num() > 16) return false;
        for (const TSharedPtr<FJsonValue>& Value45 : Array44)
        {
            if (!Value45.IsValid() || Value45->Type != EJson::Object) return false;
            const TSharedPtr<FJsonObject> Object46 = Value45->AsObject();
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object46->Values) if (Entry.Key != TEXT("code") && Entry.Key != TEXT("subject") && Entry.Key != TEXT("message")) return false;
            const TSharedPtr<FJsonValue> Value47 = Object46->TryGetField(TEXT("code"));
            if (!Value47.IsValid()) return false;
            if (!Value47.IsValid() || Value47->Type != EJson::String) return false;
            const FString Text48 = Value47->AsString();
            if (MagiAxiUnicodeScalarCount(Text48) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text48) > 32) return false;
            const TSharedPtr<FJsonValue> Value49 = Object46->TryGetField(TEXT("subject"));
            if (!Value49.IsValid()) return false;
            if (!Value49.IsValid() || Value49->Type != EJson::String) return false;
            const FString Text50 = Value49->AsString();
            if (MagiAxiUnicodeScalarCount(Text50) > 128) return false;
            const TSharedPtr<FJsonValue> Value51 = Object46->TryGetField(TEXT("message"));
            if (!Value51.IsValid()) return false;
            if (!Value51.IsValid() || Value51->Type != EJson::String) return false;
            const FString Text52 = Value51->AsString();
            if (MagiAxiUnicodeScalarCount(Text52) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text52) > 512) return false;
        }
        const TSharedPtr<FJsonValue> Value53 = Object40->TryGetField(TEXT("catalogHash"));
        if (!Value53.IsValid()) return false;
        if (!Value53.IsValid() || Value53->Type != EJson::String) return false;
        const FString Text54 = Value53->AsString();
        if (MagiAxiUnicodeScalarCount(Text54) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text54) > 64) return false;
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
        if (Number2 < 0.0) return false;
        if (Number2 > 50.0) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("total"));
        if (!Value3.IsValid()) return false;
        double Number4 = 0;
        if (!Value3.IsValid() || !Value3->TryGetNumber(Number4) || !FMath::IsFinite(Number4) || FMath::FloorToDouble(Number4) != Number4) return false;
        if (Number4 < 0.0) return false;
        if (Number4 > 9007199254740991.0) return false;
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
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object10->Values) if (Entry.Key != TEXT("id") && Entry.Key != TEXT("domain") && Entry.Key != TEXT("summary") && Entry.Key != TEXT("availability") && Entry.Key != TEXT("reasons")) return false;
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
            const TSharedPtr<FJsonValue> Value17 = Object10->TryGetField(TEXT("availability"));
            if (!Value17.IsValid()) return false;
            if (!Value17.IsValid() || Value17->Type != EJson::String) return false;
            const FString Text18 = Value17->AsString();
            if (MagiAxiUnicodeScalarCount(Text18) > 11) return false;
            if (Text18 != TEXT("available") && Text18 != TEXT("unavailable") && Text18 != TEXT("unknown")) return false;
            const TSharedPtr<FJsonValue> Value19 = Object10->TryGetField(TEXT("reasons"));
            if (!Value19.IsValid()) return false;
            if (!Value19.IsValid() || Value19->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array20 = Value19->AsArray();
            if (Array20.Num() > 16) return false;
            for (const TSharedPtr<FJsonValue>& Value21 : Array20)
            {
                if (!Value21.IsValid() || Value21->Type != EJson::Object) return false;
                const TSharedPtr<FJsonObject> Object22 = Value21->AsObject();
                for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object22->Values) if (Entry.Key != TEXT("code") && Entry.Key != TEXT("subject") && Entry.Key != TEXT("message")) return false;
                const TSharedPtr<FJsonValue> Value23 = Object22->TryGetField(TEXT("code"));
                if (!Value23.IsValid()) return false;
                if (!Value23.IsValid() || Value23->Type != EJson::String) return false;
                const FString Text24 = Value23->AsString();
                if (MagiAxiUnicodeScalarCount(Text24) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text24) > 32) return false;
                const TSharedPtr<FJsonValue> Value25 = Object22->TryGetField(TEXT("subject"));
                if (!Value25.IsValid()) return false;
                if (!Value25.IsValid() || Value25->Type != EJson::String) return false;
                const FString Text26 = Value25->AsString();
                if (MagiAxiUnicodeScalarCount(Text26) > 128) return false;
                const TSharedPtr<FJsonValue> Value27 = Object22->TryGetField(TEXT("message"));
                if (!Value27.IsValid()) return false;
                if (!Value27.IsValid() || Value27->Type != EJson::String) return false;
                const FString Text28 = Value27->AsString();
                if (MagiAxiUnicodeScalarCount(Text28) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text28) > 512) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value29 = Object0->TryGetField(TEXT("nextCursor"));
        if (!Value29.IsValid()) return false;
        if (Value29->Type != EJson::Null)
        {
            if (!Value29.IsValid() || Value29->Type != EJson::String) return false;
            const FString Text30 = Value29->AsString();
            if (MagiAxiUnicodeScalarCount(Text30) > 256) return false;
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
        if (Number6 < 0.0) return false;
        if (Number6 > 4294967295.0) return false;
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
        if (Number12 < 0.0) return false;
        if (Number12 > 9007199254740991.0) return false;
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
        if (Number2 < 0.0) return false;
        if (Number2 > 100.0) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("total"));
        if (!Value3.IsValid()) return false;
        double Number4 = 0;
        if (!Value3.IsValid() || !Value3->TryGetNumber(Number4) || !FMath::IsFinite(Number4) || FMath::FloorToDouble(Number4) != Number4) return false;
        if (Number4 < 0.0) return false;
        if (Number4 > 9007199254740991.0) return false;
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
        if (Number10 < 1.0) return false;
        if (Number10 > 4294967295.0) return false;
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("target"));
        if (!Value11.IsValid()) return false;
        if (!Value11.IsValid() || Value11->Type != EJson::String) return false;
        const FString Text12 = Value11->AsString();
        if (MagiAxiUnicodeScalarCount(Text12) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text12) > 8192) return false;
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
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object31->Values) if (Entry.Key != TEXT("readback") && Entry.Key != TEXT("target") && Entry.Key != TEXT("matched") && Entry.Key != TEXT("exists") && Entry.Key != TEXT("observedRevision") && Entry.Key != TEXT("accepted") && Entry.Key != TEXT("beforeRevision") && Entry.Key != TEXT("afterRevision") && Entry.Key != TEXT("observedStatus") && Entry.Key != TEXT("failureType") && Entry.Key != TEXT("errorCount") && Entry.Key != TEXT("warningCount") && Entry.Key != TEXT("diagnostics") && Entry.Key != TEXT("changedObjects") && Entry.Key != TEXT("requestPath") && Entry.Key != TEXT("requestParentClass") && Entry.Key != TEXT("requestBlueprintId") && Entry.Key != TEXT("requestGraphId") && Entry.Key != TEXT("requestAgentKey") && Entry.Key != TEXT("requestIntent") && Entry.Key != TEXT("requestPinId") && Entry.Key != TEXT("requestSourcePinId") && Entry.Key != TEXT("requestTargetPinId") && Entry.Key != TEXT("requestValueType") && Entry.Key != TEXT("requestValue") && Entry.Key != TEXT("requestFunction") && Entry.Key != TEXT("requestInterfaceId") && Entry.Key != TEXT("requestName") && Entry.Key != TEXT("requestClass") && Entry.Key != TEXT("requestParent") && Entry.Key != TEXT("requestVariableGuid") && Entry.Key != TEXT("requestForce") && Entry.Key != TEXT("requestDryRun") && Entry.Key != TEXT("requestLocation") && Entry.Key != TEXT("requestRotation") && Entry.Key != TEXT("requestScale") && Entry.Key != TEXT("requestCollisionEnabled") && Entry.Key != TEXT("requestCollisionProfile") && Entry.Key != TEXT("requestGenerateOverlapEvents") && Entry.Key != TEXT("requestSimulatePhysics") && Entry.Key != TEXT("requestGravityEnabled") && Entry.Key != TEXT("requestMassOverride") && Entry.Key != TEXT("requestBoxExtent") && Entry.Key != TEXT("requestSphereRadius") && Entry.Key != TEXT("requestRootName") && Entry.Key != TEXT("requestRootClass") && Entry.Key != TEXT("requestParentWidgetId") && Entry.Key != TEXT("requestWidgetId") && Entry.Key != TEXT("requestProperty") && Entry.Key != TEXT("requestText") && Entry.Key != TEXT("requestVisibility") && Entry.Key != TEXT("requestEnabled") && Entry.Key != TEXT("requestActions") && Entry.Key != TEXT("requestHostBlueprintId") && Entry.Key != TEXT("requestWidgetBlueprintId") && Entry.Key != TEXT("requestInputKey") && Entry.Key != TEXT("requestZOrder")) return false;
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
        if (MagiAxiUnicodeScalarCount(Text35) > 8192) return false;
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
        const TSharedPtr<FJsonValue> Value45 = Object31->TryGetField(TEXT("observedStatus"));
        if (Value45.IsValid())
        {
            if (!Value45.IsValid() || Value45->Type != EJson::String) return false;
            const FString Text46 = Value45->AsString();
            if (MagiAxiUnicodeScalarCount(Text46) > 16) return false;
            if (Text46 != TEXT("error")) return false;
        }
        const TSharedPtr<FJsonValue> Value47 = Object31->TryGetField(TEXT("failureType"));
        if (Value47.IsValid())
        {
            if (!Value47.IsValid() || Value47->Type != EJson::String) return false;
            const FString Text48 = Value47->AsString();
            if (MagiAxiUnicodeScalarCount(Text48) > 32) return false;
            if (Text48 != TEXT("blueprint_compile_failed")) return false;
        }
        const TSharedPtr<FJsonValue> Value49 = Object31->TryGetField(TEXT("errorCount"));
        if (Value49.IsValid())
        {
            double Number50 = 0;
            if (!Value49.IsValid() || !Value49->TryGetNumber(Number50) || !FMath::IsFinite(Number50) || FMath::FloorToDouble(Number50) != Number50) return false;
            if (Number50 < 1.0) return false;
            if (Number50 > 100.0) return false;
        }
        const TSharedPtr<FJsonValue> Value51 = Object31->TryGetField(TEXT("warningCount"));
        if (Value51.IsValid())
        {
            double Number52 = 0;
            if (!Value51.IsValid() || !Value51->TryGetNumber(Number52) || !FMath::IsFinite(Number52) || FMath::FloorToDouble(Number52) != Number52) return false;
            if (Number52 < 0.0) return false;
            if (Number52 > 100.0) return false;
        }
        const TSharedPtr<FJsonValue> Value53 = Object31->TryGetField(TEXT("diagnostics"));
        if (Value53.IsValid())
        {
            if (!Value53.IsValid() || Value53->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array54 = Value53->AsArray();
            if (Array54.Num() < 1) return false;
            if (Array54.Num() > 100) return false;
            for (const TSharedPtr<FJsonValue>& Value55 : Array54)
            {
                if (!Value55.IsValid() || Value55->Type != EJson::Object) return false;
                const TSharedPtr<FJsonObject> Object56 = Value55->AsObject();
                for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object56->Values) if (Entry.Key != TEXT("severity") && Entry.Key != TEXT("message") && Entry.Key != TEXT("graph") && Entry.Key != TEXT("nodeGuid") && Entry.Key != TEXT("nodeTitle")) return false;
                const TSharedPtr<FJsonValue> Value57 = Object56->TryGetField(TEXT("severity"));
                if (!Value57.IsValid()) return false;
                if (!Value57.IsValid() || Value57->Type != EJson::String) return false;
                const FString Text58 = Value57->AsString();
                if (MagiAxiUnicodeScalarCount(Text58) > 8) return false;
                if (Text58 != TEXT("error") && Text58 != TEXT("warning")) return false;
                const TSharedPtr<FJsonValue> Value59 = Object56->TryGetField(TEXT("message"));
                if (!Value59.IsValid()) return false;
                if (!Value59.IsValid() || Value59->Type != EJson::String) return false;
                const FString Text60 = Value59->AsString();
                if (MagiAxiUnicodeScalarCount(Text60) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text60) > 1024) return false;
                const TSharedPtr<FJsonValue> Value61 = Object56->TryGetField(TEXT("graph"));
                if (!Value61.IsValid()) return false;
                if (!Value61.IsValid() || Value61->Type != EJson::String) return false;
                const FString Text62 = Value61->AsString();
                if (MagiAxiUnicodeScalarCount(Text62) > 512) return false;
                const TSharedPtr<FJsonValue> Value63 = Object56->TryGetField(TEXT("nodeGuid"));
                if (!Value63.IsValid()) return false;
                if (!Value63.IsValid() || Value63->Type != EJson::String) return false;
                const FString Text64 = Value63->AsString();
                if (MagiAxiUnicodeScalarCount(Text64) > 128) return false;
                const TSharedPtr<FJsonValue> Value65 = Object56->TryGetField(TEXT("nodeTitle"));
                if (!Value65.IsValid()) return false;
                if (!Value65.IsValid() || Value65->Type != EJson::String) return false;
                const FString Text66 = Value65->AsString();
                if (MagiAxiUnicodeScalarCount(Text66) > 512) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value67 = Object31->TryGetField(TEXT("changedObjects"));
        if (Value67.IsValid())
        {
            if (!Value67.IsValid() || Value67->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array68 = Value67->AsArray();
            if (Array68.Num() > 100) return false;
            for (const TSharedPtr<FJsonValue>& Value69 : Array68)
            {
                if (!Value69.IsValid() || Value69->Type != EJson::String) return false;
                const FString Text70 = Value69->AsString();
                if (MagiAxiUnicodeScalarCount(Text70) > 512) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value71 = Object31->TryGetField(TEXT("requestPath"));
        if (Value71.IsValid())
        {
            if (!Value71.IsValid() || Value71->Type != EJson::String) return false;
            const FString Text72 = Value71->AsString();
            if (MagiAxiUnicodeScalarCount(Text72) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text72) > 512) return false;
        }
        const TSharedPtr<FJsonValue> Value73 = Object31->TryGetField(TEXT("requestParentClass"));
        if (Value73.IsValid())
        {
            if (!Value73.IsValid() || Value73->Type != EJson::String) return false;
            const FString Text74 = Value73->AsString();
            if (MagiAxiUnicodeScalarCount(Text74) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text74) > 512) return false;
        }
        const TSharedPtr<FJsonValue> Value75 = Object31->TryGetField(TEXT("requestBlueprintId"));
        if (Value75.IsValid())
        {
            if (!Value75.IsValid() || Value75->Type != EJson::String) return false;
            const FString Text76 = Value75->AsString();
            if (MagiAxiUnicodeScalarCount(Text76) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text76) > 512) return false;
        }
        const TSharedPtr<FJsonValue> Value77 = Object31->TryGetField(TEXT("requestGraphId"));
        if (Value77.IsValid())
        {
            if (!Value77.IsValid() || Value77->Type != EJson::String) return false;
            const FString Text78 = Value77->AsString();
            if (MagiAxiUnicodeScalarCount(Text78) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text78) > 1024) return false;
        }
        const TSharedPtr<FJsonValue> Value79 = Object31->TryGetField(TEXT("requestAgentKey"));
        if (Value79.IsValid())
        {
            if (!Value79.IsValid() || Value79->Type != EJson::String) return false;
            const FString Text80 = Value79->AsString();
            if (MagiAxiUnicodeScalarCount(Text80) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text80) > 128) return false;
        }
        const TSharedPtr<FJsonValue> Value81 = Object31->TryGetField(TEXT("requestIntent"));
        if (Value81.IsValid())
        {
            if (!Value81.IsValid() || Value81->Type != EJson::String) return false;
            const FString Text82 = Value81->AsString();
            if (MagiAxiUnicodeScalarCount(Text82) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text82) > 128) return false;
        }
        const TSharedPtr<FJsonValue> Value83 = Object31->TryGetField(TEXT("requestPinId"));
        if (Value83.IsValid())
        {
            if (!Value83.IsValid() || Value83->Type != EJson::String) return false;
            const FString Text84 = Value83->AsString();
            if (MagiAxiUnicodeScalarCount(Text84) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text84) > 2048) return false;
        }
        const TSharedPtr<FJsonValue> Value85 = Object31->TryGetField(TEXT("requestSourcePinId"));
        if (Value85.IsValid())
        {
            if (!Value85.IsValid() || Value85->Type != EJson::String) return false;
            const FString Text86 = Value85->AsString();
            if (MagiAxiUnicodeScalarCount(Text86) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text86) > 2048) return false;
        }
        const TSharedPtr<FJsonValue> Value87 = Object31->TryGetField(TEXT("requestTargetPinId"));
        if (Value87.IsValid())
        {
            if (!Value87.IsValid() || Value87->Type != EJson::String) return false;
            const FString Text88 = Value87->AsString();
            if (MagiAxiUnicodeScalarCount(Text88) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text88) > 2048) return false;
        }
        const TSharedPtr<FJsonValue> Value89 = Object31->TryGetField(TEXT("requestValueType"));
        if (Value89.IsValid())
        {
            if (!Value89.IsValid() || Value89->Type != EJson::String) return false;
            const FString Text90 = Value89->AsString();
            if (MagiAxiUnicodeScalarCount(Text90) > 16) return false;
            if (Text90 != TEXT("integer") && Text90 != TEXT("real")) return false;
        }
        const TSharedPtr<FJsonValue> Value91 = Object31->TryGetField(TEXT("requestValue"));
        if (Value91.IsValid())
        {
            double Number92 = 0;
            if (!Value91.IsValid() || !Value91->TryGetNumber(Number92) || !FMath::IsFinite(Number92)) return false;
            if (Number92 < -1000000.0) return false;
            if (Number92 > 1000000.0) return false;
        }
        const TSharedPtr<FJsonValue> Value93 = Object31->TryGetField(TEXT("requestFunction"));
        if (Value93.IsValid())
        {
            if (!Value93.IsValid() || Value93->Type != EJson::String) return false;
            const FString Text94 = Value93->AsString();
            if (MagiAxiUnicodeScalarCount(Text94) > 128) return false;
            if (Text94 != TEXT("Interact")) return false;
        }
        const TSharedPtr<FJsonValue> Value95 = Object31->TryGetField(TEXT("requestInterfaceId"));
        if (Value95.IsValid())
        {
            if (!Value95.IsValid() || Value95->Type != EJson::String) return false;
            const FString Text96 = Value95->AsString();
            if (MagiAxiUnicodeScalarCount(Text96) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text96) > 512) return false;
        }
        const TSharedPtr<FJsonValue> Value97 = Object31->TryGetField(TEXT("requestName"));
        if (Value97.IsValid())
        {
            if (!Value97.IsValid() || Value97->Type != EJson::String) return false;
            const FString Text98 = Value97->AsString();
            if (MagiAxiUnicodeScalarCount(Text98) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text98) > 128) return false;
        }
        const TSharedPtr<FJsonValue> Value99 = Object31->TryGetField(TEXT("requestClass"));
        if (Value99.IsValid())
        {
            if (!Value99.IsValid() || Value99->Type != EJson::String) return false;
            const FString Text100 = Value99->AsString();
            if (MagiAxiUnicodeScalarCount(Text100) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text100) > 128) return false;
        }
        const TSharedPtr<FJsonValue> Value101 = Object31->TryGetField(TEXT("requestParent"));
        if (Value101.IsValid())
        {
            if (Value101->Type != EJson::Null)
            {
                if (!Value101.IsValid() || Value101->Type != EJson::String) return false;
                const FString Text102 = Value101->AsString();
                if (MagiAxiUnicodeScalarCount(Text102) > 128) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value103 = Object31->TryGetField(TEXT("requestVariableGuid"));
        if (Value103.IsValid())
        {
            if (!Value103.IsValid() || Value103->Type != EJson::String) return false;
            const FString Text104 = Value103->AsString();
            if (MagiAxiUnicodeScalarCount(Text104) < 36) return false;
            if (MagiAxiUnicodeScalarCount(Text104) > 36) return false;
        }
        const TSharedPtr<FJsonValue> Value105 = Object31->TryGetField(TEXT("requestForce"));
        if (Value105.IsValid())
        {
            if (!Value105.IsValid() || Value105->Type != EJson::Boolean) return false;
        }
        const TSharedPtr<FJsonValue> Value106 = Object31->TryGetField(TEXT("requestDryRun"));
        if (Value106.IsValid())
        {
            if (!Value106.IsValid() || Value106->Type != EJson::Boolean) return false;
        }
        const TSharedPtr<FJsonValue> Value107 = Object31->TryGetField(TEXT("requestLocation"));
        if (Value107.IsValid())
        {
            if (!Value107.IsValid() || Value107->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array108 = Value107->AsArray();
            if (Array108.Num() < 3) return false;
            if (Array108.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value109 : Array108)
            {
                double Number110 = 0;
                if (!Value109.IsValid() || !Value109->TryGetNumber(Number110) || !FMath::IsFinite(Number110)) return false;
                if (Number110 < -100000.0) return false;
                if (Number110 > 100000.0) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value111 = Object31->TryGetField(TEXT("requestRotation"));
        if (Value111.IsValid())
        {
            if (!Value111.IsValid() || Value111->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array112 = Value111->AsArray();
            if (Array112.Num() < 3) return false;
            if (Array112.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value113 : Array112)
            {
                double Number114 = 0;
                if (!Value113.IsValid() || !Value113->TryGetNumber(Number114) || !FMath::IsFinite(Number114)) return false;
                if (Number114 < -360.0) return false;
                if (Number114 > 360.0) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value115 = Object31->TryGetField(TEXT("requestScale"));
        if (Value115.IsValid())
        {
            if (!Value115.IsValid() || Value115->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array116 = Value115->AsArray();
            if (Array116.Num() < 3) return false;
            if (Array116.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value117 : Array116)
            {
                double Number118 = 0;
                if (!Value117.IsValid() || !Value117->TryGetNumber(Number118) || !FMath::IsFinite(Number118)) return false;
                if (Number118 < 0.0) return false;
                if (Number118 > 1000.0) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value119 = Object31->TryGetField(TEXT("requestCollisionEnabled"));
        if (Value119.IsValid())
        {
            if (!Value119.IsValid() || Value119->Type != EJson::String) return false;
            const FString Text120 = Value119->AsString();
            if (MagiAxiUnicodeScalarCount(Text120) > 32) return false;
            if (Text120 != TEXT("NoCollision") && Text120 != TEXT("QueryOnly") && Text120 != TEXT("PhysicsOnly") && Text120 != TEXT("QueryAndPhysics")) return false;
        }
        const TSharedPtr<FJsonValue> Value121 = Object31->TryGetField(TEXT("requestCollisionProfile"));
        if (Value121.IsValid())
        {
            if (!Value121.IsValid() || Value121->Type != EJson::String) return false;
            const FString Text122 = Value121->AsString();
            if (MagiAxiUnicodeScalarCount(Text122) > 128) return false;
            if (Text122 != TEXT("OverlapAllDynamic")) return false;
        }
        const TSharedPtr<FJsonValue> Value123 = Object31->TryGetField(TEXT("requestGenerateOverlapEvents"));
        if (Value123.IsValid())
        {
            if (!Value123.IsValid() || Value123->Type != EJson::Boolean) return false;
        }
        const TSharedPtr<FJsonValue> Value124 = Object31->TryGetField(TEXT("requestSimulatePhysics"));
        if (Value124.IsValid())
        {
            if (!Value124.IsValid() || Value124->Type != EJson::Boolean) return false;
        }
        const TSharedPtr<FJsonValue> Value125 = Object31->TryGetField(TEXT("requestGravityEnabled"));
        if (Value125.IsValid())
        {
            if (!Value125.IsValid() || Value125->Type != EJson::Boolean) return false;
        }
        const TSharedPtr<FJsonValue> Value126 = Object31->TryGetField(TEXT("requestMassOverride"));
        if (Value126.IsValid())
        {
            double Number127 = 0;
            if (!Value126.IsValid() || !Value126->TryGetNumber(Number127) || !FMath::IsFinite(Number127)) return false;
            if (Number127 < 0.001) return false;
            if (Number127 > 100000.0) return false;
        }
        const TSharedPtr<FJsonValue> Value128 = Object31->TryGetField(TEXT("requestBoxExtent"));
        if (Value128.IsValid())
        {
            if (!Value128.IsValid() || Value128->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array129 = Value128->AsArray();
            if (Array129.Num() < 3) return false;
            if (Array129.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value130 : Array129)
            {
                double Number131 = 0;
                if (!Value130.IsValid() || !Value130->TryGetNumber(Number131) || !FMath::IsFinite(Number131)) return false;
                if (Number131 < 0.001) return false;
                if (Number131 > 100000.0) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value132 = Object31->TryGetField(TEXT("requestSphereRadius"));
        if (Value132.IsValid())
        {
            double Number133 = 0;
            if (!Value132.IsValid() || !Value132->TryGetNumber(Number133) || !FMath::IsFinite(Number133)) return false;
            if (Number133 < 0.001) return false;
            if (Number133 > 100000.0) return false;
        }
        const TSharedPtr<FJsonValue> Value134 = Object31->TryGetField(TEXT("requestRootName"));
        if (Value134.IsValid())
        {
            if (!Value134.IsValid() || Value134->Type != EJson::String) return false;
            const FString Text135 = Value134->AsString();
            if (MagiAxiUnicodeScalarCount(Text135) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text135) > 64) return false;
        }
        const TSharedPtr<FJsonValue> Value136 = Object31->TryGetField(TEXT("requestRootClass"));
        if (Value136.IsValid())
        {
            if (!Value136.IsValid() || Value136->Type != EJson::String) return false;
            const FString Text137 = Value136->AsString();
            if (MagiAxiUnicodeScalarCount(Text137) > 64) return false;
            if (Text137 != TEXT("VerticalBox")) return false;
        }
        const TSharedPtr<FJsonValue> Value138 = Object31->TryGetField(TEXT("requestParentWidgetId"));
        if (Value138.IsValid())
        {
            if (!Value138.IsValid() || Value138->Type != EJson::String) return false;
            const FString Text139 = Value138->AsString();
            if (MagiAxiUnicodeScalarCount(Text139) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text139) > 1024) return false;
        }
        const TSharedPtr<FJsonValue> Value140 = Object31->TryGetField(TEXT("requestWidgetId"));
        if (Value140.IsValid())
        {
            if (!Value140.IsValid() || Value140->Type != EJson::String) return false;
            const FString Text141 = Value140->AsString();
            if (MagiAxiUnicodeScalarCount(Text141) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text141) > 1024) return false;
        }
        const TSharedPtr<FJsonValue> Value142 = Object31->TryGetField(TEXT("requestProperty"));
        if (Value142.IsValid())
        {
            if (!Value142.IsValid() || Value142->Type != EJson::String) return false;
            const FString Text143 = Value142->AsString();
            if (MagiAxiUnicodeScalarCount(Text143) > 16) return false;
            if (Text143 != TEXT("text") && Text143 != TEXT("visibility") && Text143 != TEXT("enabled")) return false;
        }
        const TSharedPtr<FJsonValue> Value144 = Object31->TryGetField(TEXT("requestText"));
        if (Value144.IsValid())
        {
            if (!Value144.IsValid() || Value144->Type != EJson::String) return false;
            const FString Text145 = Value144->AsString();
            if (MagiAxiUnicodeScalarCount(Text145) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value146 = Object31->TryGetField(TEXT("requestVisibility"));
        if (Value146.IsValid())
        {
            if (!Value146.IsValid() || Value146->Type != EJson::String) return false;
            const FString Text147 = Value146->AsString();
            if (MagiAxiUnicodeScalarCount(Text147) > 9) return false;
            if (Text147 != TEXT("Visible") && Text147 != TEXT("Hidden") && Text147 != TEXT("Collapsed")) return false;
        }
        const TSharedPtr<FJsonValue> Value148 = Object31->TryGetField(TEXT("requestEnabled"));
        if (Value148.IsValid())
        {
            if (!Value148.IsValid() || Value148->Type != EJson::Boolean) return false;
        }
        const TSharedPtr<FJsonValue> Value149 = Object31->TryGetField(TEXT("requestActions"));
        if (Value149.IsValid())
        {
            if (!Value149.IsValid() || Value149->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array150 = Value149->AsArray();
            if (Array150.Num() < 1) return false;
            if (Array150.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value151 : Array150)
            {
                if (!Value151.IsValid() || Value151->Type != EJson::Object) return false;
                const TSharedPtr<FJsonObject> Object152 = Value151->AsObject();
                for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object152->Values) if (Entry.Key != TEXT("kind") && Entry.Key != TEXT("targetWidgetId") && Entry.Key != TEXT("text") && Entry.Key != TEXT("visibility") && Entry.Key != TEXT("enabled")) return false;
                const TSharedPtr<FJsonValue> Value153 = Object152->TryGetField(TEXT("kind"));
                if (!Value153.IsValid()) return false;
                if (!Value153.IsValid() || Value153->Type != EJson::String) return false;
                const FString Text154 = Value153->AsString();
                if (MagiAxiUnicodeScalarCount(Text154) > 16) return false;
                if (Text154 != TEXT("text.set") && Text154 != TEXT("enabled.set") && Text154 != TEXT("visibility.set")) return false;
                const TSharedPtr<FJsonValue> Value155 = Object152->TryGetField(TEXT("targetWidgetId"));
                if (!Value155.IsValid()) return false;
                if (!Value155.IsValid() || Value155->Type != EJson::String) return false;
                const FString Text156 = Value155->AsString();
                if (MagiAxiUnicodeScalarCount(Text156) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text156) > 1024) return false;
                const TSharedPtr<FJsonValue> Value157 = Object152->TryGetField(TEXT("text"));
                if (Value157.IsValid())
                {
                    if (!Value157.IsValid() || Value157->Type != EJson::String) return false;
                    const FString Text158 = Value157->AsString();
                    if (MagiAxiUnicodeScalarCount(Text158) > 256) return false;
                }
                const TSharedPtr<FJsonValue> Value159 = Object152->TryGetField(TEXT("visibility"));
                if (Value159.IsValid())
                {
                    if (!Value159.IsValid() || Value159->Type != EJson::String) return false;
                    const FString Text160 = Value159->AsString();
                    if (MagiAxiUnicodeScalarCount(Text160) > 9) return false;
                    if (Text160 != TEXT("Visible") && Text160 != TEXT("Hidden") && Text160 != TEXT("Collapsed")) return false;
                }
                const TSharedPtr<FJsonValue> Value161 = Object152->TryGetField(TEXT("enabled"));
                if (Value161.IsValid())
                {
                    if (!Value161.IsValid() || Value161->Type != EJson::Boolean) return false;
                }
            }
        }
        const TSharedPtr<FJsonValue> Value162 = Object31->TryGetField(TEXT("requestHostBlueprintId"));
        if (Value162.IsValid())
        {
            if (!Value162.IsValid() || Value162->Type != EJson::String) return false;
            const FString Text163 = Value162->AsString();
            if (MagiAxiUnicodeScalarCount(Text163) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text163) > 512) return false;
        }
        const TSharedPtr<FJsonValue> Value164 = Object31->TryGetField(TEXT("requestWidgetBlueprintId"));
        if (Value164.IsValid())
        {
            if (!Value164.IsValid() || Value164->Type != EJson::String) return false;
            const FString Text165 = Value164->AsString();
            if (MagiAxiUnicodeScalarCount(Text165) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text165) > 512) return false;
        }
        const TSharedPtr<FJsonValue> Value166 = Object31->TryGetField(TEXT("requestInputKey"));
        if (Value166.IsValid())
        {
            if (!Value166.IsValid() || Value166->Type != EJson::String) return false;
            const FString Text167 = Value166->AsString();
            if (MagiAxiUnicodeScalarCount(Text167) > 1) return false;
            if (Text167 != TEXT("E")) return false;
        }
        const TSharedPtr<FJsonValue> Value168 = Object31->TryGetField(TEXT("requestZOrder"));
        if (Value168.IsValid())
        {
            double Number169 = 0;
            if (!Value168.IsValid() || !Value168->TryGetNumber(Number169) || !FMath::IsFinite(Number169) || FMath::FloorToDouble(Number169) != Number169) return false;
            if (Number169 < 0.0) return false;
            if (Number169 > 0.0) return false;
        }
        return true;
    }
    if (Operation == TEXT("play.ui_observe"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("sessionId") && Entry.Key != TEXT("widgetBlueprintId") && Entry.Key != TEXT("instanceId") && Entry.Key != TEXT("inViewport") && Entry.Key != TEXT("widgets") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("sessionId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 128) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("widgetBlueprintId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("instanceId"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 2048) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("inViewport"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value8 = Object0->TryGetField(TEXT("widgets"));
        if (!Value8.IsValid()) return false;
        if (!Value8.IsValid() || Value8->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array9 = Value8->AsArray();
        if (Array9.Num() < 1) return false;
        if (Array9.Num() > 16) return false;
        for (const TSharedPtr<FJsonValue>& Value10 : Array9)
        {
            if (!Value10.IsValid() || Value10->Type != EJson::Object) return false;
            const TSharedPtr<FJsonObject> Object11 = Value10->AsObject();
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object11->Values) if (Entry.Key != TEXT("widgetId") && Entry.Key != TEXT("name") && Entry.Key != TEXT("class") && Entry.Key != TEXT("text") && Entry.Key != TEXT("visibility") && Entry.Key != TEXT("enabled")) return false;
            const TSharedPtr<FJsonValue> Value12 = Object11->TryGetField(TEXT("widgetId"));
            if (!Value12.IsValid()) return false;
            if (!Value12.IsValid() || Value12->Type != EJson::String) return false;
            const FString Text13 = Value12->AsString();
            if (MagiAxiUnicodeScalarCount(Text13) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text13) > 1024) return false;
            const TSharedPtr<FJsonValue> Value14 = Object11->TryGetField(TEXT("name"));
            if (!Value14.IsValid()) return false;
            if (!Value14.IsValid() || Value14->Type != EJson::String) return false;
            const FString Text15 = Value14->AsString();
            if (MagiAxiUnicodeScalarCount(Text15) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text15) > 64) return false;
            const TSharedPtr<FJsonValue> Value16 = Object11->TryGetField(TEXT("class"));
            if (!Value16.IsValid()) return false;
            if (!Value16.IsValid() || Value16->Type != EJson::String) return false;
            const FString Text17 = Value16->AsString();
            if (MagiAxiUnicodeScalarCount(Text17) > 64) return false;
            if (Text17 != TEXT("VerticalBox") && Text17 != TEXT("TextBlock")) return false;
            const TSharedPtr<FJsonValue> Value18 = Object11->TryGetField(TEXT("text"));
            if (!Value18.IsValid()) return false;
            if (Value18->Type != EJson::Null)
            {
                if (!Value18.IsValid() || Value18->Type != EJson::String) return false;
                const FString Text19 = Value18->AsString();
                if (MagiAxiUnicodeScalarCount(Text19) > 256) return false;
            }
            const TSharedPtr<FJsonValue> Value20 = Object11->TryGetField(TEXT("visibility"));
            if (!Value20.IsValid()) return false;
            if (!Value20.IsValid() || Value20->Type != EJson::String) return false;
            const FString Text21 = Value20->AsString();
            if (MagiAxiUnicodeScalarCount(Text21) > 9) return false;
            if (Text21 != TEXT("Visible") && Text21 != TEXT("Hidden") && Text21 != TEXT("Collapsed")) return false;
            const TSharedPtr<FJsonValue> Value22 = Object11->TryGetField(TEXT("enabled"));
            if (!Value22.IsValid()) return false;
            if (!Value22.IsValid() || Value22->Type != EJson::Boolean) return false;
        }
        const TSharedPtr<FJsonValue> Value23 = Object0->TryGetField(TEXT("revision"));
        if (!Value23.IsValid()) return false;
        if (!Value23.IsValid() || Value23->Type != EJson::String) return false;
        const FString Text24 = Value23->AsString();
        if (MagiAxiUnicodeScalarCount(Text24) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text24) > 64) return false;
        return true;
    }
    if (Operation == TEXT("widget.create"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("generatedClass") && Entry.Key != TEXT("rootWidgetId") && Entry.Key != TEXT("rootName") && Entry.Key != TEXT("rootClass") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("generatedClass"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("rootWidgetId"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 1024) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("rootName"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text8) > 64) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("rootClass"));
        if (!Value9.IsValid()) return false;
        if (!Value9.IsValid() || Value9->Type != EJson::String) return false;
        const FString Text10 = Value9->AsString();
        if (MagiAxiUnicodeScalarCount(Text10) > 64) return false;
        if (Text10 != TEXT("VerticalBox")) return false;
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("changed"));
        if (!Value11.IsValid()) return false;
        if (!Value11.IsValid() || Value11->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value12 = Object0->TryGetField(TEXT("dirtyPackages"));
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
        const TSharedPtr<FJsonValue> Value16 = Object0->TryGetField(TEXT("savedPackages"));
        if (!Value16.IsValid()) return false;
        if (!Value16.IsValid() || Value16->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array17 = Value16->AsArray();
        if (Array17.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value18 : Array17)
        {
            if (!Value18.IsValid() || Value18->Type != EJson::String) return false;
            const FString Text19 = Value18->AsString();
            if (MagiAxiUnicodeScalarCount(Text19) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value20 = Object0->TryGetField(TEXT("revision"));
        if (!Value20.IsValid()) return false;
        if (!Value20.IsValid() || Value20->Type != EJson::String) return false;
        const FString Text21 = Value20->AsString();
        if (MagiAxiUnicodeScalarCount(Text21) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text21) > 64) return false;
        return true;
    }
    if (Operation == TEXT("widget.tree_view"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("generatedClass") && Entry.Key != TEXT("rootWidgetId") && Entry.Key != TEXT("count") && Entry.Key != TEXT("total") && Entry.Key != TEXT("scope") && Entry.Key != TEXT("widgets") && Entry.Key != TEXT("events") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("generatedClass"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("rootWidgetId"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 1024) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("count"));
        if (!Value7.IsValid()) return false;
        double Number8 = 0;
        if (!Value7.IsValid() || !Value7->TryGetNumber(Number8) || !FMath::IsFinite(Number8) || FMath::FloorToDouble(Number8) != Number8) return false;
        if (Number8 < 1.0) return false;
        if (Number8 > 100.0) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("total"));
        if (!Value9.IsValid()) return false;
        double Number10 = 0;
        if (!Value9.IsValid() || !Value9->TryGetNumber(Number10) || !FMath::IsFinite(Number10) || FMath::FloorToDouble(Number10) != Number10) return false;
        if (Number10 < 1.0) return false;
        if (Number10 > 100.0) return false;
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("scope"));
        if (!Value11.IsValid()) return false;
        if (!Value11.IsValid() || Value11->Type != EJson::String) return false;
        const FString Text12 = Value11->AsString();
        if (MagiAxiUnicodeScalarCount(Text12) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text12) > 512) return false;
        const TSharedPtr<FJsonValue> Value13 = Object0->TryGetField(TEXT("widgets"));
        if (!Value13.IsValid()) return false;
        if (!Value13.IsValid() || Value13->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array14 = Value13->AsArray();
        if (Array14.Num() < 1) return false;
        if (Array14.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value15 : Array14)
        {
            if (!Value15.IsValid() || Value15->Type != EJson::Object) return false;
            const TSharedPtr<FJsonObject> Object16 = Value15->AsObject();
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object16->Values) if (Entry.Key != TEXT("widgetId") && Entry.Key != TEXT("name") && Entry.Key != TEXT("class") && Entry.Key != TEXT("parentWidgetId") && Entry.Key != TEXT("index") && Entry.Key != TEXT("text") && Entry.Key != TEXT("visibility") && Entry.Key != TEXT("enabled")) return false;
            const TSharedPtr<FJsonValue> Value17 = Object16->TryGetField(TEXT("widgetId"));
            if (!Value17.IsValid()) return false;
            if (!Value17.IsValid() || Value17->Type != EJson::String) return false;
            const FString Text18 = Value17->AsString();
            if (MagiAxiUnicodeScalarCount(Text18) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text18) > 1024) return false;
            const TSharedPtr<FJsonValue> Value19 = Object16->TryGetField(TEXT("name"));
            if (!Value19.IsValid()) return false;
            if (!Value19.IsValid() || Value19->Type != EJson::String) return false;
            const FString Text20 = Value19->AsString();
            if (MagiAxiUnicodeScalarCount(Text20) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text20) > 64) return false;
            const TSharedPtr<FJsonValue> Value21 = Object16->TryGetField(TEXT("class"));
            if (!Value21.IsValid()) return false;
            if (!Value21.IsValid() || Value21->Type != EJson::String) return false;
            const FString Text22 = Value21->AsString();
            if (MagiAxiUnicodeScalarCount(Text22) > 64) return false;
            if (Text22 != TEXT("VerticalBox") && Text22 != TEXT("TextBlock")) return false;
            const TSharedPtr<FJsonValue> Value23 = Object16->TryGetField(TEXT("parentWidgetId"));
            if (!Value23.IsValid()) return false;
            if (Value23->Type != EJson::Null)
            {
                if (!Value23.IsValid() || Value23->Type != EJson::String) return false;
                const FString Text24 = Value23->AsString();
                if (MagiAxiUnicodeScalarCount(Text24) > 1024) return false;
            }
            const TSharedPtr<FJsonValue> Value25 = Object16->TryGetField(TEXT("index"));
            if (!Value25.IsValid()) return false;
            double Number26 = 0;
            if (!Value25.IsValid() || !Value25->TryGetNumber(Number26) || !FMath::IsFinite(Number26) || FMath::FloorToDouble(Number26) != Number26) return false;
            if (Number26 < 0.0) return false;
            if (Number26 > 99.0) return false;
            const TSharedPtr<FJsonValue> Value27 = Object16->TryGetField(TEXT("text"));
            if (!Value27.IsValid()) return false;
            if (Value27->Type != EJson::Null)
            {
                if (!Value27.IsValid() || Value27->Type != EJson::String) return false;
                const FString Text28 = Value27->AsString();
                if (MagiAxiUnicodeScalarCount(Text28) > 256) return false;
            }
            const TSharedPtr<FJsonValue> Value29 = Object16->TryGetField(TEXT("visibility"));
            if (!Value29.IsValid()) return false;
            if (!Value29.IsValid() || Value29->Type != EJson::String) return false;
            const FString Text30 = Value29->AsString();
            if (MagiAxiUnicodeScalarCount(Text30) > 9) return false;
            if (Text30 != TEXT("Visible") && Text30 != TEXT("Hidden") && Text30 != TEXT("Collapsed")) return false;
            const TSharedPtr<FJsonValue> Value31 = Object16->TryGetField(TEXT("enabled"));
            if (!Value31.IsValid()) return false;
            if (!Value31.IsValid() || Value31->Type != EJson::Boolean) return false;
        }
        const TSharedPtr<FJsonValue> Value32 = Object0->TryGetField(TEXT("events"));
        if (!Value32.IsValid()) return false;
        if (!Value32.IsValid() || Value32->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array33 = Value32->AsArray();
        if (Array33.Num() > 32) return false;
        for (const TSharedPtr<FJsonValue>& Value34 : Array33)
        {
            if (!Value34.IsValid() || Value34->Type != EJson::Object) return false;
            const TSharedPtr<FJsonObject> Object35 = Value34->AsObject();
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object35->Values) if (Entry.Key != TEXT("eventId") && Entry.Key != TEXT("agentKey") && Entry.Key != TEXT("event") && Entry.Key != TEXT("actions")) return false;
            const TSharedPtr<FJsonValue> Value36 = Object35->TryGetField(TEXT("eventId"));
            if (!Value36.IsValid()) return false;
            if (!Value36.IsValid() || Value36->Type != EJson::String) return false;
            const FString Text37 = Value36->AsString();
            if (MagiAxiUnicodeScalarCount(Text37) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text37) > 1024) return false;
            const TSharedPtr<FJsonValue> Value38 = Object35->TryGetField(TEXT("agentKey"));
            if (!Value38.IsValid()) return false;
            if (!Value38.IsValid() || Value38->Type != EJson::String) return false;
            const FString Text39 = Value38->AsString();
            if (MagiAxiUnicodeScalarCount(Text39) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text39) > 128) return false;
            const TSharedPtr<FJsonValue> Value40 = Object35->TryGetField(TEXT("event"));
            if (!Value40.IsValid()) return false;
            if (!Value40.IsValid() || Value40->Type != EJson::String) return false;
            const FString Text41 = Value40->AsString();
            if (MagiAxiUnicodeScalarCount(Text41) > 8) return false;
            if (Text41 != TEXT("activate")) return false;
            const TSharedPtr<FJsonValue> Value42 = Object35->TryGetField(TEXT("actions"));
            if (!Value42.IsValid()) return false;
            if (!Value42.IsValid() || Value42->Type != EJson::Array) return false;
            const TArray<TSharedPtr<FJsonValue>>& Array43 = Value42->AsArray();
            if (Array43.Num() < 1) return false;
            if (Array43.Num() > 3) return false;
            for (const TSharedPtr<FJsonValue>& Value44 : Array43)
            {
                if (!Value44.IsValid() || Value44->Type != EJson::Object) return false;
                const TSharedPtr<FJsonObject> Object45 = Value44->AsObject();
                for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object45->Values) if (Entry.Key != TEXT("kind") && Entry.Key != TEXT("targetWidgetId") && Entry.Key != TEXT("text") && Entry.Key != TEXT("visibility") && Entry.Key != TEXT("enabled")) return false;
                const TSharedPtr<FJsonValue> Value46 = Object45->TryGetField(TEXT("kind"));
                if (!Value46.IsValid()) return false;
                if (!Value46.IsValid() || Value46->Type != EJson::String) return false;
                const FString Text47 = Value46->AsString();
                if (MagiAxiUnicodeScalarCount(Text47) > 16) return false;
                if (Text47 != TEXT("text.set") && Text47 != TEXT("enabled.set") && Text47 != TEXT("visibility.set")) return false;
                const TSharedPtr<FJsonValue> Value48 = Object45->TryGetField(TEXT("targetWidgetId"));
                if (!Value48.IsValid()) return false;
                if (!Value48.IsValid() || Value48->Type != EJson::String) return false;
                const FString Text49 = Value48->AsString();
                if (MagiAxiUnicodeScalarCount(Text49) < 1) return false;
                if (MagiAxiUnicodeScalarCount(Text49) > 1024) return false;
                const TSharedPtr<FJsonValue> Value50 = Object45->TryGetField(TEXT("text"));
                if (Value50.IsValid())
                {
                    if (!Value50.IsValid() || Value50->Type != EJson::String) return false;
                    const FString Text51 = Value50->AsString();
                    if (MagiAxiUnicodeScalarCount(Text51) > 256) return false;
                }
                const TSharedPtr<FJsonValue> Value52 = Object45->TryGetField(TEXT("visibility"));
                if (Value52.IsValid())
                {
                    if (!Value52.IsValid() || Value52->Type != EJson::String) return false;
                    const FString Text53 = Value52->AsString();
                    if (MagiAxiUnicodeScalarCount(Text53) > 9) return false;
                    if (Text53 != TEXT("Visible") && Text53 != TEXT("Hidden") && Text53 != TEXT("Collapsed")) return false;
                }
                const TSharedPtr<FJsonValue> Value54 = Object45->TryGetField(TEXT("enabled"));
                if (Value54.IsValid())
                {
                    if (!Value54.IsValid() || Value54->Type != EJson::Boolean) return false;
                }
            }
        }
        const TSharedPtr<FJsonValue> Value55 = Object0->TryGetField(TEXT("revision"));
        if (!Value55.IsValid()) return false;
        if (!Value55.IsValid() || Value55->Type != EJson::String) return false;
        const FString Text56 = Value55->AsString();
        if (MagiAxiUnicodeScalarCount(Text56) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text56) > 64) return false;
        return true;
    }
    if (Operation == TEXT("widget.child_ensure"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("widgetId") && Entry.Key != TEXT("parentWidgetId") && Entry.Key != TEXT("name") && Entry.Key != TEXT("class") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("widgetId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 1024) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("parentWidgetId"));
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
        if (MagiAxiUnicodeScalarCount(Text8) > 64) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("class"));
        if (!Value9.IsValid()) return false;
        if (!Value9.IsValid() || Value9->Type != EJson::String) return false;
        const FString Text10 = Value9->AsString();
        if (MagiAxiUnicodeScalarCount(Text10) > 64) return false;
        if (Text10 != TEXT("TextBlock")) return false;
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("changed"));
        if (!Value11.IsValid()) return false;
        if (!Value11.IsValid() || Value11->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value12 = Object0->TryGetField(TEXT("dirtyPackages"));
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
        const TSharedPtr<FJsonValue> Value16 = Object0->TryGetField(TEXT("savedPackages"));
        if (!Value16.IsValid()) return false;
        if (!Value16.IsValid() || Value16->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array17 = Value16->AsArray();
        if (Array17.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value18 : Array17)
        {
            if (!Value18.IsValid() || Value18->Type != EJson::String) return false;
            const FString Text19 = Value18->AsString();
            if (MagiAxiUnicodeScalarCount(Text19) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value20 = Object0->TryGetField(TEXT("revision"));
        if (!Value20.IsValid()) return false;
        if (!Value20.IsValid() || Value20->Type != EJson::String) return false;
        const FString Text21 = Value20->AsString();
        if (MagiAxiUnicodeScalarCount(Text21) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text21) > 64) return false;
        return true;
    }
    if (Operation == TEXT("widget.property_set"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("widgetId") && Entry.Key != TEXT("property") && Entry.Key != TEXT("text") && Entry.Key != TEXT("visibility") && Entry.Key != TEXT("enabled") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("widgetId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 1024) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("property"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) > 16) return false;
        if (Text6 != TEXT("text") && Text6 != TEXT("visibility") && Text6 != TEXT("enabled")) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("text"));
        if (Value7.IsValid())
        {
            if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
            const FString Text8 = Value7->AsString();
            if (MagiAxiUnicodeScalarCount(Text8) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("visibility"));
        if (Value9.IsValid())
        {
            if (!Value9.IsValid() || Value9->Type != EJson::String) return false;
            const FString Text10 = Value9->AsString();
            if (MagiAxiUnicodeScalarCount(Text10) > 9) return false;
            if (Text10 != TEXT("Visible") && Text10 != TEXT("Hidden") && Text10 != TEXT("Collapsed")) return false;
        }
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("enabled"));
        if (Value11.IsValid())
        {
            if (!Value11.IsValid() || Value11->Type != EJson::Boolean) return false;
        }
        const TSharedPtr<FJsonValue> Value12 = Object0->TryGetField(TEXT("changed"));
        if (!Value12.IsValid()) return false;
        if (!Value12.IsValid() || Value12->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value13 = Object0->TryGetField(TEXT("dirtyPackages"));
        if (!Value13.IsValid()) return false;
        if (!Value13.IsValid() || Value13->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array14 = Value13->AsArray();
        if (Array14.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value15 : Array14)
        {
            if (!Value15.IsValid() || Value15->Type != EJson::String) return false;
            const FString Text16 = Value15->AsString();
            if (MagiAxiUnicodeScalarCount(Text16) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value17 = Object0->TryGetField(TEXT("savedPackages"));
        if (!Value17.IsValid()) return false;
        if (!Value17.IsValid() || Value17->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array18 = Value17->AsArray();
        if (Array18.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value19 : Array18)
        {
            if (!Value19.IsValid() || Value19->Type != EJson::String) return false;
            const FString Text20 = Value19->AsString();
            if (MagiAxiUnicodeScalarCount(Text20) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value21 = Object0->TryGetField(TEXT("revision"));
        if (!Value21.IsValid()) return false;
        if (!Value21.IsValid() || Value21->Type != EJson::String) return false;
        const FString Text22 = Value21->AsString();
        if (MagiAxiUnicodeScalarCount(Text22) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text22) > 64) return false;
        return true;
    }
    if (Operation == TEXT("widget.event_ensure"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("blueprintId") && Entry.Key != TEXT("eventId") && Entry.Key != TEXT("agentKey") && Entry.Key != TEXT("event") && Entry.Key != TEXT("actions") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("blueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("eventId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 1024) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("agentKey"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 128) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("event"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) > 8) return false;
        if (Text8 != TEXT("activate")) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("actions"));
        if (!Value9.IsValid()) return false;
        if (!Value9.IsValid() || Value9->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array10 = Value9->AsArray();
        if (Array10.Num() < 1) return false;
        if (Array10.Num() > 3) return false;
        for (const TSharedPtr<FJsonValue>& Value11 : Array10)
        {
            if (!Value11.IsValid() || Value11->Type != EJson::Object) return false;
            const TSharedPtr<FJsonObject> Object12 = Value11->AsObject();
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object12->Values) if (Entry.Key != TEXT("kind") && Entry.Key != TEXT("targetWidgetId") && Entry.Key != TEXT("text") && Entry.Key != TEXT("visibility") && Entry.Key != TEXT("enabled")) return false;
            const TSharedPtr<FJsonValue> Value13 = Object12->TryGetField(TEXT("kind"));
            if (!Value13.IsValid()) return false;
            if (!Value13.IsValid() || Value13->Type != EJson::String) return false;
            const FString Text14 = Value13->AsString();
            if (MagiAxiUnicodeScalarCount(Text14) > 16) return false;
            if (Text14 != TEXT("text.set") && Text14 != TEXT("enabled.set") && Text14 != TEXT("visibility.set")) return false;
            const TSharedPtr<FJsonValue> Value15 = Object12->TryGetField(TEXT("targetWidgetId"));
            if (!Value15.IsValid()) return false;
            if (!Value15.IsValid() || Value15->Type != EJson::String) return false;
            const FString Text16 = Value15->AsString();
            if (MagiAxiUnicodeScalarCount(Text16) < 1) return false;
            if (MagiAxiUnicodeScalarCount(Text16) > 1024) return false;
            const TSharedPtr<FJsonValue> Value17 = Object12->TryGetField(TEXT("text"));
            if (Value17.IsValid())
            {
                if (!Value17.IsValid() || Value17->Type != EJson::String) return false;
                const FString Text18 = Value17->AsString();
                if (MagiAxiUnicodeScalarCount(Text18) > 256) return false;
            }
            const TSharedPtr<FJsonValue> Value19 = Object12->TryGetField(TEXT("visibility"));
            if (Value19.IsValid())
            {
                if (!Value19.IsValid() || Value19->Type != EJson::String) return false;
                const FString Text20 = Value19->AsString();
                if (MagiAxiUnicodeScalarCount(Text20) > 9) return false;
                if (Text20 != TEXT("Visible") && Text20 != TEXT("Hidden") && Text20 != TEXT("Collapsed")) return false;
            }
            const TSharedPtr<FJsonValue> Value21 = Object12->TryGetField(TEXT("enabled"));
            if (Value21.IsValid())
            {
                if (!Value21.IsValid() || Value21->Type != EJson::Boolean) return false;
            }
        }
        const TSharedPtr<FJsonValue> Value22 = Object0->TryGetField(TEXT("changed"));
        if (!Value22.IsValid()) return false;
        if (!Value22.IsValid() || Value22->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value23 = Object0->TryGetField(TEXT("dirtyPackages"));
        if (!Value23.IsValid()) return false;
        if (!Value23.IsValid() || Value23->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array24 = Value23->AsArray();
        if (Array24.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value25 : Array24)
        {
            if (!Value25.IsValid() || Value25->Type != EJson::String) return false;
            const FString Text26 = Value25->AsString();
            if (MagiAxiUnicodeScalarCount(Text26) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value27 = Object0->TryGetField(TEXT("savedPackages"));
        if (!Value27.IsValid()) return false;
        if (!Value27.IsValid() || Value27->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array28 = Value27->AsArray();
        if (Array28.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value29 : Array28)
        {
            if (!Value29.IsValid() || Value29->Type != EJson::String) return false;
            const FString Text30 = Value29->AsString();
            if (MagiAxiUnicodeScalarCount(Text30) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value31 = Object0->TryGetField(TEXT("revision"));
        if (!Value31.IsValid()) return false;
        if (!Value31.IsValid() || Value31->Type != EJson::String) return false;
        const FString Text32 = Value31->AsString();
        if (MagiAxiUnicodeScalarCount(Text32) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text32) > 64) return false;
        return true;
    }
    if (Operation == TEXT("widget.viewport_ensure"))
    {
        if (!Root.IsValid() || Root->Type != EJson::Object) return false;
        const TSharedPtr<FJsonObject> Object0 = Root->AsObject();
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Object0->Values) if (Entry.Key != TEXT("hostBlueprintId") && Entry.Key != TEXT("widgetBlueprintId") && Entry.Key != TEXT("viewportId") && Entry.Key != TEXT("graphId") && Entry.Key != TEXT("inputKey") && Entry.Key != TEXT("zOrder") && Entry.Key != TEXT("widgetRevision") && Entry.Key != TEXT("changed") && Entry.Key != TEXT("dirtyPackages") && Entry.Key != TEXT("savedPackages") && Entry.Key != TEXT("revision")) return false;
        const TSharedPtr<FJsonValue> Value1 = Object0->TryGetField(TEXT("hostBlueprintId"));
        if (!Value1.IsValid()) return false;
        if (!Value1.IsValid() || Value1->Type != EJson::String) return false;
        const FString Text2 = Value1->AsString();
        if (MagiAxiUnicodeScalarCount(Text2) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text2) > 512) return false;
        const TSharedPtr<FJsonValue> Value3 = Object0->TryGetField(TEXT("widgetBlueprintId"));
        if (!Value3.IsValid()) return false;
        if (!Value3.IsValid() || Value3->Type != EJson::String) return false;
        const FString Text4 = Value3->AsString();
        if (MagiAxiUnicodeScalarCount(Text4) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text4) > 512) return false;
        const TSharedPtr<FJsonValue> Value5 = Object0->TryGetField(TEXT("viewportId"));
        if (!Value5.IsValid()) return false;
        if (!Value5.IsValid() || Value5->Type != EJson::String) return false;
        const FString Text6 = Value5->AsString();
        if (MagiAxiUnicodeScalarCount(Text6) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text6) > 1024) return false;
        const TSharedPtr<FJsonValue> Value7 = Object0->TryGetField(TEXT("graphId"));
        if (!Value7.IsValid()) return false;
        if (!Value7.IsValid() || Value7->Type != EJson::String) return false;
        const FString Text8 = Value7->AsString();
        if (MagiAxiUnicodeScalarCount(Text8) < 1) return false;
        if (MagiAxiUnicodeScalarCount(Text8) > 1024) return false;
        const TSharedPtr<FJsonValue> Value9 = Object0->TryGetField(TEXT("inputKey"));
        if (!Value9.IsValid()) return false;
        if (!Value9.IsValid() || Value9->Type != EJson::String) return false;
        const FString Text10 = Value9->AsString();
        if (MagiAxiUnicodeScalarCount(Text10) > 1) return false;
        if (Text10 != TEXT("E")) return false;
        const TSharedPtr<FJsonValue> Value11 = Object0->TryGetField(TEXT("zOrder"));
        if (!Value11.IsValid()) return false;
        double Number12 = 0;
        if (!Value11.IsValid() || !Value11->TryGetNumber(Number12) || !FMath::IsFinite(Number12) || FMath::FloorToDouble(Number12) != Number12) return false;
        if (Number12 < 0.0) return false;
        if (Number12 > 0.0) return false;
        const TSharedPtr<FJsonValue> Value13 = Object0->TryGetField(TEXT("widgetRevision"));
        if (!Value13.IsValid()) return false;
        if (!Value13.IsValid() || Value13->Type != EJson::String) return false;
        const FString Text14 = Value13->AsString();
        if (MagiAxiUnicodeScalarCount(Text14) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text14) > 64) return false;
        const TSharedPtr<FJsonValue> Value15 = Object0->TryGetField(TEXT("changed"));
        if (!Value15.IsValid()) return false;
        if (!Value15.IsValid() || Value15->Type != EJson::Boolean) return false;
        const TSharedPtr<FJsonValue> Value16 = Object0->TryGetField(TEXT("dirtyPackages"));
        if (!Value16.IsValid()) return false;
        if (!Value16.IsValid() || Value16->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array17 = Value16->AsArray();
        if (Array17.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value18 : Array17)
        {
            if (!Value18.IsValid() || Value18->Type != EJson::String) return false;
            const FString Text19 = Value18->AsString();
            if (MagiAxiUnicodeScalarCount(Text19) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value20 = Object0->TryGetField(TEXT("savedPackages"));
        if (!Value20.IsValid()) return false;
        if (!Value20.IsValid() || Value20->Type != EJson::Array) return false;
        const TArray<TSharedPtr<FJsonValue>>& Array21 = Value20->AsArray();
        if (Array21.Num() > 100) return false;
        for (const TSharedPtr<FJsonValue>& Value22 : Array21)
        {
            if (!Value22.IsValid() || Value22->Type != EJson::String) return false;
            const FString Text23 = Value22->AsString();
            if (MagiAxiUnicodeScalarCount(Text23) > 256) return false;
        }
        const TSharedPtr<FJsonValue> Value24 = Object0->TryGetField(TEXT("revision"));
        if (!Value24.IsValid()) return false;
        if (!Value24.IsValid() || Value24->Type != EJson::String) return false;
        const FString Text25 = Value24->AsString();
        if (MagiAxiUnicodeScalarCount(Text25) < 64) return false;
        if (MagiAxiUnicodeScalarCount(Text25) > 64) return false;
        return true;
    }
    return false;
}
