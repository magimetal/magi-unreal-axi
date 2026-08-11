
#if WITH_DEV_AUTOMATION_TESTS
namespace MagiP15AnimationSpike
{
constexpr TCHAR BlueprintPackageName[] = TEXT("/Game/MagiP15Spike/ABP_MagiP15StateMachineSpike");
constexpr TCHAR BlueprintObjectPath[] = TEXT("/Game/MagiP15Spike/ABP_MagiP15StateMachineSpike.ABP_MagiP15StateMachineSpike");
constexpr TCHAR SkeletonPath[] = TEXT("/Game/MagiP15Seed/magi-p15-owned-seed/SkeletalMeshes/magi-p15-owned-seed_Skeleton.magi-p15-owned-seed_Skeleton");
constexpr TCHAR IdlePath[] = TEXT("/Game/MagiP15Seed/magi-p15-owned-seed/SkeletalMeshes/magi-p15-owned-seedIdle.magi-p15-owned-seedIdle");
constexpr TCHAR MovingPath[] = TEXT("/Game/MagiP15Seed/magi-p15-owned-seed/SkeletalMeshes/magi-p15-owned-seedMoving.magi-p15-owned-seedMoving");

FGuid StableGuid(const uint32 Index)
{
    return FGuid(0x4d414749, 0x50313500, 0x00000000, Index);
}

UEdGraphPin* PosePin(UEdGraphNode& Node, const EEdGraphPinDirection Direction)
{
    for (UEdGraphPin* Pin : Node.Pins)
    {
        if (Pin && Pin->Direction == Direction && UAnimationGraphSchema::IsLocalSpacePosePin(Pin->PinType)) return Pin;
    }
    return nullptr;
}

template <typename NodeType>
NodeType* OnlyNode(UEdGraph& Graph)
{
    TArray<NodeType*> Nodes;
    Graph.GetNodesOfClass(Nodes);
    return Nodes.Num() == 1 ? Nodes[0] : nullptr;
}

UAnimationGraph* FindAnimGraph(UAnimBlueprint& Blueprint)
{
    for (UEdGraph* Graph : Blueprint.FunctionGraphs)
    {
        if (Graph && Graph->GetSchema() && Graph->GetSchema()->GetClass()->IsChildOf(UAnimationGraphSchema::StaticClass()))
        {
            return Cast<UAnimationGraph>(Graph);
        }
    }
    return nullptr;
}

UAnimStateTransitionNode* FindTransition(UAnimationStateMachineGraph& Graph, UAnimStateNode& Previous, UAnimStateNode& Next)
{
    TArray<UAnimStateTransitionNode*> Transitions;
    Graph.GetNodesOfClass(Transitions);
    for (UAnimStateTransitionNode* Transition : Transitions)
    {
        if (Transition && Transition->GetPreviousState() == &Previous && Transition->GetNextState() == &Next) return Transition;
    }
    return nullptr;
}

bool SetTransitionRule(UAnimStateTransitionNode& Transition, const bool Value, const FGuid GraphGuid, const FGuid ResultGuid)
{
    UAnimationTransitionGraph* Graph = Cast<UAnimationTransitionGraph>(Transition.BoundGraph);
    UAnimGraphNode_TransitionResult* Result = Graph ? Graph->GetResultNode() : nullptr;
    UEdGraphPin* RulePin = Result ? Result->FindPin(TEXT("bCanEnterTransition"), EGPD_Input) : nullptr;
    if (!Graph || !Result || !RulePin) return false;
    Graph->GraphGuid = GraphGuid;
    Result->NodeGuid = ResultGuid;
    Graph->GetSchema()->TrySetDefaultValue(*RulePin, Value ? TEXT("true") : TEXT("false"));
    return RulePin->DefaultValue.ToBool() == Value;
}

bool VerifyTopology(FAutomationTestBase& Test, UAnimBlueprint& Blueprint, USkeleton& Skeleton, UAnimSequence& IdleSequence, UAnimSequence& MovingSequence)
{
    bool Valid = true;
    auto Require = [&](const TCHAR* Label, const bool Condition)
    {
        Test.TestTrue(Label, Condition);
        Valid &= Condition;
    };

    Require(TEXT("Anim Blueprint binds exact seed Skeleton"), Blueprint.TargetSkeleton == &Skeleton);
    UAnimationGraph* AnimGraph = FindAnimGraph(Blueprint);
    Require(TEXT("AnimGraph exists"), AnimGraph != nullptr);
    if (!AnimGraph) return false;
    Require(TEXT("AnimGraph identity persists"), AnimGraph->GraphGuid == StableGuid(1));

    UAnimGraphNode_Root* Root = OnlyNode<UAnimGraphNode_Root>(*AnimGraph);
    UAnimGraphNode_StateMachine* Machine = OnlyNode<UAnimGraphNode_StateMachine>(*AnimGraph);
    Require(TEXT("AnimGraph has one root"), Root != nullptr);
    Require(TEXT("AnimGraph has one state machine"), Machine != nullptr);
    if (!Root || !Machine) return false;
    Require(TEXT("root identity persists"), Root->NodeGuid == StableGuid(2));
    Require(TEXT("state-machine identity persists"), Machine->NodeGuid == StableGuid(3));
    Require(TEXT("machine output feeds AnimGraph root"), PosePin(*Machine, EGPD_Output) && PosePin(*Root, EGPD_Input) && PosePin(*Machine, EGPD_Output)->LinkedTo.Contains(PosePin(*Root, EGPD_Input)));

    UAnimationStateMachineGraph* MachineGraph = Machine->EditorStateMachineGraph;
    Require(TEXT("state-machine graph exists"), MachineGraph != nullptr);
    if (!MachineGraph) return false;
    Require(TEXT("state-machine graph has deterministic name"), MachineGraph->GetName() == TEXT("locomotion"));
    Require(TEXT("state-machine graph identity persists"), MachineGraph->GraphGuid == StableGuid(4));
    Require(TEXT("state-machine owner persists"), MachineGraph->OwnerAnimGraphNode == Machine);
    Require(TEXT("entry identity persists"), MachineGraph->EntryNode && MachineGraph->EntryNode->NodeGuid == StableGuid(5));

    TArray<UAnimStateNode*> States;
    MachineGraph->GetNodesOfClass(States);
    Require(TEXT("state machine has two states"), States.Num() == 2);
    UAnimStateNode* IdleState = nullptr;
    UAnimStateNode* MovingState = nullptr;
    for (UAnimStateNode* State : States)
    {
        if (State && State->GetStateName() == TEXT("idle")) IdleState = State;
        if (State && State->GetStateName() == TEXT("moving")) MovingState = State;
    }
    Require(TEXT("idle state persists"), IdleState != nullptr);
    Require(TEXT("moving state persists"), MovingState != nullptr);
    if (!IdleState || !MovingState) return false;
    Require(TEXT("idle identity persists"), IdleState->NodeGuid == StableGuid(6) && IdleState->BoundGraph && IdleState->BoundGraph->GraphGuid == StableGuid(7));
    Require(TEXT("moving identity persists"), MovingState->NodeGuid == StableGuid(10) && MovingState->BoundGraph && MovingState->BoundGraph->GraphGuid == StableGuid(11));
    Require(TEXT("entry targets idle"), MachineGraph->EntryNode && MachineGraph->EntryNode->GetOutputNode() == IdleState);

    UAnimationStateGraph* IdleGraph = Cast<UAnimationStateGraph>(IdleState->BoundGraph);
    UAnimationStateGraph* MovingGraph = Cast<UAnimationStateGraph>(MovingState->BoundGraph);
    UAnimGraphNode_SequencePlayer* IdlePlayer = IdleGraph ? OnlyNode<UAnimGraphNode_SequencePlayer>(*IdleGraph) : nullptr;
    UAnimGraphNode_SequencePlayer* MovingPlayer = MovingGraph ? OnlyNode<UAnimGraphNode_SequencePlayer>(*MovingGraph) : nullptr;
    UAnimGraphNode_StateResult* IdleResult = IdleGraph ? IdleGraph->GetResultNode() : nullptr;
    UAnimGraphNode_StateResult* MovingResult = MovingGraph ? MovingGraph->GetResultNode() : nullptr;
    Require(TEXT("idle sequence player persists"), IdlePlayer && IdlePlayer->Node.GetSequence() == &IdleSequence && IdlePlayer->NodeGuid == StableGuid(9));
    Require(TEXT("moving sequence player persists"), MovingPlayer && MovingPlayer->Node.GetSequence() == &MovingSequence && MovingPlayer->NodeGuid == StableGuid(13));
    Require(TEXT("idle result identity persists"), IdleResult && IdleResult->NodeGuid == StableGuid(8));
    Require(TEXT("moving result identity persists"), MovingResult && MovingResult->NodeGuid == StableGuid(12));
    Require(TEXT("idle pose topology persists"), IdlePlayer && IdleResult && PosePin(*IdlePlayer, EGPD_Output) && PosePin(*IdleResult, EGPD_Input) && PosePin(*IdlePlayer, EGPD_Output)->LinkedTo.Contains(PosePin(*IdleResult, EGPD_Input)));
    Require(TEXT("moving pose topology persists"), MovingPlayer && MovingResult && PosePin(*MovingPlayer, EGPD_Output) && PosePin(*MovingResult, EGPD_Input) && PosePin(*MovingPlayer, EGPD_Output)->LinkedTo.Contains(PosePin(*MovingResult, EGPD_Input)));

    UAnimStateTransitionNode* ToMoving = FindTransition(*MachineGraph, *IdleState, *MovingState);
    UAnimStateTransitionNode* ToIdle = FindTransition(*MachineGraph, *MovingState, *IdleState);
    TArray<UAnimStateTransitionNode*> Transitions;
    MachineGraph->GetNodesOfClass(Transitions);
    Require(TEXT("state machine has exactly two transitions"), Transitions.Num() == 2);
    Require(TEXT("idle-to-moving transition persists"), ToMoving != nullptr);
    Require(TEXT("moving-to-idle transition persists"), ToIdle != nullptr);
    if (!ToMoving || !ToIdle) return false;
    Require(TEXT("idle-to-moving identity persists"), ToMoving->NodeGuid == StableGuid(14));
    Require(TEXT("moving-to-idle identity persists"), ToIdle->NodeGuid == StableGuid(17));
    UAnimationTransitionGraph* ToMovingGraph = Cast<UAnimationTransitionGraph>(ToMoving->BoundGraph);
    UAnimationTransitionGraph* ToIdleGraph = Cast<UAnimationTransitionGraph>(ToIdle->BoundGraph);
    UAnimGraphNode_TransitionResult* ToMovingResult = ToMovingGraph ? ToMovingGraph->GetResultNode() : nullptr;
    UAnimGraphNode_TransitionResult* ToIdleResult = ToIdleGraph ? ToIdleGraph->GetResultNode() : nullptr;
    UEdGraphPin* ToMovingRule = ToMovingResult ? ToMovingResult->FindPin(TEXT("bCanEnterTransition"), EGPD_Input) : nullptr;
    UEdGraphPin* ToIdleRule = ToIdleResult ? ToIdleResult->FindPin(TEXT("bCanEnterTransition"), EGPD_Input) : nullptr;
    Require(TEXT("idle-to-moving rule persists"), ToMovingGraph && ToMovingGraph->GraphGuid == StableGuid(15) && ToMovingResult && ToMovingResult->NodeGuid == StableGuid(16) && ToMovingRule && ToMovingRule->LinkedTo.IsEmpty() && ToMovingRule->DefaultValue.ToBool());
    Require(TEXT("moving-to-idle rule persists"), ToIdleGraph && ToIdleGraph->GraphGuid == StableGuid(18) && ToIdleResult && ToIdleResult->NodeGuid == StableGuid(19) && ToIdleRule && ToIdleRule->LinkedTo.IsEmpty() && ToIdleRule->DefaultValue.ToBool());
    return Valid;
}

UAnimBlueprint* CreateBlueprint(USkeleton& Skeleton, UAnimSequence& IdleSequence, UAnimSequence& MovingSequence)
{
    UPackage* Package = CreatePackage(BlueprintPackageName);
    if (!Package) return nullptr;
    Package->FullyLoad();

    UAnimBlueprintFactory* Factory = NewObject<UAnimBlueprintFactory>();
    Factory->BlueprintType = BPTYPE_Normal;
    Factory->ParentClass = UAnimInstance::StaticClass();
    Factory->TargetSkeleton = &Skeleton;
    UAnimBlueprint* Blueprint = Cast<UAnimBlueprint>(Factory->FactoryCreateNew(UAnimBlueprint::StaticClass(), Package, TEXT("ABP_MagiP15StateMachineSpike"), RF_Public | RF_Standalone | RF_Transactional, nullptr, GWarn, TEXT("MagiP15AnimationSpike")));
    if (!Blueprint || Blueprint->TargetSkeleton != &Skeleton) return nullptr;
    FAssetRegistryModule::AssetCreated(Blueprint);

    UAnimationGraph* AnimGraph = FindAnimGraph(*Blueprint);
    UAnimGraphNode_Root* Root = AnimGraph ? OnlyNode<UAnimGraphNode_Root>(*AnimGraph) : nullptr;
    if (!AnimGraph || !Root) return nullptr;
    AnimGraph->GraphGuid = StableGuid(1);
    Root->NodeGuid = StableGuid(2);

    UAnimGraphNode_StateMachine* Machine = FEdGraphSchemaAction_K2NewNode::SpawnNode<UAnimGraphNode_StateMachine>(AnimGraph, FVector2D(-300.0, 0.0), EK2NewNodeFlags::None);
    if (!Machine || !Machine->EditorStateMachineGraph) return nullptr;
    Machine->NodeGuid = StableGuid(3);
    UAnimationStateMachineGraph* MachineGraph = Machine->EditorStateMachineGraph;
    FBlueprintEditorUtils::RenameGraph(MachineGraph, TEXT("locomotion"));
    MachineGraph->GraphGuid = StableGuid(4);
    if (!MachineGraph->EntryNode) return nullptr;
    MachineGraph->EntryNode->NodeGuid = StableGuid(5);

    UAnimStateNode* IdleState = FEdGraphSchemaAction_NewStateNode::SpawnNodeFromTemplate<UAnimStateNode>(MachineGraph, NewObject<UAnimStateNode>(), FVector2D(0.0, 0.0), false);
    UAnimStateNode* MovingState = FEdGraphSchemaAction_NewStateNode::SpawnNodeFromTemplate<UAnimStateNode>(MachineGraph, NewObject<UAnimStateNode>(), FVector2D(300.0, 0.0), false);
    if (!IdleState || !MovingState || !IdleState->BoundGraph || !MovingState->BoundGraph) return nullptr;
    FBlueprintEditorUtils::RenameGraph(IdleState->BoundGraph, TEXT("idle"));
    FBlueprintEditorUtils::RenameGraph(MovingState->BoundGraph, TEXT("moving"));
    IdleState->NodeGuid = StableGuid(6);
    IdleState->BoundGraph->GraphGuid = StableGuid(7);
    MovingState->NodeGuid = StableGuid(10);
    MovingState->BoundGraph->GraphGuid = StableGuid(11);

    UAnimationStateGraph* IdleGraph = Cast<UAnimationStateGraph>(IdleState->BoundGraph);
    UAnimationStateGraph* MovingGraph = Cast<UAnimationStateGraph>(MovingState->BoundGraph);
    UAnimGraphNode_StateResult* IdleResult = IdleGraph ? IdleGraph->GetResultNode() : nullptr;
    UAnimGraphNode_StateResult* MovingResult = MovingGraph ? MovingGraph->GetResultNode() : nullptr;
    if (!IdleResult || !MovingResult) return nullptr;
    IdleResult->NodeGuid = StableGuid(8);
    MovingResult->NodeGuid = StableGuid(12);

    UAnimGraphNode_SequencePlayer* IdlePlayer = FEdGraphSchemaAction_K2NewNode::SpawnNode<UAnimGraphNode_SequencePlayer>(IdleGraph, FVector2D(-300.0, 0.0), EK2NewNodeFlags::None, [&](UAnimGraphNode_SequencePlayer* Node) { Node->Node.SetSequence(&IdleSequence); });
    UAnimGraphNode_SequencePlayer* MovingPlayer = FEdGraphSchemaAction_K2NewNode::SpawnNode<UAnimGraphNode_SequencePlayer>(MovingGraph, FVector2D(-300.0, 0.0), EK2NewNodeFlags::None, [&](UAnimGraphNode_SequencePlayer* Node) { Node->Node.SetSequence(&MovingSequence); });
    if (!IdlePlayer || !MovingPlayer) return nullptr;
    IdlePlayer->NodeGuid = StableGuid(9);
    MovingPlayer->NodeGuid = StableGuid(13);
    if (!IdleGraph->GetSchema()->TryCreateConnection(PosePin(*IdlePlayer, EGPD_Output), PosePin(*IdleResult, EGPD_Input))) return nullptr;
    if (!MovingGraph->GetSchema()->TryCreateConnection(PosePin(*MovingPlayer, EGPD_Output), PosePin(*MovingResult, EGPD_Input))) return nullptr;

    if (!MachineGraph->GetSchema()->TryCreateConnection(MachineGraph->EntryNode->GetOutputPin(), IdleState->GetInputPin())) return nullptr;
    if (!MachineGraph->GetSchema()->TryCreateConnection(IdleState->GetOutputPin(), MovingState->GetInputPin())) return nullptr;
    if (!MachineGraph->GetSchema()->TryCreateConnection(MovingState->GetOutputPin(), IdleState->GetInputPin())) return nullptr;
    UAnimStateTransitionNode* ToMoving = FindTransition(*MachineGraph, *IdleState, *MovingState);
    UAnimStateTransitionNode* ToIdle = FindTransition(*MachineGraph, *MovingState, *IdleState);
    if (!ToMoving || !ToIdle) return nullptr;
    ToMoving->NodeGuid = StableGuid(14);
    ToIdle->NodeGuid = StableGuid(17);
    // ponytail: constant rules prove UE AnimGraph authoring only; public P1.5 must replace them with Speed comparisons.
    if (!SetTransitionRule(*ToMoving, true, StableGuid(15), StableGuid(16)) || !SetTransitionRule(*ToIdle, true, StableGuid(18), StableGuid(19))) return nullptr;

    if (!AnimGraph->GetSchema()->TryCreateConnection(PosePin(*Machine, EGPD_Output), PosePin(*Root, EGPD_Input))) return nullptr;
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    return Blueprint;
}
}
#endif


