bool IsP14AiNavigationOperation(const FString& Operation)
{
    return Operation.StartsWith(TEXT("navigation.")) || Operation.StartsWith(TEXT("blackboard.")) || Operation.StartsWith(TEXT("behavior_tree.")) || Operation.StartsWith(TEXT("ai.")) || Operation == TEXT("play.ai_target_set") || Operation == TEXT("play.ai_observe");
}

static bool P14AssetPath(const FString& Path, FString& ObjectPath)
{
    if (!Path.StartsWith(TEXT("/Game/")) || !FPackageName::IsValidLongPackageName(Path) || Path.Contains(TEXT("."))) return false;
    ObjectPath = Path + TEXT(".") + FPackageName::GetShortName(Path); return true;
}

static TSharedRef<FJsonObject> P14Fields(UObject* Asset, bool Changed)
{
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    UPackage* Package = Asset ? Asset->GetOutermost() : nullptr;
    Result->SetBoolField(TEXT("changed"), Changed);
    Result->SetArrayField(TEXT("dirtyPackages"), Changed && Package ? TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueString>(Package->GetName())} : TArray<TSharedPtr<FJsonValue>>{});
    Result->SetArrayField(TEXT("savedPackages"), {}); Result->SetStringField(TEXT("revision"), ObjectContentRevision(Asset)); return Result;
}

static void P14DiscardCreatedAsset(UPackage& Package, UObject* Asset, bool AssetRegistered)
{
    if (AssetRegistered && Asset) FAssetRegistryModule::AssetDeleted(Asset);
    if (Asset)
    {
        Asset->ClearFlags(RF_Public | RF_Standalone);
        Asset->Rename(nullptr, GetTransientPackage(), REN_AllowPackageLinkerMismatch | REN_DoNotDirty | REN_DontCreateRedirectors | REN_NonTransactional);
        Asset->MarkAsGarbage();
    }
    Package.SetDirtyFlag(false);
    Package.MarkAsGarbage();
}

static bool P14IsActorKey(const UBlackboardData& Asset, FName KeyName)
{
    const FBlackboardEntry* Entry = Asset.Keys.FindByPredicate([KeyName](const FBlackboardEntry& Candidate) { return Candidate.EntryName == KeyName; });
    const UBlackboardKeyType_Object* Type = Entry ? Cast<UBlackboardKeyType_Object>(Entry->KeyType) : nullptr;
    return Type && Type->BaseClass == AActor::StaticClass();
}

static FString P14Blackboard(const FString& Id, const FString& Operation, const TSharedPtr<FJsonObject>& Args, const FString& Expected)
{
    FString AssetId;
    if (Operation == TEXT("blackboard.create"))
    {
        FString Path;
        if (!Args->TryGetStringField(TEXT("path"), Path) || !P14AssetPath(Path, AssetId)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("path must be a /Game package path"));
        UBlackboardData* Existing = LoadObject<UBlackboardData>(nullptr, *AssetId);
        if (Existing)
        {
            if (Existing->GetPathName() != AssetId) return ErrorResponse(Id, TEXT("conflict"), TEXT("blackboard identity mismatch"));
            const TSharedRef<FJsonObject> Result = P14Fields(Existing, false); Result->SetStringField(TEXT("blackboardId"), AssetId); return SuccessResponse(Id, Result, Operation, Args);
        }
        if (StaticFindObject(UObject::StaticClass(), nullptr, *AssetId) || FPackageName::DoesPackageExist(Path))
            return ErrorResponse(Id, TEXT("conflict"), TEXT("blackboard package already contains an incompatible object or exists on disk"));
        FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P14BlackboardCreate", "Magi AXI Create Blackboard"));
        UPackage* Package = CreatePackage(*Path);
        UBlackboardData* Asset = Package ? NewObject<UBlackboardData>(Package, *FPackageName::GetShortName(Path), RF_Public | RF_Standalone) : nullptr;
        if (!Package || !Asset || Asset->GetPathName() != AssetId)
        {
            if (Package) P14DiscardCreatedAsset(*Package, Asset, false);
            Transaction.Cancel();
            return ErrorResponse(Id, TEXT("operation_failed"), TEXT("failed to create BlackboardData"));
        }
        FAssetRegistryModule::AssetCreated(Asset);
#if WITH_DEV_AUTOMATION_TESTS
        const bool ForcedFailure = GP11ForceAtomicFailure;
#else
        const bool ForcedFailure = false;
#endif
        if (ForcedFailure || Asset->GetPathName() != AssetId)
        {
            P14DiscardCreatedAsset(*Package, Asset, true); Transaction.Cancel();
            return ErrorResponse(Id, TEXT("operation_failed"), TEXT("blackboard create failed readback"));
        }
        Package->MarkPackageDirty();
        const TSharedRef<FJsonObject> Result = P14Fields(Asset, true); Result->SetStringField(TEXT("blackboardId"), AssetId); return SuccessResponse(Id, Result, Operation, Args);
    }
    if (!Args->TryGetStringField(TEXT("blackboardId"), AssetId)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("blackboardId is required"));
    UBlackboardData* Asset = LoadObject<UBlackboardData>(nullptr, *AssetId);
    if (!Asset) return ErrorResponse(Id, TEXT("not_found"), TEXT("BlackboardData was not found"));
    if (Operation == TEXT("blackboard.view"))
    {
        const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("blackboardId"), AssetId); TArray<TSharedPtr<FJsonValue>> Keys;
        for (const FBlackboardEntry& Entry : Asset->Keys) if (P14IsActorKey(*Asset, Entry.EntryName)) { const TSharedRef<FJsonObject> Key = MakeShared<FJsonObject>(); Key->SetStringField(TEXT("keyName"), Entry.EntryName.ToString()); Key->SetStringField(TEXT("keyType"), TEXT("Actor")); Keys.Add(MakeShared<FJsonValueObject>(Key)); }
        Result->SetArrayField(TEXT("keys"), Keys); Result->SetStringField(TEXT("revision"), ObjectContentRevision(Asset)); return SuccessResponse(Id, Result, Operation, Args);
    }
    FString KeyName, KeyType;
    if (!Args->TryGetStringField(TEXT("keyName"), KeyName) || !Args->TryGetStringField(TEXT("keyType"), KeyType) || KeyType != TEXT("Actor")) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("only Actor keyType is supported"));
    if (Expected.IsEmpty()) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("expectedRevision is required"));
    if (Expected != ObjectContentRevision(Asset)) return ErrorResponse(Id, TEXT("conflict"), TEXT("blackboard revision is stale"));
    const FName KeyFName(*KeyName); FBlackboardEntry* Existing = Asset->Keys.FindByPredicate([KeyFName](const FBlackboardEntry& Entry) { return Entry.EntryName == KeyFName; });
    if (Existing && !P14IsActorKey(*Asset, KeyFName)) return ErrorResponse(Id, TEXT("conflict"), TEXT("key exists with incompatible type"));
    const bool Changed = Existing == nullptr;
    if (Changed)
    {
        const bool WasDirty = Asset->GetOutermost()->IsDirty(); const FString BeforeRevision = ObjectContentRevision(Asset); FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P14BlackboardKey", "Magi AXI Ensure Blackboard Key")); Asset->Modify();
        UBlackboardKeyType_Object* Type = NewObject<UBlackboardKeyType_Object>(Asset); Type->BaseClass = AActor::StaticClass(); FBlackboardEntry Entry; Entry.EntryName = KeyFName; Entry.KeyType = Type; Asset->Keys.Add(Entry); Asset->MarkPackageDirty();
#if WITH_DEV_AUTOMATION_TESTS
        const bool ForcedFailure = GP11ForceAtomicFailure;
#else
        const bool ForcedFailure = false;
#endif
        if (ForcedFailure || !P14IsActorKey(*Asset, KeyFName) || BeforeRevision == ObjectContentRevision(Asset))
        {
            Asset->Keys.RemoveAll([KeyFName](const FBlackboardEntry& Candidate) { return Candidate.EntryName == KeyFName; }); Type->MarkAsGarbage(); Asset->GetOutermost()->SetDirtyFlag(WasDirty); Transaction.Cancel();
            return ErrorResponse(Id, TEXT("operation_failed"), TEXT("blackboard key readback failed"));
        }
    }
    const TSharedRef<FJsonObject> Result = P14Fields(Asset, Changed); Result->SetStringField(TEXT("blackboardId"), AssetId); Result->SetStringField(TEXT("keyName"), KeyName); Result->SetStringField(TEXT("keyType"), TEXT("Actor")); return SuccessResponse(Id, Result, Operation, Args);
}

static FGuid P14BehaviorTreeGuid(const UBehaviorTree& Tree, const FString& NodeId)
{
    FGuid Guid;
    FGuid::ParseExact(Sha256(Tree.GetPathName() + TEXT("#") + NodeId).Left(32), EGuidFormats::Digits, Guid);
    return Guid;
}

static FString P14BehaviorTreeNodeType(const UEdGraphNode& Node);
static UBehaviorTreeGraphNode_Root* P14BehaviorTreeRoot(UBehaviorTreeGraph& Graph);
static bool P14BehaviorTreeReaches(UEdGraphNode& Start, UEdGraphNode& Wanted);
static bool P14BehaviorTreeNodeIdValid(const FString& NodeId) { return !NodeId.IsEmpty() && NodeId != TEXT("root") && !NodeId.Contains(TEXT("\n")) && !NodeId.Contains(TEXT("\r")) && !NodeId.Contains(TEXT("->")); }
static bool P14BehaviorTreeSchemaValid(const UBehaviorTreeGraph& Graph) { return Graph.GetSchema() && Graph.GetSchema()->GetClass() == UEdGraphSchema_BehaviorTree::StaticClass(); }
static UEdGraphNode* P14FindBehaviorTreeNode(UBehaviorTreeGraph& Graph, const FString& NodeId)
{
    if (NodeId == TEXT("root")) return P14BehaviorTreeRoot(Graph);
    for (UEdGraphNode* Node : Graph.Nodes) if (Node && Node->NodeComment == NodeId) return Node;
    return nullptr;
}
static UBehaviorTreeGraphNode_Root* P14BehaviorTreeRoot(UBehaviorTreeGraph& Graph)
{
    UBehaviorTreeGraphNode_Root* Root = nullptr;
    for (UEdGraphNode* Node : Graph.Nodes) if (UBehaviorTreeGraphNode_Root* Candidate = Cast<UBehaviorTreeGraphNode_Root>(Node)) { if (Root) return nullptr; Root = Candidate; }
    return Root;
}
static UEdGraphPin* P14BehaviorTreeOutput(UEdGraphNode& Node)
{
    UEdGraphPin* Output = nullptr;
    for (UEdGraphPin* Pin : Node.Pins) if (Pin && Pin->Direction == EGPD_Output) { if (Output) return nullptr; Output = Pin; }
    return Output;
}
static UEdGraphPin* P14BehaviorTreeInput(UEdGraphNode& Node)
{
    UEdGraphPin* Input = nullptr;
    for (UEdGraphPin* Pin : Node.Pins) if (Pin && Pin->Direction == EGPD_Input) { if (Input) return nullptr; Input = Pin; }
    return Input;
}
static bool P14BehaviorTreePinsValid(const UEdGraphNode& Node, bool IsRoot)
{
    int32 Inputs = 0, Outputs = 0;
    for (const UEdGraphPin* Pin : Node.Pins)
    {
        if (!Pin) return false;
        if (Pin->Direction == EGPD_Input) ++Inputs;
        else if (Pin->Direction == EGPD_Output) ++Outputs;
        else return false;
    }
    if (IsRoot) return Inputs == 0 && Outputs == 1;
    if (Cast<UBehaviorTreeGraphNode_Composite>(&Node)) return Inputs == 1 && Outputs == 1;
    if (Cast<UBehaviorTreeGraphNode_Task>(&Node)) return Inputs == 1 && Outputs == 0;
    return false;
}
static TArray<UEdGraphNode*> P14BehaviorTreeChildren(UEdGraphNode& Parent)
{
    TArray<UEdGraphNode*> Children;
    if (UEdGraphPin* Output = P14BehaviorTreeOutput(Parent)) for (UEdGraphPin* Linked : Output->LinkedTo) if (Linked && Linked->GetOwningNode()) Children.Add(Linked->GetOwningNode());
    Children.Sort([](const UEdGraphNode& Left, const UEdGraphNode& Right) { return Left.NodePosX == Right.NodePosX ? Left.NodeGuid.ToString() < Right.NodeGuid.ToString() : Left.NodePosX < Right.NodePosX; });
    return Children;
}
static bool P14BehaviorTreeTopologyValid(UBehaviorTreeGraph& Graph, bool AllowEmptyRoot)
{
    if (!P14BehaviorTreeSchemaValid(Graph)) return false;
    UBehaviorTreeGraphNode_Root* Root = P14BehaviorTreeRoot(Graph);
    if (!Root || !P14BehaviorTreePinsValid(*Root, true)) return false;
    TSet<FString> NodeIds; TSet<UEdGraphNode*> Parents; const TArray<UEdGraphNode*> RootChildren = P14BehaviorTreeChildren(*Root);
    if (!AllowEmptyRoot && RootChildren.Num() != 1) return false;
    if (RootChildren.Num() > 1 || (RootChildren.Num() == 1 && P14BehaviorTreeNodeType(*RootChildren[0]) != TEXT("sequence"))) return false;
    for (UEdGraphNode* Node : Graph.Nodes)
    {
        if (!Node || Node == Root) continue;
        if (NodeIds.Num() == 32 || !P14BehaviorTreeNodeIdValid(Node->NodeComment) || NodeIds.Contains(Node->NodeComment) || !P14BehaviorTreePinsValid(*Node, false) || P14BehaviorTreeNodeType(*Node).IsEmpty()) return false;
        NodeIds.Add(Node->NodeComment);
    }
    for (UEdGraphNode* Parent : Graph.Nodes)
    {
        if (!Parent) return false;
        const TArray<UEdGraphNode*> Children = P14BehaviorTreeChildren(*Parent);
        if (Parent != Root && !Cast<UBehaviorTreeGraphNode_Composite>(Parent) && !Children.IsEmpty()) return false;
        if (Children.Num() > 32) return false;
        TSet<int32> Positions;
        for (UEdGraphNode* Child : Children)
        {
            UEdGraphPin* Output = P14BehaviorTreeOutput(*Parent); UEdGraphPin* Input = Child ? P14BehaviorTreeInput(*Child) : nullptr;
            if (!Child || !Graph.Nodes.Contains(Child) || Child == Root || !Output || !Input || Output->Direction != EGPD_Output || Input->Direction != EGPD_Input || !Output->LinkedTo.Contains(Input) || !Input->LinkedTo.Contains(Output) || Input->LinkedTo.Num() != 1 || Positions.Contains(Child->NodePosX) || Parents.Contains(Child)) return false;
            Positions.Add(Child->NodePosX); Parents.Add(Child);
        }
        for (UEdGraphPin* Pin : Parent->Pins)
        {
            if (!Pin) return false;
            for (UEdGraphPin* Link : Pin->LinkedTo)
            {
                if (!Link || !Graph.Nodes.Contains(Link->GetOwningNode()) || !Link->LinkedTo.Contains(Pin) || Link->Direction == Pin->Direction) return false;
            }
        }
    }
    for (UEdGraphNode* Node : Graph.Nodes) if (Node && P14BehaviorTreeReaches(*Node, *Node)) return false;
    return true;
}

