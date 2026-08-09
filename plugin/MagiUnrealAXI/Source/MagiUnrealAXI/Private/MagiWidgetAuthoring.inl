#if WITH_DEV_AUTOMATION_TESTS
extern bool GP11ForceAtomicFailure;
#endif

bool IsP13WidgetOperation(const FString& Operation)
{
    return Operation == TEXT("widget.create") || Operation == TEXT("widget.tree_view") || Operation == TEXT("widget.child_ensure") || Operation == TEXT("widget.property_set") || Operation == TEXT("widget.event_ensure") || Operation == TEXT("widget.viewport_ensure");
}

UWidgetBlueprint* P13LoadWidgetBlueprint(const FString& Id)
{
    return Cast<UWidgetBlueprint>(P11LoadBlueprint(Id));
}

FString P13WidgetClass(const UWidget& Widget)
{
    return Cast<UVerticalBox>(&Widget) ? TEXT("VerticalBox") : Cast<UTextBlock>(&Widget) ? TEXT("TextBlock") : FString();
}

static bool P13ValidName(const FString& Name)
{
    if (Name.IsEmpty() || Name.Len() > 64 || !((Name[0] >= TCHAR('A') && Name[0] <= TCHAR('Z')) || (Name[0] >= TCHAR('a') && Name[0] <= TCHAR('z')) || Name[0] == TCHAR('_'))) return false;
    for (int32 Index = 1; Index < Name.Len(); ++Index) if (!((Name[Index] >= TCHAR('A') && Name[Index] <= TCHAR('Z')) || (Name[Index] >= TCHAR('a') && Name[Index] <= TCHAR('z')) || (Name[Index] >= TCHAR('0') && Name[Index] <= TCHAR('9')) || Name[Index] == TCHAR('_'))) return false;
    return true;
}

static bool P13Visibility(const FString& Value, ESlateVisibility& Out)
{
    if (Value == TEXT("Visible")) Out = ESlateVisibility::Visible;
    else if (Value == TEXT("Hidden")) Out = ESlateVisibility::Hidden;
    else if (Value == TEXT("Collapsed")) Out = ESlateVisibility::Collapsed;
    else return false;
    return true;
}

bool P13WidgetTree(UWidgetBlueprint& Blueprint, TArray<UWidget*>& Widgets, FString& Error)
{
    Widgets.Reset();
    UVerticalBox* Root = Blueprint.WidgetTree ? Cast<UVerticalBox>(Blueprint.WidgetTree->RootWidget) : nullptr;
    if (!Root || !Root->bIsVariable || !P13ValidName(Root->GetName())) { Error = TEXT("widget Blueprint must have exact variable VerticalBox root"); return false; }
    if (Root->GetChildrenCount() > 99) { Error = TEXT("widget tree exceeds 100 widgets"); return false; }
    TSet<FName> Names;
    const FGuid* RootGuid = Blueprint.WidgetVariableNameToGuidMap.Find(Root->GetFName());
    if (Names.Contains(Root->GetFName()) || !RootGuid || !RootGuid->IsValid()) { Error = TEXT("widget names must be unique and every variable widget needs valid GUID"); return false; } Names.Add(Root->GetFName());
    Widgets.Add(Root);
    TArray<UTextBlock*> Children;
    for (int32 Index = 0; Index < Root->GetChildrenCount(); ++Index)
    {
        UTextBlock* Child = Cast<UTextBlock>(Root->GetChildAt(Index));
        const FGuid* ChildGuid = Child ? Blueprint.WidgetVariableNameToGuidMap.Find(Child->GetFName()) : nullptr;
        if (!Child || !Child->bIsVariable || !P13ValidName(Child->GetName()) || Names.Contains(Child->GetFName()) || !ChildGuid || !ChildGuid->IsValid()) { Error = TEXT("widget tree is outside fixed native model or has missing variable GUID"); return false; } Names.Add(Child->GetFName());
        Children.Add(Child);
    }
    Children.Sort([](const UTextBlock& Left, const UTextBlock& Right) { return Left.GetName() < Right.GetName(); });
    for (UTextBlock* Child : Children) Widgets.Add(Child);
    return true;
}

static FString P13WidgetId(const UWidgetBlueprint& Blueprint, const UWidget& Widget)
{
    return Blueprint.GetPathName() + TEXT("#widget:") + Widget.GetName();
}


struct FP13WidgetEventAction
{
    FString Kind;
    FString TargetWidgetId;
    FString Text;
    FString Visibility;
    bool Enabled = false;
};

static bool P13ValidAgentKey(const FString& AgentKey)
{
    if (AgentKey.IsEmpty() || AgentKey.Len() > 128) return false;
    for (const TCHAR Character : AgentKey)
        if (!((Character >= TCHAR('A') && Character <= TCHAR('Z')) || (Character >= TCHAR('a') && Character <= TCHAR('z')) || (Character >= TCHAR('0') && Character <= TCHAR('9')) || Character == TCHAR('.') || Character == TCHAR('_') || Character == TCHAR(':') || Character == TCHAR('-'))) return false;
    return true;
}

static FString P13WidgetEventName(const UWidgetBlueprint& Blueprint, const FString& AgentKey)
{
    return TEXT("MagiP13Activate_") + Sha256(CanonicalRow({Blueprint.GetPathName(), AgentKey})).Left(24);
}

static FString P13WidgetEventOwner(const FString& AgentKey, const FString& Role)
{
    return TEXT("widget.event:") + AgentKey + TEXT(":") + Role;
}

static bool P13WidgetEventAgentFromOwner(const FString& Owner, FString& AgentKey)
{
    const FString Prefix = TEXT("widget.event:");
    const FString Suffix = TEXT(":entry");
    if (!Owner.StartsWith(Prefix) || !Owner.EndsWith(Suffix)) return false;
    const int32 AgentLength = Owner.Len() - Prefix.Len() - Suffix.Len();
    if (AgentLength <= 0) return false;
    AgentKey = Owner.Mid(Prefix.Len(), AgentLength);
    return P13ValidAgentKey(AgentKey) && Owner == P13WidgetEventOwner(AgentKey, TEXT("entry"));
}

static bool P13WidgetEventOwnerMatches(const FString& Owner, const FString& AgentKey, const FString& Role)
{
    return Owner == P13WidgetEventOwner(AgentKey, Role);
}

static FGuid P13WidgetEventGuid(const UWidgetBlueprint& Blueprint, const UEdGraph& Graph, const FString& AgentKey, const FString& Role)
{
    FGuid Guid;
    FGuid::ParseExact(Sha256(CanonicalRow({Blueprint.GetPathName(), BlueprintGraphIdentity(Blueprint, Graph), AgentKey, Role})).Left(32), EGuidFormats::Digits, Guid);
    return Guid;
}

static UEdGraph* P13WidgetEventGraph(UWidgetBlueprint& Blueprint)
{
    return Blueprint.UbergraphPages.Num() == 1 && Blueprint.UbergraphPages[0] ? Blueprint.UbergraphPages[0] : nullptr;
}

static int32 P13WidgetActionRank(const FString& Kind)
{
    return Kind == TEXT("text.set") ? 0 : Kind == TEXT("enabled.set") ? 1 : Kind == TEXT("visibility.set") ? 2 : INDEX_NONE;
}

static bool P13CanonicalWidgetEventActions(const TArray<FP13WidgetEventAction>& Actions)
{
    int32 PreviousRank = INDEX_NONE;
    FString PreviousTarget;
    TSet<FString> Seen;
    for (const FP13WidgetEventAction& Action : Actions)
    {
        const int32 Rank = P13WidgetActionRank(Action.Kind);
        const FString Key = Action.Kind + TEXT("\n") + Action.TargetWidgetId;
        if (Rank == INDEX_NONE || Seen.Contains(Key) || (PreviousRank != INDEX_NONE && (Rank < PreviousRank || (Rank == PreviousRank && Action.TargetWidgetId.Compare(PreviousTarget, ESearchCase::CaseSensitive) <= 0)))) return false;
        Seen.Add(Key);
        PreviousRank = Rank;
        PreviousTarget = Action.TargetWidgetId;
    }
    return Actions.Num() >= 1 && Actions.Num() <= 3;
}

static bool P13ParseWidgetEventActions(const TSharedPtr<FJsonObject>& Args, TArray<FP13WidgetEventAction>& Actions, FString& Error)
{
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Args.IsValid() || !Args->TryGetArrayField(TEXT("actions"), Values) || !Values || Values->Num() < 1 || Values->Num() > 3) { Error = TEXT("actions must contain one to three entries"); return false; }
    for (const TSharedPtr<FJsonValue>& Value : *Values)
    {
        const TSharedPtr<FJsonObject>* Object = nullptr;
        if (!Value.IsValid() || !Value->TryGetObject(Object) || !Object || !Object->IsValid()) { Error = TEXT("each action must be an object"); return false; }
        FP13WidgetEventAction Action;
        if (!(*Object)->TryGetStringField(TEXT("kind"), Action.Kind) || !(*Object)->TryGetStringField(TEXT("targetWidgetId"), Action.TargetWidgetId) || Action.TargetWidgetId.IsEmpty()) { Error = TEXT("action requires kind and targetWidgetId"); return false; }
        const int32 ExpectedFields = Action.Kind == TEXT("text.set") || Action.Kind == TEXT("enabled.set") || Action.Kind == TEXT("visibility.set") ? 3 : 0;
        if (ExpectedFields == 0 || (*Object)->Values.Num() != ExpectedFields) { Error = TEXT("action kind or fields are outside fixed model"); return false; }
        if (Action.Kind == TEXT("text.set") && (!(*Object)->TryGetStringField(TEXT("text"), Action.Text) || MagiAxiUnicodeScalarCount(Action.Text) > 256)) { Error = TEXT("text.set requires text of at most 256 characters"); return false; }
        if (Action.Kind == TEXT("enabled.set") && !(*Object)->TryGetBoolField(TEXT("enabled"), Action.Enabled)) { Error = TEXT("enabled.set requires enabled"); return false; }
        if (Action.Kind == TEXT("visibility.set") && (!(*Object)->TryGetStringField(TEXT("visibility"), Action.Visibility) || (Action.Visibility != TEXT("Visible") && Action.Visibility != TEXT("Hidden") && Action.Visibility != TEXT("Collapsed")))) { Error = TEXT("visibility.set requires canonical visibility"); return false; }
        Actions.Add(MoveTemp(Action));
    }
    if (!P13CanonicalWidgetEventActions(Actions)) { Error = TEXT("actions must use canonical kind and target ordering without duplicates"); return false; }
    return true;
}

static TSharedRef<FJsonObject> P13WidgetEventActionJson(const FP13WidgetEventAction& Action)
{
    const TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
    Row->SetStringField(TEXT("kind"), Action.Kind); Row->SetStringField(TEXT("targetWidgetId"), Action.TargetWidgetId);
    if (Action.Kind == TEXT("text.set")) Row->SetStringField(TEXT("text"), Action.Text);
    else if (Action.Kind == TEXT("enabled.set")) Row->SetBoolField(TEXT("enabled"), Action.Enabled);
    else Row->SetStringField(TEXT("visibility"), Action.Visibility);
    return Row;
}

static TSharedRef<FJsonObject> P13WidgetEventResult(UWidgetBlueprint& Blueprint, const FString& AgentKey, const TArray<FP13WidgetEventAction>& Actions, bool Changed)
{
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("blueprintId"), Blueprint.GetPathName()); Result->SetStringField(TEXT("eventId"), Blueprint.GetPathName() + TEXT("#event:") + AgentKey); Result->SetStringField(TEXT("agentKey"), AgentKey); Result->SetStringField(TEXT("event"), TEXT("activate"));
    TArray<TSharedPtr<FJsonValue>> Rows; for (const FP13WidgetEventAction& Action : Actions) Rows.Add(MakeShared<FJsonValueObject>(P13WidgetEventActionJson(Action)));
    Result->SetArrayField(TEXT("actions"), Rows); Result->SetBoolField(TEXT("changed"), Changed); Result->SetArrayField(TEXT("dirtyPackages"), P11DirtyPackages(Blueprint, Changed)); Result->SetArrayField(TEXT("savedPackages"), {}); Result->SetStringField(TEXT("revision"), BlueprintContentRevision(Blueprint));
    return Result;
}

static int32 P13NodeLinkCount(const UEdGraphNode& Node)
{
    int32 Count = 0;
    for (const UEdGraphPin* Pin : Node.Pins) if (Pin) Count += Pin->LinkedTo.Num();
    return Count;
}

static bool P13WidgetEventReadback(UWidgetBlueprint& Blueprint, UEdGraph& Graph, const FString& AgentKey, TArray<FP13WidgetEventAction>& Actions)
{
    Actions.Reset();
    const FString EventName = P13WidgetEventName(Blueprint, AgentKey);
    UK2Node_CustomEvent* Event = nullptr;
    for (UEdGraphNode* Node : Graph.Nodes)
    {
        UK2Node_CustomEvent* Candidate = Node && Node->GetClass() == UK2Node_CustomEvent::StaticClass() ? Cast<UK2Node_CustomEvent>(Node) : nullptr;
        if (Candidate && Candidate->CustomFunctionName == FName(*EventName)) { if (Event) return false; Event = Candidate; }
    }
    if (!Event || Event->NodeGuid != P13WidgetEventGuid(Blueprint, Graph, AgentKey, TEXT("entry")) || !P13WidgetEventOwnerMatches(P11NodeOwner(Blueprint, *Event), AgentKey, TEXT("entry")) || Event->bCallInEditor || Event->bOverrideFunction || Event->bInternalEvent || !Event->UserDefinedPins.IsEmpty()) return false;
    UEdGraphPin* PreviousExec = Event->FindPin(UEdGraphSchema_K2::PN_Then, EGPD_Output);
    if (!PreviousExec || PreviousExec->LinkedTo.Num() != 1 || P13NodeLinkCount(*Event) != 1) return false;
    for (int32 Index = 0; Index < 3 && PreviousExec && PreviousExec->LinkedTo.Num() == 1; ++Index)
    {
        UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(PreviousExec->LinkedTo[0] ? PreviousExec->LinkedTo[0]->GetOwningNode() : nullptr);
        if (!Call || Call->GetClass() != UK2Node_CallFunction::StaticClass() || Call->NodeGuid != P13WidgetEventGuid(Blueprint, Graph, AgentKey, FString::Printf(TEXT("action.%d.call"), Index)) || !P13WidgetEventOwnerMatches(P11NodeOwner(Blueprint, *Call), AgentKey, FString::Printf(TEXT("action.%d.call"), Index))) return false;
        FP13WidgetEventAction Action;
        const UFunction* Function = Call->GetTargetFunction();
        if (Function == UTextBlock::StaticClass()->FindFunctionByName(FName(TEXT("SetText")))) Action.Kind = TEXT("text.set");
        else if (Function == UWidget::StaticClass()->FindFunctionByName(FName(TEXT("SetIsEnabled")))) Action.Kind = TEXT("enabled.set");
        else if (Function == UWidget::StaticClass()->FindFunctionByName(FName(TEXT("SetVisibility")))) Action.Kind = TEXT("visibility.set");
        else return false;
        UEdGraphPin* TargetPin = Call->FindPin(UEdGraphSchema_K2::PN_Self, EGPD_Input);
        UK2Node_VariableGet* Getter = TargetPin && TargetPin->LinkedTo.Num() == 1 && TargetPin->LinkedTo[0] ? Cast<UK2Node_VariableGet>(TargetPin->LinkedTo[0]->GetOwningNode()) : nullptr;
        if (!Getter || Getter->GetClass() != UK2Node_VariableGet::StaticClass() || Getter->NodeGuid != P13WidgetEventGuid(Blueprint, Graph, AgentKey, FString::Printf(TEXT("action.%d.get"), Index)) || !P13WidgetEventOwnerMatches(P11NodeOwner(Blueprint, *Getter), AgentKey, FString::Printf(TEXT("action.%d.get"), Index)) || !Getter->VariableReference.IsSelfContext() || P13NodeLinkCount(*Getter) != 1) return false;
        UWidget* TargetWidget = Blueprint.WidgetTree ? Blueprint.WidgetTree->FindWidget(Getter->VariableReference.GetMemberName()) : nullptr;
        if (!TargetWidget || P13WidgetClass(*TargetWidget).IsEmpty() || (Action.Kind == TEXT("text.set") && !Cast<UTextBlock>(TargetWidget))) return false;
        Action.TargetWidgetId = P13WidgetId(Blueprint, *TargetWidget);
        const FName BoundProperty = Action.Kind == TEXT("text.set") ? FName(TEXT("Text")) : Action.Kind == TEXT("enabled.set") ? FName(TEXT("bIsEnabled")) : FName(TEXT("Visibility")); for (const FDelegateEditorBinding& Binding : Blueprint.Bindings) if (Binding.ObjectName == TargetWidget->GetFName() && Binding.PropertyName == BoundProperty) return false;
        UEdGraphPin* ValuePin = Action.Kind == TEXT("text.set") ? Call->FindPin(TEXT("InText"), EGPD_Input) : Action.Kind == TEXT("enabled.set") ? Call->FindPin(TEXT("bInIsEnabled"), EGPD_Input) : Call->FindPin(TEXT("InVisibility"), EGPD_Input);
        if (!ValuePin || !ValuePin->LinkedTo.IsEmpty()) return false;
        if (Action.Kind == TEXT("text.set")) Action.Text = ValuePin->DefaultTextValue.ToString();
        else if (Action.Kind == TEXT("enabled.set")) { if (ValuePin->DefaultValue != TEXT("true") && ValuePin->DefaultValue != TEXT("false")) return false; Action.Enabled = ValuePin->DefaultValue == TEXT("true"); }
        else { Action.Visibility = ValuePin->DefaultValue; ESlateVisibility Visibility; if (!P13Visibility(Action.Visibility, Visibility)) return false; }
        UEdGraphPin* Then = Call->FindPin(UEdGraphSchema_K2::PN_Then, EGPD_Output);
        const bool HasNext = Then && Then->LinkedTo.Num() == 1;
        if (!Then || Then->LinkedTo.Num() > 1 || P13NodeLinkCount(*Call) != (HasNext ? 3 : 2)) return false;
        Actions.Add(MoveTemp(Action));
        PreviousExec = Then;
        if (!HasNext) break;
    }
    if (PreviousExec && !PreviousExec->LinkedTo.IsEmpty()) return false;
    if (!P13CanonicalWidgetEventActions(Actions)) return false;
    TSet<FString> ExpectedOwners;
    ExpectedOwners.Add(P13WidgetEventOwner(AgentKey, TEXT("entry")));
    for (int32 Index = 0; Index < Actions.Num(); ++Index)
    {
        ExpectedOwners.Add(P13WidgetEventOwner(AgentKey, FString::Printf(TEXT("action.%d.get"), Index)));
        ExpectedOwners.Add(P13WidgetEventOwner(AgentKey, FString::Printf(TEXT("action.%d.call"), Index)));
    }
    int32 OwnedNodes = 0;
    for (UEdGraphNode* Node : Graph.Nodes) if (Node && ExpectedOwners.Contains(P11NodeOwner(Blueprint, *Node))) ++OwnedNodes;
    return OwnedNodes == ExpectedOwners.Num();
}