bool IsP15AnimationOperation(const FString& Operation)
{
    return Operation == TEXT("animation_blueprint.create") || Operation == TEXT("animation.graph_view");
}

static UAnimBlueprint* P15LoadAnimationBlueprint(const FString& Id)
{
    if (Id.IsEmpty()) return nullptr;
    UAnimBlueprint* Blueprint = Cast<UAnimBlueprint>(StaticLoadObject(UObject::StaticClass(), nullptr, *Id, nullptr, LOAD_NoWarn));
    return Blueprint && Blueprint->GetPathName() == Id ? Blueprint : nullptr;
}

static FString P15Guid(const FGuid& Guid)
{
    return Guid.ToString(EGuidFormats::DigitsWithHyphens).ToLower();
}

static FGuid P15DeterministicGuid(const FString& Identity)
{
    FGuid Guid;
    FGuid::ParseExact(Sha256(Identity).Left(32), EGuidFormats::Digits, Guid);
    return Guid;
}

template <typename NodeType>
static NodeType* P15OnlyNode(UEdGraph& Graph)
{
    TArray<NodeType*> Nodes;
    Graph.GetNodesOfClass(Nodes);
    return Nodes.Num() == 1 ? Nodes[0] : nullptr;
}

static UAnimationGraph* P15AnimGraph(UAnimBlueprint& Blueprint)
{
    UAnimationGraph* Match = nullptr;
    for (UEdGraph* Graph : Blueprint.FunctionGraphs)
    {
        if (!Graph || !Graph->GetSchema() || !Graph->GetSchema()->GetClass()->IsChildOf(UAnimationGraphSchema::StaticClass())) continue;
        if (Match) return nullptr;
        Match = Cast<UAnimationGraph>(Graph);
        if (!Match) return nullptr;
    }
    return Match;
}