static bool P14SetBehaviorTreeChildOrder(UEdGraphNode& Parent, UEdGraphNode& Child, int32 RequestedIndex)
{
    TArray<UEdGraphNode*> Children = P14BehaviorTreeChildren(Parent);
    const int32 ExistingIndex = Children.IndexOfByKey(&Child);
    if (RequestedIndex < 0 || RequestedIndex >= Children.Num()) return false;
    if (ExistingIndex != INDEX_NONE) Children.RemoveAt(ExistingIndex);
    if (RequestedIndex > Children.Num()) return false;
    Children.Insert(&Child, RequestedIndex);
    for (int32 Index = 0; Index < Children.Num(); ++Index)
    {
        Children[Index]->Modify();
        Children[Index]->NodePosX = Parent.NodePosX + 256 + Index * 256;
    }
    return true;
}

static bool P14BehaviorTreeReaches(UEdGraphNode& Start, UEdGraphNode& Wanted)
{
    TArray<UEdGraphNode*> Pending = P14BehaviorTreeChildren(Start); TSet<UEdGraphNode*> Seen;
    while (Pending.Num()) { UEdGraphNode* Node = Pending.Pop(); if (Node == &Wanted) return true; if (!Node || Seen.Contains(Node)) continue; Seen.Add(Node); Pending.Append(P14BehaviorTreeChildren(*Node)); }
    return false;
}

static FString P14BehaviorTreeNodeType(const UEdGraphNode& Node)
{
    if (const UBehaviorTreeGraphNode_Composite* Composite = Cast<UBehaviorTreeGraphNode_Composite>(&Node)) return Composite->NodeInstance && Composite->NodeInstance->GetClass() == UBTComposite_Sequence::StaticClass() ? TEXT("sequence") : FString();
    const UBehaviorTreeGraphNode_Task* Task = Cast<UBehaviorTreeGraphNode_Task>(&Node);
    if (!Task || !Task->NodeInstance) return FString();
    if (Task->NodeInstance->GetClass() == UBTTask_MoveTo::StaticClass()) return TEXT("move_to");
    if (Task->NodeInstance->GetClass() == UBTTask_Wait::StaticClass()) return TEXT("wait");
    return FString();
}

static FBlackboardKeySelector* P14MoveToSelector(UBTTask_MoveTo& MoveTo);
static bool P14BehaviorTreeTaskValid(const UBehaviorTree& Tree, const UEdGraphNode& Node);
static bool P14BehaviorTreeRuntimeValid(const UBehaviorTree& Tree, UBehaviorTreeGraph& Graph, bool AllowEmptyRoot)
{
    if (!P14BehaviorTreeTopologyValid(Graph, AllowEmptyRoot) || !Tree.BlackboardAsset) return false;
    const UBehaviorTreeGraphNode_Root* Root = P14BehaviorTreeRoot(Graph); const TArray<UEdGraphNode*> Children = P14BehaviorTreeChildren(*const_cast<UBehaviorTreeGraphNode_Root*>(Root));
    if (Root->BlackboardAsset != Tree.BlackboardAsset) return false;
    for (UEdGraphNode* Node : Graph.Nodes) if (Node && Node != Root && Cast<UBehaviorTreeGraphNode_Task>(Node) && !P14BehaviorTreeTaskValid(Tree, *Node)) return false;
    if (Children.IsEmpty()) return Tree.RootNode == nullptr;
    const UBehaviorTreeGraphNode_Composite* Composite = Cast<UBehaviorTreeGraphNode_Composite>(Children[0]);
    return Composite && Tree.RootNode == Composite->NodeInstance;
}

static bool P14BehaviorTreeTaskValid(const UBehaviorTree& Tree, const UEdGraphNode& Node)
{
    const UBehaviorTreeGraphNode_Task* Task = Cast<UBehaviorTreeGraphNode_Task>(&Node); if (!Task || !Task->NodeInstance) return false;
    if (Task->NodeInstance->GetClass() == UBTTask_MoveTo::StaticClass()) { const UBTTask_MoveTo* MoveTo = Cast<UBTTask_MoveTo>(Task->NodeInstance); const FBlackboardKeySelector* Selector = MoveTo ? P14MoveToSelector(*const_cast<UBTTask_MoveTo*>(MoveTo)) : nullptr; return Selector && Selector->SelectedKeyName == FName(TEXT("TargetActor")) && Tree.BlackboardAsset && P14IsActorKey(*Tree.BlackboardAsset, Selector->SelectedKeyName); }
    if (Task->NodeInstance->GetClass() == UBTTask_Wait::StaticClass()) { const UBTTask_Wait* Wait = Cast<UBTTask_Wait>(Task->NodeInstance); return Wait && FMath::IsNearlyEqual(Wait->WaitTime, 0.5f); }
    return false;
}


static FBlackboardKeySelector* P14MoveToSelector(UBTTask_MoveTo& MoveTo)
{
    FStructProperty* Property = FindFProperty<FStructProperty>(MoveTo.GetClass(), TEXT("BlackboardKey"));
    if (!Property || Property->Struct != FBlackboardKeySelector::StaticStruct()) return nullptr;
    return Property->ContainerPtrToValuePtr<FBlackboardKeySelector>(&MoveTo);
}
static TSharedRef<FJsonObject> P14BehaviorTreeNodeResult(UBehaviorTree& Tree, const FString& NodeId, const FString& NodeType, UEdGraphNode* Node, bool Changed)
{
    const TSharedRef<FJsonObject> Result = P14Fields(&Tree, Changed);
    Result->SetStringField(TEXT("behaviorTreeId"), Tree.GetPathName()); Result->SetStringField(TEXT("nodeId"), NodeId); Result->SetStringField(TEXT("nodeType"), NodeType);
    if (const UBehaviorTreeGraphNode_Task* Task = Cast<UBehaviorTreeGraphNode_Task>(Node))
    {
        if (const UBTTask_MoveTo* MoveTo = Cast<UBTTask_MoveTo>(Task->NodeInstance)) Result->SetStringField(TEXT("keyName"), MoveTo->GetSelectedBlackboardKey().ToString());
        else Result->SetField(TEXT("keyName"), MakeShared<FJsonValueNull>());
        if (const UBTTask_Wait* Wait = Cast<UBTTask_Wait>(Task->NodeInstance)) Result->SetNumberField(TEXT("waitSeconds"), Wait->WaitTime);
        else Result->SetField(TEXT("waitSeconds"), MakeShared<FJsonValueNull>());
    }
    else
    {
        Result->SetField(TEXT("keyName"), MakeShared<FJsonValueNull>()); Result->SetField(TEXT("waitSeconds"), MakeShared<FJsonValueNull>());
    }
    return Result;
}