TSharedRef<FJsonObject> P13TreeResult(UWidgetBlueprint& Blueprint)
{
    TArray<UWidget*> Widgets; FString Error;
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    if (!P13WidgetTree(Blueprint, Widgets, Error) || Widgets.IsEmpty()) return Result;
    UVerticalBox* Root = Cast<UVerticalBox>(Widgets[0]);
    if (!Root) return Result;
    Result->SetStringField(TEXT("blueprintId"), Blueprint.GetPathName());
    Result->SetStringField(TEXT("generatedClass"), Blueprint.GeneratedClass ? Blueprint.GeneratedClass->GetPathName() : Blueprint.GetPathName() + TEXT("_C"));
    Result->SetStringField(TEXT("rootWidgetId"), P13WidgetId(Blueprint, *Root));
    Result->SetNumberField(TEXT("count"), Widgets.Num()); Result->SetNumberField(TEXT("total"), Widgets.Num()); Result->SetStringField(TEXT("scope"), Blueprint.GetPathName());
    TArray<TSharedPtr<FJsonValue>> Rows;
    for (UWidget* Widget : Widgets)
    {
        const bool IsRoot = Widget == Root; const TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("widgetId"), P13WidgetId(Blueprint, *Widget)); Row->SetStringField(TEXT("name"), Widget->GetName()); Row->SetStringField(TEXT("class"), P13WidgetClass(*Widget));
        if (IsRoot) Row->SetField(TEXT("parentWidgetId"), MakeShared<FJsonValueNull>()); else Row->SetStringField(TEXT("parentWidgetId"), P13WidgetId(Blueprint, *Root));
        Row->SetNumberField(TEXT("index"), IsRoot ? 0 : Root->GetChildIndex(Widget));
        if (const UTextBlock* Text = Cast<UTextBlock>(Widget)) Row->SetStringField(TEXT("text"), Text->GetText().ToString()); else Row->SetField(TEXT("text"), MakeShared<FJsonValueNull>());
        const ESlateVisibility Visibility = Widget->GetVisibility();
        if (Visibility != ESlateVisibility::Visible && Visibility != ESlateVisibility::Hidden && Visibility != ESlateVisibility::Collapsed) return MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("visibility"), Visibility == ESlateVisibility::Hidden ? TEXT("Hidden") : Visibility == ESlateVisibility::Collapsed ? TEXT("Collapsed") : TEXT("Visible")); Row->SetBoolField(TEXT("enabled"), Widget->GetIsEnabled()); Rows.Add(MakeShared<FJsonValueObject>(Row));
    }
    TArray<TSharedPtr<FJsonValue>> EventRows; UEdGraph* EventGraph = P13WidgetEventGraph(Blueprint); if (!EventGraph) return MakeShared<FJsonObject>();
    TArray<TPair<FString, UK2Node_CustomEvent*>> Events;
    for (UEdGraphNode* Node : EventGraph->Nodes)
    {
        UK2Node_CustomEvent* Event = Node && Node->GetClass() == UK2Node_CustomEvent::StaticClass() ? Cast<UK2Node_CustomEvent>(Node) : nullptr; if (!Event) continue;
        FString AgentKey; const FString Owner = P11NodeOwner(Blueprint, *Event); const bool OwnedEvent = P13WidgetEventAgentFromOwner(Owner, AgentKey);
        if (OwnedEvent) Events.Emplace(AgentKey, Event); else if (Event->CustomFunctionName.ToString().StartsWith(TEXT("MagiP13Activate_")) || Owner.StartsWith(TEXT("widget.event:"))) return MakeShared<FJsonObject>();
    }
    if (Events.Num() > 32) return MakeShared<FJsonObject>();
    Events.Sort([](const TPair<FString, UK2Node_CustomEvent*>& Left, const TPair<FString, UK2Node_CustomEvent*>& Right) { return Left.Key.Compare(Right.Key, ESearchCase::CaseSensitive) < 0; });
    for (const TPair<FString, UK2Node_CustomEvent*>& Event : Events)
    {
        TArray<FP13WidgetEventAction> Actions; if (!Event.Value || Event.Value->CustomFunctionName != FName(*P13WidgetEventName(Blueprint, Event.Key)) || !P13WidgetEventReadback(Blueprint, *EventGraph, Event.Key, Actions)) return MakeShared<FJsonObject>();
        const TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>(); Row->SetStringField(TEXT("eventId"), Blueprint.GetPathName() + TEXT("#event:") + Event.Key); Row->SetStringField(TEXT("agentKey"), Event.Key); Row->SetStringField(TEXT("event"), TEXT("activate"));
        TArray<TSharedPtr<FJsonValue>> ActionRows; for (const FP13WidgetEventAction& Action : Actions) ActionRows.Add(MakeShared<FJsonValueObject>(P13WidgetEventActionJson(Action))); Row->SetArrayField(TEXT("actions"), ActionRows); EventRows.Add(MakeShared<FJsonValueObject>(Row));
    }
    Result->SetArrayField(TEXT("widgets"), Rows); Result->SetArrayField(TEXT("events"), EventRows); Result->SetStringField(TEXT("revision"), BlueprintContentRevision(Blueprint)); return Result;
}

static TSharedRef<FJsonObject> P13MutationResult(UWidgetBlueprint& Blueprint, const FString& WidgetId, const FString& Property, bool Changed)
{
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("blueprintId"), Blueprint.GetPathName());
    if (!WidgetId.IsEmpty()) Result->SetStringField(TEXT("widgetId"), WidgetId); if (!Property.IsEmpty()) Result->SetStringField(TEXT("property"), Property); Result->SetBoolField(TEXT("changed"), Changed);
    Result->SetArrayField(TEXT("dirtyPackages"), P11DirtyPackages(Blueprint, Changed)); Result->SetArrayField(TEXT("savedPackages"), {}); Result->SetStringField(TEXT("revision"), BlueprintContentRevision(Blueprint)); return Result;
}

static TSharedRef<FJsonObject> P13CreateResult(UWidgetBlueprint& Blueprint, bool Changed, const FString& RootName)
{
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); const FString Id = Blueprint.GetPathName();
    Result->SetStringField(TEXT("blueprintId"), Id); Result->SetStringField(TEXT("generatedClass"), Blueprint.GeneratedClass ? Blueprint.GeneratedClass->GetPathName() : Id + TEXT("_C")); Result->SetStringField(TEXT("rootWidgetId"), Id + TEXT("#widget:") + RootName); Result->SetStringField(TEXT("rootName"), RootName); Result->SetStringField(TEXT("rootClass"), TEXT("VerticalBox")); Result->SetBoolField(TEXT("changed"), Changed); Result->SetArrayField(TEXT("dirtyPackages"), P11DirtyPackages(Blueprint, Changed)); Result->SetArrayField(TEXT("savedPackages"), {}); Result->SetStringField(TEXT("revision"), BlueprintContentRevision(Blueprint)); return Result;
}

static FString P13ViewportOwner(const FString& AgentKey, const FString& Role) { return TEXT("widget.viewport:") + AgentKey + TEXT(":") + Role; }

static FGuid P13ViewportGuid(const UBlueprint& Blueprint, const UEdGraph& Graph, const FString& AgentKey, const FString& Role)
{
    FGuid Guid; FGuid::ParseExact(Sha256(CanonicalRow({Blueprint.GetPathName(), BlueprintGraphIdentity(Blueprint, Graph), AgentKey, Role})).Left(32), EGuidFormats::Digits, Guid); return Guid;
}

static UK2Node_CallFunction* P13ViewportCall(UEdGraph& Graph, UFunction* Function, const FGuid& Guid, const FString& Owner, int32 X, int32 Y)
{
    if (!Function) return nullptr;
    UK2Node_CallFunction* Node = NewObject<UK2Node_CallFunction>(&Graph, NAME_None, RF_Transactional); Node->SetFromFunction(Function); Node->NodeGuid = Guid; Node->NodePosX = X; Node->NodePosY = Y; P11SetNodeOwner(*CastChecked<UBlueprint>(Graph.GetOuter()), *Node, Owner); Graph.AddNode(Node, true, false); Node->PostPlacedNewNode(); Node->AllocateDefaultPins(); return Node;
}