static bool P15RootReadback(UAnimBlueprint& Blueprint, USkeleton*& Skeleton, UAnimationGraph*& Graph, UAnimGraphNode_Root*& Root)
{
    Skeleton = Blueprint.TargetSkeleton;
    Graph = P15AnimGraph(Blueprint);
    Root = Graph ? P15OnlyNode<UAnimGraphNode_Root>(*Graph) : nullptr;
    if (!Skeleton || !Graph || !Root || !Blueprint.GeneratedClass || Blueprint.Status != BS_UpToDate ||
        !Blueprint.NewVariables.IsEmpty() || Graph->Nodes.Num() != 1 ||
        Graph->GraphGuid != P15DeterministicGuid(Blueprint.GetPathName() + TEXT("#anim-graph")) ||
        Root->NodeGuid != P15DeterministicGuid(Blueprint.GetPathName() + TEXT("#anim-root"))) return false;
    for (const UEdGraphPin* Pin : Root->Pins) if (Pin && !Pin->LinkedTo.IsEmpty()) return false;
    return true;
}

static TSharedRef<FJsonObject> P15AnimationResult(UAnimBlueprint& Blueprint, USkeleton& Skeleton, UAnimationGraph& Graph, UAnimGraphNode_Root& Root, const bool Changed, const bool GraphView)
{
    const FString BlueprintId = Blueprint.GetPathName();
    const FString GraphId = BlueprintId + TEXT("#graph:other:") + P15Guid(Graph.GraphGuid);
    const FString RootId = GraphId + TEXT("#node:") + P15Guid(Root.NodeGuid);
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("animationBlueprintId"), BlueprintId);
    Result->SetStringField(TEXT("skeletonId"), Skeleton.GetPathName());
    Result->SetStringField(TEXT("generatedClass"), Blueprint.GeneratedClass->GetPathName());
    Result->SetStringField(TEXT("animGraphId"), GraphId);
    Result->SetStringField(TEXT("rootNodeId"), RootId);
    if (GraphView)
    {
        Result->SetArrayField(TEXT("variables"), {});
        Result->SetArrayField(TEXT("stateMachines"), {});
    }
    else
    {
        Result->SetBoolField(TEXT("changed"), Changed);
        Result->SetArrayField(TEXT("dirtyPackages"), Blueprint.GetOutermost()->IsDirty() ? TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueString>(Blueprint.GetOutermost()->GetName())} : TArray<TSharedPtr<FJsonValue>>{});
        Result->SetArrayField(TEXT("savedPackages"), {});
    }
    Result->SetStringField(TEXT("revision"), BlueprintContentRevision(Blueprint));
    return Result;
}