static FString P14BehaviorTree(const FString& Id, const FString& Operation, const TSharedPtr<FJsonObject>& Args, const FString& Expected)
{
    FString TreeId; Args->TryGetStringField(TEXT("behaviorTreeId"), TreeId); UBehaviorTree* Tree = nullptr;
    if (Operation == TEXT("behavior_tree.create"))
    {
        FString Path, BlackboardId;
        if (!Args->TryGetStringField(TEXT("path"), Path) || !Args->TryGetStringField(TEXT("blackboardId"), BlackboardId) || !P14AssetPath(Path, TreeId)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("invalid behavior tree path"));
        UBlackboardData* Blackboard = LoadObject<UBlackboardData>(nullptr, *BlackboardId);
        if (!Blackboard) return ErrorResponse(Id, TEXT("not_found"), TEXT("blackboard was not found"));
        Tree = LoadObject<UBehaviorTree>(nullptr, *TreeId);
        bool Changed = false;
        if (!Tree)
        {
            if (StaticFindObject(UObject::StaticClass(), nullptr, *TreeId) || FPackageName::DoesPackageExist(Path))
                return ErrorResponse(Id, TEXT("conflict"), TEXT("behavior tree package already contains an incompatible object or exists on disk"));
            FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P14BehaviorTreeCreate", "Magi AXI Create Behavior Tree"));
            UPackage* Package = CreatePackage(*Path);
            Tree = Package ? NewObject<UBehaviorTree>(Package, *FPackageName::GetShortName(Path), RF_Public | RF_Standalone) : nullptr;
            if (!Package || !Tree)
            {
                if (Package) P14DiscardCreatedAsset(*Package, Tree, false);
                Transaction.Cancel();
                return ErrorResponse(Id, TEXT("operation_failed"), TEXT("failed to create BehaviorTree"));
            }
            Tree->BlackboardAsset = Blackboard;
            Tree->BTGraph = FBlueprintEditorUtils::CreateNewGraph(Tree, TEXT("BTGraph"), UBehaviorTreeGraph::StaticClass(), UEdGraphSchema_BehaviorTree::StaticClass());
            UBehaviorTreeGraph* BehaviorGraph = Cast<UBehaviorTreeGraph>(Tree->BTGraph);
            if (!BehaviorGraph || !BehaviorGraph->GetSchema())
            {
                P14DiscardCreatedAsset(*Package, Tree, false);
                Transaction.Cancel();
                return ErrorResponse(Id, TEXT("operation_failed"), TEXT("failed to create BehaviorTree graph"));
            }
            BehaviorGraph->GetSchema()->CreateDefaultNodesForGraph(*BehaviorGraph);
            BehaviorGraph->OnCreated();
            BehaviorGraph->Initialize();
            UBehaviorTreeGraphNode_Root* Root = P14BehaviorTreeRoot(*BehaviorGraph);
            if (Root) Root->BlackboardAsset = Blackboard;
            Tree->BlackboardAsset = Blackboard;
            const bool Valid = Root && P14BehaviorTreeRuntimeValid(*Tree, *BehaviorGraph, true);
#if WITH_DEV_AUTOMATION_TESTS
            const bool ForcedFailure = GP11ForceAtomicFailure;
#else
            const bool ForcedFailure = false;
#endif
            if (!Valid || ForcedFailure)
            {
                P14DiscardCreatedAsset(*Package, Tree, false);
                Transaction.Cancel();
                return ErrorResponse(Id, TEXT("operation_failed"), TEXT("behavior tree create failed graph readback"));
            }
            BehaviorGraph->UpdateAsset(UBehaviorTreeGraph::ClearDebuggerFlags | UBehaviorTreeGraph::KeepRebuildCounter);
            FAssetRegistryModule::AssetCreated(Tree);
            Package->MarkPackageDirty();
            Changed = true;
        }
        UBehaviorTreeGraph* ExistingGraph = Cast<UBehaviorTreeGraph>(Tree->BTGraph);
        UBehaviorTreeGraphNode_Root* ExistingRoot = ExistingGraph ? P14BehaviorTreeRoot(*ExistingGraph) : nullptr;
        if (Tree->GetPathName() != TreeId || Tree->BlackboardAsset != Blackboard || !ExistingGraph || !ExistingRoot || ExistingRoot->BlackboardAsset != Blackboard || !P14BehaviorTreeRuntimeValid(*Tree, *ExistingGraph, true)) return ErrorResponse(Id, TEXT("conflict"), TEXT("behavior tree identity or graph is incompatible"));
        const TSharedRef<FJsonObject> Result = P14Fields(Tree, Changed); Result->SetStringField(TEXT("behaviorTreeId"), TreeId); Result->SetStringField(TEXT("blackboardId"), Tree->BlackboardAsset->GetPathName()); return SuccessResponse(Id, Result, Operation, Args);
    }
    Tree = LoadObject<UBehaviorTree>(nullptr, *TreeId); if (!Tree) return ErrorResponse(Id, TEXT("not_found"), TEXT("behavior tree was not found"));
    UBehaviorTreeGraph* Graph = Cast<UBehaviorTreeGraph>(Tree->BTGraph); if (!Graph || !Graph->GetSchema()) return ErrorResponse(Id, TEXT("conflict"), TEXT("behavior tree graph is missing"));
    if (Operation == TEXT("behavior_tree.view"))
    {
        if (!P14BehaviorTreeRuntimeValid(*Tree, *Graph, true)) return ErrorResponse(Id, TEXT("conflict"), TEXT("behavior tree graph semantics are invalid"));
        const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("behaviorTreeId"), TreeId); Result->SetStringField(TEXT("blackboardId"), Tree->BlackboardAsset ? Tree->BlackboardAsset->GetPathName() : FString()); Result->SetStringField(TEXT("revision"), ObjectContentRevision(Tree)); TArray<TSharedPtr<FJsonValue>> Nodes, Links;
        TArray<UEdGraphNode*> OrderedNodes; for (UEdGraphNode* Node : Graph->Nodes) if (Node && !Cast<UBehaviorTreeGraphNode_Root>(Node) && !Node->NodeComment.IsEmpty()) OrderedNodes.Add(Node); OrderedNodes.Sort([](const UEdGraphNode& Left, const UEdGraphNode& Right) { return Left.NodeComment < Right.NodeComment; });
        for (UEdGraphNode* Node : OrderedNodes) { const FString Type = P14BehaviorTreeNodeType(*Node); const TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>(); Row->SetStringField(TEXT("nodeId"), Node->NodeComment); Row->SetStringField(TEXT("nodeType"), Type); if (const UBehaviorTreeGraphNode_Task* Task = Cast<UBehaviorTreeGraphNode_Task>(Node)) { if (const UBTTask_MoveTo* MoveTo = Cast<UBTTask_MoveTo>(Task->NodeInstance)) Row->SetStringField(TEXT("keyName"), MoveTo->GetSelectedBlackboardKey().ToString()); else Row->SetField(TEXT("keyName"), MakeShared<FJsonValueNull>()); if (const UBTTask_Wait* Wait = Cast<UBTTask_Wait>(Task->NodeInstance)) Row->SetNumberField(TEXT("waitSeconds"), Wait->WaitTime); else Row->SetField(TEXT("waitSeconds"), MakeShared<FJsonValueNull>()); } else { Row->SetField(TEXT("keyName"), MakeShared<FJsonValueNull>()); Row->SetField(TEXT("waitSeconds"), MakeShared<FJsonValueNull>()); } Nodes.Add(MakeShared<FJsonValueObject>(Row)); }
        TArray<UEdGraphNode*> OrderedParents; if (UBehaviorTreeGraphNode_Root* Root = P14BehaviorTreeRoot(*Graph)) OrderedParents.Add(Root); for (UEdGraphNode* Node : OrderedNodes) OrderedParents.Add(Node);
        for (UEdGraphNode* Parent : OrderedParents) { const FString ParentId = Cast<UBehaviorTreeGraphNode_Root>(Parent) ? TEXT("root") : Parent->NodeComment; const TArray<UEdGraphNode*> Children = P14BehaviorTreeChildren(*Parent); for (int32 Index = 0; Index < Children.Num(); ++Index) { const TSharedRef<FJsonObject> Link = MakeShared<FJsonObject>(); Link->SetStringField(TEXT("linkId"), ParentId + TEXT("->") + Children[Index]->NodeComment); Link->SetStringField(TEXT("parentNodeId"), ParentId); Link->SetStringField(TEXT("childNodeId"), Children[Index]->NodeComment); Link->SetNumberField(TEXT("childIndex"), Index); Links.Add(MakeShared<FJsonValueObject>(Link)); } }
        Links.Sort([](const TSharedPtr<FJsonValue>& Left, const TSharedPtr<FJsonValue>& Right) { return Left->AsObject()->GetStringField(TEXT("parentNodeId")) == Right->AsObject()->GetStringField(TEXT("parentNodeId")) ? Left->AsObject()->GetIntegerField(TEXT("childIndex")) < Right->AsObject()->GetIntegerField(TEXT("childIndex")) : Left->AsObject()->GetStringField(TEXT("parentNodeId")) < Right->AsObject()->GetStringField(TEXT("parentNodeId")); });
        Result->SetArrayField(TEXT("nodes"), Nodes); Result->SetArrayField(TEXT("links"), Links); return SuccessResponse(Id, Result, Operation, Args);
    }
    if (Expected.IsEmpty()) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("expectedRevision is required")); if (Expected != ObjectContentRevision(Tree)) return ErrorResponse(Id, TEXT("conflict"), TEXT("behavior tree revision is stale"));
    FString ParentId, ChildId, NodeId, NodeType;
    if (Operation == TEXT("behavior_tree.node_ensure"))
    {
        if (!Args->TryGetStringField(TEXT("nodeId"), NodeId) || !P14BehaviorTreeNodeIdValid(NodeId) || !Args->TryGetStringField(TEXT("nodeType"), NodeType) || (NodeType != TEXT("sequence") && NodeType != TEXT("move_to") && NodeType != TEXT("wait"))) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("nodeId is invalid and supported nodeType is required"));
        if (!P14BehaviorTreeRuntimeValid(*Tree, *Graph, true)) return ErrorResponse(Id, TEXT("conflict"), TEXT("behavior tree node semantics are invalid"));
        UEdGraphNode* Existing = P14FindBehaviorTreeNode(*Graph, NodeId);
        if (Existing && P14BehaviorTreeNodeType(*Existing) != NodeType) return ErrorResponse(Id, TEXT("conflict"), TEXT("node exists with incompatible type"));
        if (Existing && Cast<UBehaviorTreeGraphNode_Task>(Existing) && !P14BehaviorTreeTaskValid(*Tree, *Existing)) return ErrorResponse(Id, TEXT("conflict"), TEXT("task node semantics are invalid"));
        if (Existing == nullptr && Graph->Nodes.Num() >= 33) return ErrorResponse(Id, TEXT("conflict"), TEXT("behavior tree node limit exceeded"));
        const bool Changed = Existing == nullptr;
        if (Changed)
        {
            if (NodeType == TEXT("move_to") && (!Tree->BlackboardAsset || !P14IsActorKey(*Tree->BlackboardAsset, FName(TEXT("TargetActor"))))) return ErrorResponse(Id, TEXT("conflict"), TEXT("Behavior Tree Blackboard requires Actor key TargetActor"));
            const bool WasDirty = Tree->GetOutermost()->IsDirty(); const FString BeforeRevision = ObjectContentRevision(Tree); FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P14BehaviorTreeNode", "Magi AXI Ensure Behavior Tree Node")); Tree->Modify(); Graph->Modify();
            if (NodeType == TEXT("sequence")) { UBehaviorTreeGraphNode_Composite* Node = NewObject<UBehaviorTreeGraphNode_Composite>(Graph); Node->NodeInstance = NewObject<UBTComposite_Sequence>(Node); Existing = Node; }
            else
            {
                UBehaviorTreeGraphNode_Task* Node = NewObject<UBehaviorTreeGraphNode_Task>(Graph);
                if (NodeType == TEXT("move_to"))
                {
                    check(Tree->BlackboardAsset);
                    UBTTask_MoveTo* MoveTo = NewObject<UBTTask_MoveTo>(Node);
                    FBlackboardKeySelector* Selector = P14MoveToSelector(*MoveTo);
                    if (!Selector) { MoveTo->MarkAsGarbage(); Node->MarkAsGarbage(); Tree->GetOutermost()->SetDirtyFlag(WasDirty); Transaction.Cancel(); return ErrorResponse(Id, TEXT("operation_failed"), TEXT("MoveTo Blackboard selector is unavailable")); }
                    Selector->SelectedKeyName = FName(TEXT("TargetActor"));
                    Selector->ResolveSelectedKey(*Tree->BlackboardAsset);
                    Node->NodeInstance = MoveTo;
                }
                else { UBTTask_Wait* Wait = NewObject<UBTTask_Wait>(Node); Wait->WaitTime = 0.5f; Node->NodeInstance = Wait; }
                Existing = Node;
            }
            Existing->CreateNewGuid();
            Existing->NodeGuid = P14BehaviorTreeGuid(*Tree, NodeId);
            Existing->NodeComment = NodeId;
            Graph->AddNode(Existing, true, true);
            Existing->AllocateDefaultPins();
            Graph->NotifyGraphChanged();
            Graph->UpdateAsset();
            Tree->MarkPackageDirty();
#if WITH_DEV_AUTOMATION_TESTS
            const bool ForcedFailure = GP11ForceAtomicFailure;
#else
            const bool ForcedFailure = false;
#endif
            if (ForcedFailure || P14BehaviorTreeNodeType(*Existing) != NodeType || BeforeRevision == ObjectContentRevision(Tree) || !P14BehaviorTreeRuntimeValid(*Tree, *Graph, true)) { Graph->RemoveNode(Existing); Existing->MarkAsGarbage(); Graph->UpdateAsset(); Tree->GetOutermost()->SetDirtyFlag(WasDirty); Transaction.Cancel(); return ErrorResponse(Id, TEXT("operation_failed"), TEXT("behavior tree node readback failed")); }
        }
        return SuccessResponse(Id, P14BehaviorTreeNodeResult(*Tree, NodeId, NodeType, Existing, Changed), Operation, Args);
    }
    if (Operation == TEXT("behavior_tree.connect"))
    {
        double RequestedIndexValue = -1; if (!Args->TryGetStringField(TEXT("parentNodeId"), ParentId) || !Args->TryGetStringField(TEXT("childNodeId"), ChildId) || (!P14BehaviorTreeNodeIdValid(ParentId) && ParentId != TEXT("root")) || !P14BehaviorTreeNodeIdValid(ChildId) || ParentId.Len() + ChildId.Len() + 2 > 2048 || !Args->TryGetNumberField(TEXT("childIndex"), RequestedIndexValue) || RequestedIndexValue != FMath::TruncToDouble(RequestedIndexValue) || RequestedIndexValue < 0 || RequestedIndexValue > 31) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("valid parentNodeId, childNodeId, bounded linkId, and integer childIndex 0..31 are required"));
        const int32 RequestedIndex = static_cast<int32>(RequestedIndexValue); UEdGraphNode* Parent = P14FindBehaviorTreeNode(*Graph, ParentId); UEdGraphNode* Child = P14FindBehaviorTreeNode(*Graph, ChildId); UBehaviorTreeGraphNode_Root* Root = P14BehaviorTreeRoot(*Graph);
        if (!P14BehaviorTreeRuntimeValid(*Tree, *Graph, true) || !Root || !Parent || !Child) return ErrorResponse(Id, TEXT("conflict"), TEXT("behavior tree topology or node is invalid"));
        if (Child == Root || Parent == Child || (!Cast<UBehaviorTreeGraphNode_Root>(Parent) && !Cast<UBehaviorTreeGraphNode_Composite>(Parent))) return ErrorResponse(Id, TEXT("conflict"), TEXT("behavior tree connection violates root or sequence contract"));
        if (Cast<UBehaviorTreeGraphNode_Root>(Parent) && P14BehaviorTreeNodeType(*Child) != TEXT("sequence")) return ErrorResponse(Id, TEXT("conflict"), TEXT("root accepts exactly one sequence child"));
        if (P14BehaviorTreeReaches(*Child, *Parent)) return ErrorResponse(Id, TEXT("conflict"), TEXT("behavior tree connection would create a cycle"));
        for (UEdGraphNode* CandidateParent : Graph->Nodes) if (CandidateParent && CandidateParent != Parent && P14BehaviorTreeChildren(*CandidateParent).Contains(Child)) return ErrorResponse(Id, TEXT("conflict"), TEXT("behavior tree child already has a parent"));
        TArray<UEdGraphNode*> OldChildren = P14BehaviorTreeChildren(*Parent); TArray<int32> OldPositions; for (UEdGraphNode* ExistingChild : OldChildren) OldPositions.Add(ExistingChild->NodePosX); const int32 ExistingIndex = OldChildren.IndexOfByKey(Child);
        if (Cast<UBehaviorTreeGraphNode_Root>(Parent) && (RequestedIndex != 0 || (OldChildren.Num() == 1 && ExistingIndex == INDEX_NONE))) return ErrorResponse(Id, TEXT("conflict"), TEXT("root accepts exactly one child at index 0"));
        if (RequestedIndex > OldChildren.Num() || (ExistingIndex != INDEX_NONE && RequestedIndex >= OldChildren.Num()) || (ExistingIndex == INDEX_NONE && OldChildren.Num() >= 32)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("childIndex must be contiguous and output bound must not be exceeded"));
        const bool AlreadyLinked = ExistingIndex != INDEX_NONE; const bool Changed = !AlreadyLinked || ExistingIndex != RequestedIndex; if (!Changed) { const TSharedRef<FJsonObject> Result = P14Fields(Tree, false); Result->SetStringField(TEXT("behaviorTreeId"), TreeId); Result->SetStringField(TEXT("parentNodeId"), ParentId); Result->SetStringField(TEXT("childNodeId"), ChildId); Result->SetStringField(TEXT("linkId"), ParentId + TEXT("->") + ChildId); Result->SetNumberField(TEXT("childIndex"), ExistingIndex); return SuccessResponse(Id, Result, Operation, Args); }
        UEdGraphPin* ParentOutput = P14BehaviorTreeOutput(*Parent); UEdGraphPin* ChildInput = P14BehaviorTreeInput(*Child);
        if (!AlreadyLinked && (!ParentOutput || !ChildInput || Graph->GetSchema()->CanCreateConnection(ParentOutput, ChildInput).Response != CONNECT_RESPONSE_MAKE)) return ErrorResponse(Id, TEXT("conflict"), TEXT("behavior tree schema rejected connection"));
        const bool WasDirty = Tree->GetOutermost()->IsDirty(); const FString BeforeRevision = ObjectContentRevision(Tree); UBTCompositeNode* OldRootNode = Tree->RootNode; FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P14BehaviorTreeConnect", "Magi AXI Connect Behavior Tree Nodes")); Tree->Modify(); Graph->Modify(); Parent->Modify(); Child->Modify();
        auto Rollback = [&]()
        {
            if (!AlreadyLinked && ParentOutput && ChildInput && ParentOutput->LinkedTo.Contains(ChildInput)) Graph->GetSchema()->BreakSinglePinLink(ParentOutput, ChildInput);
            for (int32 Index = 0; Index < OldChildren.Num(); ++Index) OldChildren[Index]->NodePosX = OldPositions[Index];
            Graph->NotifyGraphChanged(); Graph->UpdateAsset(); Tree->RootNode = OldRootNode; Tree->GetOutermost()->SetDirtyFlag(WasDirty); Transaction.Cancel();
        };
        if (!AlreadyLinked && !Graph->GetSchema()->TryCreateConnection(ParentOutput, ChildInput)) { Rollback(); return ErrorResponse(Id, TEXT("conflict"), TEXT("behavior tree schema rejected connection")); }
        if (!AlreadyLinked && (!ParentOutput->LinkedTo.Contains(ChildInput) || !ChildInput->LinkedTo.Contains(ParentOutput) || ChildInput->Direction != EGPD_Input || ParentOutput->Direction != EGPD_Output)) { Rollback(); return ErrorResponse(Id, TEXT("operation_failed"), TEXT("behavior tree connection readback failed")); }
        if (!P14SetBehaviorTreeChildOrder(*Parent, *Child, RequestedIndex)) { Rollback(); return ErrorResponse(Id, TEXT("conflict"), TEXT("behavior tree child order rejected")); }
        Graph->NotifyGraphChanged(); Graph->UpdateAsset(); Tree->MarkPackageDirty();
#if WITH_DEV_AUTOMATION_TESTS
        const bool ForcedFailure = GP11ForceAtomicFailure;
#else
        const bool ForcedFailure = false;