static FString P13ViewportVariable(const UBlueprint& Blueprint, const FString& AgentKey) { return TEXT("MagiP13Viewport_") + Sha256(CanonicalRow({Blueprint.GetPathName(), AgentKey})).Left(16); }
static bool P13ViewportOwnerParts(const FString& Owner, FString& AgentKey, FString& Role)
{
    const FString Namespace = TEXT("widget.viewport:"); int32 Separator = INDEX_NONE;
    if (Owner.Left(Namespace.Len()) != Namespace || !Owner.FindLastChar(TCHAR(':'), Separator) || Separator <= Namespace.Len() || Separator == Owner.Len() - 1) return false;
    AgentKey = Owner.Mid(Namespace.Len(), Separator - Namespace.Len()); Role = Owner.Mid(Separator + 1); return P13ValidAgentKey(AgentKey) && P13ValidName(Role) && Owner == P13ViewportOwner(AgentKey, Role);
}
static bool P13ViewportReadback(UBlueprint& Blueprint, UEdGraph& Graph, const FString& AgentKey, UClass* WidgetClass, UEdGraphNode*& Begin, UEdGraphNode*& Input, UK2Node_CreateWidget*& Create, UK2Node_CallFunction*& Add, UK2Node_CallFunction*& Enable, UK2Node_CallFunction*& Activate)
{
    Begin = Input = nullptr; Create = nullptr; Add = Enable = Activate = nullptr; UK2Node_VariableSet* Set = nullptr; UK2Node_VariableGet* Get = nullptr; UK2Node_CallFunction* Controller = nullptr;
    const TArray<FString> Roles = {TEXT("begin"), TEXT("input"), TEXT("create"), TEXT("set"), TEXT("add"), TEXT("enable"), TEXT("controller"), TEXT("get"), TEXT("activate")}; int32 OwnedCount = 0;
    for (UEdGraphNode* Node : Graph.Nodes)
    {
        if (!Node) continue; FString OwnedAgent, Role; if (!P13ViewportOwnerParts(P11NodeOwner(Blueprint, *Node), OwnedAgent, Role) || OwnedAgent != AgentKey) continue;
        ++OwnedCount; if (!Roles.Contains(Role)) return false;
        if (Role == TEXT("begin")) { if (Begin) return false; Begin = Node; } else if (Role == TEXT("input")) { if (Input) return false; Input = Node; } else if (Role == TEXT("create")) { if (Create) return false; Create = Cast<UK2Node_CreateWidget>(Node); } else if (Role == TEXT("set")) { if (Set) return false; Set = Cast<UK2Node_VariableSet>(Node); } else if (Role == TEXT("add")) { if (Add) return false; Add = Cast<UK2Node_CallFunction>(Node); } else if (Role == TEXT("enable")) { if (Enable) return false; Enable = Cast<UK2Node_CallFunction>(Node); } else if (Role == TEXT("controller")) { if (Controller) return false; Controller = Cast<UK2Node_CallFunction>(Node); } else if (Role == TEXT("get")) { if (Get) return false; Get = Cast<UK2Node_VariableGet>(Node); } else if (Role == TEXT("activate")) { if (Activate) return false; Activate = Cast<UK2Node_CallFunction>(Node); }
    }
    if (OwnedCount != Roles.Num() || !Begin || !Input || !Create || !Set || !Add || !Enable || !Controller || !Get || !Activate) return false;
    const FName VariableName(*P13ViewportVariable(Blueprint, AgentKey)); const FBPVariableDescription* Variable = nullptr; for (const FBPVariableDescription& Candidate : Blueprint.NewVariables) if (Candidate.VarName == VariableName) { if (Variable) return false; Variable = &Candidate; }
    const UWidgetBlueprint* WidgetBlueprint = WidgetClass ? Cast<UWidgetBlueprint>(WidgetClass->ClassGeneratedBy) : nullptr;
    if (!Variable || !WidgetBlueprint || Variable->VarType.PinCategory != UEdGraphSchema_K2::PC_Object || Variable->VarType.PinSubCategoryObject.Get() != WidgetClass || Get->GetClass() != UK2Node_VariableGet::StaticClass() || Set->GetClass() != UK2Node_VariableSet::StaticClass() || !Get->VariableReference.IsSelfContext() || !Set->VariableReference.IsSelfContext() || Get->VariableReference.GetMemberName() != VariableName || Set->VariableReference.GetMemberName() != VariableName) return false;
    if (Begin->NodeGuid != P13ViewportGuid(Blueprint, Graph, AgentKey, TEXT("begin")) || Input->NodeGuid != P13ViewportGuid(Blueprint, Graph, AgentKey, TEXT("input")) || Create->NodeGuid != P13ViewportGuid(Blueprint, Graph, AgentKey, TEXT("create")) || Set->NodeGuid != P13ViewportGuid(Blueprint, Graph, AgentKey, TEXT("set")) || Add->NodeGuid != P13ViewportGuid(Blueprint, Graph, AgentKey, TEXT("add")) || Enable->NodeGuid != P13ViewportGuid(Blueprint, Graph, AgentKey, TEXT("enable")) || Controller->NodeGuid != P13ViewportGuid(Blueprint, Graph, AgentKey, TEXT("controller")) || Get->NodeGuid != P13ViewportGuid(Blueprint, Graph, AgentKey, TEXT("get")) || Activate->NodeGuid != P13ViewportGuid(Blueprint, Graph, AgentKey, TEXT("activate"))) return false;
    const UK2Node_Event* BeginEvent = Cast<UK2Node_Event>(Begin); const UK2Node_InputKey* InputEvent = Cast<UK2Node_InputKey>(Input); UEdGraphPin* CreateClass = Create->GetClassPin();
    if (!BeginEvent || BeginEvent->GetClass() != UK2Node_Event::StaticClass() || !BeginEvent->bOverrideFunction || BeginEvent->EventReference.GetMemberName() != FName(TEXT("ReceiveBeginPlay")) || BeginEvent->EventReference.GetMemberParentClass(BeginEvent->GetBlueprintClassFromNode()) != AActor::StaticClass() || !InputEvent || InputEvent->GetClass() != UK2Node_InputKey::StaticClass() || InputEvent->InputKey != EKeys::E || !InputEvent->bConsumeInput || InputEvent->bExecuteWhenPaused || !InputEvent->bOverrideParentBinding || InputEvent->bControl || InputEvent->bAlt || InputEvent->bShift || InputEvent->bCommand || !CreateClass || CreateClass->DefaultObject != WidgetClass || Create->GetClassToSpawn() != WidgetClass || Add->GetTargetFunction() != UUserWidget::StaticClass()->FindFunctionByName(TEXT("AddToViewport")) || Enable->GetTargetFunction() != AActor::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(AActor, EnableInput)) || Controller->GetTargetFunction() != UGameplayStatics::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerController)) || Activate->FunctionReference.GetMemberName() != FName(*P13WidgetEventName(*WidgetBlueprint, AgentKey)) || Activate->FunctionReference.GetMemberParentClass(Activate->GetBlueprintClassFromNode()) != WidgetClass) return false;
    UEdGraphPin* CreateOut = Create->GetResultPin(); UEdGraphPin* SetValue = Set->FindPin(VariableName, EGPD_Input); UEdGraphPin* SetThen = Set->FindPin(UEdGraphSchema_K2::PN_Then, EGPD_Output); UEdGraphPin* GetValue = Get->GetValuePin(); UEdGraphPin* AddSelf = Add->FindPin(UEdGraphSchema_K2::PN_Self, EGPD_Input); UEdGraphPin* ActivateSelf = Activate->FindPin(UEdGraphSchema_K2::PN_Self, EGPD_Input); UEdGraphPin* ZOrder = Add->FindPin(TEXT("ZOrder"), EGPD_Input); UEdGraphPin* BeginThen = Begin->FindPin(UEdGraphSchema_K2::PN_Then, EGPD_Output); UEdGraphPin* InputPressed = Input->FindPin(TEXT("Pressed"), EGPD_Output); UEdGraphPin* EnableThen = Enable->FindPin(UEdGraphSchema_K2::PN_Then, EGPD_Output); UEdGraphPin* ControllerOut = Controller->FindPin(UEdGraphSchema_K2::PN_ReturnValue, EGPD_Output); UEdGraphPin* EnableController = Enable->FindPin(TEXT("PlayerController"), EGPD_Input); UEdGraphPin* CreateExec = Create->GetExecPin(); UEdGraphPin* CreateThen = Create->GetThenPin(); UEdGraphPin* CreateOwner = Create->GetOwningPlayerPin(); UEdGraphPin* SetExec = Set->FindPin(UEdGraphSchema_K2::PN_Execute, EGPD_Input); UEdGraphPin* AddExec = Add->FindPin(UEdGraphSchema_K2::PN_Execute, EGPD_Input); UEdGraphPin* ActivateExec = Activate->FindPin(UEdGraphSchema_K2::PN_Execute, EGPD_Input);
    auto One = [](UEdGraphPin* Pin, UEdGraphNode* Node) { return Pin && Pin->LinkedTo.Num() == 1 && Pin->LinkedTo[0] && Pin->LinkedTo[0]->GetOwningNode() == Node; }; auto HasLink = [](UEdGraphPin* Pin, UEdGraphPin* Linked) { return Pin && Linked && Pin->LinkedTo.Contains(Linked); }; auto Incoming = [](UEdGraphPin* Pin, UEdGraphNode* Node) { return Pin && Pin->LinkedTo.Num() == 1 && Pin->LinkedTo[0] && Pin->LinkedTo[0]->GetOwningNode() == Node; };
    auto Terminal = [](UEdGraphPin* Pin) { return Pin && Pin->LinkedTo.IsEmpty(); }; UEdGraphPin* InputReleased = Input->FindPin(TEXT("Released"), EGPD_Output); UEdGraphPin* AddThen = Add->FindPin(UEdGraphSchema_K2::PN_Then, EGPD_Output); UEdGraphPin* ActivateThen = Activate->FindPin(UEdGraphSchema_K2::PN_Then, EGPD_Output);
    return ZOrder && ZOrder->DefaultValue == TEXT("0") && CreateOut && SetValue && CreateOut->LinkedTo.Num() == 1 && CreateOut->LinkedTo.Contains(SetValue) && GetValue && GetValue->LinkedTo.Num() == 2 && AddSelf && AddSelf->LinkedTo.Num() == 1 && AddSelf->LinkedTo[0] == GetValue && ActivateSelf && ActivateSelf->LinkedTo.Num() == 1 && ActivateSelf->LinkedTo[0] == GetValue && One(BeginThen, Enable) && One(EnableThen, Create) && ControllerOut && ControllerOut->LinkedTo.Num() == 2 && HasLink(ControllerOut, EnableController) && HasLink(ControllerOut, CreateOwner) && Incoming(CreateExec, Enable) && One(CreateThen, Set) && Incoming(SetExec, Create) && One(SetThen, Add) && Incoming(AddExec, Set) && One(InputPressed, Activate) && Incoming(ActivateExec, Input) && Terminal(AddThen) && Terminal(ActivateThen) && Terminal(InputReleased) && P13NodeLinkCount(*Begin) == 1 && P13NodeLinkCount(*Input) == 1 && P13NodeLinkCount(*Create) == 4 && P13NodeLinkCount(*Set) == 3 && P13NodeLinkCount(*Add) == 2 && P13NodeLinkCount(*Enable) == 3 && P13NodeLinkCount(*Controller) == 2 && P13NodeLinkCount(*Get) == 2 && P13NodeLinkCount(*Activate) == 2;
}
static TSharedRef<FJsonObject> P13ViewportResult(UBlueprint& Host, const FString& WidgetId, const FString& AgentKey, const FString& GraphId, const FString& WidgetRevision, bool Changed)
{
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); const FString ViewportId = Host.GetPathName() + TEXT("#viewport:") + AgentKey;
    Result->SetStringField(TEXT("hostBlueprintId"), Host.GetPathName()); Result->SetStringField(TEXT("widgetBlueprintId"), WidgetId); Result->SetStringField(TEXT("viewportId"), ViewportId); Result->SetStringField(TEXT("graphId"), GraphId); Result->SetStringField(TEXT("inputKey"), TEXT("E")); Result->SetNumberField(TEXT("zOrder"), 0); Result->SetStringField(TEXT("widgetRevision"), WidgetRevision); Result->SetBoolField(TEXT("changed"), Changed); Result->SetArrayField(TEXT("dirtyPackages"), P11DirtyPackages(Host, Changed)); Result->SetArrayField(TEXT("savedPackages"), {}); Result->SetStringField(TEXT("revision"), BlueprintContentRevision(Host)); return Result;
}
FString HandleP13WidgetOperation(const FString& Id, const FString& Operation, const TSharedPtr<FJsonObject>& Args, const FString& ExpectedRevision)
{
    if (!Args.IsValid()) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("widget operation requires arguments"));
    if (Operation == TEXT("widget.create"))
    {
        FString Path, RootName, RootClass; Args->TryGetStringField(TEXT("path"), Path); Args->TryGetStringField(TEXT("rootName"), RootName); Args->TryGetStringField(TEXT("rootClass"), RootClass);
        if (!Path.StartsWith(TEXT("/Game/")) || !FPackageName::IsValidLongPackageName(Path, false) || Path.Contains(TEXT(".")) || !P13ValidName(RootName) || RootClass != TEXT("VerticalBox")) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("path, valid rootName, and VerticalBox rootClass required"));
        const FString AssetId = Path + TEXT(".") + FPackageName::GetLongPackageAssetName(Path); UWidgetBlueprint* Existing = P13LoadWidgetBlueprint(AssetId);
        if (Existing)
        {
            TArray<UWidget*> Widgets; FString Error; if (!P13WidgetTree(*Existing, Widgets, Error) || Widgets.IsEmpty() || Widgets[0]->GetName() != RootName) return ErrorResponse(Id, TEXT("conflict"), TEXT("asset path exists with incompatible widget intent"));
            const FString ExistingRevision = BlueprintContentRevision(*Existing); const FString Response = SuccessResponse(Id, P13CreateResult(*Existing, false, RootName), Operation, Args);
            return ResponseStatusIsOk(Response) ? Response : P11FailedAtomicResponse(Id, Operation, Args, AssetId, ExistingRevision, ExistingRevision, true, P11DirtyPackages(*Existing, false), TEXT("widget create no-op receipt postcondition failed"));
        }
        if (FindPackage(nullptr, *Path) || FPackageName::DoesPackageExist(Path)) return ErrorResponse(Id, TEXT("conflict"), TEXT("widget package already exists"));
        const FString AbsentRevision = Sha256(AssetId + TEXT("\nabsent"));
        UPackage* Package = CreatePackage(*Path);
        if (!Package)
        {
            const bool Absent = !FPackageName::DoesPackageExist(Path) && !FindPackage(nullptr, *Path) && !FindObject<UObject>(nullptr, *AssetId) && !FAssetRegistryModule::GetRegistry().GetAssetByObjectPath(FSoftObjectPath(AssetId)).IsValid();
            const FString Observed = Absent ? AbsentRevision : (P13LoadWidgetBlueprint(AssetId) ? BlueprintContentRevision(*P13LoadWidgetBlueprint(AssetId)) : Sha256(AssetId + TEXT("\npresent")));
            TArray<TSharedPtr<FJsonValue>> DirtyPackages; if (UPackage* ObservedPackage = FindPackage(nullptr, *Path); ObservedPackage && ObservedPackage->IsDirty()) DirtyPackages.Add(MakeShared<FJsonValueString>(Path));
            return P11FailedAtomicResponse(Id, Operation, Args, AssetId, AbsentRevision, Observed, Absent, DirtyPackages, Absent ? TEXT("widget package creation failed") : TEXT("widget package creation outcome is unknown"));
        }
        FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P13CreateWidget", "Magi AXI Create Widget")); Package->Modify();
        UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(UUserWidget::StaticClass(), Package, FName(*FPackageName::GetLongPackageAssetName(Path)), BPTYPE_Normal, UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass(), TEXT("MagiUnrealAXI")));
        bool AssetRegistered = false;
        auto CreatedAssetAbsent = [&]()
        {
            return !FPackageName::DoesPackageExist(Path) && !FindPackage(nullptr, *Path) && !FindObject<UObject>(nullptr, *AssetId) && !FAssetRegistryModule::GetRegistry().GetAssetByObjectPath(FSoftObjectPath(AssetId)).IsValid();
        };
        auto FailedCreate = [&](const TCHAR* Message)
        {
            Transaction.Cancel(); P11DiscardCreatedBlueprint(*Package, Blueprint, AssetRegistered);
            const bool Absent = CreatedAssetAbsent();
            const FString Observed = Absent ? AbsentRevision : (P13LoadWidgetBlueprint(AssetId) ? BlueprintContentRevision(*P13LoadWidgetBlueprint(AssetId)) : Sha256(AssetId + TEXT("\npresent")));
            TArray<TSharedPtr<FJsonValue>> DirtyPackages; if (UPackage* ObservedPackage = FindPackage(nullptr, *Path); ObservedPackage && ObservedPackage->IsDirty()) DirtyPackages.Add(MakeShared<FJsonValueString>(Path));
            return P11FailedAtomicResponse(Id, Operation, Args, AssetId, AbsentRevision, Observed, Absent, DirtyPackages, Absent ? Message : TEXT("widget Blueprint creation rollback verification failed"));
        };
        if (!Blueprint || !Blueprint->WidgetTree) return FailedCreate(TEXT("widget Blueprint creation failed"));
        Blueprint->Modify(); UVerticalBox* Root = Blueprint->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName(*RootName)); if (!Root) return FailedCreate(TEXT("widget root creation failed"));
        Blueprint->WidgetTree->RootWidget = Root; Root->bIsVariable = true; Root->SetVisibility(ESlateVisibility::Visible); Root->SetIsEnabled(true); Blueprint->OnVariableAdded(Root->GetFName()); FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint); FAssetRegistryModule::AssetCreated(Blueprint); AssetRegistered = true; Package->MarkPackageDirty();
        TArray<UWidget*> Widgets; FString Error; const bool TreeValid = P13WidgetTree(*Blueprint, Widgets, Error);
        if (Blueprint->GetPathName() != AssetId || !TreeValid || Widgets.IsEmpty() || Widgets[0]->GetName() != RootName) return FailedCreate(TEXT("widget Blueprint readback failed"));
        const FString Response = P11AtomicResponse(Id, SuccessResponse(Id, P13CreateResult(*Blueprint, true, RootName), Operation, Args));
        return ResponseStatusIsOk(Response) ? Response : FailedCreate(TEXT("widget Blueprint postcondition failed"));
    }
    FString BlueprintId; Args->TryGetStringField(TEXT("blueprintId"), BlueprintId); UWidgetBlueprint* Blueprint = Operation == TEXT("widget.viewport_ensure") ? nullptr : P13LoadWidgetBlueprint(BlueprintId); if (Operation != TEXT("widget.viewport_ensure") && !Blueprint) return ErrorResponse(Id, TEXT("not_found"), TEXT("widget Blueprint was not found"));
    if (Operation == TEXT("widget.viewport_ensure"))
    {
        FString HostId, WidgetId, AgentKey, InputKey; int32 ZOrder = -1; Args->TryGetStringField(TEXT("hostBlueprintId"), HostId); Args->TryGetStringField(TEXT("widgetBlueprintId"), WidgetId); Args->TryGetStringField(TEXT("agentKey"), AgentKey); Args->TryGetStringField(TEXT("inputKey"), InputKey); double Z = -1; if (Args->TryGetNumberField(TEXT("zOrder"), Z)) ZOrder = static_cast<int32>(Z);
        if (!P13ValidAgentKey(AgentKey) || InputKey != TEXT("E") || ZOrder != 0 || HostId.IsEmpty() || WidgetId.IsEmpty()) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("viewport requires fixed E input and zOrder 0"));
        UBlueprint* Host = P11LoadBlueprint(HostId); UWidgetBlueprint* Widget = P13LoadWidgetBlueprint(WidgetId);
        if (!Host || !Host->ParentClass || !Host->ParentClass->IsChildOf(AActor::StaticClass())) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("hostBlueprintId must identify an Actor Blueprint"));
        if (!Widget || !Widget->GeneratedClass) return ErrorResponse(Id, TEXT("conflict"), TEXT("widget Blueprint must be compiled before viewport binding"));
        UEdGraph* WidgetGraph = P13WidgetEventGraph(*Widget); bool HasActivateEvent = false; if (WidgetGraph) for (UEdGraphNode* Node : WidgetGraph->Nodes) if (UK2Node_CustomEvent* Event = Cast<UK2Node_CustomEvent>(Node)) if (Event->CustomFunctionName == FName(*P13WidgetEventName(*Widget, AgentKey))) HasActivateEvent = true;
        if (!HasActivateEvent) return ErrorResponse(Id, TEXT("not_found"), TEXT("widget activate event for agentKey was not found"));
        UEdGraph* Graph = Host->UbergraphPages.Num() == 1 ? Host->UbergraphPages[0] : nullptr; if (!Graph || BlueprintGraphKind(*Host, *Graph) != TEXT("ubergraph")) return ErrorResponse(Id, TEXT("unsupported"), TEXT("host Blueprint must have one fixed EventGraph"));
        const FString HostRevision = BlueprintContentRevision(*Host); if (ExpectedRevision.IsEmpty() || ExpectedRevision != HostRevision) return ErrorResponse(Id, ExpectedRevision.IsEmpty() ? TEXT("invalid_input") : TEXT("conflict"), ExpectedRevision.IsEmpty() ? TEXT("expectedRevision is required") : TEXT("host Blueprint revision is stale"));
        UEdGraphNode* Begin = nullptr; UEdGraphNode* Input = nullptr; UK2Node_CreateWidget* Create = nullptr; UK2Node_CallFunction* Add = nullptr; UK2Node_CallFunction* Enable = nullptr; UK2Node_CallFunction* Activate = nullptr;
        const bool HasExisting = P13ViewportReadback(*Host, *Graph, AgentKey, Widget->GeneratedClass, Begin, Input, Create, Add, Enable, Activate);
        bool HasOwned = false; for (UEdGraphNode* Node : Graph->Nodes) { FString OwnedAgent, Role; if (Node && P13ViewportOwnerParts(P11NodeOwner(*Host, *Node), OwnedAgent, Role) && OwnedAgent == AgentKey) { HasOwned = true; break; } }
        if (HasExisting)
        {
            const FString Response = SuccessResponse(Id, P13ViewportResult(*Host, WidgetId, AgentKey, BlueprintGraphIdentity(*Host, *Graph), BlueprintContentRevision(*Widget), false), Operation, Args);
            return ResponseStatusIsOk(Response) ? Response : P11FailedAtomicResponse(Id, Operation, Args, HostId + TEXT("#viewport:") + AgentKey, HostRevision, HostRevision, true, P11DirtyPackages(*Host, false), TEXT("viewport no-op receipt postcondition failed"));
        }
        if (HasOwned) return ErrorResponse(Id, TEXT("conflict"), TEXT("viewport natural key has malformed or incompatible authored graph"));
        const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>(); UFunction* AddFn = UUserWidget::StaticClass()->FindFunctionByName(TEXT("AddToViewport")); UFunction* EnableFn = AActor::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(AActor, EnableInput)); UFunction* ControllerFn = UGameplayStatics::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerController));
        if (!Schema || !AddFn || !EnableFn || !ControllerFn) return ErrorResponse(Id, TEXT("unsupported"), TEXT("fixed viewport graph functions are unavailable"));
        const FName VariableName(*P13ViewportVariable(*Host, AgentKey)); FEdGraphPinType VariableType; VariableType.PinCategory = UEdGraphSchema_K2::PC_Object; VariableType.PinSubCategoryObject = Widget->GeneratedClass;
        for (const FBPVariableDescription& Existing : Host->NewVariables) if (Existing.VarName == VariableName) return ErrorResponse(Id, TEXT("conflict"), TEXT("viewport member variable exists without exact graph intent"));
        const bool WasDirty = Host->GetOutermost()->IsDirty(); const EBlueprintStatus BeforeStatus = Host->Status; FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P13Viewport", "Magi AXI Ensure Widget Viewport Input")); Host->Modify(); Graph->Modify(); TArray<UEdGraphNode*> Created; bool VariableAdded = false;
        auto Rollback = [&](const TCHAR* Message) { for (int32 Index = Created.Num() - 1; Index >= 0; --Index) if (Created[Index]) { P11ClearNodeOwner(*Host, *Created[Index]); Created[Index]->BreakAllNodeLinks(); Graph->RemoveNode(Created[Index]); Created[Index]->MarkAsGarbage(); } if (VariableAdded) FBlueprintEditorUtils::RemoveMemberVariable(Host, VariableName); FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Host); Transaction.Cancel(); Host->Status = BeforeStatus; Host->GetOutermost()->SetDirtyFlag(WasDirty); const FString Observed = BlueprintContentRevision(*Host); const bool Rolled = Observed == HostRevision && Host->Status == BeforeStatus && Host->GetOutermost()->IsDirty() == WasDirty; return P11FailedAtomicResponse(Id, Operation, Args, HostId + TEXT("#viewport:") + AgentKey, HostRevision, Observed, Rolled, P11DirtyPackages(*Host, false), Message); };
        if (!FBlueprintEditorUtils::AddMemberVariable(Host, VariableName, VariableType)) return Rollback(TEXT("viewport member variable creation failed")); VariableAdded = true;
        auto AddOwned = [&](UEdGraphNode* Node, const FString& Role) { if (!Node) return false; Node->NodeGuid = P13ViewportGuid(*Host, *Graph, AgentKey, Role); P11SetNodeOwner(*Host, *Node, P13ViewportOwner(AgentKey, Role)); Graph->AddNode(Node, true, false); Node->PostPlacedNewNode(); Node->AllocateDefaultPins(); Created.Add(Node); return true; };
        UK2Node_Event* BeginNode = NewObject<UK2Node_Event>(Graph, NAME_None, RF_Transactional); BeginNode->EventReference.SetExternalMember(FName(TEXT("ReceiveBeginPlay")), AActor::StaticClass()); BeginNode->bOverrideFunction = true; AddOwned(BeginNode, TEXT("begin")); Begin = BeginNode;
        UK2Node_InputKey* InputNode = NewObject<UK2Node_InputKey>(Graph, NAME_None, RF_Transactional); InputNode->InputKey = EKeys::E; InputNode->bConsumeInput = true; InputNode->bExecuteWhenPaused = false; InputNode->bOverrideParentBinding = true; InputNode->bControl = false; InputNode->bAlt = false; InputNode->bShift = false; InputNode->bCommand = false; AddOwned(InputNode, TEXT("input")); Input = InputNode;
        Create = NewObject<UK2Node_CreateWidget>(Graph, NAME_None, RF_Transactional); AddOwned(Create, TEXT("create"));
        UK2Node_VariableSet* Set = NewObject<UK2Node_VariableSet>(Graph, NAME_None, RF_Transactional); Set->VariableReference.SetSelfMember(VariableName); AddOwned(Set, TEXT("set"));
        Add = NewObject<UK2Node_CallFunction>(Graph, NAME_None, RF_Transactional); Add->SetFromFunction(AddFn); AddOwned(Add, TEXT("add"));
        Enable = NewObject<UK2Node_CallFunction>(Graph, NAME_None, RF_Transactional); Enable->SetFromFunction(EnableFn); AddOwned(Enable, TEXT("enable"));
        UK2Node_CallFunction* Controller = NewObject<UK2Node_CallFunction>(Graph, NAME_None, RF_Transactional); Controller->SetFromFunction(ControllerFn); AddOwned(Controller, TEXT("controller"));
        UK2Node_VariableGet* Get = NewObject<UK2Node_VariableGet>(Graph, NAME_None, RF_Transactional); Get->VariableReference.SetSelfMember(VariableName); AddOwned(Get, TEXT("get"));
        Activate = NewObject<UK2Node_CallFunction>(Graph, NAME_None, RF_Transactional); Activate->FunctionReference.SetExternalMember(FName(*P13WidgetEventName(*Widget, AgentKey)), Widget->GeneratedClass); AddOwned(Activate, TEXT("activate"));
        UEdGraphPin* CreateType = Create->GetClassPin(); UEdGraphPin* CreateOut = Create->GetResultPin(); UEdGraphPin* SetValue = Set->FindPin(VariableName, EGPD_Input); UEdGraphPin* SetExec = Set->FindPin(UEdGraphSchema_K2::PN_Execute, EGPD_Input); UEdGraphPin* AddSelf = Add->FindPin(UEdGraphSchema_K2::PN_Self, EGPD_Input); UEdGraphPin* AddExec = Add->FindPin(UEdGraphSchema_K2::PN_Execute, EGPD_Input); UEdGraphPin* EnableExec = Enable->FindPin(UEdGraphSchema_K2::PN_Execute, EGPD_Input); UEdGraphPin* EnableController = Enable->FindPin(TEXT("PlayerController"), EGPD_Input); UEdGraphPin* CreateOwner = Create->GetOwningPlayerPin(); UEdGraphPin* ActivateSelf = Activate->FindPin(UEdGraphSchema_K2::PN_Self, EGPD_Input); UEdGraphPin* ActivateExec = Activate->FindPin(UEdGraphSchema_K2::PN_Execute, EGPD_Input); UEdGraphPin* GetValue = Get->GetValuePin();
        bool Built = CreateType && CreateOut && SetValue && SetExec && AddSelf && AddExec && EnableExec && EnableController && CreateOwner && ActivateSelf && ActivateExec && GetValue;
        if (Built) { Schema->TrySetDefaultObject(*CreateType, Widget->GeneratedClass); Built = CreateType->DefaultObject == Widget->GeneratedClass; }
        UEdGraphPin* BeginThen = Begin->FindPin(UEdGraphSchema_K2::PN_Then, EGPD_Output); UEdGraphPin* InputPressed = Input->FindPin(TEXT("Pressed"), EGPD_Output); UEdGraphPin* EnableThen = Enable->FindPin(UEdGraphSchema_K2::PN_Then, EGPD_Output); UEdGraphPin* ControllerOut = Controller->FindPin(UEdGraphSchema_K2::PN_ReturnValue, EGPD_Output); UEdGraphPin* CreateExec = Create->GetExecPin(); UEdGraphPin* CreateThen = Create->GetThenPin(); UEdGraphPin* SetThen = Set->FindPin(UEdGraphSchema_K2::PN_Then, EGPD_Output); UEdGraphPin* ZPin = Add->FindPin(TEXT("ZOrder"), EGPD_Input);
        Built = Built && BeginThen && InputPressed && EnableThen && ControllerOut && CreateExec && CreateThen && SetThen && ZPin && Schema->TryCreateConnection(BeginThen, EnableExec) && Schema->TryCreateConnection(EnableThen, CreateExec) && Schema->TryCreateConnection(ControllerOut, EnableController) && Schema->TryCreateConnection(ControllerOut, CreateOwner) && Schema->TryCreateConnection(CreateThen, SetExec) && Schema->TryCreateConnection(CreateOut, SetValue) && Schema->TryCreateConnection(SetThen, AddExec) && Schema->TryCreateConnection(GetValue, AddSelf) && Schema->TryCreateConnection(GetValue, ActivateSelf) && Schema->TryCreateConnection(InputPressed, ActivateExec); if (Built) { Schema->TrySetDefaultValue(*ZPin, TEXT("0")); Built = ZPin->DefaultValue == TEXT("0"); } FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Host);