FString HandleP15AnimationOperation(const FString& Id, const FString& Operation, const TSharedPtr<FJsonObject>& Args, const FString& ExpectedRevision)
{
    (void)ExpectedRevision; // create has absent-revision semantics; graph_view is read-only.
    if (!Args.IsValid()) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("animation operation requires arguments"));
    if (Operation == TEXT("animation.graph_view"))
    {
        FString BlueprintId;
        if (!Args->TryGetStringField(TEXT("animationBlueprintId"), BlueprintId)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("animationBlueprintId is required"));
        UAnimBlueprint* Blueprint = P15LoadAnimationBlueprint(BlueprintId);
        if (!Blueprint) return ErrorResponse(Id, TEXT("not_found"), TEXT("Animation Blueprint was not found"));
        USkeleton* Skeleton = nullptr; UAnimationGraph* Graph = nullptr; UAnimGraphNode_Root* Root = nullptr;
        if (!P15RootReadback(*Blueprint, Skeleton, Graph, Root)) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("Animation Blueprint root readback failed"));
        return SuccessResponse(Id, P15AnimationResult(*Blueprint, *Skeleton, *Graph, *Root, false, true), Operation, Args);
    }
    if (Operation != TEXT("animation_blueprint.create")) return ErrorResponse(Id, TEXT("unsupported"), TEXT("P1.5 operation is not available in this native build"));
    FString Path, SkeletonId, BlueprintId;
    if (!Args->TryGetStringField(TEXT("path"), Path) || !Args->TryGetStringField(TEXT("skeletonId"), SkeletonId) || !P14AssetPath(Path, BlueprintId)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("path and skeletonId are required; path must be a /Game package path"));
    USkeleton* Skeleton = Cast<USkeleton>(StaticLoadObject(UObject::StaticClass(), nullptr, *SkeletonId));
    if (!Skeleton || Skeleton->GetPathName() != SkeletonId) return ErrorResponse(Id, TEXT("not_found"), TEXT("exact Skeleton was not found"));
    if (UObject* Existing = StaticLoadObject(UObject::StaticClass(), nullptr, *BlueprintId, nullptr, LOAD_NoWarn))
    {
        UAnimBlueprint* Blueprint = Cast<UAnimBlueprint>(Existing);
        if (!Blueprint) return ErrorResponse(Id, TEXT("conflict"), TEXT("existing asset is not an Animation Blueprint"));
        USkeleton* ExistingSkeleton = nullptr; UAnimationGraph* Graph = nullptr; UAnimGraphNode_Root* Root = nullptr;
        if (Blueprint->TargetSkeleton != Skeleton || !P15RootReadback(*Blueprint, ExistingSkeleton, Graph, Root)) return ErrorResponse(Id, TEXT("conflict"), TEXT("existing Animation Blueprint does not match requested Skeleton or root contract"));
        return SuccessResponse(Id, P15AnimationResult(*Blueprint, *Skeleton, *Graph, *Root, false, false), Operation, Args);
    }
    if (FPackageName::DoesPackageExist(Path)) return ErrorResponse(Id, TEXT("conflict"), TEXT("Animation Blueprint package already exists with incompatible content"));
    const FString AbsentRevision = Sha256(BlueprintId + TEXT("\nabsent"));
    auto CreatedAssetAbsent = [&]()
    {
        return !FPackageName::DoesPackageExist(Path) && !FindPackage(nullptr, *Path) &&
            !FindObject<UObject>(nullptr, *BlueprintId) &&
            !FAssetRegistryModule::GetRegistry().GetAssetByObjectPath(FSoftObjectPath(BlueprintId)).IsValid();
    };
    FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P15AnimationBlueprintCreate", "Magi AXI Create Animation Blueprint"));
    UPackage* Package = CreatePackage(*Path);
    UAnimBlueprintFactory* Factory = NewObject<UAnimBlueprintFactory>();
    Factory->BlueprintType = BPTYPE_Normal; Factory->ParentClass = UAnimInstance::StaticClass(); Factory->TargetSkeleton = Skeleton;
    UAnimBlueprint* Blueprint = Package ? Cast<UAnimBlueprint>(Factory->FactoryCreateNew(UAnimBlueprint::StaticClass(), Package, *FPackageName::GetShortName(Path), RF_Public | RF_Standalone | RF_Transactional, nullptr, GWarn, TEXT("MagiP15AnimationCreate"))) : nullptr;
    bool AssetRegistered = false;
    auto FailedCreate = [&](const TCHAR* Message)
    {
        Transaction.Cancel();
        if (Package) P11DiscardCreatedBlueprint(*Package, Blueprint, AssetRegistered);
        const bool Absent = CreatedAssetAbsent();
        FString ObservedRevision = AbsentRevision;
        if (!Absent)
        {
            if (UAnimBlueprint* Observed = P15LoadAnimationBlueprint(BlueprintId)) ObservedRevision = BlueprintContentRevision(*Observed);
            else ObservedRevision = Sha256(BlueprintId + (FindPackage(nullptr, *Path) ? TEXT("\npackage-present") : TEXT("\nregistry-present")));
        }
        TArray<TSharedPtr<FJsonValue>> DirtyPackages;
        if (UPackage* ObservedPackage = FindPackage(nullptr, *Path); ObservedPackage && ObservedPackage->IsDirty()) DirtyPackages.Add(MakeShared<FJsonValueString>(Path));
        return P11FailedAtomicResponse(Id, Operation, Args, BlueprintId, AbsentRevision, ObservedRevision, Absent, DirtyPackages, Absent ? Message : TEXT("Animation Blueprint creation rollback verification failed"));
    };
    if (!Package || !Blueprint || Blueprint->GetPathName() != BlueprintId || Blueprint->TargetSkeleton != Skeleton)
        return FailedCreate(TEXT("failed to create exact Skeleton-bound Animation Blueprint"));
    FAssetRegistryModule::AssetCreated(Blueprint); AssetRegistered = true;
    UAnimationGraph* Graph = P15AnimGraph(*Blueprint); UAnimGraphNode_Root* Root = Graph ? P15OnlyNode<UAnimGraphNode_Root>(*Graph) : nullptr;
    if (!Graph || !Root) return FailedCreate(TEXT("Animation Blueprint root graph is unavailable"));
    Graph->GraphGuid = P15DeterministicGuid(BlueprintId + TEXT("#anim-graph"));
    Root->NodeGuid = P15DeterministicGuid(BlueprintId + TEXT("#anim-root"));
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FCompilerResultsLog Results; Results.bSilentMode = true;
    FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave, &Results);
    USkeleton* ReadbackSkeleton = nullptr; UAnimationGraph* ReadbackGraph = nullptr; UAnimGraphNode_Root* ReadbackRoot = nullptr;
    if (Results.NumErrors != 0 || Results.NumWarnings != 0 || Blueprint->Status != BS_UpToDate || !P15RootReadback(*Blueprint, ReadbackSkeleton, ReadbackGraph, ReadbackRoot) || ReadbackSkeleton != Skeleton || ReadbackGraph != Graph || ReadbackRoot != Root)
        return FailedCreate(TEXT("Animation Blueprint compile or atomic readback failed"));
    Package->MarkPackageDirty();
    const FString Response = P11AtomicResponse(Id, SuccessResponse(Id, P15AnimationResult(*Blueprint, *Skeleton, *Graph, *Root, true, false), Operation, Args));
    if (ResponseStatusIsOk(Response)) return Response;
    TSharedPtr<FJsonObject> ResponseObject; const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response); const TSharedPtr<FJsonObject>* Error = nullptr; FString FailureMessage;
    if (FJsonSerializer::Deserialize(Reader, ResponseObject) && ResponseObject && ResponseObject->TryGetObjectField(TEXT("error"), Error) && Error) (*Error)->TryGetStringField(TEXT("message"), FailureMessage);
    return FailedCreate(FailureMessage.IsEmpty() ? TEXT("Animation Blueprint creation postcondition failed") : *FailureMessage);
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiP15PublicCreateGraphViewPersistence, "MagiUnrealAXI.P15.PublicCreateGraphViewPersistence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiP15PublicCreateGraphViewPersistence::RunTest(const FString&)
{
    using namespace MagiP15AnimationSpike;
    constexpr TCHAR PackagePath[] = TEXT("/Game/MagiP15Public/ABP_MagiP15Public");
    constexpr TCHAR BlueprintId[] = TEXT("/Game/MagiP15Public/ABP_MagiP15Public.ABP_MagiP15Public");
    constexpr TCHAR RollbackPath[] = TEXT("/Game/MagiP15Public/ABP_MagiP15Rollback");
    constexpr TCHAR RollbackId[] = TEXT("/Game/MagiP15Public/ABP_MagiP15Rollback.ABP_MagiP15Rollback");
    auto Envelope = [](const FString& Response)
    {
        TSharedPtr<FJsonObject> Value;
        const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response);
        FJsonSerializer::Deserialize(Reader, Value);
        return Value;
    };
    auto Result = [&](const FString& Response)
    {
        const TSharedPtr<FJsonObject> ResponseObject = Envelope(Response);
        const TSharedPtr<FJsonObject>* Value = nullptr;
        return ResponseObject && ResponseObject->TryGetObjectField(TEXT("result"), Value) && Value ? *Value : TSharedPtr<FJsonObject>();
    };
    auto ReceiptFor = [&](const FString& Response)
    {
        const TSharedPtr<FJsonObject> ResponseObject = Envelope(Response);
        const TSharedPtr<FJsonObject>* Value = nullptr;
        return ResponseObject && ResponseObject->TryGetObjectField(TEXT("receipt"), Value) && Value ? *Value : TSharedPtr<FJsonObject>();
    };
    USkeleton* Skeleton = LoadObject<USkeleton>(nullptr, SkeletonPath);
    TestNotNull(TEXT("owned seed Skeleton loads for public create"), Skeleton);
    if (!Skeleton) return false;
    FlushAsyncLoading();
    FAssetCompilingManager::Get().FinishAllCompilation();
    const TSharedPtr<FJsonObject> DescribeResult = Result(ReadResponseOnGameThread(TEXT("p15-public-describe"), TEXT("bridge.describe"), MakeShared<FJsonObject>()));
    const TArray<TSharedPtr<FJsonValue>>* NativeOperations = nullptr;
    int32 P15Operations = 0, AvailableOperations = 0, PendingOperations = 0;
    if (DescribeResult) DescribeResult->TryGetArrayField(TEXT("nativeOperations"), NativeOperations);
    if (NativeOperations) for (const TSharedPtr<FJsonValue>& Value : *NativeOperations)
    {
        const TSharedPtr<FJsonObject> Entry = Value ? Value->AsObject() : nullptr;
        FString NativeOperation;
        if (!Entry || !Entry->TryGetStringField(TEXT("operation"), NativeOperation) ||
            (NativeOperation != TEXT("animation_blueprint.create") && !NativeOperation.StartsWith(TEXT("animation.")) && NativeOperation != TEXT("play.animation_observe"))) continue;
        ++P15Operations;
        if (IsP15AnimationOperation(NativeOperation))
        {
            if (Entry->GetStringField(TEXT("availability")) == TEXT("available")) ++AvailableOperations;
            continue;
        }
        const TArray<TSharedPtr<FJsonValue>>* Reasons = nullptr;
        Entry->TryGetArrayField(TEXT("reasons"), Reasons);
        if (Entry->GetStringField(TEXT("availability")) == TEXT("unavailable") && Reasons && Reasons->ContainsByPredicate([](const TSharedPtr<FJsonValue>& Reason)
            { const TSharedPtr<FJsonObject> Object = Reason ? Reason->AsObject() : nullptr; return Object && Object->GetStringField(TEXT("code")) == TEXT("implementation_pending"); })) ++PendingOperations;
    }
    TestTrue(TEXT("bridge advertises only implemented P1.5 slice as available"), P15Operations == 9 && AvailableOperations == 2 && PendingOperations == 7);

    if (!FPackageName::DoesPackageExist(PackagePath))
    {
        const TSharedRef<FJsonObject> RollbackArgs = MakeShared<FJsonObject>();
        RollbackArgs->SetStringField(TEXT("path"), RollbackPath); RollbackArgs->SetStringField(TEXT("skeletonId"), SkeletonPath);
        GP11ForceAtomicFailure = true;
        const FString RollbackResponse = ReadResponseOnGameThread(TEXT("p15-public-create-rollback"), TEXT("animation_blueprint.create"), RollbackArgs);
        GP11ForceAtomicFailure = false;
        const TSharedPtr<FJsonObject> RollbackEnvelope = Envelope(RollbackResponse); const TSharedPtr<FJsonObject>* Receipt = nullptr;
        if (!RollbackEnvelope || !RollbackEnvelope->HasField(TEXT("receipt"))) AddInfo(FString(TEXT("rollback response: ")) + RollbackResponse);
        TestFalse(TEXT("injected public create failure returns error"), ResponseStatusIsOk(RollbackResponse));
        TestTrue(TEXT("public create failure emits verified failed receipt"), RollbackEnvelope && RollbackEnvelope->TryGetObjectField(TEXT("receipt"), Receipt) && Receipt && (*Receipt)->GetStringField(TEXT("state")) == TEXT("failed"));
        TestTrue(TEXT("public create failure removes object, package, disk, and registry evidence"), !FPackageName::DoesPackageExist(RollbackPath) && !FindPackage(nullptr, RollbackPath) && !FindObject<UObject>(nullptr, RollbackId) && !FAssetRegistryModule::GetRegistry().GetAssetByObjectPath(FSoftObjectPath(RollbackId)).IsValid());
    }

    const bool PersistedBeforeRun = FPackageName::DoesPackageExist(PackagePath);
    const FString OraclePath = FPaths::ProjectSavedDir() / TEXT("Automation/MagiP15PublicCreateGraphViewOracle.json");
    TSharedPtr<FJsonObject> PersistedOracle;
    if (PersistedBeforeRun)
    {
        FString OracleText;
        TestTrue(TEXT("first-session identity oracle exists after restart"), FFileHelper::LoadFileToString(OracleText, *OraclePath));
        if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(OracleText), PersistedOracle) || !PersistedOracle)
        {
            AddError(TEXT("first-session identity oracle is unreadable"));
            return false;
        }
    }
    const TSharedRef<FJsonObject> CreateArgs = MakeShared<FJsonObject>();
    CreateArgs->SetStringField(TEXT("path"), PackagePath); CreateArgs->SetStringField(TEXT("skeletonId"), SkeletonPath);
    const FString CreateResponse = ReadResponseOnGameThread(TEXT("p15-public-create"), TEXT("animation_blueprint.create"), CreateArgs);
    const TSharedPtr<FJsonObject> CreateResult = Result(CreateResponse);
    if (!ResponseStatusIsOk(CreateResponse)) AddInfo(FString(TEXT("create response: ")) + CreateResponse);
    TestTrue(PersistedBeforeRun ? TEXT("persisted public Animation Blueprint reloads through create no-op") : TEXT("public Animation Blueprint create succeeds"), ResponseStatusIsOk(CreateResponse));
    TestTrue(TEXT("create reports exact changed state"), CreateResult && CreateResult->GetBoolField(TEXT("changed")) == !PersistedBeforeRun);
    if (!CreateResult) return false;
    const FString StableRevision = CreateResult->GetStringField(TEXT("revision"));
    TestEqual(TEXT("create returns canonical Animation Blueprint identity"), CreateResult->GetStringField(TEXT("animationBlueprintId")), FString(BlueprintId));
    TestEqual(TEXT("create returns exact Skeleton identity"), CreateResult->GetStringField(TEXT("skeletonId")), FString(SkeletonPath));
    const FString StableGeneratedClass = CreateResult->GetStringField(TEXT("generatedClass"));
    const FString StableAnimGraphId = CreateResult->GetStringField(TEXT("animGraphId"));
    const FString StableRootNodeId = CreateResult->GetStringField(TEXT("rootNodeId"));
    const FString ExpectedGraphId = FString(BlueprintId) + TEXT("#graph:other:") + P15Guid(P15DeterministicGuid(FString(BlueprintId) + TEXT("#anim-graph")));
    TestEqual(TEXT("create returns exact generated class identity"), StableGeneratedClass, FString(BlueprintId) + TEXT("_C"));
    TestEqual(TEXT("create returns deterministic AnimGraph identity"), StableAnimGraphId, ExpectedGraphId);
    TestEqual(TEXT("create returns deterministic root identity"), StableRootNodeId, ExpectedGraphId + TEXT("#node:") + P15Guid(P15DeterministicGuid(FString(BlueprintId) + TEXT("#anim-root"))));
    TestTrue(TEXT("create dirtyPackages reflects actual package state"), CreateResult->GetArrayField(TEXT("dirtyPackages")).Num() == (PersistedBeforeRun ? 0 : 1));
    const TSharedPtr<FJsonObject> CreateReceipt = ReceiptFor(CreateResponse);
    TestTrue(TEXT("create receipt persistence reflects actual package state"), CreateReceipt && CreateReceipt->GetStringField(TEXT("persistence")) == (PersistedBeforeRun ? TEXT("unchanged") : TEXT("dirty")));
    if (PersistedOracle)
    {
        TestEqual(TEXT("restart preserves revision from first-session oracle"), StableRevision, PersistedOracle->GetStringField(TEXT("revision")));
        TestEqual(TEXT("restart preserves generated class from first-session oracle"), StableGeneratedClass, PersistedOracle->GetStringField(TEXT("generatedClass")));
        TestEqual(TEXT("restart preserves AnimGraph identity from first-session oracle"), StableAnimGraphId, PersistedOracle->GetStringField(TEXT("animGraphId")));
        TestEqual(TEXT("restart preserves root identity from first-session oracle"), StableRootNodeId, PersistedOracle->GetStringField(TEXT("rootNodeId")));
    }

    const TSharedRef<FJsonObject> ViewArgs = MakeShared<FJsonObject>(); ViewArgs->SetStringField(TEXT("animationBlueprintId"), BlueprintId);
    const FString ViewResponse = ReadResponseOnGameThread(TEXT("p15-public-view"), TEXT("animation.graph_view"), ViewArgs);
    const TSharedPtr<FJsonObject> ViewResult = Result(ViewResponse);
    TestTrue(TEXT("public graph view succeeds"), ResponseStatusIsOk(ViewResponse));
    TestTrue(TEXT("public graph view binds stable empty-root contract"), ViewResult && ViewResult->GetStringField(TEXT("revision")) == StableRevision && ViewResult->GetStringField(TEXT("generatedClass")) == StableGeneratedClass && ViewResult->GetStringField(TEXT("animGraphId")) == StableAnimGraphId && ViewResult->GetStringField(TEXT("rootNodeId")) == StableRootNodeId && ViewResult->GetArrayField(TEXT("variables")).IsEmpty() && ViewResult->GetArrayField(TEXT("stateMachines")).IsEmpty());
    if (!ResponseStatusIsOk(ViewResponse)) AddInfo(FString(TEXT("view response: ")) + ViewResponse);
    FlushAsyncLoading();
    FAssetCompilingManager::Get().FinishAllCompilation();

    const FString RepeatResponse = ReadResponseOnGameThread(TEXT("p15-public-create-repeat"), TEXT("animation_blueprint.create"), CreateArgs);
    const TSharedPtr<FJsonObject> RepeatResult = Result(RepeatResponse);
    TestTrue(TEXT("public create repeat succeeds as exact no-op"), ResponseStatusIsOk(RepeatResponse));
    TestTrue(TEXT("public create repeat preserves exact contract and reports unchanged"), RepeatResult && !RepeatResult->GetBoolField(TEXT("changed")) && RepeatResult->GetStringField(TEXT("revision")) == StableRevision && RepeatResult->GetStringField(TEXT("generatedClass")) == StableGeneratedClass && RepeatResult->GetStringField(TEXT("animGraphId")) == StableAnimGraphId && RepeatResult->GetStringField(TEXT("rootNodeId")) == StableRootNodeId);
    TestTrue(TEXT("repeat dirtyPackages reflects actual package state"), RepeatResult && RepeatResult->GetArrayField(TEXT("dirtyPackages")).Num() == (PersistedBeforeRun ? 0 : 1));
    const TSharedPtr<FJsonObject> RepeatReceipt = ReceiptFor(RepeatResponse);
    TestTrue(TEXT("repeat receipt persistence reflects actual package state"), RepeatReceipt && RepeatReceipt->GetStringField(TEXT("persistence")) == (PersistedBeforeRun ? TEXT("unchanged") : TEXT("dirty")));

    if (!ResponseStatusIsOk(RepeatResponse)) AddInfo(FString(TEXT("repeat response: ")) + RepeatResponse);
    UAnimBlueprint* Blueprint = P15LoadAnimationBlueprint(BlueprintId);
    TestNotNull(TEXT("public Animation Blueprint loads for persistence save"), Blueprint);
    if (!Blueprint) return false;
    const FString Filename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs; SaveArgs.TopLevelFlags = RF_Public | RF_Standalone; SaveArgs.SaveFlags = SAVE_NoError;
    TestTrue(TEXT("public Animation Blueprint saves for restart readback"), UPackage::SavePackage(Blueprint->GetOutermost(), Blueprint, *Filename, SaveArgs));
    TestEqual(TEXT("save preserves deterministic public revision"), BlueprintContentRevision(*Blueprint), StableRevision);
    TestFalse(TEXT("saved public Animation Blueprint package is clean"), Blueprint->GetOutermost()->IsDirty());
    if (!PersistedBeforeRun)
    {
        const TSharedRef<FJsonObject> Oracle = MakeShared<FJsonObject>();
        Oracle->SetStringField(TEXT("revision"), StableRevision);
        Oracle->SetStringField(TEXT("generatedClass"), StableGeneratedClass);
        Oracle->SetStringField(TEXT("animGraphId"), StableAnimGraphId);
        Oracle->SetStringField(TEXT("rootNodeId"), StableRootNodeId);
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(OraclePath), true);
        TestTrue(TEXT("first session persists external restart oracle"), FFileHelper::SaveStringToFile(Serialize(Oracle), *OraclePath));
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiP15AnimationAuthoringSpike, "MagiUnrealAXI.P15.AnimationAuthoringSpike", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiP15AnimationAuthoringSpike::RunTest(const FString&)
{
    using namespace MagiP15AnimationSpike;
    USkeleton* Skeleton = LoadObject<USkeleton>(nullptr, SkeletonPath);
    UAnimSequence* IdleSequence = LoadObject<UAnimSequence>(nullptr, IdlePath);
    UAnimSequence* MovingSequence = LoadObject<UAnimSequence>(nullptr, MovingPath);
    TestNotNull(TEXT("owned seed Skeleton loads"), Skeleton);
    TestNotNull(TEXT("owned idle sequence loads"), IdleSequence);
    TestNotNull(TEXT("owned moving sequence loads"), MovingSequence);
    if (!Skeleton || !IdleSequence || !MovingSequence) return false;
    TestTrue(TEXT("idle uses exact seed Skeleton"), IdleSequence->GetSkeleton() == Skeleton);
    TestTrue(TEXT("moving uses exact seed Skeleton"), MovingSequence->GetSkeleton() == Skeleton);
    if (IdleSequence->GetSkeleton() != Skeleton || MovingSequence->GetSkeleton() != Skeleton) return false;

    const bool PersistedBeforeRun = FPackageName::DoesPackageExist(BlueprintPackageName);
    UAnimBlueprint* Blueprint = PersistedBeforeRun ? LoadObject<UAnimBlueprint>(nullptr, BlueprintObjectPath) : CreateBlueprint(*Skeleton, *IdleSequence, *MovingSequence);
    TestNotNull(PersistedBeforeRun ? TEXT("persisted Anim Blueprint reloads") : TEXT("Anim Blueprint is authored"), Blueprint);
    if (!Blueprint) return false;
    if (PersistedBeforeRun) TestTrue(TEXT("persisted topology reads back before compile"), VerifyTopology(*this, *Blueprint, *Skeleton, *IdleSequence, *MovingSequence));

    FCompilerResultsLog Results;
    Results.bSilentMode = true;
    FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &Results);
    TestEqual(TEXT("Anim Blueprint compiles without errors"), Results.NumErrors, 0);
    TestEqual(TEXT("Anim Blueprint compiles without warnings"), Results.NumWarnings, 0);
    TestEqual(TEXT("Anim Blueprint status is up to date"), Blueprint->Status, BS_UpToDate);
    TestTrue(TEXT("authored topology reads back after compile"), VerifyTopology(*this, *Blueprint, *Skeleton, *IdleSequence, *MovingSequence));
    if (Results.NumErrors != 0 || Results.NumWarnings != 0 || Blueprint->Status != BS_UpToDate) return false;

    UPackage* Package = Blueprint->GetOutermost();
    const FString Filename = FPackageName::LongPackageNameToFilename(BlueprintPackageName, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    TestTrue(TEXT("Anim Blueprint package saves for restart readback"), UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs));
    TestTrue(TEXT("saved Anim Blueprint package exists"), FPackageName::DoesPackageExist(BlueprintPackageName));
    return !HasAnyErrors();
}

#endif