#endif
        const bool RuntimeValid = P14BehaviorTreeRuntimeValid(*Tree, *Graph, true) && BeforeRevision != ObjectContentRevision(Tree);
        if (ForcedFailure || !RuntimeValid) { Rollback(); return ErrorResponse(Id, TEXT("operation_failed"), TEXT("behavior tree link readback failed")); }
        const TSharedRef<FJsonObject> Result = P14Fields(Tree, true); Result->SetStringField(TEXT("behaviorTreeId"), TreeId); Result->SetStringField(TEXT("parentNodeId"), ParentId); Result->SetStringField(TEXT("childNodeId"), ChildId); Result->SetStringField(TEXT("linkId"), ParentId + TEXT("->") + ChildId); Result->SetNumberField(TEXT("childIndex"), RequestedIndex); return SuccessResponse(Id, Result, Operation, Args);
    }
    return ErrorResponse(Id, TEXT("unsupported"), TEXT("unsupported behavior tree operation"));
}
static bool P14ControllerContract(UBlueprint& Blueprint, UBehaviorTree* Tree)
{
    if (!Blueprint.ParentClass || !Blueprint.ParentClass->IsChildOf(AAIController::StaticClass()) || Blueprint.UbergraphPages.Num() != 1) return false;
    UEdGraph* Graph = Blueprint.UbergraphPages[0];
    UK2Node_Event* Possess = nullptr;
    UK2Node_CallFunction* Run = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (UK2Node_Event* Event = Cast<UK2Node_Event>(Node))
            if (Event->bOverrideFunction && Event->EventReference.GetMemberName() == FName(TEXT("ReceivePossess")) && Event->EventReference.GetMemberParentClass(Event->GetBlueprintClassFromNode()) == AController::StaticClass()) Possess = Event;
        if (UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node))
            if (Call->GetTargetFunction() == AAIController::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(AAIController, RunBehaviorTree))) Run = Call;
    }
    if (!Possess || !Run) return false;
    UEdGraphPin* Pin = Run->FindPin(TEXT("BTAsset"), EGPD_Input);
    UEdGraphPin* Then = Possess->FindPin(TEXT("then"), EGPD_Output);
    UEdGraphPin* Execute = Run->FindPin(TEXT("execute"), EGPD_Input);
    return Pin && Pin->DefaultObject == Tree && Pin->LinkedTo.IsEmpty() && Then && Execute && Then->LinkedTo.Contains(Execute);
}

static TSharedRef<FJsonObject> P14ControllerResult(UBlueprint& Blueprint, UBehaviorTree& Tree, bool Changed)
{
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("blueprintId"), Blueprint.GetPathName()); Result->SetStringField(TEXT("behaviorTreeId"), Tree.GetPathName()); Result->SetStringField(TEXT("semantic"), TEXT("on_possess.run_behavior_tree")); Result->SetBoolField(TEXT("changed"), Changed); Result->SetArrayField(TEXT("dirtyPackages"), P11DirtyPackages(Blueprint, Changed)); Result->SetArrayField(TEXT("savedPackages"), {}); Result->SetStringField(TEXT("revision"), BlueprintContentRevision(Blueprint)); return Result;
}

static TSharedRef<FJsonObject> P14PawnResult(UBlueprint& Blueprint, UBlueprint& Controller, bool Changed)
{
    const ACharacter* Defaults = Blueprint.GeneratedClass ? Cast<ACharacter>(Blueprint.GeneratedClass->GetDefaultObject()) : nullptr; const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("blueprintId"), Blueprint.GetPathName()); Result->SetStringField(TEXT("controllerBlueprintId"), Controller.GetPathName()); const TSharedRef<FJsonObject> Typed = MakeShared<FJsonObject>(); Typed->SetStringField(TEXT("controllerClass"), Defaults && Defaults->AIControllerClass ? Defaults->AIControllerClass->GetPathName() : FString()); Typed->SetStringField(TEXT("autoPossessAI"), TEXT("PlacedInWorldOrSpawned")); Typed->SetNumberField(TEXT("maxWalkSpeed"), Defaults && Defaults->GetCharacterMovement() ? Defaults->GetCharacterMovement()->MaxWalkSpeed : 0.0); Result->SetObjectField(TEXT("typedDefaults"), Typed); Result->SetBoolField(TEXT("changed"), Changed); Result->SetArrayField(TEXT("dirtyPackages"), P11DirtyPackages(Blueprint, Changed)); Result->SetArrayField(TEXT("savedPackages"), {}); Result->SetStringField(TEXT("revision"), BlueprintContentRevision(Blueprint)); return Result;
}

static FString P14AiConfigure(const FString& Id, const FString& Operation, const TSharedPtr<FJsonObject>& Args, const FString& Expected)
{
    FString BlueprintId; if (!Args->TryGetStringField(TEXT("blueprintId"), BlueprintId)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("blueprintId is required")); UBlueprint* Blueprint = P11LoadBlueprint(BlueprintId); if (!Blueprint) return ErrorResponse(Id, TEXT("not_found"), TEXT("Blueprint was not found"));
    if (Expected.IsEmpty()) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("expectedRevision is required")); if (Expected != BlueprintContentRevision(*Blueprint)) return ErrorResponse(Id, TEXT("conflict"), TEXT("Blueprint revision is stale"));
    if (Operation == TEXT("ai.controller_configure"))
    {
        FString TreeId; if (!Args->TryGetStringField(TEXT("behaviorTreeId"), TreeId)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("behaviorTreeId is required")); UBehaviorTree* Tree = LoadObject<UBehaviorTree>(nullptr, *TreeId); if (!Tree) return ErrorResponse(Id, TEXT("not_found"), TEXT("behavior tree was not found")); if (!Blueprint->ParentClass || !Blueprint->ParentClass->IsChildOf(AAIController::StaticClass())) return ErrorResponse(Id, TEXT("conflict"), TEXT("controller Blueprint parent must derive from AAIController"));
        if (P14ControllerContract(*Blueprint, Tree)) return SuccessResponse(Id, P14ControllerResult(*Blueprint, *Tree, false), Operation, Args);
        if (Blueprint->UbergraphPages.Num() != 1) return ErrorResponse(Id, TEXT("conflict"), TEXT("controller Blueprint must have one event graph")); UEdGraph* Graph = Blueprint->UbergraphPages[0]; UK2Node_Event* Event = nullptr; UK2Node_CallFunction* Call = nullptr; for (UEdGraphNode* Node : Graph->Nodes) { if (UK2Node_Event* Candidate = Cast<UK2Node_Event>(Node)) if (Candidate->bOverrideFunction && Candidate->EventReference.GetMemberName() == FName(TEXT("ReceivePossess"))) Event = Candidate; if (UK2Node_CallFunction* Candidate = Cast<UK2Node_CallFunction>(Node)) if (Candidate->GetTargetFunction() == AAIController::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(AAIController, RunBehaviorTree))) Call = Candidate; }
        if (Event || Call) return ErrorResponse(Id, TEXT("conflict"), TEXT("controller graph contains incompatible authored possess or behavior nodes"));
        FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P14ControllerConfigure", "Magi AXI Configure AI Controller"));
        const bool WasDirty = Blueprint->GetOutermost()->IsDirty(); const EBlueprintStatus OldStatus = Blueprint->Status; const FString BeforeRevision = BlueprintContentRevision(*Blueprint); const int32 BeforeNodeCount = Graph->Nodes.Num(); Blueprint->Modify(); Graph->Modify();
        Event = NewObject<UK2Node_Event>(Graph, NAME_None, RF_Transactional); Event->EventReference.SetExternalMember(FName(TEXT("ReceivePossess")), AController::StaticClass()); Event->bOverrideFunction = true; Event->NodeGuid = P11DeterministicGuid(*Blueprint, *Graph, TEXT("ai.controller_configure.possess")); Graph->AddNode(Event, true, false); Event->PostPlacedNewNode(); Event->AllocateDefaultPins();
        Call = NewObject<UK2Node_CallFunction>(Graph, NAME_None, RF_Transactional); Call->SetFromFunction(AAIController::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(AAIController, RunBehaviorTree))); Call->NodeGuid = P11DeterministicGuid(*Blueprint, *Graph, TEXT("ai.controller_configure.run_behavior_tree")); Graph->AddNode(Call, true, false); Call->PostPlacedNewNode(); Call->AllocateDefaultPins();
        auto RollbackController = [&]()
        {
            if (Event && Graph->Nodes.Contains(Event)) Graph->RemoveNode(Event);
            if (Call && Graph->Nodes.Contains(Call)) Graph->RemoveNode(Call);
            Graph->NotifyGraphChanged(); FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
            FCompilerResultsLog RollbackResults; RollbackResults.bSilentMode = true; FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave, &RollbackResults);
            const bool Restored = RollbackResults.NumErrors == 0 && Blueprint->Status != BS_Error && Blueprint->GeneratedClass && Blueprint->GeneratedClass->IsChildOf(AAIController::StaticClass()) && Graph->Nodes.Num() == BeforeNodeCount && BlueprintContentRevision(*Blueprint) == BeforeRevision;
            Blueprint->Status = OldStatus; Blueprint->GetOutermost()->SetDirtyFlag(WasDirty); Transaction.Cancel(); return Restored;
        };
        UEdGraphPin* TreePin = Call->FindPin(TEXT("BTAsset"), EGPD_Input); UEdGraphPin* ThenPin = Event->FindPin(TEXT("then"), EGPD_Output); UEdGraphPin* ExecutePin = Call->FindPin(TEXT("execute"), EGPD_Input);
        if (!TreePin || !ThenPin || !ExecutePin || !Graph->GetSchema()->TryCreateConnection(ThenPin, ExecutePin)) { const bool Restored = RollbackController(); return ErrorResponse(Id, TEXT("operation_failed"), Restored ? TEXT("controller graph pins are unavailable or incompatible") : TEXT("controller graph rollback recompilation failed")); }
        TreePin->DefaultObject = Tree; FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint); FCompilerResultsLog Results; Results.bSilentMode = true; FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave, &Results); const bool Valid = Results.NumErrors == 0 && Blueprint->Status != BS_Error && P14ControllerContract(*Blueprint, Tree);
#if WITH_DEV_AUTOMATION_TESTS
        const bool ForcedFailure = GP11ForceAtomicFailure;
#else
        const bool ForcedFailure = false;
#endif
        if (!Valid || ForcedFailure) { const bool Restored = RollbackController(); return ErrorResponse(Id, TEXT("operation_failed"), !Restored ? TEXT("controller Blueprint rollback recompilation failed") : ForcedFailure ? TEXT("injected P14 atomic failure") : TEXT("controller Blueprint configuration failed readback or compile")); }
        return SuccessResponse(Id, P14ControllerResult(*Blueprint, *Tree, true), Operation, Args);
    }
    FString ControllerId; if (!Args->TryGetStringField(TEXT("controllerBlueprintId"), ControllerId)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("controllerBlueprintId is required")); UBlueprint* Controller = P11LoadBlueprint(ControllerId); if (!Controller) return ErrorResponse(Id, TEXT("not_found"), TEXT("controller Blueprint was not found")); if (!Controller->GeneratedClass || !Controller->GeneratedClass->IsChildOf(AAIController::StaticClass())) return ErrorResponse(Id, TEXT("conflict"), TEXT("controller Blueprint generated class must derive from AAIController")); if (!Blueprint->ParentClass || !Blueprint->ParentClass->IsChildOf(ACharacter::StaticClass())) return ErrorResponse(Id, TEXT("conflict"), TEXT("pawn Blueprint parent must derive from ACharacter")); ACharacter* Defaults = Blueprint->GeneratedClass ? Cast<ACharacter>(Blueprint->GeneratedClass->GetDefaultObject()) : nullptr; if (!Defaults || !Defaults->GetCharacterMovement()) return ErrorResponse(Id, TEXT("conflict"), TEXT("pawn generated class defaults are unavailable")); const bool Already = Defaults->AutoPossessAI == EAutoPossessAI::PlacedInWorldOrSpawned && Defaults->AIControllerClass == Controller->GeneratedClass && FMath::IsNearlyEqual(Defaults->GetCharacterMovement()->MaxWalkSpeed, 600.0f); if (Already) return SuccessResponse(Id, P14PawnResult(*Blueprint, *Controller, false), Operation, Args); const bool WasDirty = Blueprint->GetOutermost()->IsDirty(); const EBlueprintStatus OldStatus = Blueprint->Status; const EAutoPossessAI OldPossess = Defaults->AutoPossessAI; TSubclassOf<AController> OldClass = Defaults->AIControllerClass; const float OldSpeed = Defaults->GetCharacterMovement()->MaxWalkSpeed; FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P14PawnConfigure", "Magi AXI Configure AI Pawn")); Blueprint->Modify(); Defaults->Modify(); Defaults->GetCharacterMovement()->Modify(); Defaults->AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned; Defaults->AIControllerClass = Controller->GeneratedClass; Defaults->GetCharacterMovement()->MaxWalkSpeed = 600.0f; FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint); FCompilerResultsLog Results; Results.bSilentMode = true; FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave, &Results); ACharacter* CurrentDefaults = Blueprint->GeneratedClass ? Cast<ACharacter>(Blueprint->GeneratedClass->GetDefaultObject()) : nullptr; const bool Valid = Results.NumErrors == 0 && Blueprint->Status != BS_Error && CurrentDefaults && CurrentDefaults->GetCharacterMovement() && CurrentDefaults->AutoPossessAI == EAutoPossessAI::PlacedInWorldOrSpawned && CurrentDefaults->AIControllerClass == Controller->GeneratedClass && FMath::IsNearlyEqual(CurrentDefaults->GetCharacterMovement()->MaxWalkSpeed, 600.0f);
#if WITH_DEV_AUTOMATION_TESTS
    const bool ForcedFailure = GP11ForceAtomicFailure;
#else
    const bool ForcedFailure = false;
#endif
    if (!Valid || ForcedFailure) { for (ACharacter* Restore : {Defaults, CurrentDefaults}) if (Restore && Restore->GetCharacterMovement()) { Restore->AutoPossessAI = OldPossess; Restore->AIControllerClass = OldClass; Restore->GetCharacterMovement()->MaxWalkSpeed = OldSpeed; } Blueprint->Status = OldStatus; Blueprint->GetOutermost()->SetDirtyFlag(WasDirty); Transaction.Cancel(); return ErrorResponse(Id, TEXT("operation_failed"), ForcedFailure ? TEXT("injected P14 atomic failure") : TEXT("pawn Blueprint configuration failed readback or compile")); }
    return SuccessResponse(Id, P14PawnResult(*Blueprint, *Controller, true), Operation, Args);
}
struct FP14NavigationTicket
{
    FString OperationId;
    FString LevelId;
    FString WorldId;
    double StartedAt = 0.0;
    FString State = TEXT("scheduled");
    FString Message;
    bool FailureReceiptStored = false;
};
static TMap<FString, FP14NavigationTicket> P14NavigationTickets;
static constexpr int32 P14MaxNavigationTickets = 1024;
static constexpr double P14NavigationTicketTtl = 24.0 * 60.0 * 60.0;
static void P14PruneNavigationTickets()
{
    const double Now = FPlatformTime::Seconds();
    for (auto It = P14NavigationTickets.CreateIterator(); It; ++It) if (Now - It.Value().StartedAt > P14NavigationTicketTtl) It.RemoveCurrent();
    while (P14NavigationTickets.Num() > P14MaxNavigationTickets)
    {
        FString OldestId; double OldestTime = TNumericLimits<double>::Max();
        for (const TPair<FString, FP14NavigationTicket>& Pair : P14NavigationTickets) if (Pair.Value.StartedAt < OldestTime) { OldestTime = Pair.Value.StartedAt; OldestId = Pair.Key; }
        if (OldestId.IsEmpty()) break;
        P14NavigationTickets.Remove(OldestId);
    }
}
bool P14NavigationTicketExists(const FString& TicketId) { P14PruneNavigationTickets(); return P14NavigationTickets.Contains(TicketId); }
static uint64 P14NavigationTicketCounter = 0;