#if WITH_DEV_AUTOMATION_TESTS
        const bool Injected = GP11ForceAtomicFailure;
#else
        const bool Injected = false;
#endif
        UEdGraphNode* ReadBegin = nullptr; UEdGraphNode* ReadInput = nullptr; UK2Node_CreateWidget* ReadCreate = nullptr; UK2Node_CallFunction* ReadAdd = nullptr; UK2Node_CallFunction* ReadEnable = nullptr; UK2Node_CallFunction* ReadActivate = nullptr; const bool Verified = Built && P13ViewportReadback(*Host, *Graph, AgentKey, Widget->GeneratedClass, ReadBegin, ReadInput, ReadCreate, ReadAdd, ReadEnable, ReadActivate) && !Injected && BlueprintContentRevision(*Host) != HostRevision;
        if (!Verified) return Rollback(TEXT("viewport graph atomic postcondition failed"));
        const FString Response = P11AtomicResponse(Id, SuccessResponse(Id, P13ViewportResult(*Host, WidgetId, AgentKey, BlueprintGraphIdentity(*Host, *Graph), BlueprintContentRevision(*Widget), true), Operation, Args)); return ResponseStatusIsOk(Response) ? Response : Rollback(TEXT("viewport receipt postcondition failed"));
    }
    TArray<UWidget*> Widgets; FString TreeError; if (!P13WidgetTree(*Blueprint, Widgets, TreeError) || Widgets.IsEmpty()) return ErrorResponse(Id, TEXT("unsupported"), *TreeError);
    const FString CurrentRevision = BlueprintContentRevision(*Blueprint); if (Operation != TEXT("widget.tree_view") && (ExpectedRevision.IsEmpty() || ExpectedRevision != CurrentRevision)) return ErrorResponse(Id, ExpectedRevision.IsEmpty() ? TEXT("invalid_input") : TEXT("conflict"), ExpectedRevision.IsEmpty() ? TEXT("expectedRevision is required") : TEXT("widget Blueprint revision is stale"));
    if (Operation == TEXT("widget.event_ensure"))
    {
        FString AgentKey, Event; Args->TryGetStringField(TEXT("agentKey"), AgentKey); Args->TryGetStringField(TEXT("event"), Event);
        if (!P13ValidAgentKey(AgentKey) || Event != TEXT("activate")) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("event must be activate and agentKey must use the fixed durable-key grammar"));
        TArray<FP13WidgetEventAction> Requested; FString ActionError; if (!P13ParseWidgetEventActions(Args, Requested, ActionError)) return ErrorResponse(Id, TEXT("invalid_input"), *ActionError);
        TMap<FString, UWidget*> WidgetById; for (UWidget* Widget : Widgets) WidgetById.Add(P13WidgetId(*Blueprint, *Widget), Widget);
        for (const FP13WidgetEventAction& Action : Requested)
        {
            UWidget* Target = WidgetById.FindRef(Action.TargetWidgetId); if (!Target) return ErrorResponse(Id, TEXT("not_found"), TEXT("event action target widget was not found"));
            if (Action.Kind == TEXT("text.set") && !Cast<UTextBlock>(Target)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("text.set target must be TextBlock"));
            const FName Property = Action.Kind == TEXT("text.set") ? FName(TEXT("Text")) : Action.Kind == TEXT("enabled.set") ? FName(TEXT("bIsEnabled")) : FName(TEXT("Visibility"));
            for (const FDelegateEditorBinding& Binding : Blueprint->Bindings) if (Binding.ObjectName == Target->GetFName() && Binding.PropertyName == Property) return ErrorResponse(Id, TEXT("conflict"), TEXT("event action target property is bound"));
        }
        UEdGraph* Graph = P13WidgetEventGraph(*Blueprint); if (!Graph || BlueprintGraphKind(*Blueprint, *Graph) != TEXT("ubergraph")) return ErrorResponse(Id, TEXT("unsupported"), TEXT("widget Blueprint must have one fixed EventGraph"));
        const FString EventName = P13WidgetEventName(*Blueprint, AgentKey); UK2Node_CustomEvent* ExistingEvent = nullptr;
        for (UEdGraphNode* Node : Graph->Nodes) if (UK2Node_CustomEvent* Candidate = Node && Node->GetClass() == UK2Node_CustomEvent::StaticClass() ? Cast<UK2Node_CustomEvent>(Node) : nullptr) if (Candidate->CustomFunctionName == FName(*EventName)) { if (ExistingEvent) return ErrorResponse(Id, TEXT("conflict"), TEXT("duplicate deterministic activate event")); ExistingEvent = Candidate; }
        int32 AuthoredEventCount = 0; for (UEdGraphNode* Node : Graph->Nodes) if (Node) { FString OwnedAgent; if (P13WidgetEventAgentFromOwner(P11NodeOwner(*Blueprint, *Node), OwnedAgent)) ++AuthoredEventCount; } if (!ExistingEvent && AuthoredEventCount >= 32) return ErrorResponse(Id, TEXT("unsupported"), TEXT("widget event count exceeds fixed limit"));
        auto ActionsEqual = [](const TArray<FP13WidgetEventAction>& Left, const TArray<FP13WidgetEventAction>& Right)
        {
            if (Left.Num() != Right.Num()) return false;
            for (int32 Index = 0; Index < Left.Num(); ++Index) if (Left[Index].Kind != Right[Index].Kind || Left[Index].TargetWidgetId != Right[Index].TargetWidgetId || Left[Index].Text != Right[Index].Text || Left[Index].Visibility != Right[Index].Visibility || Left[Index].Enabled != Right[Index].Enabled) return false;
            return true;
        };
        if (ExistingEvent)
        {
            TArray<FP13WidgetEventAction> ExistingActions;
            if (!P13WidgetEventReadback(*Blueprint, *Graph, AgentKey, ExistingActions) || !ActionsEqual(ExistingActions, Requested)) return ErrorResponse(Id, TEXT("conflict"), TEXT("deterministic activate event has different or malformed graph intent"));
            const FString Response = SuccessResponse(Id, P13WidgetEventResult(*Blueprint, AgentKey, ExistingActions, false), Operation, Args);
            const FString Target = BlueprintId + TEXT("#") + BlueprintId + TEXT("#event:") + AgentKey;
            return ResponseStatusIsOk(Response) ? Response : P11FailedAtomicResponse(Id, Operation, Args, Target, CurrentRevision, CurrentRevision, true, P11DirtyPackages(*Blueprint, false), TEXT("widget activate event no-op receipt postcondition failed"));
        }
        TArray<TPair<FGuid, FString>> Identities; Identities.Emplace(P13WidgetEventGuid(*Blueprint, *Graph, AgentKey, TEXT("entry")), TEXT("entry"));
        for (int32 Index = 0; Index < Requested.Num(); ++Index) { Identities.Emplace(P13WidgetEventGuid(*Blueprint, *Graph, AgentKey, FString::Printf(TEXT("action.%d.get"), Index)), FString::Printf(TEXT("action.%d.get"), Index)); Identities.Emplace(P13WidgetEventGuid(*Blueprint, *Graph, AgentKey, FString::Printf(TEXT("action.%d.call"), Index)), FString::Printf(TEXT("action.%d.call"), Index)); }
        for (const TPair<FGuid, FString>& Identity : Identities)
        {
            if (!Identity.Key.IsValid()) return ErrorResponse(Id, TEXT("unsupported"), TEXT("deterministic widget event identity derivation failed"));
            for (UEdGraphNode* Node : Graph->Nodes) if (Node && Node->NodeGuid == Identity.Key) return ErrorResponse(Id, TEXT("conflict"), TEXT("deterministic widget event identity is already occupied"));
        }
        const bool WasDirty = Blueprint->GetOutermost() && Blueprint->GetOutermost()->IsDirty(); const EBlueprintStatus BeforeStatus = Blueprint->Status;
        FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P13Event", "Magi AXI Ensure Widget Activate Event")); Blueprint->Modify(); Graph->Modify();
        TArray<UEdGraphNode*> CreatedNodes;
        UK2Node_CustomEvent* NewEvent = NewObject<UK2Node_CustomEvent>(Graph, NAME_None, RF_Transactional); NewEvent->CustomFunctionName = FName(*EventName); NewEvent->NodeGuid = Identities[0].Key; NewEvent->NodePosX = -900; NewEvent->NodePosY = FCString::Strtoi(*Sha256(AgentKey).Left(4), nullptr, 16) % 4000; NewEvent->bCallInEditor = false; NewEvent->bOverrideFunction = false; P11SetNodeOwner(*Blueprint, *NewEvent, P13WidgetEventOwner(AgentKey, TEXT("entry"))); Graph->AddNode(NewEvent, true, false); NewEvent->PostPlacedNewNode(); NewEvent->AllocateDefaultPins(); CreatedNodes.Add(NewEvent);
        const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>(); bool Built = Schema != nullptr; UEdGraphPin* PreviousExec = NewEvent->FindPin(UEdGraphSchema_K2::PN_Then, EGPD_Output);
        for (int32 Index = 0; Built && Index < Requested.Num(); ++Index)
        {
            const FP13WidgetEventAction& Action = Requested[Index]; UWidget* Target = WidgetById.FindRef(Action.TargetWidgetId); const FString GetRole = FString::Printf(TEXT("action.%d.get"), Index); const FString CallRole = FString::Printf(TEXT("action.%d.call"), Index);
            UK2Node_VariableGet* Getter = NewObject<UK2Node_VariableGet>(Graph, NAME_None, RF_Transactional); Getter->VariableReference.SetSelfMember(Target->GetFName()); Getter->NodeGuid = P13WidgetEventGuid(*Blueprint, *Graph, AgentKey, GetRole); Getter->NodePosX = -650 + Index * 400; Getter->NodePosY = NewEvent->NodePosY + 170; P11SetNodeOwner(*Blueprint, *Getter, P13WidgetEventOwner(AgentKey, GetRole)); Graph->AddNode(Getter, true, false); Getter->PostPlacedNewNode(); Getter->AllocateDefaultPins(); CreatedNodes.Add(Getter);
            UFunction* Function = Action.Kind == TEXT("text.set") ? UTextBlock::StaticClass()->FindFunctionByName(FName(TEXT("SetText"))) : Action.Kind == TEXT("enabled.set") ? UWidget::StaticClass()->FindFunctionByName(FName(TEXT("SetIsEnabled"))) : UWidget::StaticClass()->FindFunctionByName(FName(TEXT("SetVisibility")));
            UK2Node_CallFunction* Call = NewObject<UK2Node_CallFunction>(Graph, NAME_None, RF_Transactional); if (Function) Call->SetFromFunction(Function); Call->NodeGuid = P13WidgetEventGuid(*Blueprint, *Graph, AgentKey, CallRole); Call->NodePosX = -500 + Index * 400; Call->NodePosY = NewEvent->NodePosY; P11SetNodeOwner(*Blueprint, *Call, P13WidgetEventOwner(AgentKey, CallRole)); Graph->AddNode(Call, true, false); Call->PostPlacedNewNode(); Call->AllocateDefaultPins(); CreatedNodes.Add(Call);
            UEdGraphPin* SelfPin = Call->FindPin(UEdGraphSchema_K2::PN_Self, EGPD_Input); UEdGraphPin* ExecutePin = Call->FindPin(UEdGraphSchema_K2::PN_Execute, EGPD_Input); UEdGraphPin* ValuePin = Action.Kind == TEXT("text.set") ? Call->FindPin(TEXT("InText"), EGPD_Input) : Action.Kind == TEXT("enabled.set") ? Call->FindPin(TEXT("bInIsEnabled"), EGPD_Input) : Call->FindPin(TEXT("InVisibility"), EGPD_Input);
            Built = Function && PreviousExec && Getter->GetValuePin() && SelfPin && ExecutePin && ValuePin && Schema->TryCreateConnection(Getter->GetValuePin(), SelfPin) && Schema->TryCreateConnection(PreviousExec, ExecutePin);
            if (Built)
            {
                if (Action.Kind == TEXT("text.set")) Schema->TrySetDefaultText(*ValuePin, FText::FromString(Action.Text));
                else Schema->TrySetDefaultValue(*ValuePin, Action.Kind == TEXT("enabled.set") ? (Action.Enabled ? TEXT("true") : TEXT("false")) : Action.Visibility);
                Built = Action.Kind == TEXT("text.set") ? ValuePin->DefaultTextValue.ToString() == Action.Text : Action.Kind == TEXT("enabled.set") ? ValuePin->DefaultValue == (Action.Enabled ? TEXT("true") : TEXT("false")) : ValuePin->DefaultValue == Action.Visibility;
            }
            PreviousExec = Call->FindPin(UEdGraphSchema_K2::PN_Then, EGPD_Output);
        }
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
        TArray<FP13WidgetEventAction> Readback;
#if WITH_DEV_AUTOMATION_TESTS
        const bool Injected = GP11ForceAtomicFailure;
#else
        const bool Injected = false;
#endif
        auto Rollback = [&](const TCHAR* Message)
        {
            for (int32 Index = CreatedNodes.Num() - 1; Index >= 0; --Index) if (UEdGraphNode* Node = CreatedNodes[Index]) { P11ClearNodeOwner(*Blueprint, *Node); Node->BreakAllNodeLinks(); Graph->RemoveNode(Node); Node->MarkAsGarbage(); }
            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint); Transaction.Cancel(); Blueprint->Status = BeforeStatus; Blueprint->GetOutermost()->SetDirtyFlag(WasDirty); const FString Observed = BlueprintContentRevision(*Blueprint); const bool Rolled = Observed == CurrentRevision && Blueprint->Status == BeforeStatus && Blueprint->GetOutermost()->IsDirty() == WasDirty;
            return P11FailedAtomicResponse(Id, Operation, Args, BlueprintId + TEXT("#") + BlueprintId + TEXT("#event:") + AgentKey, CurrentRevision, Observed, Rolled, P11DirtyPackages(*Blueprint, false), Message);
        };
        if (!Built || !P13WidgetEventReadback(*Blueprint, *Graph, AgentKey, Readback) || !ActionsEqual(Readback, Requested) || BlueprintContentRevision(*Blueprint) == CurrentRevision || Injected) return Rollback(TEXT("widget activate event atomic postcondition failed"));
        const FString Response = P11AtomicResponse(Id, SuccessResponse(Id, P13WidgetEventResult(*Blueprint, AgentKey, Requested, true), Operation, Args));
        return ResponseStatusIsOk(Response) ? Response : Rollback(TEXT("widget activate event receipt postcondition failed"));
    }
    if (Operation == TEXT("widget.tree_view")) return SuccessResponse(Id, P13TreeResult(*Blueprint), Operation, Args);
    UVerticalBox* Root = Cast<UVerticalBox>(Widgets[0]);
    if (Operation == TEXT("widget.child_ensure"))
    {
        FString ParentId, Name, Class; Args->TryGetStringField(TEXT("parentWidgetId"), ParentId); Args->TryGetStringField(TEXT("name"), Name); Args->TryGetStringField(TEXT("class"), Class); const FString RootId = P13WidgetId(*Blueprint, *Root);
        if (ParentId != RootId || Class != TEXT("TextBlock") || !P13ValidName(Name)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("only valid TextBlock direct children of root VerticalBox supported"));
        for (UWidget* Widget : Widgets) if (Widget->GetName() == Name) { if (!Cast<UTextBlock>(Widget)) return ErrorResponse(Id, TEXT("conflict"), TEXT("widget name exists with incompatible class")); const TSharedRef<FJsonObject> R = P13MutationResult(*Blueprint, P13WidgetId(*Blueprint, *Widget), FString(), false); R->SetStringField(TEXT("name"), Name); R->SetStringField(TEXT("class"), Class); R->SetStringField(TEXT("parentWidgetId"), RootId); const FString Response = SuccessResponse(Id, R, Operation, Args); return ResponseStatusIsOk(Response) ? Response : P11FailedAtomicResponse(Id, Operation, Args, BlueprintId + TEXT("#") + P13WidgetId(*Blueprint, *Widget), CurrentRevision, CurrentRevision, true, P11DirtyPackages(*Blueprint, false), TEXT("widget child no-op receipt postcondition failed")); }
        const bool WasDirty = Blueprint->GetOutermost() && Blueprint->GetOutermost()->IsDirty(); const EBlueprintStatus BeforeStatus = Blueprint->Status; FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P13Child", "Magi AXI Ensure Widget Child")); Blueprint->Modify(); Root->Modify();
        UTextBlock* Child = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*Name));
        if (!Child)
        {
            Transaction.Cancel(); Blueprint->Status = BeforeStatus; Blueprint->GetOutermost()->SetDirtyFlag(WasDirty); const FString Observed = BlueprintContentRevision(*Blueprint); const bool Rolled = Observed == CurrentRevision && Blueprint->Status == BeforeStatus && Blueprint->GetOutermost()->IsDirty() == WasDirty;
            return P11FailedAtomicResponse(Id, Operation, Args, BlueprintId + TEXT("#") + BlueprintId + TEXT("#widget:") + Name, CurrentRevision, Observed, Rolled, P11DirtyPackages(*Blueprint, false), TEXT("widget child creation failed"));
        }
        Child->bIsVariable = true; Child->SetText(FText::GetEmpty()); Child->SetVisibility(ESlateVisibility::Visible); Child->SetIsEnabled(true); Root->AddChild(Child); Blueprint->OnVariableAdded(Child->GetFName()); FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint); Blueprint->GetOutermost()->MarkPackageDirty();
        auto RollbackChild = [&](const TCHAR* Message)
        {
            Blueprint->OnVariableRemoved(Child->GetFName()); Root->RemoveChild(Child); Child->MarkAsGarbage(); Transaction.Cancel(); Blueprint->Status = BeforeStatus; Blueprint->GetOutermost()->SetDirtyFlag(WasDirty); const FString Observed = BlueprintContentRevision(*Blueprint); const bool Rolled = Root->GetChildIndex(Child) < 0 && Observed == CurrentRevision && Blueprint->Status == BeforeStatus && Blueprint->GetOutermost()->IsDirty() == WasDirty;
            return P11FailedAtomicResponse(Id, Operation, Args, BlueprintId + TEXT("#") + BlueprintId + TEXT("#widget:") + Name, CurrentRevision, Observed, Rolled, P11DirtyPackages(*Blueprint, false), Message);
        };
        TArray<UWidget*> After; FString Error; const bool Verified = P13WidgetTree(*Blueprint, After, Error) && After.Contains(Child) && Child->GetName() == Name && Child->GetClass() == UTextBlock::StaticClass() && Root->GetChildIndex(Child) >= 0;
#if WITH_DEV_AUTOMATION_TESTS
        const bool Injected = GP11ForceAtomicFailure;
#else
        const bool Injected = false;