static FString P14NavigationBuildFailureResponse(const FString& TicketId, const FP14NavigationTicket& Ticket)
{
    const FString Revision = Sha256(TicketId + Ticket.State + Ticket.Message);
    const TSharedRef<FJsonObject> Error = MakeShared<FJsonObject>(); Error->SetStringField(TEXT("type"), TEXT("navigation_build_failed")); Error->SetStringField(TEXT("message"), Ticket.Message); Error->SetBoolField(TEXT("retryable"), false); Error->SetNumberField(TEXT("dirtyPackageCount"), 0); Error->SetArrayField(TEXT("dirtyPackages"), {});
    const TSharedRef<FJsonObject> Verification = MakeShared<FJsonObject>(); Verification->SetStringField(TEXT("target"), TicketId); Verification->SetStringField(TEXT("readback"), TEXT("navigation.status")); Verification->SetBoolField(TEXT("matched"), true); Verification->SetStringField(TEXT("ticketId"), TicketId); Verification->SetStringField(TEXT("levelId"), Ticket.LevelId); Verification->SetStringField(TEXT("requestLevelId"), Ticket.LevelId); Verification->SetStringField(TEXT("observedRevision"), Revision); Verification->SetStringField(TEXT("observedStatus"), TEXT("failed")); Verification->SetBoolField(TEXT("terminal"), true); Verification->SetStringField(TEXT("failureType"), TEXT("navigation_build_failed")); Verification->SetStringField(TEXT("failureMessage"), Ticket.Message);
    const TSharedRef<FJsonObject> Receipt = MakeShared<FJsonObject>(); Receipt->SetStringField(TEXT("operationId"), Ticket.OperationId); Receipt->SetStringField(TEXT("operation"), TEXT("navigation.build")); Receipt->SetStringField(TEXT("state"), TEXT("failed")); Receipt->SetStringField(TEXT("projectId"), ProjectId); Receipt->SetNumberField(TEXT("editorPid"), FPlatformProcess::GetCurrentProcessId()); Receipt->SetStringField(TEXT("target"), TicketId); Receipt->SetBoolField(TEXT("changed"), false); Receipt->SetStringField(TEXT("transaction"), TEXT("non-atomic")); Receipt->SetStringField(TEXT("reversibility"), TEXT("none")); Receipt->SetArrayField(TEXT("dirtyPackages"), {}); Receipt->SetArrayField(TEXT("savedPackages"), {}); Receipt->SetStringField(TEXT("revision"), Revision); Receipt->SetStringField(TEXT("persistence"), TEXT("unchanged")); Receipt->SetObjectField(TEXT("verification"), Verification);
    if (!MagiAxiValidateOutput(TEXT("operation.view"), Receipt)) return ErrorResponse(Ticket.OperationId, TEXT("operation_failed"), TEXT("terminal navigation receipt violates generated operation.view schema"));
    const TSharedRef<FJsonObject> Response = MakeShared<FJsonObject>(); Response->SetNumberField(TEXT("protocol"), ProtocolVersion); Response->SetStringField(TEXT("id"), Ticket.OperationId); Response->SetStringField(TEXT("status"), TEXT("error")); Response->SetObjectField(TEXT("error"), Error); Response->SetObjectField(TEXT("receipt"), Receipt); return Serialize(Response);
}

static bool P14Vector(const TSharedPtr<FJsonObject>& Args, const TCHAR* Name, FVector& Out)
{
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Args->TryGetArrayField(Name, Values) || !Values || Values->Num() != 3) return false;
    Out = FVector((*Values)[0]->AsNumber(), (*Values)[1]->AsNumber(), (*Values)[2]->AsNumber());
    return FMath::IsFinite(Out.X) && FMath::IsFinite(Out.Y) && FMath::IsFinite(Out.Z);
}
static TArray<TSharedPtr<FJsonValue>> P14VectorJson(const FVector& Value)
{
    return {MakeShared<FJsonValueNumber>(Value.X), MakeShared<FJsonValueNumber>(Value.Y), MakeShared<FJsonValueNumber>(Value.Z)};
}
static FString P14NavigationRevision(const FString& Text) { return Sha256(Text); }

static FString P14Navigation(const FString& Id, const FString& Operation, const TSharedPtr<FJsonObject>& Args, const FString& Expected)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World || !World->GetOutermost()) return ErrorResponse(Id, TEXT("unsafe_editor_state"), TEXT("editor world is unavailable"));
    const FString LevelId = World->GetOutermost()->GetName();
    FString RequestedLevel;
    if (Args->TryGetStringField(TEXT("levelId"), RequestedLevel) && RequestedLevel != LevelId) return ErrorResponse(Id, TEXT("conflict"), TEXT("levelId is not current editor level"));
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetNavigationSystem(World);
    if (!NavSys) return ErrorResponse(Id, TEXT("unsafe_editor_state"), TEXT("navigation system is unavailable"));

    if (Operation == TEXT("navigation.bounds_ensure"))
    {
        FString AgentKey; FVector RequestedLocation, RequestedExtent;
        if (!Args->TryGetStringField(TEXT("levelId"), RequestedLevel) || !Args->TryGetStringField(TEXT("agentKey"), AgentKey) || AgentKey.IsEmpty() || !FChar::IsAlnum(AgentKey[0]) || AgentKey.Contains(TEXT(" ")) || !P14Vector(Args, TEXT("location"), RequestedLocation) || !P14Vector(Args, TEXT("extent"), RequestedExtent) || RequestedExtent.X < 0.001f || RequestedExtent.Y < 0.001f || RequestedExtent.Z < 0.001f) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("levelId, canonical agentKey, location, and positive extent are required"));
        const FName OwnershipTag(*FString::Printf(TEXT("MagiAXI.NavBounds.%s"), *AgentKey));
        ANavMeshBoundsVolume* Existing = nullptr;
        for (TActorIterator<ANavMeshBoundsVolume> It(World); It; ++It) if (It->ActorHasTag(OwnershipTag)) { Existing = *It; break; }
        if (Existing)
        {
            const FBox Actual = Existing->GetComponentsBoundingBox(true); const FVector ActualLocation = Actual.GetCenter(); const FVector ActualExtent = Actual.GetExtent();
            if (!Actual.IsValid || !ActualLocation.Equals(RequestedLocation, 0.1f) || !ActualExtent.Equals(RequestedExtent, 0.1f)) return ErrorResponse(Id, TEXT("conflict"), TEXT("owned navigation bounds conflict with requested bounds"));
            const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("levelId"), LevelId); Result->SetStringField(TEXT("agentKey"), AgentKey); Result->SetStringField(TEXT("boundsId"), ActorId(*Existing)); Result->SetArrayField(TEXT("location"), P14VectorJson(ActualLocation)); Result->SetArrayField(TEXT("extent"), P14VectorJson(ActualExtent)); Result->SetBoolField(TEXT("changed"), false); Result->SetArrayField(TEXT("dirtyPackages"), {}); Result->SetArrayField(TEXT("savedPackages"), {}); Result->SetStringField(TEXT("revision"), P14NavigationRevision(Existing->GetPathName() + ActualLocation.ToString() + ActualExtent.ToString())); return SuccessResponse(Id, Result, Operation, Args);
        }
        FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P14NavigationBounds", "Magi AXI Ensure Navigation Bounds"));
        FActorSpawnParameters Parameters; Parameters.OverrideLevel = World->PersistentLevel; Parameters.ObjectFlags |= RF_Transactional;
        ANavMeshBoundsVolume* Volume = World->SpawnActor<ANavMeshBoundsVolume>(ANavMeshBoundsVolume::StaticClass(), RequestedLocation, FRotator::ZeroRotator, Parameters);
        if (!Volume) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("failed to create navigation bounds volume"));
        Volume->Modify(); Volume->Tags.Add(OwnershipTag); Volume->SetActorLabel(FString::Printf(TEXT("MagiAXI_NavBounds_%s"), *AgentKey));
        UCubeBuilder* CubeBuilder = GEditor->FindBrushBuilder(UCubeBuilder::StaticClass()) ? Cast<UCubeBuilder>(GEditor->FindBrushBuilder(UCubeBuilder::StaticClass())) : nullptr;
        if (!CubeBuilder) { Volume->Destroy(); Transaction.Cancel(); return ErrorResponse(Id, TEXT("operation_failed"), TEXT("cube builder is unavailable")); }
        CubeBuilder->X = RequestedExtent.X * 2.0f; CubeBuilder->Y = RequestedExtent.Y * 2.0f; CubeBuilder->Z = RequestedExtent.Z * 2.0f; UActorFactory::CreateBrushForVolumeActor(Volume, CubeBuilder); Volume->PostEditChange(); NavSys->OnNavigationBoundsUpdated(Volume);
        const FBox Actual = Volume->GetComponentsBoundingBox(true); if (!Actual.IsValid || !Actual.GetCenter().Equals(RequestedLocation, 0.1f) || !Actual.GetExtent().Equals(RequestedExtent, 0.1f)) { Volume->Destroy(); Transaction.Cancel(); return ErrorResponse(Id, TEXT("operation_failed"), TEXT("navigation bounds readback mismatch")); }
#if WITH_DEV_AUTOMATION_TESTS
        if (GP11ForceAtomicFailure) { Volume->Destroy(); Transaction.Cancel(); return ErrorResponse(Id, TEXT("operation_failed"), TEXT("injected P14 atomic failure")); }
#endif
        World->MarkPackageDirty(); const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("levelId"), LevelId); Result->SetStringField(TEXT("agentKey"), AgentKey); Result->SetStringField(TEXT("boundsId"), ActorId(*Volume)); Result->SetArrayField(TEXT("location"), P14VectorJson(Actual.GetCenter())); Result->SetArrayField(TEXT("extent"), P14VectorJson(Actual.GetExtent())); Result->SetBoolField(TEXT("changed"), true); Result->SetArrayField(TEXT("dirtyPackages"), {MakeShared<FJsonValueString>(World->GetOutermost()->GetName())}); Result->SetArrayField(TEXT("savedPackages"), {}); Result->SetStringField(TEXT("revision"), P14NavigationRevision(Volume->GetPathName() + Actual.GetCenter().ToString() + Actual.GetExtent().ToString())); return SuccessResponse(Id, Result, Operation, Args);
    }
    if (Operation == TEXT("navigation.build"))
    {
        if (!Args->TryGetStringField(TEXT("levelId"), RequestedLevel)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("levelId is required"));
        P14PruneNavigationTickets();
        for (const TPair<FString, FP14NavigationTicket>& Pair : P14NavigationTickets) if (Pair.Value.WorldId == World->GetPathName() && Pair.Value.State != TEXT("succeeded") && Pair.Value.State != TEXT("failed")) return ErrorResponse(Id, TEXT("busy"), TEXT("navigation build is already active for this editor world"), true);
        const bool HadWork = NavSys->IsThereAnywhereToBuildNavigation();
        FP14NavigationTicket Ticket; Ticket.OperationId = Id; Ticket.LevelId = LevelId; Ticket.WorldId = World->GetPathName(); Ticket.StartedAt = FPlatformTime::Seconds();
        const FString TicketId = FString::Printf(TEXT("nav-%s-%llu"), *Sha256(LevelId).Left(12), static_cast<unsigned long long>(++P14NavigationTicketCounter)); P14NavigationTickets.Add(TicketId, Ticket); NavSys->Build();
        FP14NavigationTicket& Stored = P14NavigationTickets.FindChecked(TicketId); const bool Built = NavSys->GetDefaultNavDataInstance(FNavigationSystem::ECreateIfMissing::DontCreate) && NavSys->IsNavigationBuilt(World->GetWorldSettings()); Stored.State = Built ? TEXT("succeeded") : TEXT("failed"); if (!Built) Stored.Message = HadWork ? TEXT("navigation build did not produce usable navigation data") : TEXT("navigation data has no buildable bounds");
        if (!Built) { SetReceipt(Id, Operation, P14NavigationBuildFailureResponse(TicketId, Stored), TEXT("failed")); Stored.FailureReceiptStored = true; }
        const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("levelId"), LevelId); Result->SetStringField(TEXT("ticketId"), TicketId); Result->SetStringField(TEXT("state"), TEXT("scheduled")); Result->SetBoolField(TEXT("changed"), HadWork); Result->SetStringField(TEXT("revision"), P14NavigationRevision(TicketId + TEXT("scheduled"))); return SuccessResponse(Id, Result, Operation, Args);
    }
    if (Operation == TEXT("navigation.status"))
    {
        FString TicketId; if (!Args->TryGetStringField(TEXT("ticketId"), TicketId)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("ticketId is required")); P14PruneNavigationTickets(); FP14NavigationTicket* Ticket = P14NavigationTickets.Find(TicketId); if (!Ticket) return ErrorResponse(Id, TEXT("not_found"), TEXT("navigation ticket is unknown or expired"));
        if (Ticket->WorldId != World->GetPathName()) return ErrorResponse(Id, TEXT("stale"), TEXT("navigation ticket belongs to another editor world"));
        if (Ticket->State == TEXT("failed") && !Ticket->FailureReceiptStored) { SetReceipt(Ticket->OperationId, TEXT("navigation.build"), P14NavigationBuildFailureResponse(TicketId, *Ticket), TEXT("failed")); Ticket->FailureReceiptStored = true; }
        const bool Terminal = Ticket->State == TEXT("succeeded") || Ticket->State == TEXT("failed"); const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("ticketId"), TicketId); Result->SetStringField(TEXT("levelId"), Ticket->LevelId); Result->SetStringField(TEXT("state"), Ticket->State); Result->SetBoolField(TEXT("terminal"), Terminal); if (Ticket->Message.IsEmpty()) Result->SetField(TEXT("message"), MakeShared<FJsonValueNull>()); else Result->SetStringField(TEXT("message"), Ticket->Message); Result->SetStringField(TEXT("revision"), P14NavigationRevision(TicketId + Ticket->State + Ticket->Message)); return SuccessResponse(Id, Result, Operation, Args);
    }
    if (Operation == TEXT("navigation.path_query"))
    {
        FVector Start, Target; if (!Args->TryGetStringField(TEXT("levelId"), RequestedLevel) || !P14Vector(Args, TEXT("start"), Start) || !P14Vector(Args, TEXT("target"), Target)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("levelId, start, and target are required"));
        UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(World, Start, Target); const bool HasPath = Path && Path->IsValid() && !Path->PathPoints.IsEmpty(); const bool Reached = HasPath && Path->PathPoints.Last().Equals(Target, 1.0f); TArray<TSharedPtr<FJsonValue>> Points; if (HasPath) for (int32 Index = 0; Index < FMath::Min(Path->PathPoints.Num(), 128); ++Index) Points.Add(MakeShared<FJsonValueArray>(P14VectorJson(Path->PathPoints[Index]))); const bool Partial = HasPath && !Reached; double Length = 0.0; if (HasPath) for (int32 Index = 1; Index < Path->PathPoints.Num(); ++Index) Length += FVector::Distance(Path->PathPoints[Index - 1], Path->PathPoints[Index]);
        const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("levelId"), LevelId); Result->SetArrayField(TEXT("start"), P14VectorJson(Start)); Result->SetArrayField(TEXT("target"), P14VectorJson(Target)); Result->SetBoolField(TEXT("reachable"), Reached); Result->SetBoolField(TEXT("partial"), Partial); Result->SetNumberField(TEXT("pathLength"), Length); Result->SetArrayField(TEXT("points"), Points); Result->SetStringField(TEXT("revision"), P14NavigationRevision(LevelId + Start.ToString() + Target.ToString() + FString::SanitizeFloat(Length))); return SuccessResponse(Id, Result, Operation, Args);
    }
    return ErrorResponse(Id, TEXT("unsupported"), TEXT("unsupported navigation operation"));
}
static FString P14AiObservation(const FString& Id, const FString& Operation, const TSharedPtr<FJsonObject>& Args);
FString HandleP14AiNavigationOperation(const FString& Id, const FString& Operation, const TSharedPtr<FJsonObject>& Args, const FString& ExpectedRevision)
{
    if (!Args.IsValid()) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("operation requires arguments"));
    if (Operation.StartsWith(TEXT("blackboard."))) return P14Blackboard(Id, Operation, Args, ExpectedRevision);
    if (Operation.StartsWith(TEXT("behavior_tree."))) return P14BehaviorTree(Id, Operation, Args, ExpectedRevision);
    if (Operation.StartsWith(TEXT("navigation."))) return P14Navigation(Id, Operation, Args, ExpectedRevision);
    if (Operation == TEXT("ai.controller_configure") || Operation == TEXT("ai.pawn_configure")) return P14AiConfigure(Id, Operation, Args, ExpectedRevision);
    if (Operation == TEXT("play.ai_target_set") || Operation == TEXT("play.ai_observe")) return P14AiObservation(Id, Operation, Args);
    return ErrorResponse(Id, TEXT("unsupported"), TEXT("P1.4 operation is not available in this native build"));
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIP14BehaviorTreeContracts, "MagiUnrealAXI.P14.BehaviorTreeContracts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIP14BehaviorTreeContracts::RunTest(const FString&)
{
    FModuleManager::Get().LoadModule(TEXT("AIGraph")); FModuleManager::Get().LoadModule(TEXT("BehaviorTreeEditor"));
    auto Dispatch = [](const FString& Id, const FString& Operation, const TSharedRef<FJsonObject>& Args, const FString& Expected = FString()) { return ReadResponseOnGameThread(Id, Operation, Args, Expected); };
    auto EnsureNode = [&](UBehaviorTree& Tree, const FString& NodeId, const FString& NodeType, const FString& Id) { TSharedRef<FJsonObject> Args = MakeShared<FJsonObject>(); Args->SetStringField(TEXT("behaviorTreeId"), Tree.GetPathName()); Args->SetStringField(TEXT("nodeId"), NodeId); Args->SetStringField(TEXT("nodeType"), NodeType); return Dispatch(Id, TEXT("behavior_tree.node_ensure"), Args, ObjectContentRevision(&Tree)); };
    auto Connect = [&](UBehaviorTree& Tree, const FString& ParentId, const FString& ChildId, int32 ChildIndex, const FString& Id) { TSharedRef<FJsonObject> Args = MakeShared<FJsonObject>(); Args->SetStringField(TEXT("behaviorTreeId"), Tree.GetPathName()); Args->SetStringField(TEXT("parentNodeId"), ParentId); Args->SetStringField(TEXT("childNodeId"), ChildId); Args->SetNumberField(TEXT("childIndex"), ChildIndex); return Dispatch(Id, TEXT("behavior_tree.connect"), Args, ObjectContentRevision(&Tree)); };
    const FString BlackboardPath = TEXT("/Game/MagiP14Automation/BB_BTContracts"); TSharedRef<FJsonObject> BlackboardCreate = MakeShared<FJsonObject>(); BlackboardCreate->SetStringField(TEXT("path"), BlackboardPath); const FString BlackboardCreateResponse = Dispatch(TEXT("p14-bt-bb-create"), TEXT("blackboard.create"), BlackboardCreate); TestTrue(TEXT("public Blackboard create succeeds"), ResponseStatusIsOk(BlackboardCreateResponse)); UBlackboardData* Blackboard = LoadObject<UBlackboardData>(nullptr, *(BlackboardPath + TEXT(".BB_BTContracts"))); TestNotNull(TEXT("Behavior Tree Blackboard exists"), Blackboard); if (!Blackboard) return false;
    TSharedRef<FJsonObject> KeyArgs = MakeShared<FJsonObject>(); KeyArgs->SetStringField(TEXT("blackboardId"), Blackboard->GetPathName()); KeyArgs->SetStringField(TEXT("keyName"), TEXT("TargetActor")); KeyArgs->SetStringField(TEXT("keyType"), TEXT("Actor")); TestTrue(TEXT("public Actor key ensure succeeds"), ResponseStatusIsOk(Dispatch(TEXT("p14-bt-key"), TEXT("blackboard.key_ensure"), KeyArgs, ObjectContentRevision(Blackboard))));
    const FString TreePath = TEXT("/Game/MagiP14Automation/BT_Contracts"); TSharedRef<FJsonObject> TreeCreate = MakeShared<FJsonObject>(); TreeCreate->SetStringField(TEXT("path"), TreePath); TreeCreate->SetStringField(TEXT("blackboardId"), Blackboard->GetPathName()); const FString TreeCreateResponse = Dispatch(TEXT("p14-bt-create"), TEXT("behavior_tree.create"), TreeCreate); TestTrue(TEXT("public Behavior Tree create succeeds"), ResponseStatusIsOk(TreeCreateResponse)); UBehaviorTree* Tree = LoadObject<UBehaviorTree>(nullptr, *(TreePath + TEXT(".BT_Contracts"))); TestNotNull(TEXT("Behavior Tree exists"), Tree); if (!Tree) return false;
    TestTrue(TEXT("sequence ensure succeeds"), ResponseStatusIsOk(EnsureNode(*Tree, TEXT("loop"), TEXT("sequence"), TEXT("p14-bt-sequence")))); TestTrue(TEXT("MoveTo ensure succeeds"), ResponseStatusIsOk(EnsureNode(*Tree, TEXT("move"), TEXT("move_to"), TEXT("p14-bt-move")))); TestTrue(TEXT("Wait ensure succeeds"), ResponseStatusIsOk(EnsureNode(*Tree, TEXT("wait"), TEXT("wait"), TEXT("p14-bt-wait"))));
    TestTrue(TEXT("MoveTo connects before root at index zero"), ResponseStatusIsOk(Connect(*Tree, TEXT("loop"), TEXT("move"), 0, TEXT("p14-bt-move-link")))); TestNull(TEXT("subtree authoring preserves empty runtime root"), Tree->RootNode); TestTrue(TEXT("Wait connects before root at index one"), ResponseStatusIsOk(Connect(*Tree, TEXT("loop"), TEXT("wait"), 1, TEXT("p14-bt-wait-link")))); TestTrue(TEXT("root connects to sequence after subtree authoring"), ResponseStatusIsOk(Connect(*Tree, TEXT("root"), TEXT("loop"), 0, TEXT("p14-bt-root-link"))));
    UBehaviorTreeGraph* Graph = Cast<UBehaviorTreeGraph>(Tree->BTGraph); UEdGraphNode* Loop = Graph ? P14FindBehaviorTreeNode(*Graph, TEXT("loop")) : nullptr; UEdGraphNode* Move = Graph ? P14FindBehaviorTreeNode(*Graph, TEXT("move")) : nullptr; UEdGraphNode* Wait = Graph ? P14FindBehaviorTreeNode(*Graph, TEXT("wait")) : nullptr; UBehaviorTreeGraphNode_Composite* LoopComposite = Cast<UBehaviorTreeGraphNode_Composite>(Loop); TestTrue(TEXT("engine root executes authored sequence"), LoopComposite && Tree->RootNode == LoopComposite->NodeInstance); const TArray<UEdGraphNode*> Ordered = Loop ? P14BehaviorTreeChildren(*Loop) : TArray<UEdGraphNode*>(); TestTrue(TEXT("authored child order is MoveTo then Wait"), Ordered.Num() == 2 && Ordered[0] == Move && Ordered[1] == Wait);
    TSharedRef<FJsonObject> ViewArgs = MakeShared<FJsonObject>(); ViewArgs->SetStringField(TEXT("behaviorTreeId"), Tree->GetPathName()); const FString View = Dispatch(TEXT("p14-bt-view"), TEXT("behavior_tree.view"), ViewArgs); TSharedPtr<FJsonObject> ParsedView; const TSharedPtr<FJsonObject>* ViewResult = nullptr; const TArray<TSharedPtr<FJsonValue>>* ViewLinks = nullptr; const TSharedRef<TJsonReader<>> ViewReader = TJsonReaderFactory<>::Create(View); bool RootLink = false, MoveLink = false, WaitLink = false; if (FJsonSerializer::Deserialize(ViewReader, ParsedView) && ParsedView.IsValid() && ParsedView->TryGetObjectField(TEXT("result"), ViewResult) && ViewResult && ViewResult->IsValid() && (*ViewResult)->TryGetArrayField(TEXT("links"), ViewLinks) && ViewLinks) for (const TSharedPtr<FJsonValue>& Value : *ViewLinks) { const TSharedPtr<FJsonObject> Link = Value ? Value->AsObject() : nullptr; if (!Link) continue; const FString Parent = Link->GetStringField(TEXT("parentNodeId")); const FString Child = Link->GetStringField(TEXT("childNodeId")); const int32 Index = Link->GetIntegerField(TEXT("childIndex")); RootLink |= Parent == TEXT("root") && Child == TEXT("loop") && Index == 0; MoveLink |= Parent == TEXT("loop") && Child == TEXT("move") && Index == 0; WaitLink |= Parent == TEXT("loop") && Child == TEXT("wait") && Index == 1; } TestTrue(TEXT("public view exposes root and ordered links"), ResponseStatusIsOk(View) && RootLink && MoveLink && WaitLink);
    const FString StableRevision = ObjectContentRevision(Tree); TestTrue(TEXT("repeat ordered connection is idempotent"), ResponseStatusIsOk(Connect(*Tree, TEXT("loop"), TEXT("wait"), 1, TEXT("p14-bt-repeat")))); TestEqual(TEXT("idempotent connection preserves revision"), ObjectContentRevision(Tree), StableRevision);
    TSharedRef<FJsonObject> Reserved = MakeShared<FJsonObject>(); Reserved->SetStringField(TEXT("behaviorTreeId"), Tree->GetPathName()); Reserved->SetStringField(TEXT("nodeId"), TEXT("root")); Reserved->SetStringField(TEXT("nodeType"), TEXT("sequence")); TestTrue(TEXT("reserved root cannot be authored"), Dispatch(TEXT("p14-bt-reserved"), TEXT("behavior_tree.node_ensure"), Reserved, StableRevision).Contains(TEXT("invalid_input")));
    TSharedRef<FJsonObject> DelimiterNode = MakeShared<FJsonObject>(); DelimiterNode->SetStringField(TEXT("behaviorTreeId"), Tree->GetPathName()); DelimiterNode->SetStringField(TEXT("nodeId"), TEXT("bad->id")); DelimiterNode->SetStringField(TEXT("nodeType"), TEXT("wait")); TestTrue(TEXT("node ID link delimiter is rejected"), Dispatch(TEXT("p14-bt-delimiter"), TEXT("behavior_tree.node_ensure"), DelimiterNode, StableRevision).Contains(TEXT("invalid_input")));
    DelimiterNode->SetStringField(TEXT("nodeId"), TEXT("bad\nid")); TestTrue(TEXT("node ID newline is rejected"), Dispatch(TEXT("p14-bt-newline"), TEXT("behavior_tree.node_ensure"), DelimiterNode, StableRevision).Contains(TEXT("invalid_input")));
    GP11ForceAtomicFailure = true; const FString Rollback = Connect(*Tree, TEXT("loop"), TEXT("wait"), 0, TEXT("p14-bt-rollback")); GP11ForceAtomicFailure = false; TestTrue(TEXT("forced ordered-link failure rejects"), Rollback.Contains(TEXT("operation_failed"))); TestEqual(TEXT("forced ordered-link failure restores revision"), ObjectContentRevision(Tree), StableRevision); const TArray<UEdGraphNode*> Restored = Loop ? P14BehaviorTreeChildren(*Loop) : TArray<UEdGraphNode*>(); TestTrue(TEXT("forced ordered-link failure restores runtime order"), Restored.Num() == 2 && Restored[0] == Move && Restored[1] == Wait);
    TSharedRef<FJsonObject> OversizedLink = MakeShared<FJsonObject>(); OversizedLink->SetStringField(TEXT("behaviorTreeId"), Tree->GetPathName()); OversizedLink->SetStringField(TEXT("parentNodeId"), FString::ChrN(1024, TEXT('a'))); OversizedLink->SetStringField(TEXT("childNodeId"), FString::ChrN(1024, TEXT('b'))); OversizedLink->SetNumberField(TEXT("childIndex"), 0); TestTrue(TEXT("link identity exceeding output bound rejects before mutation"), Dispatch(TEXT("p14-bt-link-bound"), TEXT("behavior_tree.connect"), OversizedLink, StableRevision).Contains(TEXT("invalid_input"))); TestEqual(TEXT("link bound rejection preserves revision"), ObjectContentRevision(Tree), StableRevision);
    UBehaviorTreeGraphNode_Task* WaitGraphNode = Cast<UBehaviorTreeGraphNode_Task>(Wait); UBTTask_Wait* WaitTask = WaitGraphNode ? Cast<UBTTask_Wait>(WaitGraphNode->NodeInstance) : nullptr; TestNotNull(TEXT("Wait task is available for malformed persistence check"), WaitTask); if (WaitTask) { WaitTask->WaitTime = 1.0f; TestTrue(TEXT("malformed persisted Wait has no revision"), ObjectContentRevision(Tree).IsEmpty()); TestTrue(TEXT("view rejects malformed persisted Wait"), Dispatch(TEXT("p14-bt-malformed-wait"), TEXT("behavior_tree.view"), ViewArgs).Contains(TEXT("conflict"))); WaitTask->WaitTime = 0.5f; TestEqual(TEXT("restoring Wait semantics restores revision"), ObjectContentRevision(Tree), StableRevision); }
    UBTCompositeNode* RuntimeRoot = Tree->RootNode; Tree->RootNode = nullptr; TestTrue(TEXT("stale runtime root has no revision"), ObjectContentRevision(Tree).IsEmpty()); TestTrue(TEXT("view rejects stale runtime root"), Dispatch(TEXT("p14-bt-stale-root"), TEXT("behavior_tree.view"), ViewArgs).Contains(TEXT("conflict"))); Tree->RootNode = RuntimeRoot; TestEqual(TEXT("restoring runtime root restores revision"), ObjectContentRevision(Tree), StableRevision);
    UEdGraphPin* MoveInput = Move ? P14BehaviorTreeInput(*Move) : nullptr; UEdGraphPin* LoopOutput = Loop ? P14BehaviorTreeOutput(*Loop) : nullptr; TestTrue(TEXT("linked pins exist for malformed reciprocity check"), MoveInput && LoopOutput); if (MoveInput && LoopOutput) { MoveInput->LinkedTo.Reset(); TestTrue(TEXT("non-reciprocal persisted link has no revision"), ObjectContentRevision(Tree).IsEmpty()); TestTrue(TEXT("view rejects non-reciprocal persisted link"), Dispatch(TEXT("p14-bt-one-way-link"), TEXT("behavior_tree.view"), ViewArgs).Contains(TEXT("conflict"))); MoveInput->LinkedTo.Add(LoopOutput); TestEqual(TEXT("restoring reciprocal link restores revision"), ObjectContentRevision(Tree), StableRevision); }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIP14BlackboardContracts, "MagiUnrealAXI.P14.BlackboardContracts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIP14BlackboardContracts::RunTest(const FString&)
{
    const FString Path = TEXT("/Game/MagiP14Automation/BB_Contracts"); TSharedRef<FJsonObject> CreateArgs = MakeShared<FJsonObject>(); CreateArgs->SetStringField(TEXT("path"), Path); const FString Created = P14Blackboard(TEXT("p14-bb-create"), TEXT("blackboard.create"), CreateArgs, FString()); TestTrue(TEXT("blackboard create succeeds"), ResponseStatusIsOk(Created)); UBlackboardData* Asset = LoadObject<UBlackboardData>(nullptr, *(Path + TEXT(".") + FPackageName::GetShortName(Path))); TestNotNull(TEXT("blackboard asset exists"), Asset); if (!Asset) return false;
    TSharedRef<FJsonObject> KeyArgs = MakeShared<FJsonObject>(); KeyArgs->SetStringField(TEXT("blackboardId"), Asset->GetPathName()); KeyArgs->SetStringField(TEXT("keyName"), TEXT("TargetActor")); KeyArgs->SetStringField(TEXT("keyType"), TEXT("Actor")); const FString First = P14Blackboard(TEXT("p14-bb-key"), TEXT("blackboard.key_ensure"), KeyArgs, ObjectContentRevision(Asset)); TestTrue(TEXT("Actor key ensure succeeds"), ResponseStatusIsOk(First)); const FString Repeat = P14Blackboard(TEXT("p14-bb-repeat"), TEXT("blackboard.key_ensure"), KeyArgs, ObjectContentRevision(Asset)); TestTrue(TEXT("Actor key ensure is idempotent"), ResponseStatusIsOk(Repeat)); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIP14AiObserveContracts, "MagiUnrealAXI.P14.AiObserveContracts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIP14AiObserveContracts::RunTest(const FString&)
{
    TSharedRef<FJsonObject> Args = MakeShared<FJsonObject>(); Args->SetStringField(TEXT("sessionId"), TEXT("missing-session")); Args->SetStringField(TEXT("pawnId"), TEXT("missing")); Args->SetStringField(TEXT("keyName"), TEXT("TargetActor")); const FString Response = P14AiObservation(TEXT("p14-observe"), TEXT("play.ai_observe"), Args); TestTrue(TEXT("observe rejects absent PIE world or actor"), Response.Contains(TEXT("stale")) || Response.Contains(TEXT("unsafe_editor_state")) || Response.Contains(TEXT("not_found"))); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIP14AiControllerContracts, "MagiUnrealAXI.P14.AiControllerContracts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIP14AiControllerContracts::RunTest(const FString&)
{
    auto Create = [](const FString& Path, UClass* Parent) { UPackage* Package = CreatePackage(*Path); return Package ? FKismetEditorUtilities::CreateBlueprint(Parent, Package, FName(*FPackageName::GetShortName(Path)), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), TEXT("MagiUnrealAXI")) : nullptr; };
    UBlueprint* Controller = Create(TEXT("/Game/MagiP14Automation/AIController_Contracts"), AAIController::StaticClass()); UBlueprint* Pawn = Create(TEXT("/Game/MagiP14Automation/AIPawn_Contracts"), ACharacter::StaticClass()); TestTrue(TEXT("controller and pawn Blueprints created"), Controller && Pawn); if (!Controller || !Pawn) return false;
    FCompilerResultsLog Compile; Compile.bSilentMode = true; FKismetEditorUtilities::CompileBlueprint(Controller, EBlueprintCompileOptions::SkipSave, &Compile); Compile.Messages.Reset(); Compile.NumErrors = 0; FKismetEditorUtilities::CompileBlueprint(Pawn, EBlueprintCompileOptions::SkipSave, &Compile); TestTrue(TEXT("controller and pawn compile"), Controller->GeneratedClass && Pawn->GeneratedClass && Controller->Status != BS_Error && Pawn->Status != BS_Error);
    UPackage* TreePackage = CreatePackage(TEXT("/Game/MagiP14Automation/AIControllerTree")); UBehaviorTree* Tree = NewObject<UBehaviorTree>(TreePackage, TEXT("AIControllerTree"), RF_Public | RF_Standalone); Tree->BTGraph = NewObject<UBehaviorTreeGraph>(Tree, TEXT("BTGraph"), RF_Transactional); Tree->BTGraph->Schema = UEdGraphSchema_BehaviorTree::StaticClass(); FAssetRegistryModule::AssetCreated(Tree);
    UBlueprint* RollbackController = Create(TEXT("/Game/MagiP14Automation/AIController_Rollback"), AAIController::StaticClass()); TestNotNull(TEXT("rollback controller created"), RollbackController); if (!RollbackController) return false; Compile.Messages.Reset(); Compile.NumErrors = 0; FKismetEditorUtilities::CompileBlueprint(RollbackController, EBlueprintCompileOptions::SkipSave, &Compile); RollbackController->GetOutermost()->SetDirtyFlag(false); const FString ControllerRollbackRevision = BlueprintContentRevision(*RollbackController); const EBlueprintStatus ControllerRollbackStatus = RollbackController->Status; const int32 ControllerRollbackNodes = RollbackController->UbergraphPages.Num() == 1 ? RollbackController->UbergraphPages[0]->Nodes.Num() : -1; TSharedRef<FJsonObject> ControllerRollbackArgs = MakeShared<FJsonObject>(); ControllerRollbackArgs->SetStringField(TEXT("blueprintId"), RollbackController->GetPathName()); ControllerRollbackArgs->SetStringField(TEXT("behaviorTreeId"), Tree->GetPathName()); GP11ForceAtomicFailure = true; const FString ControllerRollback = P14AiConfigure(TEXT("p14-ai-controller-rollback"), TEXT("ai.controller_configure"), ControllerRollbackArgs, ControllerRollbackRevision); GP11ForceAtomicFailure = false; TestTrue(TEXT("controller atomic failure returns operation_failed"), ControllerRollback.Contains(TEXT("operation_failed"))); TestTrue(TEXT("controller rollback recompiles usable generated class"), RollbackController->GeneratedClass && RollbackController->GeneratedClass->IsChildOf(AAIController::StaticClass()) && RollbackController->Status == ControllerRollbackStatus); TestTrue(TEXT("controller rollback restores graph and clean package"), RollbackController->UbergraphPages.Num() == 1 && RollbackController->UbergraphPages[0]->Nodes.Num() == ControllerRollbackNodes && !RollbackController->GetOutermost()->IsDirty()); TestEqual(TEXT("controller rollback restores revision"), BlueprintContentRevision(*RollbackController), ControllerRollbackRevision); TestTrue(TEXT("controller configure succeeds after rollback recompilation"), ResponseStatusIsOk(P14AiConfigure(TEXT("p14-ai-controller-after-rollback"), TEXT("ai.controller_configure"), ControllerRollbackArgs, ControllerRollbackRevision)));
    const FString Before = BlueprintContentRevision(*Controller); TSharedRef<FJsonObject> ControllerArgs = MakeShared<FJsonObject>(); ControllerArgs->SetStringField(TEXT("blueprintId"), Controller->GetPathName()); ControllerArgs->SetStringField(TEXT("behaviorTreeId"), Tree->GetPathName()); const FString First = P14AiConfigure(TEXT("p14-ai-controller"), TEXT("ai.controller_configure"), ControllerArgs, Before); TestTrue(TEXT("controller configure succeeds"), ResponseStatusIsOk(First)); const FString Configured = BlueprintContentRevision(*Controller); const FString Repeat = P14AiConfigure(TEXT("p14-ai-controller-repeat"), TEXT("ai.controller_configure"), ControllerArgs, Configured); TestTrue(TEXT("controller configure is idempotent"), ResponseStatusIsOk(Repeat)); const FString Stale = P14AiConfigure(TEXT("p14-ai-controller-stale"), TEXT("ai.controller_configure"), ControllerArgs, Before); TestTrue(TEXT("stale controller revision rejects"), Stale.Contains(TEXT("conflict")));
    TSharedRef<FJsonObject> PawnArgs = MakeShared<FJsonObject>(); PawnArgs->SetStringField(TEXT("blueprintId"), Pawn->GetPathName()); PawnArgs->SetStringField(TEXT("controllerBlueprintId"), Controller->GetPathName()); const FString PawnBefore = BlueprintContentRevision(*Pawn); const FString PawnFirst = P14AiConfigure(TEXT("p14-ai-pawn"), TEXT("ai.pawn_configure"), PawnArgs, PawnBefore); TestTrue(TEXT("pawn configure succeeds"), ResponseStatusIsOk(PawnFirst)); const FString PawnConfigured = BlueprintContentRevision(*Pawn); const FString PawnRepeat = P14AiConfigure(TEXT("p14-ai-pawn-repeat"), TEXT("ai.pawn_configure"), PawnArgs, PawnConfigured); TestTrue(TEXT("pawn configure is idempotent"), ResponseStatusIsOk(PawnRepeat)); UBlueprint* RollbackPawn = Create(TEXT("/Game/MagiP14Automation/AIPawn_Rollback"), ACharacter::StaticClass()); TestNotNull(TEXT("rollback pawn created"), RollbackPawn); if (!RollbackPawn) return false; Compile.Messages.Reset(); Compile.NumErrors = 0; FKismetEditorUtilities::CompileBlueprint(RollbackPawn, EBlueprintCompileOptions::SkipSave, &Compile); TSharedRef<FJsonObject> RollbackArgs = MakeShared<FJsonObject>(); RollbackArgs->SetStringField(TEXT("blueprintId"), RollbackPawn->GetPathName()); RollbackArgs->SetStringField(TEXT("controllerBlueprintId"), Controller->GetPathName()); const FString RollbackRevision = BlueprintContentRevision(*RollbackPawn); GP11ForceAtomicFailure = true; const FString Rollback = P14AiConfigure(TEXT("p14-ai-pawn-rollback"), TEXT("ai.pawn_configure"), RollbackArgs, RollbackRevision); GP11ForceAtomicFailure = false; TestTrue(TEXT("pawn atomic failure returns operation_failed"), Rollback.Contains(TEXT("operation_failed"))); TestEqual(TEXT("pawn atomic failure restores revision"), BlueprintContentRevision(*RollbackPawn), RollbackRevision); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIP14NavigationBuildContracts, "MagiUnrealAXI.P14.NavigationBuildContracts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIP14NavigationBuildContracts::RunTest(const FString&)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr; TestNotNull(TEXT("editor world exists"), World); if (!World || !World->GetOutermost()) return false;
    const FString LevelId = World->GetOutermost()->GetName(); TSharedRef<FJsonObject> Unknown = MakeShared<FJsonObject>(); Unknown->SetStringField(TEXT("ticketId"), TEXT("p14-unknown-ticket")); const FString UnknownResponse = P14Navigation(TEXT("p14-nav-unknown"), TEXT("navigation.status"), Unknown, FString()); TestTrue(TEXT("unknown ticket is rejected"), UnknownResponse.Contains(TEXT("not_found")));
    TSharedRef<FJsonObject> BuildArgs = MakeShared<FJsonObject>(); BuildArgs->SetStringField(TEXT("levelId"), LevelId); const FString Response = P14Navigation(TEXT("p14-nav-build"), TEXT("navigation.build"), BuildArgs, FString()); TestTrue(TEXT("real navigation build is scheduled"), ResponseStatusIsOk(Response));
    TSharedPtr<FJsonObject> ParsedBuild; const TSharedRef<TJsonReader<>> BuildReader = TJsonReaderFactory<>::Create(Response); const TSharedPtr<FJsonObject>* BuildResult = nullptr; TestTrue(TEXT("build response parses"), FJsonSerializer::Deserialize(BuildReader, ParsedBuild) && ParsedBuild.IsValid() && ParsedBuild->TryGetObjectField(TEXT("result"), BuildResult) && BuildResult && BuildResult->IsValid()); if (!BuildResult || !BuildResult->IsValid()) return false; const FString TicketId = (*BuildResult)->GetStringField(TEXT("ticketId"));
    FP14NavigationTicket* BuiltTicket = P14NavigationTickets.Find(TicketId); TestNotNull(TEXT("navigation ticket is retained"), BuiltTicket); if (!BuiltTicket) return false;
    if (BuiltTicket->State == TEXT("failed"))
    {
        FLedgerRecord* BeforePoll = FindLedger(TEXT("p14-nav-build"));
        TestTrue(TEXT("terminal failure finalizes ledger before status polling"), BeforePoll && BeforePoll->State == TEXT("failed") && BeforePoll->Response.Contains(TEXT("navigation_build_failed")));
        const FString TerminalResponse = BeforePoll ? BeforePoll->Response : FString();
        TSharedRef<FJsonObject> ViewArgs = MakeShared<FJsonObject>(); ViewArgs->SetStringField(TEXT("id"), TEXT("p14-nav-build"));
        const FString BeforePollView = ReadResponseOnGameThread(TEXT("p14-nav-operation-view"), TEXT("operation.view"), ViewArgs);
        TSharedPtr<FJsonObject> ParsedView; const TSharedPtr<FJsonObject>* ViewedReceipt = nullptr; const TSharedRef<TJsonReader<>> ViewReader = TJsonReaderFactory<>::Create(BeforePollView); const bool ViewFailed = FJsonSerializer::Deserialize(ViewReader, ParsedView) && ParsedView.IsValid() && ParsedView->TryGetObjectField(TEXT("result"), ViewedReceipt) && ViewedReceipt && ViewedReceipt->IsValid() && (*ViewedReceipt)->GetStringField(TEXT("state")) == TEXT("failed"); TestTrue(TEXT("operation.view exposes terminal failure before status polling"), ResponseStatusIsOk(BeforePollView) && ViewFailed);
        SetReceipt(TEXT("p14-nav-build"), TEXT("navigation.build"), Response, TEXT("completed"));
        FLedgerRecord* AfterStaleCompletion = FindLedger(TEXT("p14-nav-build"));
        TestTrue(TEXT("queue completion cannot overwrite terminal failure"), AfterStaleCompletion && AfterStaleCompletion->State == TEXT("failed") && AfterStaleCompletion->Response == TerminalResponse);
    }
    TSharedRef<FJsonObject> StatusArgs = MakeShared<FJsonObject>(); StatusArgs->SetStringField(TEXT("ticketId"), TicketId); const FString FirstStatus = P14Navigation(TEXT("p14-nav-status"), TEXT("navigation.status"), StatusArgs, FString()); const FString SecondStatus = P14Navigation(TEXT("p14-nav-status-repeat"), TEXT("navigation.status"), StatusArgs, FString()); TSharedPtr<FJsonObject> ParsedFirst, ParsedSecond; const TSharedRef<TJsonReader<>> FirstReader = TJsonReaderFactory<>::Create(FirstStatus); const TSharedRef<TJsonReader<>> SecondReader = TJsonReaderFactory<>::Create(SecondStatus); const TSharedPtr<FJsonObject>* FirstResult = nullptr; const TSharedPtr<FJsonObject>* SecondResult = nullptr; TestTrue(TEXT("terminal status responses parse"), FJsonSerializer::Deserialize(FirstReader, ParsedFirst) && FJsonSerializer::Deserialize(SecondReader, ParsedSecond) && ParsedFirst.IsValid() && ParsedSecond.IsValid() && ParsedFirst->TryGetObjectField(TEXT("result"), FirstResult) && ParsedSecond->TryGetObjectField(TEXT("result"), SecondResult) && FirstResult && SecondResult && FirstResult->IsValid() && SecondResult->IsValid()); if (!FirstResult || !SecondResult || !FirstResult->IsValid() || !SecondResult->IsValid()) return false; TestTrue(TEXT("navigation ticket is terminal after synchronous build"), (*FirstResult)->GetBoolField(TEXT("terminal"))); TestEqual(TEXT("terminal ticket state is immutable"), (*SecondResult)->GetStringField(TEXT("state")), (*FirstResult)->GetStringField(TEXT("state"))); TestEqual(TEXT("terminal ticket revision is immutable"), (*SecondResult)->GetStringField(TEXT("revision")), (*FirstResult)->GetStringField(TEXT("revision"))); if ((*FirstResult)->GetStringField(TEXT("state")) == TEXT("failed")) { FLedgerRecord* Record = FindLedger(TEXT("p14-nav-build")); TestTrue(TEXT("terminal failure replaces build ledger receipt"), Record && Record->State == TEXT("failed") && Record->Response.Contains(TEXT("navigation_build_failed"))); }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiUnrealAXIP14NavigationPathContracts, "MagiUnrealAXI.P14.NavigationPathContracts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiUnrealAXIP14NavigationPathContracts::RunTest(const FString&)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr; TestNotNull(TEXT("editor world exists"), World); if (!World || !World->GetOutermost()) return false;
    TSharedRef<FJsonObject> Args = MakeShared<FJsonObject>(); Args->SetStringField(TEXT("levelId"), World->GetOutermost()->GetName()); Args->SetArrayField(TEXT("start"), P14VectorJson(FVector::ZeroVector)); Args->SetArrayField(TEXT("target"), P14VectorJson(FVector(100.0f, 0.0f, 0.0f))); const FString Response = P14Navigation(TEXT("p14-nav-path"), TEXT("navigation.path_query"), Args, FString()); TestTrue(TEXT("synchronous path query returns real result"), ResponseStatusIsOk(Response));
    return true;
}
#endif


static AActor* P14RuntimeActorByStableId(UWorld& World, const FString& Wanted, bool& Ambiguous)
{
    Ambiguous = false; AActor* Found = nullptr;
    for (TActorIterator<AActor> It(&World); It; ++It)
    {
        AActor* Candidate = *It;
        if (!IsValid(Candidate) || !Candidate->GetActorGuid().IsValid() || ActorId(*Candidate) != Wanted) continue;
        if (Found) { Ambiguous = true; return nullptr; }
        Found = Candidate;
    }
    return Found;
}
static void P14Null(TSharedRef<FJsonObject> Object, const TCHAR* Field) { Object->SetField(Field, MakeShared<FJsonValueNull>()); }
static void P14Null(TSharedRef<FJsonObject>& Object, const TCHAR* Field) { Object->SetField(Field, MakeShared<FJsonValueNull>()); }

static FString P14AiObservation(const FString& Id, const FString& Operation, const TSharedPtr<FJsonObject>& Args)
{
    FString SessionId;
    FString PawnId;
    FString KeyName;
    if (!Args->TryGetStringField(TEXT("sessionId"), SessionId) || !Args->TryGetStringField(TEXT("pawnId"), PawnId) || !Args->TryGetStringField(TEXT("keyName"), KeyName) || SessionId.IsEmpty() || PawnId.IsEmpty() || KeyName.IsEmpty())
        return ErrorResponse(Id, TEXT("invalid_input"), TEXT("sessionId, pawnId, and keyName are required"));
    if (SessionId != PlaySessionId)
        return ErrorResponse(Id, TEXT("stale"), TEXT("sessionId does not identify the active play session"));
    UWorld* World = PieWorld();
    if (!World)
        return ErrorResponse(Id, TEXT("unsafe_editor_state"), TEXT("PIE world is not running"));

    bool Ambiguous = false;
    AActor* PawnActor = P14RuntimeActorByStableId(*World, PawnId, Ambiguous);
    if (Ambiguous) return ErrorResponse(Id, TEXT("conflict"), TEXT("pawnId resolves to multiple PIE actors"));
    APawn* Pawn = Cast<APawn>(PawnActor);
    if (!Pawn) return ErrorResponse(Id, TEXT("not_found"), TEXT("pawnId does not identify a PIE pawn"));
    AAIController* Controller = Cast<AAIController>(Pawn->GetController());
    if (!Controller) return ErrorResponse(Id, TEXT("conflict"), TEXT("pawn is not possessed by an AIController"));
    UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent();
    if (!Blackboard || !Blackboard->GetBlackboardAsset()) return ErrorResponse(Id, TEXT("conflict"), TEXT("AI controller has no Blackboard component"));

    const FName Key(*KeyName);
    const FBlackboardEntry* Entry = nullptr;
    for (const FBlackboardEntry& Candidate : Blackboard->GetBlackboardAsset()->Keys)
        if (Candidate.EntryName == Key) { Entry = &Candidate; break; }
    const UBlackboardKeyType_Object* ObjectType = Entry ? Cast<UBlackboardKeyType_Object>(Entry->KeyType) : nullptr;
    if (!Entry || !ObjectType || ObjectType->BaseClass != AActor::StaticClass()) return ErrorResponse(Id, TEXT("not_found"), TEXT("keyName is not an Actor Blackboard key"));

    const FString ControllerId = Controller->GetPathName();
    if (Operation == TEXT("play.ai_target_set"))
    {
        FString TargetId;
        if (!Args->TryGetStringField(TEXT("targetActorId"), TargetId) || TargetId.IsEmpty()) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("targetActorId is required"));
        bool TargetAmbiguous = false;
        AActor* Target = P14RuntimeActorByStableId(*World, TargetId, TargetAmbiguous);
        if (TargetAmbiguous) return ErrorResponse(Id, TEXT("conflict"), TEXT("targetActorId resolves to multiple PIE actors"));
        if (!Target) return ErrorResponse(Id, TEXT("not_found"), TEXT("targetActorId was not found in PIE world"));

        UObject* PriorValue = Blackboard->GetValueAsObject(Key);
        const bool Changed = PriorValue != Target;
        if (Changed) Blackboard->SetValueAsObject(Key, Target);
        const bool ReadbackMatches = Blackboard->GetValueAsObject(Key) == Target;
#if WITH_DEV_AUTOMATION_TESTS
        const bool ForcedFailure = GP11ForceAtomicFailure;
#else
        const bool ForcedFailure = false;
#endif
        if (!ReadbackMatches || ForcedFailure)
        {
            if (Changed) Blackboard->SetValueAsObject(Key, PriorValue);
            if (Changed && Blackboard->GetValueAsObject(Key) != PriorValue) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("Blackboard target readback failed and prior value could not be restored"));
            return ErrorResponse(Id, TEXT("operation_failed"), TEXT("Blackboard target readback failed"));
        }

        UBehaviorTreeComponent* Behavior = Cast<UBehaviorTreeComponent>(Controller->GetBrainComponent());
        bool Restarted = false;
        if (Behavior && (Changed || !Behavior->IsRunning()))
            if (UBehaviorTree* Tree = Behavior->GetCurrentTree()) Restarted = Controller->RunBehaviorTree(Tree);

        const FVector TargetLocation = Target->GetActorLocation();
        const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetStringField(TEXT("sessionId"), PlaySessionId);
        Result->SetStringField(TEXT("pawnId"), ActorId(*Pawn));
        Result->SetStringField(TEXT("controllerId"), ControllerId);
        Result->SetStringField(TEXT("keyName"), KeyName);
        Result->SetStringField(TEXT("targetActorId"), ActorId(*Target));
        Result->SetArrayField(TEXT("targetLocation"), P14VectorJson(TargetLocation));
        Result->SetBoolField(TEXT("changed"), Changed);
        Result->SetBoolField(TEXT("restarted"), Restarted);
        Result->SetStringField(TEXT("revision"), Sha256(CanonicalRow({PlaySessionId, ActorId(*Pawn), ControllerId, KeyName, ActorId(*Target), TargetLocation.ToString(), Changed ? TEXT("changed") : TEXT("same"), Restarted ? TEXT("restarted") : TEXT("not-restarted")})));
        return SuccessResponse(Id, Result, Operation, Args);
    }

    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("sessionId"), PlaySessionId);
    Result->SetStringField(TEXT("pawnId"), ActorId(*Pawn));
    Result->SetStringField(TEXT("controllerId"), ControllerId);
    Result->SetBoolField(TEXT("possessed"), Pawn->GetController() == Controller);
    Result->SetArrayField(TEXT("pawnLocation"), P14VectorJson(Pawn->GetActorLocation()));
    AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(Key));
    if (Target)
    {
        Result->SetStringField(TEXT("targetActorId"), ActorId(*Target));
        Result->SetArrayField(TEXT("targetLocation"), P14VectorJson(Target->GetActorLocation()));
        Result->SetNumberField(TEXT("distanceToTarget"), FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation()));
    }
    else
    {
        P14Null(Result, TEXT("targetActorId")); P14Null(Result, TEXT("targetLocation")); P14Null(Result, TEXT("distanceToTarget"));
    }

    UPathFollowingComponent* Path = Controller->GetPathFollowingComponent();
    const EPathFollowingStatus::Type PathStatus = Path ? Path->GetStatus() : EPathFollowingStatus::Idle;
    const bool Reached = Path && Path->DidMoveReachGoal();
    Result->SetStringField(TEXT("moveStatus"), Reached ? TEXT("reached") : PathStatus == EPathFollowingStatus::Moving ? TEXT("moving") : TEXT("idle"));
    if (Path)
    {
        if (AActor* MoveGoal = Path->GetMoveGoal()) Result->SetArrayField(TEXT("destination"), P14VectorJson(MoveGoal->GetActorLocation()));
        else if (Path->GetPath().IsValid() && Path->GetPath()->IsValid()) Result->SetArrayField(TEXT("destination"), P14VectorJson(Path->GetPathDestination()));
        else P14Null(Result, TEXT("destination"));
    }
    else P14Null(Result, TEXT("destination"));

    TArray<TSharedPtr<FJsonValue>> Values;
    const TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
    Row->SetStringField(TEXT("keyName"), KeyName); Row->SetStringField(TEXT("keyType"), TEXT("Actor"));
    if (Target) Row->SetStringField(TEXT("valueActorId"), ActorId(*Target)); else P14Null(Row, TEXT("valueActorId"));
    Values.Add(MakeShared<FJsonValueObject>(Row)); Result->SetArrayField(TEXT("blackboardValues"), Values);

    UBehaviorTreeComponent* Behavior = Cast<UBehaviorTreeComponent>(Controller->GetBrainComponent());
    UBehaviorTree* Tree = Behavior ? Behavior->GetCurrentTree() : nullptr;
    if (Tree) Result->SetStringField(TEXT("behaviorTreeId"), Tree->GetPathName()); else P14Null(Result, TEXT("behaviorTreeId"));
    Result->SetArrayField(TEXT("activeNodeIds"), {}); Result->SetArrayField(TEXT("completedNodeIds"), {});
    Result->SetStringField(TEXT("behavior"), Behavior && Behavior->IsRunning() ? TEXT("running") : TEXT("inactive"));
    Result->SetStringField(TEXT("revision"), Sha256(Serialize(Result)));
    return SuccessResponse(Id, Result, Operation, Args);
}