#endif
        if (!Verified || Injected) return RollbackChild(TEXT("widget child atomic postcondition failed"));
        const TSharedRef<FJsonObject> R = P13MutationResult(*Blueprint, P13WidgetId(*Blueprint, *Child), FString(), true); R->SetStringField(TEXT("name"), Name); R->SetStringField(TEXT("class"), Class); R->SetStringField(TEXT("parentWidgetId"), RootId);
        const FString Response = SuccessResponse(Id, R, Operation, Args); return ResponseStatusIsOk(Response) ? Response : RollbackChild(TEXT("widget child receipt postcondition failed"));
    }
    FString WidgetId, Property; Args->TryGetStringField(TEXT("widgetId"), WidgetId); Args->TryGetStringField(TEXT("property"), Property); UWidget* Widget = nullptr; for (UWidget* Candidate : Widgets) if (P13WidgetId(*Blueprint, *Candidate) == WidgetId) Widget = Candidate; if (!Widget) return ErrorResponse(Id, TEXT("not_found"), TEXT("widget was not found"));
    if (Property != TEXT("text") && Property != TEXT("visibility") && Property != TEXT("enabled")) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("property must be text, visibility, or enabled"));
    const FName BoundProperty = Property == TEXT("text") ? FName(TEXT("Text")) : Property == TEXT("visibility") ? FName(TEXT("Visibility")) : FName(TEXT("bIsEnabled"));
    for (const FDelegateEditorBinding& Binding : Blueprint->Bindings) if (Binding.ObjectName == Widget->GetName() && Binding.PropertyName == BoundProperty) return ErrorResponse(Id, TEXT("conflict"), TEXT("widget property is bound"));
    FString TextValue, VisibilityValue; bool BoolValue = false; ESlateVisibility NewVisibility = ESlateVisibility::Visible;
    if (Property == TEXT("text")) { if (!Cast<UTextBlock>(Widget) || !Args->TryGetStringField(TEXT("text"), TextValue)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("text property requires TextBlock and text")); }
    else if (Property == TEXT("visibility")) { if (!Args->TryGetStringField(TEXT("visibility"), VisibilityValue) || !P13Visibility(VisibilityValue, NewVisibility)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("visibility must be Visible, Hidden, or Collapsed")); }
    else if (!Args->TryGetBoolField(TEXT("enabled"), BoolValue)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("enabled property requires boolean enabled"));
    FText OldText = FText::GetEmpty(); ESlateVisibility OldVisibility = Widget->GetVisibility(); const bool OldEnabled = Widget->GetIsEnabled(); if (const UTextBlock* Text = Cast<UTextBlock>(Widget)) OldText = Text->GetText();
    const bool Changed = Property == TEXT("text") ? OldText.ToString() != TextValue : Property == TEXT("visibility") ? OldVisibility != NewVisibility : OldEnabled != BoolValue;
    auto PropertyResult = [&]() { const TSharedRef<FJsonObject> R = P13MutationResult(*Blueprint, WidgetId, Property, Changed); if (Property == TEXT("text")) R->SetStringField(TEXT("text"), TextValue); else if (Property == TEXT("visibility")) R->SetStringField(TEXT("visibility"), VisibilityValue); else R->SetBoolField(TEXT("enabled"), BoolValue); return R; };
    if (!Changed)
    {
        const FString Response = SuccessResponse(Id, PropertyResult(), Operation, Args);
        return ResponseStatusIsOk(Response) ? Response : P11FailedAtomicResponse(Id, Operation, Args, BlueprintId + TEXT("#") + WidgetId + TEXT("#") + Property, CurrentRevision, CurrentRevision, true, P11DirtyPackages(*Blueprint, false), TEXT("widget property receipt postcondition failed"));
    }
    const bool WasDirty = Blueprint->GetOutermost() && Blueprint->GetOutermost()->IsDirty(); const EBlueprintStatus BeforeStatus = Blueprint->Status; const FString BeforeRevision = CurrentRevision; FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P13Property", "Magi AXI Set Widget Property")); Blueprint->Modify(); Widget->Modify(); if (Property == TEXT("text")) Cast<UTextBlock>(Widget)->SetText(FText::FromString(TextValue)); else if (Property == TEXT("visibility")) Widget->SetVisibility(NewVisibility); else Widget->SetIsEnabled(BoolValue); FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); Blueprint->GetOutermost()->MarkPackageDirty();
    auto RollbackProperty = [&](const TCHAR* Message)
    {
        if (Property == TEXT("text")) Cast<UTextBlock>(Widget)->SetText(OldText); else if (Property == TEXT("visibility")) Widget->SetVisibility(OldVisibility); else Widget->SetIsEnabled(OldEnabled); Transaction.Cancel(); Blueprint->Status = BeforeStatus; Blueprint->GetOutermost()->SetDirtyFlag(WasDirty); const FString Observed = BlueprintContentRevision(*Blueprint); const bool TextRestored = Property != TEXT("text") || Cast<UTextBlock>(Widget)->GetText().IdenticalTo(OldText); const bool Rolled = TextRestored && Observed == BeforeRevision && Blueprint->Status == BeforeStatus && Blueprint->GetOutermost()->IsDirty() == WasDirty;
        return P11FailedAtomicResponse(Id, Operation, Args, BlueprintId + TEXT("#") + WidgetId + TEXT("#") + Property, BeforeRevision, Observed, Rolled, P11DirtyPackages(*Blueprint, false), Message);
    };
#if WITH_DEV_AUTOMATION_TESTS
    const bool Injected = GP11ForceAtomicFailure;
#else
    const bool Injected = false;
#endif
    const bool Applied = Property == TEXT("text") ? Cast<UTextBlock>(Widget)->GetText().ToString() == TextValue : Property == TEXT("visibility") ? Widget->GetVisibility() == NewVisibility : Widget->GetIsEnabled() == BoolValue;
    if (!Applied || Injected) return RollbackProperty(TEXT("widget property atomic postcondition failed"));
    const FString Response = SuccessResponse(Id, PropertyResult(), Operation, Args); return ResponseStatusIsOk(Response) ? Response : RollbackProperty(TEXT("widget property receipt postcondition failed"));
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIP13WidgetCoreContracts, "MagiUnrealAXI.P13.WidgetCoreContracts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIP13WidgetCoreContracts::RunTest(const FString&)
{
    const FString Suffix = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    const FString Path = TEXT("/Game/MagiP13Automation/P13WidgetCoreContracts_") + Suffix;
    const FString AssetId = Path + TEXT(".") + FPackageName::GetLongPackageAssetName(Path);
    const TSharedRef<FJsonObject> Create = MakeShared<FJsonObject>();
    Create->SetStringField(TEXT("path"), Path); Create->SetStringField(TEXT("rootName"), TEXT("Root")); Create->SetStringField(TEXT("rootClass"), TEXT("VerticalBox"));
    const FString Created = HandleP13WidgetOperation(TEXT("p13-contract-create"), TEXT("widget.create"), Create, FString());
    if (!ResponseStatusIsOk(Created)) AddError(Created);
    TestTrue(TEXT("create returns exact success envelope"), ResponseStatusIsOk(Created));
    UWidgetBlueprint* Blueprint = P13LoadWidgetBlueprint(AssetId);
    TestNotNull(TEXT("created widget Blueprint exists"), Blueprint);
    if (!Blueprint) return false;
    const FString RootId = AssetId + TEXT("#widget:Root");
    const TSharedRef<FJsonObject> Tree = MakeShared<FJsonObject>(); Tree->SetStringField(TEXT("blueprintId"), AssetId);
    const FString TreeResponse = HandleP13WidgetOperation(TEXT("p13-contract-tree"), TEXT("widget.tree_view"), Tree, FString());
    TestTrue(TEXT("tree view returns root readback"), ResponseStatusIsOk(TreeResponse) && TreeResponse.Contains(RootId));
    if (!ResponseStatusIsOk(TreeResponse)) { AddError(TreeResponse); AddError(Serialize(P13TreeResult(*Blueprint))); }
    const TSharedRef<FJsonObject> Child = MakeShared<FJsonObject>();
    Child->SetStringField(TEXT("blueprintId"), AssetId); Child->SetStringField(TEXT("parentWidgetId"), RootId); Child->SetStringField(TEXT("name"), TEXT("Label")); Child->SetStringField(TEXT("class"), TEXT("TextBlock"));
    const FString ChildResponse = HandleP13WidgetOperation(TEXT("p13-contract-child"), TEXT("widget.child_ensure"), Child, BlueprintContentRevision(*Blueprint));
    if (!ResponseStatusIsOk(ChildResponse)) AddError(ChildResponse);
    TestTrue(TEXT("child ensure returns exact success envelope"), ResponseStatusIsOk(ChildResponse) && ChildResponse.Contains(TEXT("TextBlock")));
    const FString ChildId = AssetId + TEXT("#widget:Label");
    FString Revision = BlueprintContentRevision(*Blueprint);
    const TSharedRef<FJsonObject> Text = MakeShared<FJsonObject>(); Text->SetStringField(TEXT("blueprintId"), AssetId); Text->SetStringField(TEXT("widgetId"), ChildId); Text->SetStringField(TEXT("property"), TEXT("text")); Text->SetStringField(TEXT("text"), TEXT("P13"));
    const FString TextResponse = HandleP13WidgetOperation(TEXT("p13-contract-text"), TEXT("widget.property_set"), Text, Revision);
    TestTrue(TEXT("text property returns exact success envelope"), ResponseStatusIsOk(TextResponse) && TextResponse.Contains(TEXT("P13")));
    Revision = BlueprintContentRevision(*Blueprint);
    const TSharedRef<FJsonObject> Visibility = MakeShared<FJsonObject>(); Visibility->SetStringField(TEXT("blueprintId"), AssetId); Visibility->SetStringField(TEXT("widgetId"), ChildId); Visibility->SetStringField(TEXT("property"), TEXT("visibility")); Visibility->SetStringField(TEXT("visibility"), TEXT("Hidden"));
    const FString VisibilityResponse = HandleP13WidgetOperation(TEXT("p13-contract-visibility"), TEXT("widget.property_set"), Visibility, Revision);
    TestTrue(TEXT("visibility property returns exact success envelope"), ResponseStatusIsOk(VisibilityResponse) && VisibilityResponse.Contains(TEXT("Hidden")));

    Revision = BlueprintContentRevision(*Blueprint);
    const TSharedRef<FJsonObject> Enabled = MakeShared<FJsonObject>(); Enabled->SetStringField(TEXT("blueprintId"), AssetId); Enabled->SetStringField(TEXT("widgetId"), ChildId); Enabled->SetStringField(TEXT("property"), TEXT("enabled")); Enabled->SetBoolField(TEXT("enabled"), false);
    const FString EnabledResponse = HandleP13WidgetOperation(TEXT("p13-contract-enabled"), TEXT("widget.property_set"), Enabled, Revision);
    TestTrue(TEXT("enabled property returns exact success envelope"), ResponseStatusIsOk(EnabledResponse) && EnabledResponse.Contains(TEXT("enabled")));
    const FString RepeatResponse = HandleP13WidgetOperation(TEXT("p13-contract-repeat"), TEXT("widget.create"), Create, FString());
    TSharedPtr<FJsonObject> RepeatEnvelope; const TSharedPtr<FJsonObject>* RepeatResult = nullptr;
    const bool RepeatNoop = ResponseStatusIsOk(RepeatResponse) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(RepeatResponse), RepeatEnvelope) && RepeatEnvelope.IsValid() && RepeatEnvelope->TryGetObjectField(TEXT("result"), RepeatResult) && RepeatResult && (*RepeatResult)->GetBoolField(TEXT("changed")) == false;
    TestTrue(TEXT("unique fixture repeat is no-op success"), RepeatNoop);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIP13WidgetCoreRollback, "MagiUnrealAXI.P13.WidgetCoreRollback", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIP13WidgetCoreRollback::RunTest(const FString&)
{
    const FString Suffix = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    const FString Path = TEXT("/Game/MagiP13Automation/P13WidgetCoreRollback_") + Suffix;
    const FString AssetId = Path + TEXT(".") + FPackageName::GetLongPackageAssetName(Path);
    const TSharedRef<FJsonObject> Create = MakeShared<FJsonObject>(); Create->SetStringField(TEXT("path"), Path); Create->SetStringField(TEXT("rootName"), TEXT("Root")); Create->SetStringField(TEXT("rootClass"), TEXT("VerticalBox"));
    const FString CreateResponse = HandleP13WidgetOperation(TEXT("p13-rollback-create"), TEXT("widget.create"), Create, FString());
    if (!ResponseStatusIsOk(CreateResponse)) AddError(CreateResponse);
    TestTrue(TEXT("rollback fixture creates independently"), ResponseStatusIsOk(CreateResponse));
    UWidgetBlueprint* Blueprint = P13LoadWidgetBlueprint(AssetId);
    TestNotNull(TEXT("rollback fixture exists"), Blueprint);
    if (!Blueprint) return false;
    const FString RootId = AssetId + TEXT("#widget:Root");
    const TSharedRef<FJsonObject> Child = MakeShared<FJsonObject>(); Child->SetStringField(TEXT("blueprintId"), AssetId); Child->SetStringField(TEXT("parentWidgetId"), RootId); Child->SetStringField(TEXT("name"), TEXT("Label")); Child->SetStringField(TEXT("class"), TEXT("TextBlock"));
    const FString ChildResponse = HandleP13WidgetOperation(TEXT("p13-rollback-fixture-child"), TEXT("widget.child_ensure"), Child, BlueprintContentRevision(*Blueprint));
    TestTrue(TEXT("rollback fixture child creates"), ResponseStatusIsOk(ChildResponse));
    if (!ResponseStatusIsOk(ChildResponse)) return false;
    auto GuidMap = [&]()
    {
        TArray<FString> Rows;
        for (const TPair<FName, FGuid>& Variable : Blueprint->WidgetVariableNameToGuidMap) Rows.Add(Variable.Key.ToString() + TEXT("=") + Variable.Value.ToString());
        Rows.Sort(); return FString::Join(Rows, TEXT(";"));
    };
    auto LedgerReceipt = [&](const FString& OperationId) -> TSharedPtr<FJsonObject>
    {
        const TSharedRef<FJsonObject> ViewArgs = MakeShared<FJsonObject>(); ViewArgs->SetStringField(TEXT("id"), OperationId);
        TSharedPtr<FJsonObject> Envelope; const TSharedPtr<FJsonObject>* Result = nullptr;
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ReadResponseOnGameThread(TEXT("p13-operation-view"), TEXT("operation.view"), ViewArgs)), Envelope);
        return Envelope.IsValid() && Envelope->TryGetObjectField(TEXT("result"), Result) && Result && Result->IsValid() ? *Result : nullptr;
    };
    const FString CreateRollbackPath = TEXT("/Game/MagiP13Automation/P13WidgetCreateRollback_") + Suffix;
    const FString CreateRollbackId = CreateRollbackPath + TEXT(".") + FPackageName::GetLongPackageAssetName(CreateRollbackPath);
    const TSharedRef<FJsonObject> CreateRollbackArgs = MakeShared<FJsonObject>(); CreateRollbackArgs->SetStringField(TEXT("path"), CreateRollbackPath); CreateRollbackArgs->SetStringField(TEXT("rootName"), TEXT("Root")); CreateRollbackArgs->SetStringField(TEXT("rootClass"), TEXT("VerticalBox"));
    GP11ForceAtomicFailure = true; const FString CreateRollbackResponse = HandleP13WidgetOperation(TEXT("p13-create-rollback"), TEXT("widget.create"), CreateRollbackArgs, FString()); GP11ForceAtomicFailure = false;
    TSharedPtr<FJsonObject> CreateRollbackEnvelope; const TSharedPtr<FJsonObject>* CreateRollbackReceipt = nullptr;
    const bool HasCreateRollbackReceipt = FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(CreateRollbackResponse), CreateRollbackEnvelope) && CreateRollbackEnvelope.IsValid() && CreateRollbackEnvelope->TryGetObjectField(TEXT("receipt"), CreateRollbackReceipt) && CreateRollbackReceipt && CreateRollbackReceipt->IsValid();
    TestTrue(TEXT("forced create returns failed receipt"), HasCreateRollbackReceipt);
    if (HasCreateRollbackReceipt) { TestEqual(TEXT("failed create receipt state is exact"), (*CreateRollbackReceipt)->GetStringField(TEXT("state")), FString(TEXT("failed"))); TestEqual(TEXT("failed create receipt target is exact"), (*CreateRollbackReceipt)->GetStringField(TEXT("target")), CreateRollbackId); }
    const TSharedPtr<FJsonObject> CreateRollbackLedger = LedgerReceipt(TEXT("p13-create-rollback"));
    TestTrue(TEXT("failed create receipt is visible through operation.view"), CreateRollbackLedger.IsValid());
    if (HasCreateRollbackReceipt && CreateRollbackLedger.IsValid()) TestEqual(TEXT("failed create receipt round-trips through operation ledger"), Serialize(CreateRollbackLedger.ToSharedRef()), Serialize((*CreateRollbackReceipt).ToSharedRef()));
    TestFalse(TEXT("failed create leaves no package on disk"), FPackageName::DoesPackageExist(CreateRollbackPath)); TestNull(TEXT("failed create leaves no loaded package"), FindPackage(nullptr, *CreateRollbackPath)); TestNull(TEXT("failed create leaves no object"), FindObject<UObject>(nullptr, *CreateRollbackId)); TestFalse(TEXT("failed create leaves no registry asset"), FAssetRegistryModule::GetRegistry().GetAssetByObjectPath(FSoftObjectPath(CreateRollbackId)).IsValid());
    TestTrue(TEXT("failed create path can be retried"), ResponseStatusIsOk(HandleP13WidgetOperation(TEXT("p13-create-retry"), TEXT("widget.create"), CreateRollbackArgs, FString())));
    auto RunForced = [&](const FString& Operation, const TSharedRef<FJsonObject>& Args, const FString& ExpectedTarget)
    {
        const FString OperationId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
        const FString BeforeTree = Serialize(P13TreeResult(*Blueprint)); const FString BeforeGuids = GuidMap(); const FString BeforeRevision = BlueprintContentRevision(*Blueprint); const EBlueprintStatus BeforeStatus = Blueprint->Status; const bool BeforeDirty = Blueprint->GetOutermost()->IsDirty();
        GP11ForceAtomicFailure = true; const FString Response = HandleP13WidgetOperation(OperationId, Operation, Args, BeforeRevision); GP11ForceAtomicFailure = false;
        TSharedPtr<FJsonObject> Envelope; const TSharedPtr<FJsonObject>* Receipt = nullptr;
        const bool HasReceipt = FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response), Envelope) && Envelope.IsValid() && Envelope->TryGetObjectField(TEXT("receipt"), Receipt) && Receipt && Receipt->IsValid();
        const TSharedPtr<FJsonObject> Ledger = LedgerReceipt(OperationId);
        TestTrue(TEXT("forced operation returns failed receipt"), HasReceipt);
        if (HasReceipt)
        {
            TestEqual(TEXT("failed receipt state is exact"), (*Receipt)->GetStringField(TEXT("state")), FString(TEXT("failed")));
            TestEqual(TEXT("failed receipt operation is exact"), (*Receipt)->GetStringField(TEXT("operation")), Operation);
            TestEqual(TEXT("failed receipt target is exact"), (*Receipt)->GetStringField(TEXT("target")), ExpectedTarget);
        }
        TestTrue(TEXT("failed receipt is visible through operation.view"), Ledger.IsValid());
        if (HasReceipt && Ledger.IsValid()) TestEqual(TEXT("failed receipt round-trips through operation ledger"), Serialize(Ledger.ToSharedRef()), Serialize((*Receipt).ToSharedRef()));
        TestEqual(TEXT("rollback restores public tree"), Serialize(P13TreeResult(*Blueprint)), BeforeTree);
        TestEqual(TEXT("rollback restores GUID map"), GuidMap(), BeforeGuids);
        TestEqual(TEXT("rollback restores revision"), BlueprintContentRevision(*Blueprint), BeforeRevision);
        TestEqual(TEXT("rollback restores status"), Blueprint->Status, BeforeStatus);
        TestEqual(TEXT("rollback restores dirty state"), Blueprint->GetOutermost()->IsDirty(), BeforeDirty);
    };
    const TSharedRef<FJsonObject> FailedChild = MakeShared<FJsonObject>(); FailedChild->SetStringField(TEXT("blueprintId"), AssetId); FailedChild->SetStringField(TEXT("parentWidgetId"), RootId); FailedChild->SetStringField(TEXT("name"), TEXT("ForcedChild")); FailedChild->SetStringField(TEXT("class"), TEXT("TextBlock"));
    RunForced(TEXT("widget.child_ensure"), FailedChild, AssetId + TEXT("#") + AssetId + TEXT("#widget:ForcedChild"));
    const FString ChildId = AssetId + TEXT("#widget:Label");
    UTextBlock* RollbackText = nullptr; TArray<UWidget*> RollbackWidgets; FString RollbackTreeError; P13WidgetTree(*Blueprint, RollbackWidgets, RollbackTreeError); for (UWidget* Candidate : RollbackWidgets) if (P13WidgetId(*Blueprint, *Candidate) == ChildId) RollbackText = Cast<UTextBlock>(Candidate);
    const FText KeyedText = FText::ChangeKey(TEXT("MagiP13"), TEXT("RollbackLabel"), FText::FromString(TEXT("before"))); if (RollbackText) RollbackText->SetText(KeyedText);
    for (const TCHAR* Property : {TEXT("text"), TEXT("visibility"), TEXT("enabled")})
    {
        const TSharedRef<FJsonObject> Args = MakeShared<FJsonObject>(); Args->SetStringField(TEXT("blueprintId"), AssetId); Args->SetStringField(TEXT("widgetId"), ChildId); Args->SetStringField(TEXT("property"), Property);
        if (FCString::Strcmp(Property, TEXT("text")) == 0) Args->SetStringField(TEXT("text"), TEXT("forced")); else if (FCString::Strcmp(Property, TEXT("visibility")) == 0) Args->SetStringField(TEXT("visibility"), TEXT("Hidden")); else Args->SetBoolField(TEXT("enabled"), false);
        RunForced(TEXT("widget.property_set"), Args, AssetId + TEXT("#") + ChildId + TEXT("#") + Property);
        if (FCString::Strcmp(Property, TEXT("text")) == 0) TestTrue(TEXT("text rollback preserves FText identity"), RollbackText && RollbackText->GetText().IdenticalTo(KeyedText));
    }
    const TSharedRef<FJsonObject> FailedEvent = MakeShared<FJsonObject>(); FailedEvent->SetStringField(TEXT("blueprintId"), AssetId); FailedEvent->SetStringField(TEXT("agentKey"), TEXT("rollback-event")); FailedEvent->SetStringField(TEXT("event"), TEXT("activate")); const TSharedRef<FJsonObject> FailedAction = MakeShared<FJsonObject>(); FailedAction->SetStringField(TEXT("kind"), TEXT("text.set")); FailedAction->SetStringField(TEXT("targetWidgetId"), ChildId); FailedAction->SetStringField(TEXT("text"), TEXT("forced event")); FailedEvent->SetArrayField(TEXT("actions"), {MakeShared<FJsonValueObject>(FailedAction)});
    RunForced(TEXT("widget.event_ensure"), FailedEvent, AssetId + TEXT("#") + AssetId + TEXT("#event:rollback-event"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIP13WidgetEventContracts, "MagiUnrealAXI.P13.WidgetEventContracts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIP13WidgetEventContracts::RunTest(const FString&)
{
    const FString Suffix = FGuid::NewGuid().ToString(EGuidFormats::Digits); const FString Path = TEXT("/Game/MagiP13Automation/P13WidgetEvent_") + Suffix; const FString AssetId = Path + TEXT(".") + FPackageName::GetLongPackageAssetName(Path);
    const TSharedRef<FJsonObject> Create = MakeShared<FJsonObject>(); Create->SetStringField(TEXT("path"), Path); Create->SetStringField(TEXT("rootName"), TEXT("Root")); Create->SetStringField(TEXT("rootClass"), TEXT("VerticalBox")); const FString CreateResponse = HandleP13WidgetOperation(TEXT("p13-event-create"), TEXT("widget.create"), Create, FString()); if (!ResponseStatusIsOk(CreateResponse)) AddError(CreateResponse); TestTrue(TEXT("event fixture creates"), ResponseStatusIsOk(CreateResponse));
    UWidgetBlueprint* Blueprint = P13LoadWidgetBlueprint(AssetId); TestNotNull(TEXT("event fixture exists"), Blueprint); if (!Blueprint) return false;
    const FString RootId = AssetId + TEXT("#widget:Root"); const TSharedRef<FJsonObject> Child = MakeShared<FJsonObject>(); Child->SetStringField(TEXT("blueprintId"), AssetId); Child->SetStringField(TEXT("parentWidgetId"), RootId); Child->SetStringField(TEXT("name"), TEXT("Label")); Child->SetStringField(TEXT("class"), TEXT("TextBlock")); const FString ChildResponse = HandleP13WidgetOperation(TEXT("p13-event-child"), TEXT("widget.child_ensure"), Child, BlueprintContentRevision(*Blueprint)); if (!ResponseStatusIsOk(ChildResponse)) AddError(ChildResponse); TestTrue(TEXT("event fixture child creates"), ResponseStatusIsOk(ChildResponse));
    const FString ChildId = AssetId + TEXT("#widget:Label"); const TSharedRef<FJsonObject> Args = MakeShared<FJsonObject>(); Args->SetStringField(TEXT("blueprintId"), AssetId); Args->SetStringField(TEXT("agentKey"), TEXT("ui.activate")); Args->SetStringField(TEXT("event"), TEXT("activate"));
    const TSharedRef<FJsonObject> TextAction = MakeShared<FJsonObject>(); TextAction->SetStringField(TEXT("kind"), TEXT("text.set")); TextAction->SetStringField(TEXT("targetWidgetId"), ChildId); TextAction->SetStringField(TEXT("text"), TEXT("ACTIVE"));
    const TSharedRef<FJsonObject> EnabledAction = MakeShared<FJsonObject>(); EnabledAction->SetStringField(TEXT("kind"), TEXT("enabled.set")); EnabledAction->SetStringField(TEXT("targetWidgetId"), ChildId); EnabledAction->SetBoolField(TEXT("enabled"), true);
    const TSharedRef<FJsonObject> VisibilityAction = MakeShared<FJsonObject>(); VisibilityAction->SetStringField(TEXT("kind"), TEXT("visibility.set")); VisibilityAction->SetStringField(TEXT("targetWidgetId"), RootId); VisibilityAction->SetStringField(TEXT("visibility"), TEXT("Visible"));
    Args->SetArrayField(TEXT("actions"), {MakeShared<FJsonValueObject>(TextAction), MakeShared<FJsonValueObject>(EnabledAction), MakeShared<FJsonValueObject>(VisibilityAction)});
    const FString Success = HandleP13WidgetOperation(TEXT("p13-event-success"), TEXT("widget.event_ensure"), Args, BlueprintContentRevision(*Blueprint)); if (!ResponseStatusIsOk(Success)) AddError(Success); TestTrue(TEXT("three-action event ensure succeeds with exact event identity"), ResponseStatusIsOk(Success) && Success.Contains(AssetId + TEXT("#event:ui.activate")));
    UEdGraph* Graph = P13WidgetEventGraph(*Blueprint); TArray<FP13WidgetEventAction> Readback; TestTrue(TEXT("underlying fixed K2 graph reads back exactly"), Graph && P13WidgetEventReadback(*Blueprint, *Graph, TEXT("ui.activate"), Readback) && Readback.Num() == 3);
    const FString Tree = Serialize(P13TreeResult(*Blueprint)); TestTrue(TEXT("tree view exposes deterministic event and actions"), Tree.Contains(AssetId + TEXT("#event:ui.activate")) && Tree.Contains(TEXT("visibility.set")) && Tree.Contains(TEXT("ACTIVE")));
    const FString BeforeNoopRevision = BlueprintContentRevision(*Blueprint); const EBlueprintStatus BeforeNoopStatus = Blueprint->Status; const bool BeforeNoopDirty = Blueprint->GetOutermost()->IsDirty(); const FString Noop = HandleP13WidgetOperation(TEXT("p13-event-noop"), TEXT("widget.event_ensure"), Args, BeforeNoopRevision); if (!ResponseStatusIsOk(Noop)) AddError(Noop); TSharedPtr<FJsonObject> NoopEnvelope; const TSharedPtr<FJsonObject>* NoopResult = nullptr; const bool NoopVerified = FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Noop), NoopEnvelope) && NoopEnvelope.IsValid() && NoopEnvelope->TryGetObjectField(TEXT("result"), NoopResult) && NoopResult && (*NoopResult)->GetBoolField(TEXT("changed")) == false; TestTrue(TEXT("event ensure repeats as no-op"), NoopVerified); TestEqual(TEXT("event no-op preserves revision"), BlueprintContentRevision(*Blueprint), BeforeNoopRevision); TestEqual(TEXT("event no-op preserves status"), Blueprint->Status, BeforeNoopStatus); TestEqual(TEXT("event no-op preserves dirty state"), Blueprint->GetOutermost()->IsDirty(), BeforeNoopDirty);
    const TSharedRef<FJsonObject> Foo = MakeShared<FJsonObject>(); Foo->SetStringField(TEXT("blueprintId"), AssetId); Foo->SetStringField(TEXT("agentKey"), TEXT("foo")); Foo->SetStringField(TEXT("event"), TEXT("activate")); Foo->SetArrayField(TEXT("actions"), {MakeShared<FJsonValueObject>(TextAction)}); const FString FooEnsure = HandleP13WidgetOperation(TEXT("p13-event-foo"), TEXT("widget.event_ensure"), Foo, BlueprintContentRevision(*Blueprint)); TestTrue(TEXT("short owner event ensure succeeds"), ResponseStatusIsOk(FooEnsure));
    const TSharedRef<FJsonObject> FooBar = MakeShared<FJsonObject>(); FooBar->SetStringField(TEXT("blueprintId"), AssetId); FooBar->SetStringField(TEXT("agentKey"), TEXT("foo:bar")); FooBar->SetStringField(TEXT("event"), TEXT("activate")); FooBar->SetArrayField(TEXT("actions"), {MakeShared<FJsonValueObject>(TextAction)}); const FString FooBarEnsure = HandleP13WidgetOperation(TEXT("p13-event-foobar"), TEXT("widget.event_ensure"), FooBar, BlueprintContentRevision(*Blueprint)); TestTrue(TEXT("colon owner event ensure succeeds independently"), ResponseStatusIsOk(FooBarEnsure));
    const TSharedRef<FJsonObject> CoexistTree = P13TreeResult(*Blueprint); const TArray<TSharedPtr<FJsonValue>>* CoexistEvents = nullptr; const bool CoexistSorted = CoexistTree->TryGetArrayField(TEXT("events"), CoexistEvents) && CoexistEvents && CoexistEvents->Num() == 3 && (*CoexistEvents)[0]->AsObject()->GetStringField(TEXT("agentKey")) == TEXT("foo") && (*CoexistEvents)[1]->AsObject()->GetStringField(TEXT("agentKey")) == TEXT("foo:bar") && (*CoexistEvents)[2]->AsObject()->GetStringField(TEXT("agentKey")) == TEXT("ui.activate"); TestTrue(TEXT("overlapping owner keys produce independent sorted tree rows"), CoexistSorted);
    TArray<FP13WidgetEventAction> FooReadback; TArray<FP13WidgetEventAction> FooBarReadback; TestTrue(TEXT("overlapping owner keys have exact independent node ownership"), Graph && P13WidgetEventReadback(*Blueprint, *Graph, TEXT("foo"), FooReadback) && FooReadback.Num() == 1 && P13WidgetEventReadback(*Blueprint, *Graph, TEXT("foo:bar"), FooBarReadback) && FooBarReadback.Num() == 1);
    auto IsEventNoop = [](const FString& Response) { TSharedPtr<FJsonObject> Envelope; const TSharedPtr<FJsonObject>* Result = nullptr; return ResponseStatusIsOk(Response) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response), Envelope) && Envelope.IsValid() && Envelope->TryGetObjectField(TEXT("result"), Result) && Result && !(*Result)->GetBoolField(TEXT("changed")); }; const FString OverlapRevision = BlueprintContentRevision(*Blueprint); const FString FooNoop = HandleP13WidgetOperation(TEXT("p13-event-foo-noop"), TEXT("widget.event_ensure"), Foo, OverlapRevision); const FString FooBarNoop = HandleP13WidgetOperation(TEXT("p13-event-foobar-noop"), TEXT("widget.event_ensure"), FooBar, OverlapRevision); TestTrue(TEXT("both overlapping owner keys repeat as no-ops"), IsEventNoop(FooNoop) && IsEventNoop(FooBarNoop));
    const TSharedRef<FJsonObject> Conflict = MakeShared<FJsonObject>(); Conflict->SetStringField(TEXT("blueprintId"), AssetId); Conflict->SetStringField(TEXT("agentKey"), TEXT("ui.activate")); Conflict->SetStringField(TEXT("event"), TEXT("activate")); const TSharedRef<FJsonObject> Different = MakeShared<FJsonObject>(); Different->SetStringField(TEXT("kind"), TEXT("text.set")); Different->SetStringField(TEXT("targetWidgetId"), ChildId); Different->SetStringField(TEXT("text"), TEXT("DIFFERENT")); Conflict->SetArrayField(TEXT("actions"), {MakeShared<FJsonValueObject>(Different)}); const FString BeforeConflictTree = Serialize(P13TreeResult(*Blueprint)); const FString ConflictResponse = HandleP13WidgetOperation(TEXT("p13-event-conflict"), TEXT("widget.event_ensure"), Conflict, BlueprintContentRevision(*Blueprint)); TSharedPtr<FJsonObject> ConflictEnvelope; const TSharedPtr<FJsonObject>* ConflictError = nullptr; const bool ConflictVerified = FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ConflictResponse), ConflictEnvelope) && ConflictEnvelope.IsValid() && ConflictEnvelope->TryGetObjectField(TEXT("error"), ConflictError) && ConflictError && (*ConflictError)->GetStringField(TEXT("type")) == TEXT("conflict"); TestTrue(TEXT("same natural key with different actions conflicts"), ConflictVerified); TestEqual(TEXT("event conflict preserves public tree"), Serialize(P13TreeResult(*Blueprint)), BeforeConflictTree);
    const TSharedRef<FJsonObject> NonCanonical = MakeShared<FJsonObject>(); NonCanonical->SetStringField(TEXT("blueprintId"), AssetId); NonCanonical->SetStringField(TEXT("agentKey"), TEXT("ui.invalid")); NonCanonical->SetStringField(TEXT("event"), TEXT("activate")); NonCanonical->SetArrayField(TEXT("actions"), {MakeShared<FJsonValueObject>(VisibilityAction), MakeShared<FJsonValueObject>(TextAction)}); TestTrue(TEXT("noncanonical action ordering is rejected"), !ResponseStatusIsOk(HandleP13WidgetOperation(TEXT("p13-event-order"), TEXT("widget.event_ensure"), NonCanonical, BlueprintContentRevision(*Blueprint))));
    FKismetEditorUtilities::CompileBlueprint(Blueprint); TestTrue(TEXT("authored widget event graph compiles"), Blueprint->Status != BS_Error); Readback.Reset(); TestTrue(TEXT("event identity and graph survive compile"), Graph && P13WidgetEventReadback(*Blueprint, *Graph, TEXT("ui.activate"), Readback) && Readback.Num() == 3);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIP13ViewportContracts, "MagiUnrealAXI.P13.WidgetViewportContracts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIP13ViewportContracts::RunTest(const FString&)
{
    const FString Suffix = FGuid::NewGuid().ToString(EGuidFormats::Digits); const FString HostPath = TEXT("/Game/MagiP13Automation/P13ViewportHost_") + Suffix; const FString HostId = HostPath + TEXT(".") + FPackageName::GetLongPackageAssetName(HostPath); const FString WidgetPath = TEXT("/Game/MagiP13Automation/P13ViewportWidget_") + Suffix; const FString WidgetId = WidgetPath + TEXT(".") + FPackageName::GetLongPackageAssetName(WidgetPath);
    const TSharedRef<FJsonObject> HostArgs = MakeShared<FJsonObject>(); HostArgs->SetStringField(TEXT("path"), HostPath); HostArgs->SetStringField(TEXT("parentClass"), TEXT("/Script/Engine.Actor")); TestTrue(TEXT("viewport host creates"), ResponseStatusIsOk(HandleP11BlueprintOperation(TEXT("p13-viewport-host"), TEXT("blueprint.create"), HostArgs, FString()))); UBlueprint* Host = P11LoadBlueprint(HostId); TestNotNull(TEXT("viewport host exists"), Host); if (!Host) return false;
    const TSharedRef<FJsonObject> WidgetArgs = MakeShared<FJsonObject>(); WidgetArgs->SetStringField(TEXT("path"), WidgetPath); WidgetArgs->SetStringField(TEXT("rootName"), TEXT("Root")); WidgetArgs->SetStringField(TEXT("rootClass"), TEXT("VerticalBox")); TestTrue(TEXT("viewport widget creates"), ResponseStatusIsOk(HandleP13WidgetOperation(TEXT("p13-viewport-widget"), TEXT("widget.create"), WidgetArgs, FString()))); UWidgetBlueprint* Widget = P13LoadWidgetBlueprint(WidgetId); TestNotNull(TEXT("viewport widget exists"), Widget); if (!Widget) return false;
    const TSharedRef<FJsonObject> EventArgs = MakeShared<FJsonObject>(); EventArgs->SetStringField(TEXT("blueprintId"), WidgetId); EventArgs->SetStringField(TEXT("agentKey"), TEXT("viewport")); EventArgs->SetStringField(TEXT("event"), TEXT("activate")); const TSharedRef<FJsonObject> Action = MakeShared<FJsonObject>(); Action->SetStringField(TEXT("kind"), TEXT("visibility.set")); Action->SetStringField(TEXT("targetWidgetId"), WidgetId + TEXT("#widget:Root")); Action->SetStringField(TEXT("visibility"), TEXT("Visible")); EventArgs->SetArrayField(TEXT("actions"), {MakeShared<FJsonValueObject>(Action)}); TestTrue(TEXT("viewport custom activation event creates"), ResponseStatusIsOk(HandleP13WidgetOperation(TEXT("p13-viewport-event"), TEXT("widget.event_ensure"), EventArgs, BlueprintContentRevision(*Widget)))); FKismetEditorUtilities::CompileBlueprint(Widget); TestTrue(TEXT("viewport widget compiles"), Widget->Status != BS_Error);
    EventArgs->SetStringField(TEXT("agentKey"), TEXT("rollback")); TestTrue(TEXT("rollback activation event creates"), ResponseStatusIsOk(HandleP13WidgetOperation(TEXT("p13-viewport-rollback-event"), TEXT("widget.event_ensure"), EventArgs, BlueprintContentRevision(*Widget)))); FKismetEditorUtilities::CompileBlueprint(Widget); const FString GraphId = BlueprintGraphIdentity(*Host, *Host->UbergraphPages[0]); TSharedRef<FJsonObject> Viewport = MakeShared<FJsonObject>(); Viewport->SetStringField(TEXT("hostBlueprintId"), HostId); Viewport->SetStringField(TEXT("widgetBlueprintId"), WidgetId); Viewport->SetStringField(TEXT("agentKey"), TEXT("viewport")); Viewport->SetStringField(TEXT("inputKey"), TEXT("E")); Viewport->SetNumberField(TEXT("zOrder"), 0); FString Revision = BlueprintContentRevision(*Host); const FString Success = HandleP13WidgetOperation(TEXT("p13-viewport-ensure"), TEXT("widget.viewport_ensure"), Viewport, Revision); if (!ResponseStatusIsOk(Success)) AddError(Success); TestTrue(TEXT("viewport graph creates"), ResponseStatusIsOk(Success)); UEdGraphNode* Begin = nullptr; UEdGraphNode* Input = nullptr; UK2Node_CreateWidget* Create = nullptr; UK2Node_CallFunction* Add = nullptr; UK2Node_CallFunction* Enable = nullptr; UK2Node_CallFunction* Activate = nullptr; TestTrue(TEXT("viewport graph reads back exact"), P13ViewportReadback(*Host, *Host->UbergraphPages[0], TEXT("viewport"), Widget->GeneratedClass, Begin, Input, Create, Add, Enable, Activate)); FKismetEditorUtilities::CompileBlueprint(Host); TestTrue(TEXT("viewport host compiles"), Host->Status != BS_Error); TestTrue(TEXT("viewport graph survives compile"), P13ViewportReadback(*Host, *Host->UbergraphPages[0], TEXT("viewport"), Widget->GeneratedClass, Begin, Input, Create, Add, Enable, Activate)); auto IsChanged = [](const FString& Response, bool Expected) { TSharedPtr<FJsonObject> Envelope; const TSharedPtr<FJsonObject>* Result = nullptr; return ResponseStatusIsOk(Response) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response), Envelope) && Envelope.IsValid() && Envelope->TryGetObjectField(TEXT("result"), Result) && Result && (*Result)->GetBoolField(TEXT("changed")) == Expected; }; Revision = BlueprintContentRevision(*Host); const EBlueprintStatus NoopStatus = Host->Status; const bool NoopDirty = Host->GetOutermost()->IsDirty(); const FString Noop = HandleP13WidgetOperation(TEXT("p13-viewport-noop"), TEXT("widget.viewport_ensure"), Viewport, Revision); if (!ResponseStatusIsOk(Noop)) AddError(Noop); TestTrue(TEXT("viewport repeat is no-op"), IsChanged(Noop, false)); TestEqual(TEXT("viewport no-op preserves revision"), BlueprintContentRevision(*Host), Revision); TestEqual(TEXT("viewport no-op preserves status"), Host->Status, NoopStatus); TestEqual(TEXT("viewport no-op preserves dirty state"), Host->GetOutermost()->IsDirty(), NoopDirty); TSharedRef<FJsonObject> RollbackViewport = MakeShared<FJsonObject>(); RollbackViewport->SetStringField(TEXT("hostBlueprintId"), HostId); RollbackViewport->SetStringField(TEXT("widgetBlueprintId"), WidgetId); RollbackViewport->SetStringField(TEXT("agentKey"), TEXT("rollback")); RollbackViewport->SetStringField(TEXT("inputKey"), TEXT("E")); RollbackViewport->SetNumberField(TEXT("zOrder"), 0); GP11ForceAtomicFailure = true; const FString Failed = HandleP13WidgetOperation(TEXT("p13-viewport-rollback"), TEXT("widget.viewport_ensure"), RollbackViewport, Revision); GP11ForceAtomicFailure = false; TSharedPtr<FJsonObject> FailedEnvelope; const TSharedPtr<FJsonObject>* FailedReceipt = nullptr; const bool FailedReceiptValid = FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Failed), FailedEnvelope) && FailedEnvelope.IsValid() && FailedEnvelope->TryGetObjectField(TEXT("receipt"), FailedReceipt) && FailedReceipt && (*FailedReceipt)->GetStringField(TEXT("state")) == TEXT("failed"); if (!FailedReceiptValid) AddError(Failed); TestTrue(TEXT("viewport rollback returns failed receipt"), FailedReceiptValid); TestEqual(TEXT("viewport rollback preserves host revision"), BlueprintContentRevision(*Host), Revision); TestEqual(TEXT("viewport result graph identity is fixed"), GraphId, BlueprintGraphIdentity(*Host, *Host->UbergraphPages[0])); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIP13UiObserveContracts, "MagiUnrealAXI.P13.UiObserveContracts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIP13UiObserveContracts::RunTest(const FString&)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr; TestNotNull(TEXT("UI observe editor world exists"), World); if (!World) return false;
    const FString Suffix = FGuid::NewGuid().ToString(EGuidFormats::Digits); const FString Path = TEXT("/Game/MagiP13Automation/P13UiObserve_") + Suffix; const FString BlueprintId = Path + TEXT(".") + FPackageName::GetLongPackageAssetName(Path);
    const TSharedRef<FJsonObject> CreateArgs = MakeShared<FJsonObject>(); CreateArgs->SetStringField(TEXT("path"), Path); CreateArgs->SetStringField(TEXT("rootName"), TEXT("Root")); CreateArgs->SetStringField(TEXT("rootClass"), TEXT("VerticalBox")); TestTrue(TEXT("UI observe fixture creates"), ResponseStatusIsOk(HandleP13WidgetOperation(TEXT("p13-ui-observe-create"), TEXT("widget.create"), CreateArgs, FString())));
    UWidgetBlueprint* Blueprint = P13LoadWidgetBlueprint(BlueprintId); TestNotNull(TEXT("UI observe fixture exists"), Blueprint); if (!Blueprint) return false;
    const FString RootId = BlueprintId + TEXT("#widget:Root"); const FString LabelId = BlueprintId + TEXT("#widget:Label"); const TSharedRef<FJsonObject> ChildArgs = MakeShared<FJsonObject>(); ChildArgs->SetStringField(TEXT("blueprintId"), BlueprintId); ChildArgs->SetStringField(TEXT("parentWidgetId"), RootId); ChildArgs->SetStringField(TEXT("name"), TEXT("Label")); ChildArgs->SetStringField(TEXT("class"), TEXT("TextBlock")); TestTrue(TEXT("UI observe fixture child creates"), ResponseStatusIsOk(HandleP13WidgetOperation(TEXT("p13-ui-observe-child"), TEXT("widget.child_ensure"), ChildArgs, BlueprintContentRevision(*Blueprint))));
    FKismetEditorUtilities::CompileBlueprint(Blueprint); TestTrue(TEXT("UI observe fixture compiles"), Blueprint->Status != BS_Error && Blueprint->GeneratedClass); if (!Blueprint->GeneratedClass) return false;
    UUserWidget* Instance = CreateWidget<UUserWidget>(World, TSubclassOf<UUserWidget>(Blueprint->GeneratedClass.Get())); TestNotNull(TEXT("UI observe runtime instance creates"), Instance); if (!Instance) return false;
    UTextBlock* Label = Instance->WidgetTree ? Cast<UTextBlock>(Instance->WidgetTree->FindWidget(TEXT("Label"))) : nullptr; TestNotNull(TEXT("UI observe runtime label exists"), Label); if (!Label) { Instance->MarkAsGarbage(); return false; } Label->SetText(FText::FromString(TEXT("READY")));
    const TArray<FString> OrderedIds = {LabelId, RootId}; FString ErrorType, Error; const TSharedRef<FJsonObject> First = P13UiObserveResult(TEXT("session"), BlueprintId, OrderedIds, *World, ErrorType, Error); if (!ErrorType.IsEmpty()) AddError(Error); TestTrue(TEXT("UI observe returns schema-valid runtime state"), ErrorType.IsEmpty() && MagiAxiValidateOutput(TEXT("play.ui_observe"), First)); TestFalse(TEXT("detached UI instance reports outside viewport"), First->GetBoolField(TEXT("inViewport"))); const FString FirstRevision = First->GetStringField(TEXT("revision"));
    ErrorType.Empty(); Error.Empty(); const TSharedRef<FJsonObject> Repeat = P13UiObserveResult(TEXT("session"), BlueprintId, OrderedIds, *World, ErrorType, Error); TestEqual(TEXT("unchanged UI observation revision is deterministic"), Repeat->GetStringField(TEXT("revision")), FirstRevision);
    Label->SetText(FText::FromString(TEXT("ACTIVE"))); ErrorType.Empty(); Error.Empty(); const TSharedRef<FJsonObject> Changed = P13UiObserveResult(TEXT("session"), BlueprintId, OrderedIds, *World, ErrorType, Error); TestNotEqual(TEXT("runtime text transition changes UI revision"), Changed->GetStringField(TEXT("revision")), FirstRevision); TestTrue(TEXT("runtime text transition is observed"), Serialize(Changed).Contains(TEXT("ACTIVE")));
    ErrorType.Empty(); Error.Empty(); P13UiObserveResult(TEXT("session"), BlueprintId, {RootId, LabelId}, *World, ErrorType, Error); TestEqual(TEXT("unordered widget IDs fail closed"), ErrorType, FString(TEXT("invalid_input")));
    UUserWidget* Second = CreateWidget<UUserWidget>(World, TSubclassOf<UUserWidget>(Blueprint->GeneratedClass.Get())); TestNotNull(TEXT("second UI instance creates"), Second); ErrorType.Empty(); Error.Empty(); P13UiObserveResult(TEXT("session"), BlueprintId, OrderedIds, *World, ErrorType, Error); TestEqual(TEXT("ambiguous UI instances fail closed"), ErrorType, FString(TEXT("conflict")));
    if (Second) Second->MarkAsGarbage(); Instance->MarkAsGarbage(); return true;
}
#endif
