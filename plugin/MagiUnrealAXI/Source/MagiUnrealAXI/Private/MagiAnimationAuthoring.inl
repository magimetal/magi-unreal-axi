
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
    return Operation == TEXT("animation_blueprint.create") || Operation == TEXT("animation.character_configure") ||
        Operation == TEXT("animation.character_view") || Operation == TEXT("animation.graph_view") ||
        Operation == TEXT("animation.variable_ensure") || Operation == TEXT("animation.state_machine_ensure") ||
        Operation == TEXT("animation.state_ensure") || Operation == TEXT("animation.transition_ensure") ||
        Operation == TEXT("play.animation_observe");
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
    if (!Skeleton || !Graph || !Root || !Blueprint.GeneratedClass || (Blueprint.Status != BS_UpToDate && Blueprint.Status != BS_UpToDateWithWarnings) ||
        Graph->GraphGuid != P15DeterministicGuid(Blueprint.GetPathName() + TEXT("#anim-graph")) ||
        Root->NodeGuid != P15DeterministicGuid(Blueprint.GetPathName() + TEXT("#anim-root"))) return false;
    for (const UEdGraphPin* Pin : Root->Pins) if (Pin && Pin->Direction == EGPD_Input && Pin->LinkedTo.Num() > 1) return false;
    return true;
}

static TArray<TSharedPtr<FJsonValue>> P15Dirty(UAnimBlueprint& Blueprint, bool Changed);
static UEdGraph* P15UpdateGraph(UAnimBlueprint& Blueprint);
static bool P15VariableReadback(UAnimBlueprint& Blueprint, UEdGraph& Graph, TArray<UEdGraphNode*>& Nodes);
static bool P15SemanticGraphReadback(UAnimBlueprint& Blueprint, TArray<TSharedPtr<FJsonValue>>& Variables, TArray<TSharedPtr<FJsonValue>>& Machines);
static bool P15TransitionReadback(UAnimBlueprint& Blueprint, UAnimStateTransitionNode& Transition, const FString& Expression);
static bool P15CompleteLocomotionReadback(UAnimBlueprint& Blueprint, TSharedPtr<FJsonObject>& Machine);
static FString P15Identity(const FString& BlueprintId, const FString& Kind, const FString& Key);
static TSharedRef<FJsonObject> P15AnimationResult(UAnimBlueprint& Blueprint, USkeleton& Skeleton, UAnimationGraph& Graph, UAnimGraphNode_Root& Root, const bool Changed, const bool GraphView, bool* ReadbackValid = nullptr)
{
    if (ReadbackValid) *ReadbackValid = true;
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
        TArray<TSharedPtr<FJsonValue>> GraphVariables;
        TArray<TSharedPtr<FJsonValue>> GraphMachines;
        if (ReadbackValid) *ReadbackValid = P15SemanticGraphReadback(Blueprint, GraphVariables, GraphMachines);
        Result->SetArrayField(TEXT("variables"), GraphVariables);
        Result->SetArrayField(TEXT("stateMachines"), GraphMachines);
    }
    else
    {
        Result->SetBoolField(TEXT("changed"), Changed);
        Result->SetArrayField(TEXT("dirtyPackages"), P15Dirty(Blueprint, Changed));
        Result->SetArrayField(TEXT("savedPackages"), {});
    }
    Result->SetStringField(TEXT("revision"), BlueprintContentRevision(Blueprint));
    return Result;
}

static FString P15Expected(const FString& ExpectedRevision, UAnimBlueprint& Blueprint)
{
    if (ExpectedRevision.IsEmpty()) return TEXT("expectedRevision is required");
    return ExpectedRevision == BlueprintContentRevision(Blueprint) ? FString() : TEXT("Blueprint revision is stale; re-read animation.graph_view before retrying");
}

static TArray<TSharedPtr<FJsonValue>> P15Dirty(UAnimBlueprint& Blueprint, bool)
{
    return Blueprint.GetOutermost() && Blueprint.GetOutermost()->IsDirty()
        ? TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueString>(Blueprint.GetOutermost()->GetName())} : TArray<TSharedPtr<FJsonValue>>{};
}

static FString P15Identity(const FString& BlueprintId, const FString& Kind, const FString& Key)
{
    return BlueprintId + TEXT("#") + Kind + TEXT(":") + P15Guid(P15DeterministicGuid(BlueprintId + TEXT("#") + Kind + TEXT(":") + Key));
}
static UAnimBlueprint* P15AnimationBlueprintForClass(const UClass* AnimClass)
{
    UAnimBlueprint* Blueprint = AnimClass ? Cast<UAnimBlueprint>(AnimClass->ClassGeneratedBy) : nullptr;
    return Blueprint && Blueprint->GeneratedClass == AnimClass ? Blueprint : nullptr;
}

static FString P15AnimationMode(const EAnimationMode::Type Mode)
{
    if (Mode == EAnimationMode::AnimationBlueprint) return TEXT("AnimationBlueprint");
    if (Mode == EAnimationMode::AnimationSingleNode) return TEXT("AnimationSingleNode");
    return TEXT("AnimationCustomMode");
}

static FString P15CharacterMeshId(const UBlueprint& Blueprint, const USkeletalMeshComponent& Mesh)
{
    return Blueprint.GetPathName() + TEXT("#component:inherited:") + Mesh.GetName();
}

static bool P15CharacterDefaults(UBlueprint& Blueprint, ACharacter*& Character, USkeletalMeshComponent*& Mesh)
{
    Character = Blueprint.GeneratedClass && Blueprint.GeneratedClass->IsChildOf(ACharacter::StaticClass()) ? Cast<ACharacter>(Blueprint.GeneratedClass->GetDefaultObject()) : nullptr;
    Mesh = Character ? Character->GetMesh() : nullptr;
    return Blueprint.ParentClass && Blueprint.ParentClass->IsChildOf(ACharacter::StaticClass()) && Character && Mesh && Mesh->GetFName() == ACharacter::MeshComponentName;
}

static TSharedRef<FJsonObject> P15CharacterResult(UBlueprint& Blueprint, USkeletalMeshComponent& Mesh, const bool Mutation, const bool Changed = false)
{
    USkeletalMesh* SkeletalMesh = Mesh.GetSkeletalMeshAsset(); USkeleton* Skeleton = SkeletalMesh ? SkeletalMesh->GetSkeleton() : nullptr; UClass* AnimClass = Mesh.AnimClass.Get(); UAnimBlueprint* AnimationBlueprint = P15AnimationBlueprintForClass(AnimClass); TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("characterBlueprintId"), Blueprint.GetPathName()); Result->SetStringField(TEXT("meshComponentId"), P15CharacterMeshId(Blueprint, Mesh));
    auto Nullable = [&](const TCHAR* Field, const UObject* Value) { if (Value) Result->SetStringField(Field, Value->GetPathName()); else Result->SetField(Field, MakeShared<FJsonValueNull>()); };
    Nullable(TEXT("skeletalMeshId"), SkeletalMesh); Nullable(TEXT("skeletonId"), Skeleton); Result->SetStringField(TEXT("animationMode"), P15AnimationMode(Mesh.GetAnimationMode())); Nullable(TEXT("animationBlueprintId"), AnimationBlueprint); Nullable(TEXT("animClass"), AnimClass); if (Mutation) { Result->SetBoolField(TEXT("changed"), Changed); Result->SetArrayField(TEXT("dirtyPackages"), Blueprint.GetOutermost() && Blueprint.GetOutermost()->IsDirty() ? TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueString>(Blueprint.GetOutermost()->GetName())} : TArray<TSharedPtr<FJsonValue>>{}); Result->SetArrayField(TEXT("savedPackages"), {}); } Result->SetStringField(TEXT("revision"), BlueprintContentRevision(Blueprint)); return Result;
}

static bool P15CharacterBindingMatches(UBlueprint& CharacterBlueprint, USkeletalMesh& SkeletalMesh, UAnimBlueprint& AnimationBlueprint, USkeletalMeshComponent*& Mesh)
{
    ACharacter* Character = nullptr; if (!P15CharacterDefaults(CharacterBlueprint, Character, Mesh)) return false; return SkeletalMesh.GetSkeleton() && AnimationBlueprint.TargetSkeleton == SkeletalMesh.GetSkeleton() && AnimationBlueprint.GeneratedClass && Mesh->GetSkeletalMeshAsset() == &SkeletalMesh && Mesh->GetAnimationMode() == EAnimationMode::AnimationBlueprint && Mesh->AnimClass.Get() == AnimationBlueprint.GeneratedClass && P15AnimationBlueprintForClass(Mesh->AnimClass.Get()) == &AnimationBlueprint;
}

enum class EP15FailureStage : uint8
{
    None,
    Helper,
    Compile,
    Readback,
    RollbackCompile,
    RollbackReadback,
};

#if WITH_DEV_AUTOMATION_TESTS
static EP15FailureStage GP15FailureStage = EP15FailureStage::None;
#endif

static bool P15FailureInjected(const EP15FailureStage Stage)
{
#if WITH_DEV_AUTOMATION_TESTS
    return GP15FailureStage == Stage;
#else
    return false;
#endif
#if !WITH_DEV_AUTOMATION_TESTS
    static_cast<void>(Stage);
#endif
}

static bool P15EnsureMachine(UAnimBlueprint& Blueprint, UAnimGraphNode_StateMachine*& Machine, UAnimationStateMachineGraph*& Graph)
{
    UAnimationGraph* AnimGraph = P15AnimGraph(Blueprint); Machine = AnimGraph ? P15OnlyNode<UAnimGraphNode_StateMachine>(*AnimGraph) : nullptr; Graph = Machine ? Machine->EditorStateMachineGraph : nullptr;
    UAnimGraphNode_Root* Root = AnimGraph ? P15OnlyNode<UAnimGraphNode_Root>(*AnimGraph) : nullptr;
    if (Machine)
    {
        UEdGraphPin* Out = Machine->FindPin(TEXT("Pose"), EGPD_Output); UEdGraphPin* In = Root ? Root->FindPin(TEXT("Result"), EGPD_Input) : nullptr;
        const bool Exact = Graph && Root && Graph->GetName() == TEXT("locomotion") && Graph->OwnerAnimGraphNode == Machine && Graph->EntryNode &&
            Machine->NodeGuid == P15DeterministicGuid(Blueprint.GetPathName() + TEXT("#state-machine:locomotion")) &&
            Graph->GraphGuid == P15DeterministicGuid(Blueprint.GetPathName() + TEXT("#state-machine-graph:locomotion")) &&
            Graph->EntryNode->NodeGuid == P15DeterministicGuid(Blueprint.GetPathName() + TEXT("#state-machine-entry:locomotion")) &&
            Out && In && Out->LinkedTo.Num() == 1 && Out->LinkedTo[0] == In;
        if (!Exact) { Machine = nullptr; Graph = nullptr; }
        return false;
    }
    if (!AnimGraph || !Root) return false;
    Machine = FEdGraphSchemaAction_K2NewNode::SpawnNode<UAnimGraphNode_StateMachine>(AnimGraph, FVector2D(-300, 0), EK2NewNodeFlags::None);
    if (!Machine || !Machine->EditorStateMachineGraph) { Machine = nullptr; return false; }
    Machine->NodeGuid = P15DeterministicGuid(Blueprint.GetPathName() + TEXT("#state-machine:locomotion")); Graph = Machine->EditorStateMachineGraph; FBlueprintEditorUtils::RenameGraph(Graph, TEXT("locomotion")); Graph->GraphGuid = P15DeterministicGuid(Blueprint.GetPathName() + TEXT("#state-machine-graph:locomotion")); if (!Graph->EntryNode) { Machine = nullptr; Graph = nullptr; return false; } Graph->EntryNode->NodeGuid = P15DeterministicGuid(Blueprint.GetPathName() + TEXT("#state-machine-entry:locomotion"));
    UEdGraphPin* Out = Machine->FindPin(TEXT("Pose"), EGPD_Output); UEdGraphPin* In = Root->FindPin(TEXT("Result"), EGPD_Input); if (!Out || !In || !AnimGraph->GetSchema()->TryCreateConnection(Out, In)) { Machine->DestroyNode(); Machine = nullptr; Graph = nullptr; return false; }
    if (P15FailureInjected(EP15FailureStage::Helper)) { Machine = nullptr; Graph = nullptr; return false; } return true;
}

static TSharedRef<FJsonObject> P15SimpleResult(UAnimBlueprint& Blueprint, bool Changed)
{
    TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("animationBlueprintId"), Blueprint.GetPathName()); Result->SetBoolField(TEXT("changed"), Changed); Result->SetArrayField(TEXT("dirtyPackages"), P15Dirty(Blueprint, Changed)); Result->SetArrayField(TEXT("savedPackages"), {}); Result->SetStringField(TEXT("revision"), BlueprintContentRevision(Blueprint)); return Result;
}
static UEdGraph* P15UpdateGraph(UAnimBlueprint& Blueprint)
{
    if (Blueprint.UbergraphPages.Num() != 1 || !Blueprint.UbergraphPages[0]) return nullptr;
    return Blueprint.UbergraphPages[0];
}

static UEdGraphNode* P15NodeByGuid(UEdGraph& Graph, const FGuid& Guid)
{
    for (UEdGraphNode* Node : Graph.Nodes) if (Node && Node->NodeGuid == Guid) return Node;
    return nullptr;
}

static UEdGraphPin* P15Pin(UEdGraphNode& Node, const TCHAR* Name, EEdGraphPinDirection Direction)
{
    return Node.FindPin(Name, Direction);
}

static bool P15VariableReadback(UAnimBlueprint& Blueprint, UEdGraph& Graph, TArray<UEdGraphNode*>& Nodes)
{
    const FString Id = Blueprint.GetPathName(); int32 VariableIndex = INDEX_NONE;
    for (int32 Index = 0; Index < Blueprint.NewVariables.Num(); ++Index) if (Blueprint.NewVariables[Index].VarName == FName(TEXT("Speed"))) { if (VariableIndex != INDEX_NONE || Blueprint.NewVariables[Index].VarType.PinCategory != UEdGraphSchema_K2::PC_Real || Blueprint.NewVariables[Index].VarType.PinSubCategory != UEdGraphSchema_K2::PC_Float) return false; VariableIndex = Index; }
    if (VariableIndex == INDEX_NONE) return false;
    UEdGraphNode* Event = P15NodeByGuid(Graph, P15DeterministicGuid(Id + TEXT("#event:Speed"))); UEdGraphNode* Owner = P15NodeByGuid(Graph, P15DeterministicGuid(Id + TEXT("#owner:Speed"))); UEdGraphNode* Velocity = P15NodeByGuid(Graph, P15DeterministicGuid(Id + TEXT("#velocity:Speed"))); UEdGraphNode* Planar = P15NodeByGuid(Graph, P15DeterministicGuid(Id + TEXT("#planar-speed:Speed"))); UEdGraphNode* Setter = P15NodeByGuid(Graph, P15DeterministicGuid(Id + TEXT("#setter:Speed")));
    UK2Node_Event* E = Cast<UK2Node_Event>(Event); UK2Node_CallFunction* O = Cast<UK2Node_CallFunction>(Owner); UK2Node_CallFunction* V = Cast<UK2Node_CallFunction>(Velocity); UK2Node_CallFunction* P = Cast<UK2Node_CallFunction>(Planar); UK2Node_VariableSet* S = Cast<UK2Node_VariableSet>(Setter);
    if (!E || !O || !V || !P || !S || !E->bOverrideFunction || E->EventReference.GetMemberName() != FName(TEXT("BlueprintUpdateAnimation")) || O->GetTargetFunction() != UAnimInstance::StaticClass()->FindFunctionByName(TEXT("TryGetPawnOwner")) || V->GetTargetFunction() != AActor::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(AActor, GetVelocity)) || P->GetTargetFunction() != UKismetMathLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VSizeXY)) || S->VariableReference.GetMemberName() != FName(TEXT("Speed"))) return false;
    UEdGraphPin* ET = P15Pin(*E, TEXT("then"), EGPD_Output); UEdGraphPin* OR = P15Pin(*O, TEXT("ReturnValue"), EGPD_Output); UEdGraphPin* VT = P15Pin(*V, TEXT("self"), EGPD_Input); UEdGraphPin* VR = P15Pin(*V, TEXT("ReturnValue"), EGPD_Output); UEdGraphPin* PV = P15Pin(*P, TEXT("A"), EGPD_Input); UEdGraphPin* PR = P15Pin(*P, TEXT("ReturnValue"), EGPD_Output); UEdGraphPin* SE = P15Pin(*S, TEXT("execute"), EGPD_Input); UEdGraphPin* SV = P15Pin(*S, TEXT("Speed"), EGPD_Input);
    if (!ET || !OR || !VT || !VR || !PV || !PR || !SE || !SV || ET->LinkedTo != TArray<UEdGraphPin*>{SE} || OR->LinkedTo != TArray<UEdGraphPin*>{VT} || VR->LinkedTo != TArray<UEdGraphPin*>{PV} || PR->LinkedTo != TArray<UEdGraphPin*>{SV}) return false;
    Nodes = {Event, Owner, Velocity, Planar, Setter}; return true;
}

static bool P15EnsureVariable(UAnimBlueprint& Blueprint, UEdGraph& Graph, bool& Changed, TArray<UEdGraphNode*>& Authored)
{
    const FString Id = Blueprint.GetPathName(); FBPVariableDescription* Variable = nullptr;
    for (FBPVariableDescription& Candidate : Blueprint.NewVariables) if (Candidate.VarName == FName(TEXT("Speed"))) { if (Variable) return false; Variable = &Candidate; }
    if (Variable && (Variable->VarType.PinCategory != UEdGraphSchema_K2::PC_Real || Variable->VarType.PinSubCategory != UEdGraphSchema_K2::PC_Float)) return false;
    TArray<UEdGraphNode*> Existing; if (Variable && P15VariableReadback(Blueprint, Graph, Existing)) { Authored = Existing; return true; }
    if (Variable && P15NodeByGuid(Graph, P15DeterministicGuid(Id + TEXT("#event:Speed")))) return false;
    Blueprint.Modify(); Graph.Modify();
    if (!Variable)
    {
        FEdGraphPinType VariableType;
        VariableType.PinCategory = UEdGraphSchema_K2::PC_Real;
        VariableType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
        if (!FBlueprintEditorUtils::AddMemberVariable(&Blueprint, FName(TEXT("Speed")), VariableType)) return false;
        Changed = true;
        for (FBPVariableDescription& Candidate : Blueprint.NewVariables)
        {
            if (Candidate.VarName == FName(TEXT("Speed")))
            {
                Candidate.VarGuid = P15DeterministicGuid(Id + TEXT("#variable:Speed"));
                Variable = &Candidate;
                break;
            }
        }
        if (!Variable) return false;
    }
    auto Add = [&](UEdGraphNode* Node, const FString& Key) { Node->NodeGuid = P15DeterministicGuid(Id + Key); Graph.AddNode(Node, true, false); Node->PostPlacedNewNode(); Node->AllocateDefaultPins(); Authored.Add(Node); };
    UK2Node_Event* E = NewObject<UK2Node_Event>(&Graph); E->EventReference.SetExternalMember(FName(TEXT("BlueprintUpdateAnimation")), UAnimInstance::StaticClass()); E->bOverrideFunction = true; Add(E, TEXT("#event:Speed"));
    UK2Node_CallFunction* O = NewObject<UK2Node_CallFunction>(&Graph); O->SetFromFunction(UAnimInstance::StaticClass()->FindFunctionByName(TEXT("TryGetPawnOwner"))); Add(O, TEXT("#owner:Speed"));
    UK2Node_CallFunction* V = NewObject<UK2Node_CallFunction>(&Graph); V->SetFromFunction(AActor::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(AActor, GetVelocity))); Add(V, TEXT("#velocity:Speed"));
    UK2Node_CallFunction* P = NewObject<UK2Node_CallFunction>(&Graph); P->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VSizeXY))); Add(P, TEXT("#planar-speed:Speed"));
    UK2Node_VariableSet* S = NewObject<UK2Node_VariableSet>(&Graph); S->VariableReference.SetSelfMember(FName(TEXT("Speed"))); Add(S, TEXT("#setter:Speed"));
    UEdGraphPin* EThen = P15Pin(*E, TEXT("then"), EGPD_Output); UEdGraphPin* OReturn = P15Pin(*O, TEXT("ReturnValue"), EGPD_Output); UEdGraphPin* VSelf = P15Pin(*V, TEXT("self"), EGPD_Input); UEdGraphPin* VReturn = P15Pin(*V, TEXT("ReturnValue"), EGPD_Output); UEdGraphPin* PA = P15Pin(*P, TEXT("A"), EGPD_Input); UEdGraphPin* PReturn = P15Pin(*P, TEXT("ReturnValue"), EGPD_Output); UEdGraphPin* SExecute = P15Pin(*S, TEXT("execute"), EGPD_Input); UEdGraphPin* SSpeed = P15Pin(*S, TEXT("Speed"), EGPD_Input);
    if (!EThen || !OReturn || !VSelf || !VReturn || !PA || !PReturn || !SExecute || !SSpeed) return false;
    if (!Graph.GetSchema()->TryCreateConnection(EThen, SExecute) || !Graph.GetSchema()->TryCreateConnection(OReturn, VSelf) || !Graph.GetSchema()->TryCreateConnection(VReturn, PA) || !Graph.GetSchema()->TryCreateConnection(PReturn, SSpeed)) return false;
    Changed = true; if (P15FailureInjected(EP15FailureStage::Helper)) return false; return P15VariableReadback(Blueprint, Graph, Existing);
}
static UEdGraphPin* P15PosePin(UEdGraphNode& Node, const EEdGraphPinDirection Direction)
{
    for (UEdGraphPin* Pin : Node.Pins) if (Pin && Pin->Direction == Direction && UAnimationGraphSchema::IsLocalSpacePosePin(Pin->PinType)) return Pin;
    return nullptr;
}

static UAnimStateNode* P15State(UAnimationStateMachineGraph& Graph, const FString& Name)
{
    TArray<UAnimStateNode*> States; Graph.GetNodesOfClass(States);
    for (UAnimStateNode* State : States) if (State && State->GetStateName() == Name) return State;
    return nullptr;
}

static bool P15StateReadback(UAnimStateNode& State, UAnimSequence& Sequence, const FString& Id)
{
    UAnimationStateGraph* Graph = Cast<UAnimationStateGraph>(State.BoundGraph);
    if (!Graph || State.NodeGuid != P15DeterministicGuid(Id + TEXT("#node")) || Graph->GraphGuid != P15DeterministicGuid(Id + TEXT("#graph"))) return false;
    UAnimGraphNode_StateResult* Result = Graph->GetResultNode();
    TArray<UAnimGraphNode_SequencePlayer*> Players; Graph->GetNodesOfClass(Players);
    if (!Result || Graph->Nodes.Num() != 2 || Players.Num() != 1 || Players[0]->Node.GetSequence() != &Sequence || Sequence.GetSkeleton() == nullptr ||
        Result->NodeGuid != P15DeterministicGuid(Id + TEXT("#result")) || Players[0]->NodeGuid != P15DeterministicGuid(Id + TEXT("#player"))) return false;
    UEdGraphPin* Out = P15PosePin(*Players[0], EGPD_Output); UEdGraphPin* In = P15PosePin(*Result, EGPD_Input);
    return Out && In && Out->LinkedTo.Num() == 1 && Out->LinkedTo[0] == In && In->LinkedTo.Num() == 1 && In->LinkedTo[0] == Out;
}

static bool P15EnsureState(UAnimBlueprint& Blueprint, UAnimationStateMachineGraph& MachineGraph, const FString& Name, UAnimSequence& Sequence, bool& Changed, UAnimStateNode*& State)
{
    State = P15State(MachineGraph, Name);
    const FString Prefix = Blueprint.GetPathName() + TEXT("#state:") + Name;
    if (State)
    {
        if (!P15StateReadback(*State, Sequence, Prefix)) return false;
    }
    else
    {
        State = FEdGraphSchemaAction_NewStateNode::SpawnNodeFromTemplate<UAnimStateNode>(&MachineGraph, NewObject<UAnimStateNode>(), FVector2D(Name == TEXT("idle") ? 0 : 400, 0), false);
        if (!State || !State->BoundGraph) return false;
        FBlueprintEditorUtils::RenameGraph(State->BoundGraph, *Name);
        State->NodeGuid = P15DeterministicGuid(Prefix + TEXT("#node")); State->BoundGraph->GraphGuid = P15DeterministicGuid(Prefix + TEXT("#graph"));
        UAnimationStateGraph* Graph = Cast<UAnimationStateGraph>(State->BoundGraph); UAnimGraphNode_StateResult* Result = Graph ? Graph->GetResultNode() : nullptr;
        UAnimGraphNode_SequencePlayer* Player = Graph ? FEdGraphSchemaAction_K2NewNode::SpawnNode<UAnimGraphNode_SequencePlayer>(Graph, FVector2D(-300, 0), EK2NewNodeFlags::None, [&](UAnimGraphNode_SequencePlayer* Node) { Node->Node.SetSequence(&Sequence); }) : nullptr;
        if (!Graph || !Result || !Player) return false;
        Result->NodeGuid = P15DeterministicGuid(Prefix + TEXT("#result")); Player->NodeGuid = P15DeterministicGuid(Prefix + TEXT("#player"));
        if (!Graph->GetSchema()->TryCreateConnection(P15PosePin(*Player, EGPD_Output), P15PosePin(*Result, EGPD_Input))) return false;
        Changed = true;
    }
    if (Name == TEXT("idle") && MachineGraph.EntryNode && MachineGraph.EntryNode->GetOutputNode() != State)
    {
        if (!MachineGraph.GetSchema()->TryCreateConnection(MachineGraph.EntryNode->GetOutputPin(), State->GetInputPin())) return false;
        Changed = true;
    }
    if (P15FailureInjected(EP15FailureStage::Helper)) return false; return P15StateReadback(*State, Sequence, Prefix);
}

static bool P15SemanticGraphReadback(UAnimBlueprint& Blueprint, TArray<TSharedPtr<FJsonValue>>& Variables, TArray<TSharedPtr<FJsonValue>>& Machines)
{
    Variables.Reset(); Machines.Reset();
    const FString Id = Blueprint.GetPathName();
    UAnimationGraph* Graph = P15AnimGraph(Blueprint);
    if (!Graph) return false;
    UEdGraph* Update = P15UpdateGraph(Blueprint); TArray<UEdGraphNode*> VariableNodes;
    for (const FBPVariableDescription& Variable : Blueprint.NewVariables) if (Variable.VarName == FName(TEXT("Speed")))
    {
        if (!Update || !P15VariableReadback(Blueprint, *Update, VariableNodes)) return false;
        TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("variableId"), P15Identity(Id, TEXT("variable"), TEXT("Speed")));
        Row->SetStringField(TEXT("bindingId"), P15Identity(Id, TEXT("binding"), TEXT("Speed")));
        Row->SetStringField(TEXT("name"), TEXT("Speed")); Row->SetStringField(TEXT("type"), TEXT("float")); Row->SetStringField(TEXT("source"), TEXT("owner_planar_speed"));
        Row->SetStringField(TEXT("updateGraphId"), P15Identity(Id, TEXT("update-graph"), TEXT("Speed"))); Row->SetStringField(TEXT("eventNodeId"), P15Identity(Id, TEXT("event"), TEXT("Speed"))); Row->SetStringField(TEXT("ownerNodeId"), P15Identity(Id, TEXT("owner"), TEXT("Speed"))); Row->SetStringField(TEXT("velocityNodeId"), P15Identity(Id, TEXT("velocity"), TEXT("Speed"))); Row->SetStringField(TEXT("planarSpeedNodeId"), P15Identity(Id, TEXT("planar-speed"), TEXT("Speed"))); Row->SetStringField(TEXT("setterNodeId"), P15Identity(Id, TEXT("setter"), TEXT("Speed")));
        Variables.Add(MakeShared<FJsonValueObject>(Row));
    }
    TArray<UAnimGraphNode_StateMachine*> MachinesFound; Graph->GetNodesOfClass(MachinesFound); if (MachinesFound.Num() > 1) return false;
    for (UAnimGraphNode_StateMachine* Machine : MachinesFound)
    {
        if (!Machine || !Machine->EditorStateMachineGraph || Machine->EditorStateMachineGraph->GetName() != TEXT("locomotion")) return false;
        UAnimationStateMachineGraph& SM = *Machine->EditorStateMachineGraph;
        UAnimGraphNode_Root* Root = P15OnlyNode<UAnimGraphNode_Root>(*Graph); UEdGraphPin* MachineOut = Machine->FindPin(TEXT("Pose"), EGPD_Output); UEdGraphPin* RootIn = Root ? Root->FindPin(TEXT("Result"), EGPD_Input) : nullptr;
        if (!Root || Machine->NodeGuid != P15DeterministicGuid(Id + TEXT("#state-machine:locomotion")) || SM.GraphGuid != P15DeterministicGuid(Id + TEXT("#state-machine-graph:locomotion")) || !SM.EntryNode || SM.EntryNode->NodeGuid != P15DeterministicGuid(Id + TEXT("#state-machine-entry:locomotion")) || !MachineOut || !RootIn || MachineOut->LinkedTo.Num() != 1 || MachineOut->LinkedTo[0] != RootIn) return false;
        TArray<UAnimStateNode*> States; SM.GetNodesOfClass(States); if (States.Num() > 2) return false;
        States.RemoveAll([](const UAnimStateNode* State) { return !State; });
        States.Sort([](const UAnimStateNode& Left, const UAnimStateNode& Right) { return Left.GetStateName() < Right.GetStateName(); });
        UAnimStateNode* Idle = P15State(SM, TEXT("idle")); UAnimStateNode* Moving = P15State(SM, TEXT("moving"));
        if ((Idle && Idle->GetStateName() != TEXT("idle")) || (Moving && Moving->GetStateName() != TEXT("moving"))) return false;
        if ((Idle && Moving && Idle == Moving) || (States.Num() != ((Idle ? 1 : 0) + (Moving ? 1 : 0)))) return false;
        TArray<UAnimStateTransitionNode*> Transitions; SM.GetNodesOfClass(Transitions); if (Transitions.Num() > 2) return false;
        TSet<FString> TransitionPairs;
        for (UAnimStateTransitionNode* Transition : Transitions)
        {
            if (!Transition || !Transition->GetPreviousState() || !Transition->GetNextState()) return false;
            const FString Pair = Transition->GetPreviousState()->GetStateName() + TEXT("->") + Transition->GetNextState()->GetStateName();
            if ((Pair != TEXT("idle->moving") && Pair != TEXT("moving->idle")) || TransitionPairs.Contains(Pair)) return false;
            TransitionPairs.Add(Pair);
        }
        Transitions.Sort([](const UAnimStateTransitionNode& Left, const UAnimStateTransitionNode& Right) { return Left.GetPreviousState()->GetStateName() + TEXT("->") + Left.GetNextState()->GetStateName() < Right.GetPreviousState()->GetStateName() + TEXT("->") + Right.GetNextState()->GetStateName(); });
        UAnimStateNode* Initial = SM.EntryNode ? Cast<UAnimStateNode>(SM.EntryNode->GetOutputNode()) : nullptr;
        if (Initial != Idle) return false;
        TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("stateMachineId"), P15Identity(Id, TEXT("state-machine"), TEXT("locomotion"))); Row->SetStringField(TEXT("stateMachineGraphId"), P15Identity(Id, TEXT("state-machine-graph"), TEXT("locomotion"))); Row->SetStringField(TEXT("entryNodeId"), P15Identity(Id, TEXT("state-machine-entry"), TEXT("locomotion"))); Row->SetStringField(TEXT("name"), TEXT("locomotion"));
        if (Initial) Row->SetStringField(TEXT("initialStateId"), P15Identity(Id, TEXT("state"), TEXT("idle"))); else Row->SetField(TEXT("initialStateId"), MakeShared<FJsonValueNull>());
        TArray<TSharedPtr<FJsonValue>> StateRows;
        for (UAnimStateNode* State : States)
        {
            UAnimationStateGraph* SG = Cast<UAnimationStateGraph>(State->BoundGraph); TArray<UAnimGraphNode_SequencePlayer*> Players; if (SG) SG->GetNodesOfClass(Players); UAnimSequence* Sequence = Players.Num() == 1 ? Cast<UAnimSequence>(Players[0]->Node.GetSequence()) : nullptr; const FString N = State->GetStateName();
            if (!Sequence || Sequence->GetSkeleton() != Blueprint.TargetSkeleton || !P15StateReadback(*State, *Sequence, Id + TEXT("#state:") + N)) return false;
            TSharedRef<FJsonObject> SR = MakeShared<FJsonObject>(); SR->SetStringField(TEXT("stateId"), P15Identity(Id, TEXT("state"), N)); SR->SetStringField(TEXT("stateGraphId"), P15Identity(Id, TEXT("state-graph"), N)); SR->SetStringField(TEXT("resultNodeId"), P15Identity(Id, TEXT("state-result"), N)); SR->SetStringField(TEXT("sequencePlayerNodeId"), P15Identity(Id, TEXT("state-player"), N)); SR->SetStringField(TEXT("name"), N); SR->SetStringField(TEXT("sequenceId"), Sequence->GetPathName()); SR->SetStringField(TEXT("skeletonId"), Blueprint.TargetSkeleton->GetPathName()); SR->SetBoolField(TEXT("initial"), State == Initial); StateRows.Add(MakeShared<FJsonValueObject>(SR));
        }
        TArray<TSharedPtr<FJsonValue>> TransitionRows;
        for (UAnimStateTransitionNode* Transition : Transitions)
        {
            const FString Pair = Transition->GetPreviousState()->GetStateName() + TEXT("->") + Transition->GetNextState()->GetStateName(); const FString Prefix = Id + TEXT("#transition:") + Pair; const FString Expression = Pair == TEXT("idle->moving") ? TEXT("Speed > 10") : TEXT("Speed <= 10");
            if (!P15TransitionReadback(Blueprint, *Transition, Expression)) return false;
            TSharedRef<FJsonObject> TRRow = MakeShared<FJsonObject>(); TRRow->SetStringField(TEXT("transitionId"), P15Identity(Id, TEXT("transition"), Pair)); TRRow->SetStringField(TEXT("transitionGraphId"), P15Identity(Id, TEXT("transition-graph"), Prefix)); TRRow->SetStringField(TEXT("resultNodeId"), P15Identity(Id, TEXT("transition-result"), Prefix)); TRRow->SetStringField(TEXT("variableGetterNodeId"), P15Identity(Id, TEXT("transition-getter"), Prefix)); TRRow->SetStringField(TEXT("comparisonNodeId"), P15Identity(Id, TEXT("transition-comparison"), Prefix)); TRRow->SetStringField(TEXT("fromStateId"), P15Identity(Id, TEXT("state"), Transition->GetPreviousState()->GetStateName())); TRRow->SetStringField(TEXT("toStateId"), P15Identity(Id, TEXT("state"), Transition->GetNextState()->GetStateName())); TRRow->SetStringField(TEXT("expression"), Expression); TransitionRows.Add(MakeShared<FJsonValueObject>(TRRow));
        }
        Row->SetArrayField(TEXT("states"), StateRows); Row->SetArrayField(TEXT("transitions"), TransitionRows); Machines.Add(MakeShared<FJsonValueObject>(Row));
    }
    return true;
}

static bool P15CompleteLocomotionReadback(UAnimBlueprint& Blueprint, TSharedPtr<FJsonObject>& Machine)
{
    TArray<TSharedPtr<FJsonValue>> Variables, Machines; if (!P15SemanticGraphReadback(Blueprint, Variables, Machines) || Variables.Num() != 1 || Machines.Num() != 1 || !Variables[0].IsValid() || !Variables[0]->AsObject() || !Machines[0].IsValid() || !Machines[0]->AsObject()) return false; const TSharedPtr<FJsonObject> Variable = Variables[0]->AsObject(); Machine = Machines[0]->AsObject(); const TArray<TSharedPtr<FJsonValue>>* States = nullptr; const TArray<TSharedPtr<FJsonValue>>* Transitions = nullptr; if (Variable->GetStringField(TEXT("name")) != TEXT("Speed") || Variable->GetStringField(TEXT("type")) != TEXT("float") || Variable->GetStringField(TEXT("source")) != TEXT("owner_planar_speed") || Machine->GetStringField(TEXT("stateMachineId")) != P15Identity(Blueprint.GetPathName(), TEXT("state-machine"), TEXT("locomotion")) || Machine->GetStringField(TEXT("name")) != TEXT("locomotion") || Machine->GetStringField(TEXT("initialStateId")) != P15Identity(Blueprint.GetPathName(), TEXT("state"), TEXT("idle")) || !Machine->TryGetArrayField(TEXT("states"), States) || !States || States->Num() != 2 || !Machine->TryGetArrayField(TEXT("transitions"), Transitions) || !Transitions || Transitions->Num() != 2) return false; TSet<FString> StateNames, TransitionPairs; for (const TSharedPtr<FJsonValue>& Value : *States) { const TSharedPtr<FJsonObject> State = Value.IsValid() ? Value->AsObject() : nullptr; if (!State) return false; const FString Name = State->GetStringField(TEXT("name")); if ((Name != TEXT("idle") && Name != TEXT("moving")) || State->GetStringField(TEXT("stateId")) != P15Identity(Blueprint.GetPathName(), TEXT("state"), Name) || StateNames.Contains(Name)) return false; StateNames.Add(Name); } for (const TSharedPtr<FJsonValue>& Value : *Transitions) { const TSharedPtr<FJsonObject> Transition = Value.IsValid() ? Value->AsObject() : nullptr; if (!Transition) return false; const FString From = Transition->GetStringField(TEXT("fromStateId")); const FString To = Transition->GetStringField(TEXT("toStateId")); const FString Pair = From == P15Identity(Blueprint.GetPathName(), TEXT("state"), TEXT("idle")) && To == P15Identity(Blueprint.GetPathName(), TEXT("state"), TEXT("moving")) ? TEXT("idle->moving") : From == P15Identity(Blueprint.GetPathName(), TEXT("state"), TEXT("moving")) && To == P15Identity(Blueprint.GetPathName(), TEXT("state"), TEXT("idle")) ? TEXT("moving->idle") : FString(); const FString Expression = Pair == TEXT("idle->moving") ? TEXT("Speed > 10") : TEXT("Speed <= 10"); if (Pair.IsEmpty() || TransitionPairs.Contains(Pair) || Transition->GetStringField(TEXT("transitionId")) != P15Identity(Blueprint.GetPathName(), TEXT("transition"), Pair) || Transition->GetStringField(TEXT("expression")) != Expression) return false; TransitionPairs.Add(Pair); } return StateNames.Num() == 2 && TransitionPairs.Num() == 2;
}

static UAnimStateTransitionNode* P15Transition(UAnimationStateMachineGraph& Graph, UAnimStateNode& From, UAnimStateNode& To)
{
    TArray<UAnimStateTransitionNode*> Found; Graph.GetNodesOfClass(Found); for (UAnimStateTransitionNode* Item : Found) if (Item && Item->GetPreviousState() == &From && Item->GetNextState() == &To) return Item; return nullptr;
}

static bool P15TransitionReadback(UAnimBlueprint& Blueprint, UAnimStateTransitionNode& Transition, const FString& Expression)
{
    UAnimStateNode* From = Cast<UAnimStateNode>(Transition.GetPreviousState()); UAnimStateNode* To = Cast<UAnimStateNode>(Transition.GetNextState());
    if (!From || !To) return false;
    const FString Pair = From->GetStateName() + TEXT("->") + To->GetStateName(); const FString Prefix = Blueprint.GetPathName() + TEXT("#transition:") + Pair;
    if ((Pair == TEXT("idle->moving") && Expression != TEXT("Speed > 10")) || (Pair == TEXT("moving->idle") && Expression != TEXT("Speed <= 10")) ||
        (Pair != TEXT("idle->moving") && Pair != TEXT("moving->idle")) || Transition.NodeGuid != P15DeterministicGuid(Prefix + TEXT("#node"))) return false;
    UAnimationTransitionGraph* Graph = Cast<UAnimationTransitionGraph>(Transition.BoundGraph); UAnimGraphNode_TransitionResult* Result = Graph ? Graph->GetResultNode() : nullptr;
    if (!Graph || !Result || Graph->GraphGuid != P15DeterministicGuid(Prefix + TEXT("#graph")) || Result->NodeGuid != P15DeterministicGuid(Prefix + TEXT("#result")) || Graph->Nodes.Num() != 3) return false;
    UK2Node_VariableGet* Getter = Cast<UK2Node_VariableGet>(P15NodeByGuid(*Graph, P15DeterministicGuid(Prefix + TEXT("#getter")))); UK2Node_CallFunction* Compare = Cast<UK2Node_CallFunction>(P15NodeByGuid(*Graph, P15DeterministicGuid(Prefix + TEXT("#comparison"))));
    const FName FunctionName(Expression == TEXT("Speed > 10") ? GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_DoubleDouble) : GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, LessEqual_DoubleDouble));
    if (!Getter || !Compare || Getter->VariableReference.GetMemberName() != FName(TEXT("Speed")) || Compare->GetTargetFunction() != UKismetMathLibrary::StaticClass()->FindFunctionByName(FunctionName)) return false;
    UEdGraphPin* Value = Getter->GetValuePin(); UEdGraphPin* A = Compare->FindPin(TEXT("A"), EGPD_Input); UEdGraphPin* B = Compare->FindPin(TEXT("B"), EGPD_Input); UEdGraphPin* Out = Compare->FindPin(TEXT("ReturnValue"), EGPD_Output); UEdGraphPin* Rule = Result->FindPin(TEXT("bCanEnterTransition"), EGPD_Input);
    return Value && A && B && Out && Rule && FCString::Atod(*B->DefaultValue) == 10.0 && Value->LinkedTo.Num() == 1 && Value->LinkedTo[0] == A && A->LinkedTo.Num() == 1 && A->LinkedTo[0] == Value && Out->LinkedTo.Num() == 1 && Out->LinkedTo[0] == Rule && Rule->LinkedTo.Num() == 1 && Rule->LinkedTo[0] == Out;
}

static bool P15EnsureTransition(UAnimBlueprint& Blueprint, UAnimationStateMachineGraph& SM, UAnimStateNode& From, UAnimStateNode& To, const FString& Expression, bool& Changed, UAnimStateTransitionNode*& Transition)
{
    Transition = P15Transition(SM, From, To); const FString Prefix = Blueprint.GetPathName() + TEXT("#transition:") + From.GetStateName() + TEXT("->") + To.GetStateName();
    if (Transition) return P15TransitionReadback(Blueprint, *Transition, Expression);
    if (!SM.GetSchema()->TryCreateConnection(From.GetOutputPin(), To.GetInputPin())) return false; Transition = P15Transition(SM, From, To); if (!Transition) return false;
    Transition->NodeGuid = P15DeterministicGuid(Prefix + TEXT("#node")); UAnimationTransitionGraph* Graph = Cast<UAnimationTransitionGraph>(Transition->BoundGraph); UAnimGraphNode_TransitionResult* Result = Graph ? Graph->GetResultNode() : nullptr;
    if (!Graph || !Result || Graph->Nodes.Num() != 1) { Transition->DestroyNode(); Transition = nullptr; return false; }
    Graph->GraphGuid = P15DeterministicGuid(Prefix + TEXT("#graph")); Result->NodeGuid = P15DeterministicGuid(Prefix + TEXT("#result")); UEdGraphPin* Rule = Result->FindPin(TEXT("bCanEnterTransition"), EGPD_Input);
    const FName FunctionName(Expression == TEXT("Speed > 10") ? GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_DoubleDouble) : GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, LessEqual_DoubleDouble)); UK2Node_VariableGet* Getter = NewObject<UK2Node_VariableGet>(Graph, NAME_None, RF_Transactional); UK2Node_CallFunction* Compare = NewObject<UK2Node_CallFunction>(Graph, NAME_None, RF_Transactional);
    if (!Rule || !Getter || !Compare) { Transition->DestroyNode(); Transition = nullptr; return false; }
    Getter->VariableReference.SetSelfMember(FName(TEXT("Speed"))); Getter->NodeGuid = P15DeterministicGuid(Prefix + TEXT("#getter")); Graph->AddNode(Getter, true, false); Getter->PostPlacedNewNode(); Getter->AllocateDefaultPins();
    Compare->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(FunctionName)); Compare->NodeGuid = P15DeterministicGuid(Prefix + TEXT("#comparison")); Graph->AddNode(Compare, true, false); Compare->PostPlacedNewNode(); Compare->AllocateDefaultPins();
    UEdGraphPin* Value = Getter->GetValuePin(); UEdGraphPin* A = Compare->FindPin(TEXT("A"), EGPD_Input); UEdGraphPin* B = Compare->FindPin(TEXT("B"), EGPD_Input); UEdGraphPin* Out = Compare->FindPin(TEXT("ReturnValue"), EGPD_Output);
    if (!Value || !A || !B || !Out || !Compare->GetTargetFunction() || !Graph->GetSchema()->TryCreateConnection(Value, A)) { Transition->DestroyNode(); Transition = nullptr; return false; } Graph->GetSchema()->TrySetDefaultValue(*B, TEXT("10.0")); if (FCString::Atod(*B->DefaultValue) != 10.0 || !Graph->GetSchema()->TryCreateConnection(Out, Rule)) { Transition->DestroyNode(); Transition = nullptr; return false; }
    Changed = true; if (P15FailureInjected(EP15FailureStage::Helper)) return false; return P15TransitionReadback(Blueprint, *Transition, Expression);
}

static bool P15CompileAndReadback(UAnimBlueprint& Blueprint, const bool AllowEmptyMachineWarnings = false, const bool Rollback = false, const bool InjectFaults = true)
{
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(&Blueprint); FCompilerResultsLog Results; Results.bSilentMode = true; FKismetEditorUtilities::CompileBlueprint(&Blueprint, EBlueprintCompileOptions::SkipSave, &Results); FAssetCompilingManager::Get().FinishAllCompilation();
    if (InjectFaults && P15FailureInjected(Rollback ? EP15FailureStage::RollbackCompile : EP15FailureStage::Compile)) return false;
    USkeleton* Skeleton = nullptr; UAnimationGraph* Graph = nullptr; UAnimGraphNode_Root* Root = nullptr; TArray<TSharedPtr<FJsonValue>> Variables; TArray<TSharedPtr<FJsonValue>> Machines;
    const bool RootValid = P15RootReadback(Blueprint, Skeleton, Graph, Root); const bool SemanticValid = RootValid && P15SemanticGraphReadback(Blueprint, Variables, Machines);
    if (InjectFaults && P15FailureInjected(Rollback ? EP15FailureStage::RollbackReadback : EP15FailureStage::Readback)) return false;
    bool ExpectedEmptyMachineWarnings = false;
    if (AllowEmptyMachineWarnings && Results.NumWarnings == 2 && Blueprint.Status == BS_UpToDateWithWarnings && Machines.Num() == 1 && Machines[0].IsValid() && Machines[0]->AsObject())
    {
        const TSharedPtr<FJsonObject> Machine = Machines[0]->AsObject(); const TArray<TSharedPtr<FJsonValue>>* States = nullptr; const TArray<TSharedPtr<FJsonValue>>* Transitions = nullptr; int32 MissingEntryLink = 0; int32 MissingEntryState = 0; int32 UnexpectedWarnings = 0;
        for (const TSharedRef<FTokenizedMessage>& Message : Results.Messages) if (Message->GetSeverity() == EMessageSeverity::Warning) { const FString Text = Message->ToText().ToString(); const bool EntryLink = Text.Contains(TEXT("Entry node")) && Text.Contains(TEXT("is not connected to state")); const bool EntryState = Text.Contains(TEXT("There was no entry state connection")); MissingEntryLink += EntryLink; MissingEntryState += EntryState; UnexpectedWarnings += !EntryLink && !EntryState; }
        ExpectedEmptyMachineWarnings = MissingEntryLink == 1 && MissingEntryState == 1 && UnexpectedWarnings == 0 && Machine->TryGetArrayField(TEXT("states"), States) && States && States->IsEmpty() && Machine->TryGetArrayField(TEXT("transitions"), Transitions) && Transitions && Transitions->IsEmpty() && Machine->HasTypedField<EJson::Null>(TEXT("initialStateId"));
    }
    return Results.NumErrors == 0 && (Results.NumWarnings == 0 || ExpectedEmptyMachineWarnings) && (Blueprint.Status == BS_UpToDate || ExpectedEmptyMachineWarnings) && RootValid && SemanticValid;
}

using FP15PinGuidSnapshot = TMap<FString, FGuid>;

static FString P15PinSnapshotKey(const UEdGraph& Graph, const UEdGraphNode& Node, const UEdGraphPin& Pin, const int32 Index)
{
    return P15Guid(Graph.GraphGuid) + TEXT("|") + P15Guid(Node.NodeGuid) + TEXT("|") + FString::FromInt(Index) + TEXT("|") + Pin.PinName.ToString() + TEXT("|") + FString::FromInt(static_cast<int32>(Pin.Direction));
}

static FP15PinGuidSnapshot P15CapturePinGuids(UAnimBlueprint& Blueprint)
{
    FP15PinGuidSnapshot Snapshot; TArray<UEdGraph*> Graphs; Blueprint.GetAllGraphs(Graphs); for (UEdGraph* Graph : Graphs) if (Graph) for (UEdGraphNode* Node : Graph->Nodes) if (Node) for (int32 Index = 0; Index < Node->Pins.Num(); ++Index) if (UEdGraphPin* Pin = Node->Pins[Index]) Snapshot.Add(P15PinSnapshotKey(*Graph, *Node, *Pin, Index), Pin->PinId); return Snapshot;
}

static bool P15RestorePinGuids(UAnimBlueprint& Blueprint, const FP15PinGuidSnapshot& Snapshot)
{
    int32 Restored = 0; TArray<UEdGraph*> Graphs; Blueprint.GetAllGraphs(Graphs); for (UEdGraph* Graph : Graphs) if (Graph) for (UEdGraphNode* Node : Graph->Nodes) if (Node) for (int32 Index = 0; Index < Node->Pins.Num(); ++Index) if (UEdGraphPin* Pin = Node->Pins[Index]) if (const FGuid* Guid = Snapshot.Find(P15PinSnapshotKey(*Graph, *Node, *Pin, Index))) { Pin->PinId = *Guid; ++Restored; } return Restored == Snapshot.Num();
}

static bool P15RestoreBlueprint(UAnimBlueprint& Blueprint, const FString& BeforeRevision, const bool WasDirty, const EBlueprintStatus BeforeStatus, const FP15PinGuidSnapshot& PinGuids, const bool AllowEmptyMachineWarnings = false)
{
    const bool Compiled = P15CompileAndReadback(Blueprint, AllowEmptyMachineWarnings, true); const bool PinsRestored = P15RestorePinGuids(Blueprint, PinGuids); Blueprint.Status = BeforeStatus; if (Blueprint.GetOutermost()) Blueprint.GetOutermost()->SetDirtyFlag(WasDirty);
    return P11RollbackVerificationResult(Compiled && PinsRestored && BlueprintContentRevision(Blueprint) == BeforeRevision && Blueprint.Status == BeforeStatus && (!Blueprint.GetOutermost() || Blueprint.GetOutermost()->IsDirty() == WasDirty));
}

static bool P15MutationBaselineValid(UAnimBlueprint& Blueprint)
{
    if (Blueprint.Status == BS_UpToDate) return true; if (Blueprint.Status != BS_UpToDateWithWarnings) return false; const FString Revision = BlueprintContentRevision(Blueprint); const bool WasDirty = Blueprint.GetOutermost()->IsDirty(); const FP15PinGuidSnapshot PinGuids = P15CapturePinGuids(Blueprint); const bool Compiled = P15CompileAndReadback(Blueprint, true, false, false); const bool PinsRestored = P15RestorePinGuids(Blueprint, PinGuids); Blueprint.Status = BS_UpToDateWithWarnings; Blueprint.GetOutermost()->SetDirtyFlag(WasDirty); return Compiled && PinsRestored && BlueprintContentRevision(Blueprint) == Revision;
}

FString HandleP15AnimationOperation(const FString& Id, const FString& Operation, const TSharedPtr<FJsonObject>& Args, const FString& ExpectedRevision)
{
    if (!Args.IsValid()) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("animation operation requires arguments"));
    if (Operation == TEXT("play.animation_observe"))
    {
        FString SessionId, CharacterId, AnimationBlueprintId, StateMachineId;
        if (!Args->TryGetStringField(TEXT("sessionId"), SessionId) || !Args->TryGetStringField(TEXT("characterId"), CharacterId) || !Args->TryGetStringField(TEXT("animationBlueprintId"), AnimationBlueprintId) || !Args->TryGetStringField(TEXT("stateMachineId"), StateMachineId)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("sessionId, characterId, animationBlueprintId, and stateMachineId are required"));
        if (SessionId != PlaySessionId) return ErrorResponse(Id, TEXT("stale"), TEXT("sessionId does not identify active PIE")); UWorld* World = PieWorld(); if (!World) return ErrorResponse(Id, TEXT("unsafe_editor_state"), TEXT("PIE world is not running"));
        UAnimBlueprint* RequestedBlueprint = P15LoadAnimationBlueprint(AnimationBlueprintId); USkeleton* VerifiedSkeleton = nullptr; UAnimationGraph* VerifiedGraph = nullptr; UAnimGraphNode_Root* VerifiedRoot = nullptr; TSharedPtr<FJsonObject> VerifiedMachine; if (!RequestedBlueprint || RequestedBlueprint->Status != BS_UpToDate || !P15RootReadback(*RequestedBlueprint, VerifiedSkeleton, VerifiedGraph, VerifiedRoot) || !P15CompleteLocomotionReadback(*RequestedBlueprint, VerifiedMachine) || !VerifiedMachine || VerifiedMachine->GetStringField(TEXT("stateMachineId")) != StateMachineId) return ErrorResponse(Id, TEXT("conflict"), TEXT("source Animation Blueprint does not match exact complete P1.5 graph contract"));
        ACharacter* Character = nullptr; for (TActorIterator<ACharacter> It(World); It; ++It) if (IsValid(*It) && ActorId(**It) == CharacterId) { if (Character) return ErrorResponse(Id, TEXT("conflict"), TEXT("characterId resolves to multiple PIE Characters")); Character = *It; } if (!Character) return ErrorResponse(Id, TEXT("not_found"), TEXT("characterId does not identify a PIE Character"));
        USkeletalMeshComponent* Mesh = Character->GetMesh(); USkeletalMesh* SkeletalMesh = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr; USkeleton* Skeleton = SkeletalMesh ? SkeletalMesh->GetSkeleton() : nullptr; UAnimInstance* Instance = Mesh ? Mesh->GetAnimInstance() : nullptr; UAnimBlueprint* AnimationBlueprint = Instance ? P15AnimationBlueprintForClass(Instance->GetClass()) : nullptr; if (!Mesh || !SkeletalMesh || !Skeleton || !Instance || AnimationBlueprint != RequestedBlueprint || AnimationBlueprint->TargetSkeleton != Skeleton || Mesh->GetAnimationMode() != EAnimationMode::AnimationBlueprint || Mesh->AnimClass.Get() != Instance->GetClass()) return ErrorResponse(Id, TEXT("conflict"), TEXT("live Character mesh, Skeleton, and Animation Blueprint identity do not match request"));
        int32 MachineIndex = INDEX_NONE; const FBakedAnimationStateMachine* Description = nullptr; Instance->GetStateMachineIndexAndDescription(FName(TEXT("locomotion")), MachineIndex, &Description); const FAnimNode_StateMachine* Machine = MachineIndex != INDEX_NONE ? Instance->GetStateMachineInstance(MachineIndex) : nullptr; if (!Description || !Machine || Description->States.Num() != 2 || Description->Transitions.Num() != 2) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("live locomotion state machine topology is unavailable"));
        const int32 IdleIndex = Description->FindStateIndex(FName(TEXT("idle"))); const int32 MovingIndex = Description->FindStateIndex(FName(TEXT("moving"))); const int32 IdleMovingIndex = Description->FindTransitionIndex(IdleIndex, MovingIndex); const int32 MovingIdleIndex = Description->FindTransitionIndex(MovingIndex, IdleIndex); if (IdleIndex == INDEX_NONE || MovingIndex == INDEX_NONE || IdleMovingIndex == INDEX_NONE || MovingIdleIndex == INDEX_NONE || IdleIndex == MovingIndex || IdleMovingIndex == MovingIdleIndex) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("live locomotion state and transition identities are incompatible"));
        const FName RuntimeActiveName = Machine->GetCurrentStateName(); const FString RuntimeActive = RuntimeActiveName.ToString(); if (RuntimeActive != TEXT("idle") && RuntimeActive != TEXT("moving")) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("live locomotion current state is unavailable")); const float IdleWeight = Machine->GetStateWeight(IdleIndex); const float MovingWeight = Machine->GetStateWeight(MovingIndex); if (!FMath::IsFinite(IdleWeight) || !FMath::IsFinite(MovingWeight) || IdleWeight < 0.0f || IdleWeight > 1.0f || MovingWeight < 0.0f || MovingWeight > 1.0f || FMath::Abs((IdleWeight + MovingWeight) - 1.0f) > 0.001f) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("live locomotion state weights are invalid")); const FString Active = MovingWeight > IdleWeight ? TEXT("moving") : TEXT("idle");
        const FFloatProperty* SpeedProperty = FindFProperty<FFloatProperty>(Instance->GetClass(), TEXT("Speed")); const float Speed = SpeedProperty ? SpeedProperty->GetPropertyValue_InContainer(Instance) : -1.0f; const float OwnerSpeed = FVector(Character->GetVelocity().X, Character->GetVelocity().Y, 0.0).Size(); if (!SpeedProperty || !FMath::IsFinite(Speed) || Speed < 0.0f || !FMath::IsFinite(OwnerSpeed)) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("live Speed binding is unavailable"));
        const bool IdleMovingActive = Machine->IsTransitionActive(IdleMovingIndex); const bool MovingIdleActive = Machine->IsTransitionActive(MovingIdleIndex); if (IdleMovingActive && MovingIdleActive) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("multiple locomotion transitions are active")); const int32 ActiveTransitionIndex = IdleMovingActive ? IdleMovingIndex : MovingIdleActive ? MovingIdleIndex : INDEX_NONE; const FString FromName = IdleMovingActive ? TEXT("idle") : MovingIdleActive ? TEXT("moving") : FString(); const FString ToName = IdleMovingActive ? TEXT("moving") : MovingIdleActive ? TEXT("idle") : FString(); const double TransitionFraction = ActiveTransitionIndex != INDEX_NONE ? Instance->GetInstanceTransitionTimeElapsedFraction(MachineIndex, ActiveTransitionIndex) : 0.0; if (ActiveTransitionIndex != INDEX_NONE && (!FMath::IsFinite(TransitionFraction) || TransitionFraction < 0.0 || TransitionFraction > 1.0)) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("live transition elapsed fraction is invalid"));
        TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("sessionId"), SessionId); Result->SetStringField(TEXT("characterId"), CharacterId); Result->SetStringField(TEXT("meshComponentId"), CharacterId + TEXT("#component:") + Mesh->GetName()); Result->SetStringField(TEXT("skeletalMeshId"), SkeletalMesh->GetPathName()); Result->SetStringField(TEXT("skeletonId"), Skeleton->GetPathName()); Result->SetStringField(TEXT("animationBlueprintId"), AnimationBlueprintId); Result->SetStringField(TEXT("animClass"), Instance->GetClass()->GetPathName()); Result->SetStringField(TEXT("animationInstanceId"), CharacterId + TEXT("#animation-instance:") + Instance->GetName()); Result->SetStringField(TEXT("stateMachineId"), StateMachineId); Result->SetNumberField(TEXT("speed"), Speed); Result->SetNumberField(TEXT("ownerPlanarSpeed"), OwnerSpeed); TArray<TSharedPtr<FJsonValue>> Weights; for (const TPair<FString, float>& Weight : {TPair<FString, float>(TEXT("idle"), IdleWeight), TPair<FString, float>(TEXT("moving"), MovingWeight)}) { TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>(); Row->SetStringField(TEXT("stateId"), P15Identity(AnimationBlueprintId, TEXT("state"), Weight.Key)); Row->SetStringField(TEXT("name"), Weight.Key); Row->SetNumberField(TEXT("weight"), Weight.Value); Weights.Add(MakeShared<FJsonValueObject>(Row)); } Result->SetArrayField(TEXT("stateWeights"), Weights); Result->SetStringField(TEXT("activeStateId"), P15Identity(AnimationBlueprintId, TEXT("state"), Active)); Result->SetStringField(TEXT("activeStateName"), Active); TSharedRef<FJsonObject> Transition = MakeShared<FJsonObject>(); Transition->SetBoolField(TEXT("active"), ActiveTransitionIndex != INDEX_NONE); if (ActiveTransitionIndex != INDEX_NONE) { Transition->SetStringField(TEXT("transitionId"), P15Identity(AnimationBlueprintId, TEXT("transition"), FromName + TEXT("->") + ToName)); Transition->SetStringField(TEXT("fromStateId"), P15Identity(AnimationBlueprintId, TEXT("state"), FromName)); Transition->SetStringField(TEXT("toStateId"), P15Identity(AnimationBlueprintId, TEXT("state"), ToName)); Transition->SetNumberField(TEXT("elapsedFraction"), TransitionFraction); } else { Transition->SetField(TEXT("transitionId"), MakeShared<FJsonValueNull>()); Transition->SetField(TEXT("fromStateId"), MakeShared<FJsonValueNull>()); Transition->SetField(TEXT("toStateId"), MakeShared<FJsonValueNull>()); Transition->SetField(TEXT("elapsedFraction"), MakeShared<FJsonValueNull>()); } Result->SetObjectField(TEXT("activeTransition"), Transition); Result->SetStringField(TEXT("revision"), Sha256(CanonicalRow({SessionId, CharacterId, SkeletalMesh->GetPathName(), Skeleton->GetPathName(), AnimationBlueprintId, Instance->GetClass()->GetPathName(), StateMachineId, FString::SanitizeFloat(Speed), FString::SanitizeFloat(OwnerSpeed), FString::SanitizeFloat(IdleWeight), FString::SanitizeFloat(MovingWeight), Active, FromName, ToName, ActiveTransitionIndex == INDEX_NONE ? FString() : FString::SanitizeFloat(TransitionFraction)}))); return SuccessResponse(Id, Result, Operation, Args);
    }
    if (Operation == TEXT("animation.character_view"))
    {
        FString BlueprintId; if (!Args->TryGetStringField(TEXT("characterBlueprintId"), BlueprintId)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("characterBlueprintId is required")); UBlueprint* Blueprint = P11LoadBlueprint(BlueprintId); if (!Blueprint || Blueprint->GetPathName() != BlueprintId) return ErrorResponse(Id, TEXT("not_found"), TEXT("Character Blueprint was not found")); ACharacter* Character = nullptr; USkeletalMeshComponent* Mesh = nullptr; if (!P15CharacterDefaults(*Blueprint, Character, Mesh)) return ErrorResponse(Id, TEXT("conflict"), TEXT("Blueprint must expose exact inherited ACharacter Mesh defaults")); const FString Revision = BlueprintContentRevision(*Blueprint); if (Revision.IsEmpty()) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("Character animation binding revision is unavailable")); if (Mesh->AnimClass.Get() && (Mesh->GetAnimationMode() != EAnimationMode::AnimationBlueprint || !P15AnimationBlueprintForClass(Mesh->AnimClass.Get()))) return ErrorResponse(Id, TEXT("conflict"), TEXT("Mesh AnimClass requires AnimationBlueprint mode and an exact Animation Blueprint")); return SuccessResponse(Id, P15CharacterResult(*Blueprint, *Mesh, false), Operation, Args);
    }
    if (Operation == TEXT("animation.character_configure"))
    {
        FString BlueprintId, SkeletalMeshId, AnimationBlueprintId; if (!Args->TryGetStringField(TEXT("characterBlueprintId"), BlueprintId) || !Args->TryGetStringField(TEXT("skeletalMeshId"), SkeletalMeshId) || !Args->TryGetStringField(TEXT("animationBlueprintId"), AnimationBlueprintId)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("characterBlueprintId, skeletalMeshId, and animationBlueprintId are required")); UBlueprint* Blueprint = P11LoadBlueprint(BlueprintId); USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(StaticLoadObject(UObject::StaticClass(), nullptr, *SkeletalMeshId, nullptr, LOAD_NoWarn)); UAnimBlueprint* AnimationBlueprint = P15LoadAnimationBlueprint(AnimationBlueprintId); if (!Blueprint || Blueprint->GetPathName() != BlueprintId || !SkeletalMesh || SkeletalMesh->GetPathName() != SkeletalMeshId || !AnimationBlueprint) return ErrorResponse(Id, TEXT("not_found"), TEXT("Character Blueprint, Skeletal Mesh, or Animation Blueprint was not found")); ACharacter* Defaults = nullptr; USkeletalMeshComponent* Mesh = nullptr; if (!P15CharacterDefaults(*Blueprint, Defaults, Mesh)) return ErrorResponse(Id, TEXT("conflict"), TEXT("Blueprint must expose exact inherited ACharacter Mesh defaults")); USkeleton* AnimSkeleton = nullptr; UAnimationGraph* AnimGraph = nullptr; UAnimGraphNode_Root* AnimRoot = nullptr; TSharedPtr<FJsonObject> CompleteMachine; if (!SkeletalMesh->GetSkeleton() || AnimationBlueprint->TargetSkeleton != SkeletalMesh->GetSkeleton() || !P15RootReadback(*AnimationBlueprint, AnimSkeleton, AnimGraph, AnimRoot) || AnimationBlueprint->Status != BS_UpToDate || !P15CompleteLocomotionReadback(*AnimationBlueprint, CompleteMachine)) return ErrorResponse(Id, TEXT("conflict"), TEXT("Skeletal Mesh and complete zero-warning Animation Blueprint must share exact Skeleton and locomotion topology")); const FString BeforeRevision = BlueprintContentRevision(*Blueprint); if (ExpectedRevision.IsEmpty()) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("expectedRevision is required")); if (ExpectedRevision != BeforeRevision) return ErrorResponse(Id, TEXT("conflict"), TEXT("Character Blueprint revision is stale; re-read animation.character_view before retrying")); USkeletalMeshComponent* ExactMesh = nullptr; if (P15CharacterBindingMatches(*Blueprint, *SkeletalMesh, *AnimationBlueprint, ExactMesh)) { const TSharedRef<FJsonObject> Result = P15CharacterResult(*Blueprint, *ExactMesh, true, false); const FString Response = P11AtomicResponse(Id, SuccessResponse(Id, Result, Operation, Args)); if (ResponseStatusIsOk(Response)) return Response; return P11FailedAtomicResponse(Id, Operation, Args, BlueprintId, BeforeRevision, BeforeRevision, true, Blueprint->GetOutermost()->IsDirty() ? TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueString>(Blueprint->GetOutermost()->GetName())} : TArray<TSharedPtr<FJsonValue>>{}, TEXT("Character animation no-op postcondition failed")); }
        const bool WasDirty = Blueprint->GetOutermost()->IsDirty(); const EBlueprintStatus BeforeStatus = Blueprint->Status; USkeletalMesh* OldSkeletalMesh = Mesh->GetSkeletalMeshAsset(); const EAnimationMode::Type OldMode = Mesh->GetAnimationMode(); UClass* OldAnimClass = Mesh->AnimClass.Get(); FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P15AnimationCharacterConfigure", "Magi AXI Configure Character Animation")); Blueprint->Modify(); Defaults->Modify(); Mesh->Modify(); Mesh->SetSkeletalMeshAsset(SkeletalMesh); Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint); Mesh->SetAnimInstanceClass(AnimationBlueprint->GeneratedClass);
        auto Restore = [&](const TCHAR* Message)
        {
            Transaction.Cancel(); ACharacter* CurrentDefaults = nullptr; USkeletalMeshComponent* CurrentMesh = nullptr; P15CharacterDefaults(*Blueprint, CurrentDefaults, CurrentMesh); for (USkeletalMeshComponent* RestoreMesh : {Mesh, CurrentMesh}) if (RestoreMesh) { RestoreMesh->SetSkeletalMeshAsset(OldSkeletalMesh); RestoreMesh->SetAnimationMode(OldMode); RestoreMesh->SetAnimInstanceClass(OldAnimClass); } FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint); FCompilerResultsLog RestoreResults; RestoreResults.bSilentMode = true; FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave, &RestoreResults); FAssetCompilingManager::Get().FinishAllCompilation(); ACharacter* RestoredDefaults = nullptr; USkeletalMeshComponent* RestoredMesh = nullptr; const bool HasRestoredDefaults = P15CharacterDefaults(*Blueprint, RestoredDefaults, RestoredMesh); const bool Verified = P11RollbackVerificationResult(RestoreResults.NumErrors == 0 && RestoreResults.NumWarnings == 0 && HasRestoredDefaults && RestoredMesh->GetSkeletalMeshAsset() == OldSkeletalMesh && RestoredMesh->GetAnimationMode() == OldMode && RestoredMesh->AnimClass.Get() == OldAnimClass); Blueprint->Status = BeforeStatus; Blueprint->GetOutermost()->SetDirtyFlag(WasDirty); const bool Exact = Verified && BlueprintContentRevision(*Blueprint) == BeforeRevision; return P11FailedAtomicResponse(Id, Operation, Args, BlueprintId, BeforeRevision, BlueprintContentRevision(*Blueprint), Exact, Blueprint->GetOutermost()->IsDirty() ? TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueString>(Blueprint->GetOutermost()->GetName())} : TArray<TSharedPtr<FJsonValue>>{}, Exact ? Message : TEXT("Character animation binding rollback verification failed"));
        };
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint); FCompilerResultsLog Results; Results.bSilentMode = true; FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipSave, &Results); FAssetCompilingManager::Get().FinishAllCompilation(); USkeletalMeshComponent* CurrentMesh = nullptr; const bool Valid = Results.NumErrors == 0 && Results.NumWarnings == 0 && Blueprint->Status == BS_UpToDate && P15CharacterBindingMatches(*Blueprint, *SkeletalMesh, *AnimationBlueprint, CurrentMesh) && BlueprintContentRevision(*Blueprint) != BeforeRevision; if (!Valid) return Restore(TEXT("Character animation binding compile or exact readback failed; mutation rolled back")); const TSharedRef<FJsonObject> Result = P15CharacterResult(*Blueprint, *CurrentMesh, true, true); const FString Response = P11AtomicResponse(Id, SuccessResponse(Id, Result, Operation, Args)); if (ResponseStatusIsOk(Response)) return Response; return Restore(TEXT("Character animation binding postcondition failed; mutation rolled back"));
    }
    if (Operation == TEXT("animation.state_ensure"))
    {
        FString BlueprintId, MachineId, Name, SequenceId; if (!Args->TryGetStringField(TEXT("animationBlueprintId"), BlueprintId) || !Args->TryGetStringField(TEXT("stateMachineId"), MachineId) || !Args->TryGetStringField(TEXT("name"), Name) || !Args->TryGetStringField(TEXT("sequenceId"), SequenceId)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("animation state fields are required"));
        UAnimBlueprint* Blueprint = P15LoadAnimationBlueprint(BlueprintId); UAnimSequence* Sequence = Cast<UAnimSequence>(StaticLoadObject(UObject::StaticClass(), nullptr, *SequenceId, nullptr, LOAD_NoWarn)); if (!Blueprint || !Sequence) return ErrorResponse(Id, TEXT("not_found"), TEXT("Animation Blueprint or sequence was not found"));
        if (MachineId != P15Identity(BlueprintId, TEXT("state-machine"), TEXT("locomotion")) || Sequence->GetSkeleton() != Blueprint->TargetSkeleton) return ErrorResponse(Id, TEXT("conflict"), TEXT("state request does not match locomotion contract")); FString RevisionError = P15Expected(ExpectedRevision, *Blueprint); if (!RevisionError.IsEmpty()) return ErrorResponse(Id, ExpectedRevision.IsEmpty() ? TEXT("invalid_input") : TEXT("conflict"), *RevisionError);
        USkeleton* RootSkeleton = nullptr; UAnimationGraph* RootGraph = nullptr; UAnimGraphNode_Root* RootNode = nullptr; if (!P15RootReadback(*Blueprint, RootSkeleton, RootGraph, RootNode) || !P15MutationBaselineValid(*Blueprint)) return ErrorResponse(Id, TEXT("conflict"), TEXT("Animation Blueprint root or compiler state is incompatible"));
        UAnimGraphNode_StateMachine* Machine = RootGraph ? P15OnlyNode<UAnimGraphNode_StateMachine>(*RootGraph) : nullptr; UAnimationStateMachineGraph* SM = Machine ? Machine->EditorStateMachineGraph : nullptr; if (!SM) return ErrorResponse(Id, TEXT("conflict"), TEXT("locomotion state machine is required"));
        const FString BeforeRevision = BlueprintContentRevision(*Blueprint); const FP15PinGuidSnapshot PinGuids = P15CapturePinGuids(*Blueprint); const bool WasDirty = Blueprint->GetOutermost()->IsDirty(); const EBlueprintStatus BeforeStatus = Blueprint->Status; UAnimStateNode* BeforeState = P15State(*SM, Name); UAnimStateNode* BeforeInitial = SM->EntryNode ? Cast<UAnimStateNode>(SM->EntryNode->GetOutputNode()) : nullptr; UAnimStateNode* State = nullptr; bool Changed = false; FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P15AnimationStateEnsure", "Magi AXI Ensure Animation State"));
        auto Rollback = [&](const TCHAR* Message) { if (!BeforeState && State) State->DestroyNode(); if (SM->EntryNode && SM->EntryNode->GetOutputNode() != BeforeInitial) { SM->EntryNode->BreakAllNodeLinks(); if (BeforeInitial) SM->GetSchema()->TryCreateConnection(SM->EntryNode->GetOutputPin(), BeforeInitial->GetInputPin()); } SM->NotifyGraphChanged(); Transaction.Cancel(); const bool Verified = P15RestoreBlueprint(*Blueprint, BeforeRevision, WasDirty, BeforeStatus, PinGuids, BeforeStatus == BS_UpToDateWithWarnings); return P11FailedAtomicResponse(Id, Operation, Args, BlueprintId, BeforeRevision, BlueprintContentRevision(*Blueprint), Verified, P15Dirty(*Blueprint, false), Verified ? Message : TEXT("animation state rollback verification failed")); };
        if (!P15EnsureState(*Blueprint, *SM, Name, *Sequence, Changed, State)) return (!BeforeState && State) || BlueprintContentRevision(*Blueprint) != BeforeRevision ? Rollback(TEXT("state authoring failed; mutation rolled back")) : ErrorResponse(Id, TEXT("conflict"), TEXT("state topology is incompatible"));
        if (Changed && !P15CompileAndReadback(*Blueprint)) return Rollback(TEXT("state compile or exact readback failed; mutation rolled back"));
        TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("animationBlueprintId"), BlueprintId); Result->SetStringField(TEXT("stateMachineId"), MachineId); Result->SetStringField(TEXT("stateId"), P15Identity(BlueprintId, TEXT("state"), Name)); Result->SetStringField(TEXT("stateGraphId"), P15Identity(BlueprintId, TEXT("state-graph"), Name)); Result->SetStringField(TEXT("resultNodeId"), P15Identity(BlueprintId, TEXT("state-result"), Name)); Result->SetStringField(TEXT("sequencePlayerNodeId"), P15Identity(BlueprintId, TEXT("state-player"), Name)); Result->SetStringField(TEXT("name"), Name); Result->SetStringField(TEXT("sequenceId"), SequenceId); Result->SetStringField(TEXT("skeletonId"), Blueprint->TargetSkeleton->GetPathName()); Result->SetBoolField(TEXT("initial"), Name == TEXT("idle")); Result->SetBoolField(TEXT("changed"), Changed); Result->SetArrayField(TEXT("dirtyPackages"), P15Dirty(*Blueprint, Changed)); Result->SetArrayField(TEXT("savedPackages"), {}); Result->SetStringField(TEXT("revision"), BlueprintContentRevision(*Blueprint)); const FString Response = P11AtomicResponse(Id, SuccessResponse(Id, Result, Operation, Args)); if (ResponseStatusIsOk(Response)) return Response; return Changed ? Rollback(TEXT("state postcondition failed; mutation rolled back")) : P11FailedAtomicResponse(Id, Operation, Args, BlueprintId, BeforeRevision, BeforeRevision, true, P15Dirty(*Blueprint, false), TEXT("state no-op postcondition failed"));
    }
    if (Operation == TEXT("animation.state_machine_ensure"))
    {
        FString BlueprintId, Name; if (!Args->TryGetStringField(TEXT("animationBlueprintId"), BlueprintId) || !Args->TryGetStringField(TEXT("name"), Name)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("animationBlueprintId and name are required"));
        if (Name != TEXT("locomotion")) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("state machine name must be locomotion")); UAnimBlueprint* Blueprint = P15LoadAnimationBlueprint(BlueprintId); if (!Blueprint) return ErrorResponse(Id, TEXT("not_found"), TEXT("Animation Blueprint was not found")); FString RevisionError = P15Expected(ExpectedRevision, *Blueprint); if (!RevisionError.IsEmpty()) return ErrorResponse(Id, ExpectedRevision.IsEmpty() ? TEXT("invalid_input") : TEXT("conflict"), *RevisionError);
        USkeleton* RootSkeleton = nullptr; UAnimationGraph* RootGraph = nullptr; UAnimGraphNode_Root* RootNode = nullptr; if (!P15RootReadback(*Blueprint, RootSkeleton, RootGraph, RootNode) || !P15MutationBaselineValid(*Blueprint)) return ErrorResponse(Id, TEXT("conflict"), TEXT("Animation Blueprint root or compiler state is incompatible"));
        const FString BeforeRevision = BlueprintContentRevision(*Blueprint); const FP15PinGuidSnapshot PinGuids = P15CapturePinGuids(*Blueprint); const bool WasDirty = Blueprint->GetOutermost()->IsDirty(); const EBlueprintStatus BeforeStatus = Blueprint->Status; UAnimGraphNode_StateMachine* BeforeMachine = P15OnlyNode<UAnimGraphNode_StateMachine>(*RootGraph); UAnimGraphNode_StateMachine* Machine = nullptr; UAnimationStateMachineGraph* Graph = nullptr; FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P15AnimationStateMachineEnsure", "Magi AXI Ensure Animation State Machine"));
        auto Rollback = [&](const TCHAR* Message) { if (!BeforeMachine) { UAnimGraphNode_StateMachine* Authored = Machine ? Machine : P15OnlyNode<UAnimGraphNode_StateMachine>(*RootGraph); if (Authored) Authored->DestroyNode(); } RootGraph->NotifyGraphChanged(); Transaction.Cancel(); const bool Verified = P15RestoreBlueprint(*Blueprint, BeforeRevision, WasDirty, BeforeStatus, PinGuids); return P11FailedAtomicResponse(Id, Operation, Args, BlueprintId, BeforeRevision, BlueprintContentRevision(*Blueprint), Verified, P15Dirty(*Blueprint, false), Verified ? Message : TEXT("state machine rollback verification failed")); };
        const bool Changed = P15EnsureMachine(*Blueprint, Machine, Graph); if (!Machine || !Graph) return BlueprintContentRevision(*Blueprint) != BeforeRevision || Blueprint->GetOutermost()->IsDirty() != WasDirty || Blueprint->Status != BeforeStatus ? Rollback(TEXT("state machine authoring failed; mutation rolled back")) : ErrorResponse(Id, TEXT("conflict"), TEXT("locomotion state-machine topology is incompatible"));
        if (Changed && !P15CompileAndReadback(*Blueprint, true)) return Rollback(TEXT("state machine compile or exact readback failed; mutation rolled back"));
        TSharedRef<FJsonObject> Result = P15SimpleResult(*Blueprint, Changed); Result->SetStringField(TEXT("stateMachineId"), P15Identity(BlueprintId, TEXT("state-machine"), Name)); Result->SetStringField(TEXT("stateMachineGraphId"), P15Identity(BlueprintId, TEXT("state-machine-graph"), Name)); Result->SetStringField(TEXT("entryNodeId"), P15Identity(BlueprintId, TEXT("state-machine-entry"), Name)); Result->SetStringField(TEXT("name"), Name); const FString Response = P11AtomicResponse(Id, SuccessResponse(Id, Result, Operation, Args)); if (ResponseStatusIsOk(Response)) return Response; return Changed ? Rollback(TEXT("state machine postcondition failed; mutation rolled back")) : P11FailedAtomicResponse(Id, Operation, Args, BlueprintId, BeforeRevision, BeforeRevision, true, P15Dirty(*Blueprint, false), TEXT("state machine no-op postcondition failed"));
    }
    if (Operation == TEXT("animation.transition_ensure"))
    {
        FString BlueprintId, MachineId, FromId, ToId, Expression; if (!Args->TryGetStringField(TEXT("animationBlueprintId"), BlueprintId) || !Args->TryGetStringField(TEXT("stateMachineId"), MachineId) || !Args->TryGetStringField(TEXT("fromStateId"), FromId) || !Args->TryGetStringField(TEXT("toStateId"), ToId) || !Args->TryGetStringField(TEXT("expression"), Expression)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("animation transition fields are required"));
        UAnimBlueprint* Blueprint = P15LoadAnimationBlueprint(BlueprintId); if (!Blueprint) return ErrorResponse(Id, TEXT("not_found"), TEXT("Animation Blueprint was not found")); FString RevisionError = P15Expected(ExpectedRevision, *Blueprint); if (!RevisionError.IsEmpty()) return ErrorResponse(Id, ExpectedRevision.IsEmpty() ? TEXT("invalid_input") : TEXT("conflict"), *RevisionError);
        const bool IdleMoving = FromId == P15Identity(BlueprintId, TEXT("state"), TEXT("idle")) && ToId == P15Identity(BlueprintId, TEXT("state"), TEXT("moving")) && Expression == TEXT("Speed > 10"); const bool MovingIdle = FromId == P15Identity(BlueprintId, TEXT("state"), TEXT("moving")) && ToId == P15Identity(BlueprintId, TEXT("state"), TEXT("idle")) && Expression == TEXT("Speed <= 10"); if (MachineId != P15Identity(BlueprintId, TEXT("state-machine"), TEXT("locomotion")) || (!IdleMoving && !MovingIdle)) return ErrorResponse(Id, TEXT("conflict"), TEXT("transition does not match locomotion Speed contract"));
        UEdGraph* UpdateGraph = P15UpdateGraph(*Blueprint); TArray<UEdGraphNode*> SpeedNodes; if (!UpdateGraph || !P15VariableReadback(*Blueprint, *UpdateGraph, SpeedNodes)) return ErrorResponse(Id, TEXT("conflict"), TEXT("exact Speed binding is required before transitions"));
        USkeleton* RootSkeleton = nullptr; UAnimationGraph* RootGraph = nullptr; UAnimGraphNode_Root* RootNode = nullptr; if (!P15RootReadback(*Blueprint, RootSkeleton, RootGraph, RootNode) || !P15MutationBaselineValid(*Blueprint)) return ErrorResponse(Id, TEXT("conflict"), TEXT("Animation Blueprint root or compiler state is incompatible")); UAnimGraphNode_StateMachine* Machine = RootGraph ? P15OnlyNode<UAnimGraphNode_StateMachine>(*RootGraph) : nullptr; UAnimationStateMachineGraph* SM = Machine ? Machine->EditorStateMachineGraph : nullptr; UAnimStateNode* From = SM ? P15State(*SM, IdleMoving ? TEXT("idle") : TEXT("moving")) : nullptr; UAnimStateNode* To = SM ? P15State(*SM, IdleMoving ? TEXT("moving") : TEXT("idle")) : nullptr; if (!SM || !From || !To) return ErrorResponse(Id, TEXT("conflict"), TEXT("both transition states are required"));
        const FString BeforeRevision = BlueprintContentRevision(*Blueprint); const FP15PinGuidSnapshot PinGuids = P15CapturePinGuids(*Blueprint); const bool WasDirty = Blueprint->GetOutermost()->IsDirty(); const EBlueprintStatus BeforeStatus = Blueprint->Status; UAnimStateTransitionNode* BeforeTransition = P15Transition(*SM, *From, *To); UAnimStateTransitionNode* Transition = nullptr; bool Changed = false; FScopedTransaction Transaction(NSLOCTEXT("MagiUnrealAXI", "P15AnimationTransitionEnsure", "Magi AXI Ensure Animation Transition"));
        auto Rollback = [&](const TCHAR* Message) { if (!BeforeTransition && Transition) Transition->DestroyNode(); SM->NotifyGraphChanged(); Transaction.Cancel(); const bool Verified = P15RestoreBlueprint(*Blueprint, BeforeRevision, WasDirty, BeforeStatus, PinGuids); return P11FailedAtomicResponse(Id, Operation, Args, BlueprintId, BeforeRevision, BlueprintContentRevision(*Blueprint), Verified, P15Dirty(*Blueprint, false), Verified ? Message : TEXT("animation transition rollback verification failed")); };
        if (!P15EnsureTransition(*Blueprint, *SM, *From, *To, Expression, Changed, Transition)) return BlueprintContentRevision(*Blueprint) != BeforeRevision || Blueprint->GetOutermost()->IsDirty() != WasDirty || Blueprint->Status != BeforeStatus ? Rollback(TEXT("transition authoring failed; mutation rolled back")) : ErrorResponse(Id, TEXT("conflict"), TEXT("transition topology is incompatible")); if (Changed && !P15CompileAndReadback(*Blueprint)) return Rollback(TEXT("transition compile or exact readback failed; mutation rolled back"));
        const FString Prefix = BlueprintId + TEXT("#transition:") + From->GetStateName() + TEXT("->") + To->GetStateName(); TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("animationBlueprintId"), BlueprintId); Result->SetStringField(TEXT("stateMachineId"), MachineId); Result->SetStringField(TEXT("transitionId"), P15Identity(BlueprintId, TEXT("transition"), From->GetStateName() + TEXT("->") + To->GetStateName())); Result->SetStringField(TEXT("transitionGraphId"), P15Identity(BlueprintId, TEXT("transition-graph"), Prefix)); Result->SetStringField(TEXT("resultNodeId"), P15Identity(BlueprintId, TEXT("transition-result"), Prefix)); Result->SetStringField(TEXT("variableGetterNodeId"), P15Identity(BlueprintId, TEXT("transition-getter"), Prefix)); Result->SetStringField(TEXT("comparisonNodeId"), P15Identity(BlueprintId, TEXT("transition-comparison"), Prefix)); Result->SetStringField(TEXT("fromStateId"), FromId); Result->SetStringField(TEXT("toStateId"), ToId); Result->SetStringField(TEXT("expression"), Expression); Result->SetBoolField(TEXT("changed"), Changed); Result->SetArrayField(TEXT("dirtyPackages"), P15Dirty(*Blueprint, Changed)); Result->SetArrayField(TEXT("savedPackages"), {}); Result->SetStringField(TEXT("revision"), BlueprintContentRevision(*Blueprint)); const FString Response = P11AtomicResponse(Id, SuccessResponse(Id, Result, Operation, Args)); if (ResponseStatusIsOk(Response)) return Response; return Changed ? Rollback(TEXT("transition postcondition failed; mutation rolled back")) : P11FailedAtomicResponse(Id, Operation, Args, BlueprintId, BeforeRevision, BeforeRevision, true, P15Dirty(*Blueprint, false), TEXT("transition no-op postcondition failed"));
    }
    if (!Args.IsValid()) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("animation operation requires arguments"));
    if (Operation == TEXT("animation.graph_view"))
    {
        FString BlueprintId;
        if (!Args->TryGetStringField(TEXT("animationBlueprintId"), BlueprintId)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("animationBlueprintId is required"));
        UAnimBlueprint* Blueprint = P15LoadAnimationBlueprint(BlueprintId);
        if (!Blueprint) return ErrorResponse(Id, TEXT("not_found"), TEXT("Animation Blueprint was not found"));
        USkeleton* Skeleton = nullptr; UAnimationGraph* Graph = nullptr; UAnimGraphNode_Root* Root = nullptr;
        if (!P15RootReadback(*Blueprint, Skeleton, Graph, Root)) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("Animation Blueprint root readback failed"));
        if (P15UpdateGraph(*Blueprint)) { TArray<UEdGraphNode*> Nodes; bool HasSpeed = false; for (const FBPVariableDescription& Variable : Blueprint->NewVariables) if (Variable.VarName == FName(TEXT("Speed"))) HasSpeed = true; if (HasSpeed && !P15VariableReadback(*Blueprint, *P15UpdateGraph(*Blueprint), Nodes)) return ErrorResponse(Id, TEXT("conflict"), TEXT("Speed variable topology readback is incompatible")); }
        bool ReadbackValid = false; TSharedRef<FJsonObject> Result = P15AnimationResult(*Blueprint, *Skeleton, *Graph, *Root, false, true, &ReadbackValid); if (!ReadbackValid) return ErrorResponse(Id, TEXT("operation_failed"), TEXT("Animation Blueprint semantic graph readback failed")); return SuccessResponse(Id, Result, Operation, Args);
    }
    if (Operation == TEXT("animation.variable_ensure"))
    {
        FString BlueprintId, Name, Type, Source;
        if (!Args->TryGetStringField(TEXT("animationBlueprintId"), BlueprintId) || !Args->TryGetStringField(TEXT("name"), Name) || !Args->TryGetStringField(TEXT("type"), Type) || !Args->TryGetStringField(TEXT("source"), Source)) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("animationBlueprintId, name, type, and source are required"));
        if (Name != TEXT("Speed") || Type != TEXT("float") || Source != TEXT("owner_planar_speed")) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("variable contract requires Speed float owner_planar_speed"));
        UAnimBlueprint* Blueprint = P15LoadAnimationBlueprint(BlueprintId); if (!Blueprint) return ErrorResponse(Id, TEXT("not_found"), TEXT("Animation Blueprint was not found"));
        const FString BeforeRevision = BlueprintContentRevision(*Blueprint); if (ExpectedRevision.IsEmpty()) return ErrorResponse(Id, TEXT("invalid_input"), TEXT("expectedRevision is required")); if (ExpectedRevision != BeforeRevision) return ErrorResponse(Id, TEXT("conflict"), TEXT("Blueprint revision is stale"));
        USkeleton* ReadbackSkeleton = nullptr; UAnimationGraph* ReadbackGraph = nullptr; UAnimGraphNode_Root* ReadbackRoot = nullptr; if (!P15RootReadback(*Blueprint, ReadbackSkeleton, ReadbackGraph, ReadbackRoot) || !P15MutationBaselineValid(*Blueprint)) return ErrorResponse(Id, TEXT("conflict"), TEXT("Animation Blueprint root or compiler state is incompatible"));
        if (ReadbackSkeleton != Blueprint->TargetSkeleton || !ReadbackGraph || !ReadbackRoot) return ErrorResponse(Id, TEXT("conflict"), TEXT("Animation Blueprint root topology is incompatible"));
        UEdGraph* UpdateGraph = P15UpdateGraph(*Blueprint); if (!UpdateGraph) return ErrorResponse(Id, TEXT("conflict"), TEXT("Animation Blueprint must have exactly one update graph"));
        TArray<UEdGraphNode*> Existing; bool Changed = false; const FP15PinGuidSnapshot PinGuids = P15CapturePinGuids(*Blueprint); const bool WasDirty = Blueprint->GetOutermost()->IsDirty(); const EBlueprintStatus OldStatus = Blueprint->Status; const TArray<FBPVariableDescription> OldVariables = Blueprint->NewVariables; TArray<UEdGraphNode*> Authored; TUniquePtr<FScopedTransaction> Transaction;
        auto Rollback = [&](const TCHAR* Message) { for (int32 Index = Authored.Num() - 1; Index >= 0; --Index) if (UEdGraphNode* Node = Authored[Index]; Node && UpdateGraph->Nodes.Contains(Node)) { Node->BreakAllNodeLinks(); UpdateGraph->RemoveNode(Node); } Blueprint->NewVariables = OldVariables; UpdateGraph->NotifyGraphChanged(); if (Transaction) Transaction->Cancel(); const bool Verified = P15RestoreBlueprint(*Blueprint, BeforeRevision, WasDirty, OldStatus, PinGuids, OldStatus == BS_UpToDateWithWarnings); return P11FailedAtomicResponse(Id, Operation, Args, BlueprintId, BeforeRevision, BlueprintContentRevision(*Blueprint), Verified, P15Dirty(*Blueprint, false), Verified ? Message : TEXT("Speed variable rollback verification failed")); };
        if (!P15VariableReadback(*Blueprint, *UpdateGraph, Existing))
        {
            Transaction = MakeUnique<FScopedTransaction>(NSLOCTEXT("MagiUnrealAXI", "P15AnimationVariableEnsure", "Magi AXI Ensure Animation Speed Variable"));
            if (!P15EnsureVariable(*Blueprint, *UpdateGraph, Changed, Authored)) return Changed || !Authored.IsEmpty() || BlueprintContentRevision(*Blueprint) != BeforeRevision ? Rollback(TEXT("Speed variable authoring failed; mutation rolled back")) : ErrorResponse(Id, TEXT("conflict"), TEXT("Speed owner planar speed topology is incompatible"));
            if (!P15CompileAndReadback(*Blueprint) || BlueprintContentRevision(*Blueprint) == BeforeRevision) return Rollback(TEXT("Speed topology compile or exact readback failed; mutation rolled back"));
        }
        const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(TEXT("animationBlueprintId"), BlueprintId); Result->SetStringField(TEXT("variableId"), P15Identity(BlueprintId, TEXT("variable"), Name)); Result->SetStringField(TEXT("bindingId"), P15Identity(BlueprintId, TEXT("binding"), Name)); Result->SetStringField(TEXT("name"), Name); Result->SetStringField(TEXT("type"), Type); Result->SetStringField(TEXT("source"), Source); Result->SetStringField(TEXT("updateGraphId"), P15Identity(BlueprintId, TEXT("update-graph"), Name)); Result->SetStringField(TEXT("eventNodeId"), P15Identity(BlueprintId, TEXT("event"), Name)); Result->SetStringField(TEXT("ownerNodeId"), P15Identity(BlueprintId, TEXT("owner"), Name)); Result->SetStringField(TEXT("velocityNodeId"), P15Identity(BlueprintId, TEXT("velocity"), Name)); Result->SetStringField(TEXT("planarSpeedNodeId"), P15Identity(BlueprintId, TEXT("planar-speed"), Name)); Result->SetStringField(TEXT("setterNodeId"), P15Identity(BlueprintId, TEXT("setter"), Name)); Result->SetBoolField(TEXT("changed"), Changed); Result->SetArrayField(TEXT("dirtyPackages"), P15Dirty(*Blueprint, Changed)); Result->SetArrayField(TEXT("savedPackages"), {}); Result->SetStringField(TEXT("revision"), BlueprintContentRevision(*Blueprint)); const FString Response = P11AtomicResponse(Id, SuccessResponse(Id, Result, Operation, Args)); if (ResponseStatusIsOk(Response)) return Response; return Changed ? Rollback(TEXT("Speed variable postcondition failed; mutation rolled back")) : P11FailedAtomicResponse(Id, Operation, Args, BlueprintId, BeforeRevision, BeforeRevision, true, P15Dirty(*Blueprint, false), TEXT("Speed variable no-op postcondition failed"));
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
    TestTrue(TEXT("bridge advertises complete state-aware P1.5 slice"), P15Operations == 9 && AvailableOperations == 8 && PendingOperations == 0);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiP15PublicGraphAuthoringPersistence, "MagiUnrealAXI.P15.PublicGraphAuthoringPersistence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiP15PublicGraphAuthoringPersistence::RunTest(const FString&)
{
    using namespace MagiP15AnimationSpike;
    constexpr TCHAR PackagePath[] = TEXT("/Game/MagiP15Graph/ABP_MagiP15Graph"); constexpr TCHAR BlueprintId[] = TEXT("/Game/MagiP15Graph/ABP_MagiP15Graph.ABP_MagiP15Graph");
    auto ParsedResult = [](const FString& Response) { TSharedPtr<FJsonObject> Envelope; const TSharedPtr<FJsonObject>* Result = nullptr; return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response), Envelope) && Envelope && Envelope->TryGetObjectField(TEXT("result"), Result) && Result ? *Result : TSharedPtr<FJsonObject>(); };
    auto Execute = [&](const FString& Suffix, const FString& Operation, const TSharedRef<FJsonObject>& Args, const FString& Revision)
    {
        const FString Response = ReadResponseOnGameThread(TEXT("p15-graph-") + Suffix, Operation, Args, Revision); FlushAsyncLoading(); FAssetCompilingManager::Get().FinishAllCompilation(); if (!ResponseStatusIsOk(Response)) AddInfo(Operation + TEXT(": ") + Response); TestTrue(*FString::Printf(TEXT("%s succeeds"), *Operation), ResponseStatusIsOk(Response)); return ParsedResult(Response);
    };
    struct FDirectSnapshot
    {
        FString Revision;
        bool Dirty = false;
        EBlueprintStatus Status = BS_Unknown;
        TArray<FString> Structure;
        TArray<FString> Pins;
    };
    auto DirectSnapshot = [&]()
    {
        TSharedPtr<FDirectSnapshot> Snapshot = MakeShared<FDirectSnapshot>(); UAnimBlueprint* Blueprint = P15LoadAnimationBlueprint(BlueprintId); if (!Blueprint || !Blueprint->GetOutermost()) return TSharedPtr<FDirectSnapshot>(); Snapshot->Revision = BlueprintContentRevision(*Blueprint); Snapshot->Dirty = Blueprint->GetOutermost()->IsDirty(); Snapshot->Status = Blueprint->Status; TArray<UEdGraph*> Graphs; Blueprint->GetAllGraphs(Graphs);
        for (UEdGraph* Graph : Graphs) if (Graph)
        {
            Snapshot->Structure.Add(TEXT("graph|") + Graph->GetPathName() + TEXT("|") + Graph->GetClass()->GetPathName() + TEXT("|") + P15Guid(Graph->GraphGuid) + TEXT("|") + (Graph->GetOuter() ? Graph->GetOuter()->GetPathName() : FString()));
            for (UEdGraphNode* Node : Graph->Nodes) if (Node)
            {
                Snapshot->Structure.Add(TEXT("node|") + Graph->GetPathName() + TEXT("|") + Node->GetPathName() + TEXT("|") + Node->GetClass()->GetPathName() + TEXT("|") + P15Guid(Node->NodeGuid));
                for (int32 Index = 0; Index < Node->Pins.Num(); ++Index) if (UEdGraphPin* Pin = Node->Pins[Index]) Snapshot->Pins.Add(P15PinSnapshotKey(*Graph, *Node, *Pin, Index) + TEXT("|") + P15Guid(Pin->PinId));
            }
        }
        Snapshot->Structure.Sort(); Snapshot->Pins.Sort(); return Snapshot;
    };
    auto DirectEqual = [](const FDirectSnapshot& Before, const FDirectSnapshot& After) { return Before.Revision == After.Revision && Before.Dirty == After.Dirty && Before.Status == After.Status && Before.Structure == After.Structure && Before.Pins == After.Pins; };
    auto VerifyRejected = [&](const FString& Suffix, const FString& Operation, const TSharedRef<FJsonObject>& Args, const FString& RequestRevision, const EP15FailureStage Stage = EP15FailureStage::None, const bool ForceAtomic = false, const bool ForceVerificationFailure = false)
    {
        const TSharedPtr<FDirectSnapshot> DirectBefore = DirectSnapshot(); TSharedRef<FJsonObject> ViewArgs = MakeShared<FJsonObject>(); ViewArgs->SetStringField(TEXT("animationBlueprintId"), BlueprintId); const TSharedPtr<FJsonObject> Before = ParsedResult(ReadResponseOnGameThread(TEXT("p15-graph-before-") + Suffix, TEXT("animation.graph_view"), ViewArgs, FString())); if (!Before || !DirectBefore) return false; const FString BeforeText = Serialize(Before.ToSharedRef());
        GP15FailureStage = Stage; GP11ForceAtomicFailure = ForceAtomic; GP11ForceRollbackVerificationFailure = ForceVerificationFailure; const FString Response = ReadResponseOnGameThread(TEXT("p15-graph-rejected-") + Suffix, Operation, Args, RequestRevision); GP15FailureStage = EP15FailureStage::None; GP11ForceAtomicFailure = false; GP11ForceRollbackVerificationFailure = false; FlushAsyncLoading(); FAssetCompilingManager::Get().FinishAllCompilation(); const TSharedPtr<FDirectSnapshot> DirectAfter = DirectSnapshot(); TSharedPtr<FJsonObject> Envelope; const bool Parsed = FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response), Envelope) && Envelope; const bool Injected = Stage != EP15FailureStage::None || ForceAtomic || ForceVerificationFailure; const bool Unknown = Stage == EP15FailureStage::RollbackCompile || Stage == EP15FailureStage::RollbackReadback || ForceVerificationFailure; const TSharedPtr<FJsonObject>* Receipt = nullptr; const TSharedPtr<FJsonObject>* Verification = nullptr; const bool ReceiptVerified = !Injected || (Parsed && Envelope->TryGetObjectField(TEXT("receipt"), Receipt) && Receipt && Receipt->IsValid() && (*Receipt)->GetStringField(TEXT("state")) == (Unknown ? TEXT("outcome_unknown") : TEXT("failed")) && (*Receipt)->TryGetObjectField(TEXT("verification"), Verification) && Verification && Verification->IsValid() && (*Verification)->GetBoolField(TEXT("matched")) == !Unknown && (*Verification)->GetStringField(TEXT("beforeRevision")) == Before->GetStringField(TEXT("revision")) && (*Verification)->GetStringField(TEXT("observedRevision")) == Before->GetStringField(TEXT("revision")));
        const TSharedPtr<FJsonObject> After = ParsedResult(ReadResponseOnGameThread(TEXT("p15-graph-after-") + Suffix, TEXT("animation.graph_view"), ViewArgs, FString())); const bool Rejected = Parsed && Envelope->GetStringField(TEXT("status")) == TEXT("error") && !ResponseStatusIsOk(Response); const bool SemanticPreserved = After && Serialize(After.ToSharedRef()) == BeforeText; const bool DirectPreserved = DirectAfter && DirectEqual(*DirectBefore, *DirectAfter); if (!Rejected || !SemanticPreserved || !DirectPreserved || !ReceiptVerified) AddInfo(TEXT("rejected response: ") + Response + TEXT("\nbefore: ") + BeforeText + TEXT("\nafter: ") + (After ? Serialize(After.ToSharedRef()) : TEXT("<missing>"))); TestTrue(*FString::Printf(TEXT("%s %s is rejected"), *Operation, Injected ? TEXT("fault injection") : TEXT("stale revision")), Rejected); TestTrue(*FString::Printf(TEXT("%s rejection preserves exact semantic graph"), *Operation), SemanticPreserved); TestTrue(*FString::Printf(TEXT("%s rejection independently preserves status, dirty state, graph membership, and pin GUIDs"), *Operation), DirectPreserved); TestTrue(*FString::Printf(TEXT("%s failure receipt has authoritative rollback state"), *Operation), ReceiptVerified); return Rejected && SemanticPreserved && DirectPreserved && ReceiptVerified;
    };
    const bool Persisted = FPackageName::DoesPackageExist(PackagePath); const FString OraclePath = FPaths::ProjectSavedDir() / TEXT("Automation/MagiP15PublicGraphOracle.json"); FString OracleBefore; if (Persisted) TestTrue(TEXT("graph oracle exists after restart"), FFileHelper::LoadFileToString(OracleBefore, *OraclePath));
    auto VerifyFaultMatrix = [&](const FString& Suffix, const FString& Operation, const TSharedRef<FJsonObject>& Args, const FString& RequestRevision)
    {
        if (Persisted) return VerifyRejected(Suffix + TEXT("-noop-postcondition"), Operation, Args, RequestRevision, EP15FailureStage::None, true);
        return VerifyRejected(Suffix + TEXT("-helper"), Operation, Args, RequestRevision, EP15FailureStage::Helper) &&
            VerifyRejected(Suffix + TEXT("-compile"), Operation, Args, RequestRevision, EP15FailureStage::Compile) &&
            VerifyRejected(Suffix + TEXT("-readback"), Operation, Args, RequestRevision, EP15FailureStage::Readback) &&
            VerifyRejected(Suffix + TEXT("-rollback-compile"), Operation, Args, RequestRevision, EP15FailureStage::RollbackCompile, true) &&
            VerifyRejected(Suffix + TEXT("-rollback-readback"), Operation, Args, RequestRevision, EP15FailureStage::RollbackReadback, true) &&
            VerifyRejected(Suffix + TEXT("-rollback-verifier"), Operation, Args, RequestRevision, EP15FailureStage::None, true, true);
    };
    TSharedRef<FJsonObject> CreateArgs = MakeShared<FJsonObject>(); CreateArgs->SetStringField(TEXT("path"), PackagePath); CreateArgs->SetStringField(TEXT("skeletonId"), SkeletonPath); TSharedPtr<FJsonObject> Current = Execute(TEXT("create"), TEXT("animation_blueprint.create"), CreateArgs, FString()); if (!Current) return false; FString Revision = Current->GetStringField(TEXT("revision"));
    TSharedRef<FJsonObject> VariableArgs = MakeShared<FJsonObject>(); VariableArgs->SetStringField(TEXT("animationBlueprintId"), BlueprintId); VariableArgs->SetStringField(TEXT("name"), TEXT("Speed")); VariableArgs->SetStringField(TEXT("type"), TEXT("float")); VariableArgs->SetStringField(TEXT("source"), TEXT("owner_planar_speed")); FString PreviousRevision = Revision; if (!VerifyFaultMatrix(TEXT("variable"), TEXT("animation.variable_ensure"), VariableArgs, Revision)) return false; Current = Execute(TEXT("variable"), TEXT("animation.variable_ensure"), VariableArgs, Revision); if (!Current) return false; TestEqual(TEXT("variable exact changed state"), Current->GetBoolField(TEXT("changed")), !Persisted); Revision = Current->GetStringField(TEXT("revision")); if (!Persisted && !VerifyRejected(TEXT("variable-stale"), TEXT("animation.variable_ensure"), VariableArgs, PreviousRevision)) return false;
    const FString MachineId = P15Identity(BlueprintId, TEXT("state-machine"), TEXT("locomotion")); TSharedRef<FJsonObject> MachineArgs = MakeShared<FJsonObject>(); MachineArgs->SetStringField(TEXT("animationBlueprintId"), BlueprintId); MachineArgs->SetStringField(TEXT("name"), TEXT("locomotion")); PreviousRevision = Revision; if (!VerifyFaultMatrix(TEXT("machine"), TEXT("animation.state_machine_ensure"), MachineArgs, Revision)) return false; Current = Execute(TEXT("machine"), TEXT("animation.state_machine_ensure"), MachineArgs, Revision); if (!Current) return false; TestEqual(TEXT("machine exact changed state"), Current->GetBoolField(TEXT("changed")), !Persisted); Revision = Current->GetStringField(TEXT("revision")); if (!Persisted && !VerifyRejected(TEXT("machine-stale"), TEXT("animation.state_machine_ensure"), MachineArgs, PreviousRevision)) return false;
    auto EnsureState = [&](const FString& Name, const TCHAR* Sequence)
    {
        TSharedRef<FJsonObject> Args = MakeShared<FJsonObject>(); Args->SetStringField(TEXT("animationBlueprintId"), BlueprintId); Args->SetStringField(TEXT("stateMachineId"), MachineId); Args->SetStringField(TEXT("name"), Name); Args->SetStringField(TEXT("sequenceId"), Sequence); const FString Before = Revision; if (!VerifyFaultMatrix(TEXT("state-") + Name, TEXT("animation.state_ensure"), Args, Revision)) return false; TSharedPtr<FJsonObject> Result = Execute(TEXT("state-") + Name, TEXT("animation.state_ensure"), Args, Revision); if (Result) { TestEqual(*FString::Printf(TEXT("%s state exact changed state"), *Name), Result->GetBoolField(TEXT("changed")), !Persisted); Revision = Result->GetStringField(TEXT("revision")); } return Result.IsValid() && (Persisted || VerifyRejected(TEXT("state-") + Name + TEXT("-stale"), TEXT("animation.state_ensure"), Args, Before));
    };
    if (!EnsureState(TEXT("idle"), IdlePath) || !EnsureState(TEXT("moving"), MovingPath)) return false;
    auto EnsureTransition = [&](const FString& From, const FString& To, const FString& Expression)
    {
        TSharedRef<FJsonObject> Args = MakeShared<FJsonObject>(); Args->SetStringField(TEXT("animationBlueprintId"), BlueprintId); Args->SetStringField(TEXT("stateMachineId"), MachineId); Args->SetStringField(TEXT("fromStateId"), P15Identity(BlueprintId, TEXT("state"), From)); Args->SetStringField(TEXT("toStateId"), P15Identity(BlueprintId, TEXT("state"), To)); Args->SetStringField(TEXT("expression"), Expression); const FString Before = Revision; if (!VerifyFaultMatrix(TEXT("transition-") + From, TEXT("animation.transition_ensure"), Args, Revision)) return false; TSharedPtr<FJsonObject> Result = Execute(TEXT("transition-") + From, TEXT("animation.transition_ensure"), Args, Revision); if (Result) { TestEqual(*FString::Printf(TEXT("%s transition exact changed state"), *From), Result->GetBoolField(TEXT("changed")), !Persisted); Revision = Result->GetStringField(TEXT("revision")); } return Result.IsValid() && (Persisted || VerifyRejected(TEXT("transition-") + From + TEXT("-stale"), TEXT("animation.transition_ensure"), Args, Before));
    };
    if (!EnsureTransition(TEXT("idle"), TEXT("moving"), TEXT("Speed > 10")) || !EnsureTransition(TEXT("moving"), TEXT("idle"), TEXT("Speed <= 10"))) return false;
    TSharedRef<FJsonObject> ViewArgs = MakeShared<FJsonObject>(); ViewArgs->SetStringField(TEXT("animationBlueprintId"), BlueprintId); TSharedPtr<FJsonObject> View = Execute(TEXT("view"), TEXT("animation.graph_view"), ViewArgs, FString()); if (!View) return false; const TArray<TSharedPtr<FJsonValue>>& Machines = View->GetArrayField(TEXT("stateMachines")); TestTrue(TEXT("graph view exposes exact complete locomotion topology"), View->GetArrayField(TEXT("variables")).Num() == 1 && Machines.Num() == 1 && Machines[0]->AsObject()->GetArrayField(TEXT("states")).Num() == 2 && Machines[0]->AsObject()->GetArrayField(TEXT("transitions")).Num() == 2 && View->GetStringField(TEXT("revision")) == Revision);
    const FString Oracle = Serialize(View.ToSharedRef()); if (Persisted) TestEqual(TEXT("restart preserves complete semantic graph oracle"), Oracle, OracleBefore);
    UAnimBlueprint* Blueprint = P15LoadAnimationBlueprint(BlueprintId); if (!Blueprint) return false; FSavePackageArgs SaveArgs; SaveArgs.TopLevelFlags = RF_Public | RF_Standalone; SaveArgs.SaveFlags = SAVE_NoError; const FString Filename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension()); TestTrue(TEXT("complete graph package saves"), UPackage::SavePackage(Blueprint->GetOutermost(), Blueprint, *Filename, SaveArgs)); if (!Persisted) { IFileManager::Get().MakeDirectory(*FPaths::GetPath(OraclePath), true); TestTrue(TEXT("first session persists complete graph oracle"), FFileHelper::SaveStringToFile(Oracle, *OraclePath)); }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiP15PublicCharacterBindingPersistence, "MagiUnrealAXI.P15.PublicCharacterBindingPersistence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiP15PublicCharacterBindingPersistence::RunTest(const FString&)
{
    using namespace MagiP15AnimationSpike;
    constexpr TCHAR AnimPackage[] = TEXT("/Game/MagiP15Character/ABP_MagiP15Character"); constexpr TCHAR AnimId[] = TEXT("/Game/MagiP15Character/ABP_MagiP15Character.ABP_MagiP15Character"); constexpr TCHAR CharacterPackage[] = TEXT("/Game/MagiP15Character/BP_MagiP15Character"); constexpr TCHAR CharacterId[] = TEXT("/Game/MagiP15Character/BP_MagiP15Character.BP_MagiP15Character"); constexpr TCHAR MeshId[] = TEXT("/Game/MagiP15Seed/magi-p15-owned-seed/SkeletalMeshes/magi-p15-owned-seed.magi-p15-owned-seed");
    auto Envelope = [](const FString& Response) { TSharedPtr<FJsonObject> Value; FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response), Value); return Value; };
    auto Result = [&](const FString& Response) { const TSharedPtr<FJsonObject> Value = Envelope(Response); const TSharedPtr<FJsonObject>* Object = nullptr; return Value && Value->TryGetObjectField(TEXT("result"), Object) && Object ? *Object : TSharedPtr<FJsonObject>(); };
    auto Execute = [&](const FString& Suffix, const FString& Operation, const TSharedRef<FJsonObject>& Args, const FString& Revision)
    {
        const FString Response = ReadResponseOnGameThread(TEXT("p15-character-") + Suffix, Operation, Args, Revision); FlushAsyncLoading(); FAssetCompilingManager::Get().FinishAllCompilation(); if (!ResponseStatusIsOk(Response)) AddInfo(Operation + TEXT(": ") + Response); TestTrue(*FString::Printf(TEXT("%s succeeds for Character binding fixture"), *Operation), ResponseStatusIsOk(Response)); return Result(Response);
    };
    const bool Persisted = FPackageName::DoesPackageExist(CharacterPackage); const FString OraclePath = FPaths::ProjectSavedDir() / TEXT("Automation/MagiP15PublicCharacterOracle.json"); FString OracleBefore; if (Persisted) TestTrue(TEXT("Character binding oracle exists after restart"), FFileHelper::LoadFileToString(OracleBefore, *OraclePath));
    TSharedRef<FJsonObject> CreateAnim = MakeShared<FJsonObject>(); CreateAnim->SetStringField(TEXT("path"), AnimPackage); CreateAnim->SetStringField(TEXT("skeletonId"), SkeletonPath); TSharedPtr<FJsonObject> Current = Execute(TEXT("create-anim"), TEXT("animation_blueprint.create"), CreateAnim, FString()); if (!Current) return false; FString AnimRevision = Current->GetStringField(TEXT("revision"));
    TSharedRef<FJsonObject> Variable = MakeShared<FJsonObject>(); Variable->SetStringField(TEXT("animationBlueprintId"), AnimId); Variable->SetStringField(TEXT("name"), TEXT("Speed")); Variable->SetStringField(TEXT("type"), TEXT("float")); Variable->SetStringField(TEXT("source"), TEXT("owner_planar_speed")); Current = Execute(TEXT("variable"), TEXT("animation.variable_ensure"), Variable, AnimRevision); if (!Current) return false; AnimRevision = Current->GetStringField(TEXT("revision"));
    const FString MachineId = P15Identity(AnimId, TEXT("state-machine"), TEXT("locomotion")); TSharedRef<FJsonObject> Machine = MakeShared<FJsonObject>(); Machine->SetStringField(TEXT("animationBlueprintId"), AnimId); Machine->SetStringField(TEXT("name"), TEXT("locomotion")); Current = Execute(TEXT("machine"), TEXT("animation.state_machine_ensure"), Machine, AnimRevision); if (!Current) return false; AnimRevision = Current->GetStringField(TEXT("revision"));
    auto State = [&](const FString& Name, const TCHAR* Sequence) { TSharedRef<FJsonObject> Args = MakeShared<FJsonObject>(); Args->SetStringField(TEXT("animationBlueprintId"), AnimId); Args->SetStringField(TEXT("stateMachineId"), MachineId); Args->SetStringField(TEXT("name"), Name); Args->SetStringField(TEXT("sequenceId"), Sequence); TSharedPtr<FJsonObject> Value = Execute(TEXT("state-") + Name, TEXT("animation.state_ensure"), Args, AnimRevision); if (Value) AnimRevision = Value->GetStringField(TEXT("revision")); return Value.IsValid(); };
    if (!State(TEXT("idle"), IdlePath)) return false; TSharedRef<FJsonObject> CreateCharacter = MakeShared<FJsonObject>(); CreateCharacter->SetStringField(TEXT("path"), CharacterPackage); CreateCharacter->SetStringField(TEXT("parentClass"), TEXT("/Script/Engine.Character")); Current = Execute(TEXT("create-character"), TEXT("blueprint.create"), CreateCharacter, FString()); if (!Current) return false; if (!Persisted) { TSharedRef<FJsonObject> PartialViewArgs = MakeShared<FJsonObject>(); PartialViewArgs->SetStringField(TEXT("characterBlueprintId"), CharacterId); TSharedPtr<FJsonObject> PartialBefore = Execute(TEXT("partial-view-before"), TEXT("animation.character_view"), PartialViewArgs, FString()); if (!PartialBefore) return false; TSharedRef<FJsonObject> PartialConfigure = MakeShared<FJsonObject>(); PartialConfigure->SetStringField(TEXT("characterBlueprintId"), CharacterId); PartialConfigure->SetStringField(TEXT("skeletalMeshId"), MeshId); PartialConfigure->SetStringField(TEXT("animationBlueprintId"), AnimId); const FString PartialResponse = ReadResponseOnGameThread(TEXT("p15-character-partial-reject"), TEXT("animation.character_configure"), PartialConfigure, PartialBefore->GetStringField(TEXT("revision"))); TSharedPtr<FJsonObject> PartialAfter = Execute(TEXT("partial-view-after"), TEXT("animation.character_view"), PartialViewArgs, FString()); TestTrue(TEXT("Character configure rejects partial locomotion before mutation"), !ResponseStatusIsOk(PartialResponse) && PartialResponse.Contains(TEXT("conflict")) && PartialAfter && Serialize(PartialBefore.ToSharedRef()) == Serialize(PartialAfter.ToSharedRef())); } if (!State(TEXT("moving"), MovingPath)) return false;
    auto Transition = [&](const FString& From, const FString& To, const FString& Expression) { TSharedRef<FJsonObject> Args = MakeShared<FJsonObject>(); Args->SetStringField(TEXT("animationBlueprintId"), AnimId); Args->SetStringField(TEXT("stateMachineId"), MachineId); Args->SetStringField(TEXT("fromStateId"), P15Identity(AnimId, TEXT("state"), From)); Args->SetStringField(TEXT("toStateId"), P15Identity(AnimId, TEXT("state"), To)); Args->SetStringField(TEXT("expression"), Expression); TSharedPtr<FJsonObject> Value = Execute(TEXT("transition-") + From, TEXT("animation.transition_ensure"), Args, AnimRevision); if (Value) AnimRevision = Value->GetStringField(TEXT("revision")); return Value.IsValid(); };
    if (!Transition(TEXT("idle"), TEXT("moving"), TEXT("Speed > 10")) || !Transition(TEXT("moving"), TEXT("idle"), TEXT("Speed <= 10"))) return false;
    TSharedRef<FJsonObject> ViewArgs = MakeShared<FJsonObject>(); ViewArgs->SetStringField(TEXT("characterBlueprintId"), CharacterId); TSharedPtr<FJsonObject> Before = Execute(TEXT("view-before"), TEXT("animation.character_view"), ViewArgs, FString()); if (!Before) return false; const FString BeforeRevision = Before->GetStringField(TEXT("revision")); const FString BeforeText = Serialize(Before.ToSharedRef()); UBlueprint* CharacterBlueprint = P11LoadBlueprint(CharacterId); if (!CharacterBlueprint) return false; const bool DirtyBefore = CharacterBlueprint->GetOutermost()->IsDirty(); const EBlueprintStatus StatusBefore = CharacterBlueprint->Status;
    TSharedRef<FJsonObject> Configure = MakeShared<FJsonObject>(); Configure->SetStringField(TEXT("characterBlueprintId"), CharacterId); Configure->SetStringField(TEXT("skeletalMeshId"), MeshId); Configure->SetStringField(TEXT("animationBlueprintId"), AnimId); GP11ForceAtomicFailure = true; const FString Failed = ReadResponseOnGameThread(TEXT("p15-character-rollback"), TEXT("animation.character_configure"), Configure, BeforeRevision); GP11ForceAtomicFailure = false; const TSharedPtr<FJsonObject> FailedEnvelope = Envelope(Failed); const TSharedPtr<FJsonObject>* Receipt = nullptr; const TSharedPtr<FJsonObject>* Verification = nullptr; TSharedPtr<FJsonObject> AfterFailure = Execute(TEXT("view-after-failure"), TEXT("animation.character_view"), ViewArgs, FString()); TestFalse(TEXT("Character binding injected failure rejects"), ResponseStatusIsOk(Failed)); TestTrue(TEXT("Character binding injected failure returns verified failed receipt"), FailedEnvelope && FailedEnvelope->TryGetObjectField(TEXT("receipt"), Receipt) && Receipt && (*Receipt)->GetStringField(TEXT("state")) == TEXT("failed") && (*Receipt)->TryGetObjectField(TEXT("verification"), Verification) && Verification && (*Verification)->GetBoolField(TEXT("matched"))); TestTrue(TEXT("Character binding rollback preserves exact defaults, revision, dirty state, and status"), AfterFailure && Serialize(AfterFailure.ToSharedRef()) == BeforeText && CharacterBlueprint->GetOutermost()->IsDirty() == DirtyBefore && CharacterBlueprint->Status == StatusBefore);
    Current = Execute(TEXT("configure"), TEXT("animation.character_configure"), Configure, BeforeRevision); if (!Current) return false; const FString ConfiguredRevision = Current->GetStringField(TEXT("revision")); TestEqual(TEXT("Character binding reports exact changed state"), Current->GetBoolField(TEXT("changed")), !Persisted); TestTrue(TEXT("Character binding returns exact owned mesh, Skeleton, Animation Blueprint, class, and mode"), Current->GetStringField(TEXT("skeletalMeshId")) == MeshId && Current->GetStringField(TEXT("skeletonId")) == SkeletonPath && Current->GetStringField(TEXT("animationBlueprintId")) == AnimId && Current->GetStringField(TEXT("animClass")) == FString(AnimId) + TEXT("_C") && Current->GetStringField(TEXT("animationMode")) == TEXT("AnimationBlueprint")); if (!Persisted) { const FString Stale = ReadResponseOnGameThread(TEXT("p15-character-stale"), TEXT("animation.character_configure"), Configure, BeforeRevision); TestTrue(TEXT("Character binding stale revision rejects"), !ResponseStatusIsOk(Stale) && Stale.Contains(TEXT("conflict"))); }
    TSharedPtr<FJsonObject> View = Execute(TEXT("view"), TEXT("animation.character_view"), ViewArgs, FString()); if (!View) return false; const FString Oracle = Serialize(View.ToSharedRef()); TestEqual(TEXT("Character view revision matches configure"), View->GetStringField(TEXT("revision")), ConfiguredRevision); if (Persisted) TestEqual(TEXT("restart preserves exact Character animation oracle"), Oracle, OracleBefore);
    ACharacter* CharacterDefaults = nullptr; USkeletalMeshComponent* CharacterMesh = nullptr; if (!P15CharacterDefaults(*CharacterBlueprint, CharacterDefaults, CharacterMesh)) return false; UClass* ConfiguredAnimClass = CharacterMesh->AnimClass.Get(); CharacterMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode); CharacterMesh->AnimClass = ConfiguredAnimClass; const FString IncoherentMode = ReadResponseOnGameThread(TEXT("p15-character-view-incoherent-mode"), TEXT("animation.character_view"), ViewArgs); CharacterMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint); CharacterMesh->SetAnimInstanceClass(ConfiguredAnimClass); const TSharedPtr<FJsonObject> RestoredView = Result(ReadResponseOnGameThread(TEXT("p15-character-view-restored-mode"), TEXT("animation.character_view"), ViewArgs)); TestTrue(TEXT("Character view rejects retained AnimBP class outside AnimationBlueprint mode and exact restore preserves oracle"), !ResponseStatusIsOk(IncoherentMode) && IncoherentMode.Contains(TEXT("conflict")) && RestoredView && Serialize(RestoredView.ToSharedRef()) == Oracle);
    UAnimBlueprint* AnimationBlueprint = P15LoadAnimationBlueprint(AnimId); CharacterBlueprint = P11LoadBlueprint(CharacterId); if (!AnimationBlueprint || !CharacterBlueprint) return false; FSavePackageArgs SaveArgs; SaveArgs.TopLevelFlags = RF_Public | RF_Standalone; SaveArgs.SaveFlags = SAVE_NoError; TestTrue(TEXT("Character fixture Animation Blueprint saves"), UPackage::SavePackage(AnimationBlueprint->GetOutermost(), AnimationBlueprint, *FPackageName::LongPackageNameToFilename(AnimPackage, FPackageName::GetAssetPackageExtension()), SaveArgs)); TestTrue(TEXT("configured Character Blueprint saves"), UPackage::SavePackage(CharacterBlueprint->GetOutermost(), CharacterBlueprint, *FPackageName::LongPackageNameToFilename(CharacterPackage, FPackageName::GetAssetPackageExtension()), SaveArgs)); if (!Persisted) { IFileManager::Get().MakeDirectory(*FPaths::GetPath(OraclePath), true); TestTrue(TEXT("first session persists Character binding oracle"), FFileHelper::SaveStringToFile(Oracle, *OraclePath)); }
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

struct FP15RuntimeObserveContext
{
    FAutomationTestBase* Test = nullptr;
    TWeakObjectPtr<UWorld> EditorWorld;
    TWeakObjectPtr<ACharacter> EditorCharacter;
    FString SessionId;
    FString CharacterId;
    FString AnimationBlueprintId;
    FString StateMachineId;
    bool WasDirty = false;
    bool SawIdleMoving = false;
    bool SawMovingIdle = false;
    bool SawIdleMovingBlend = false;
    bool SawMovingIdleBlend = false;
    bool Observed = false;
    bool RejectionsChecked = false;
    bool Ending = false;
    int32 Phase = 0;
    double Deadline = 0.0;
};

class FP15RuntimeObserveCommand final : public IAutomationLatentCommand
{
public:
    explicit FP15RuntimeObserveCommand(TSharedRef<FP15RuntimeObserveContext> InContext) : Context(MoveTemp(InContext)) {}
    virtual bool Update() override
    {
        if (Context->Ending)
        {
            if (PieWorld() && FPlatformTime::Seconds() < Context->Deadline) return false; if (PieWorld()) Context->Test->AddError(TEXT("P1.5 runtime PIE did not stop")); TSharedRef<FJsonObject> StaleArgs = MakeShared<FJsonObject>(); StaleArgs->SetStringField(TEXT("sessionId"), Context->SessionId); StaleArgs->SetStringField(TEXT("characterId"), Context->CharacterId); StaleArgs->SetStringField(TEXT("animationBlueprintId"), Context->AnimationBlueprintId); StaleArgs->SetStringField(TEXT("stateMachineId"), Context->StateMachineId); const FString Stale = ReadResponseOnGameThread(TEXT("p15-runtime-stale"), TEXT("play.animation_observe"), StaleArgs); Context->Test->TestTrue(TEXT("ended PIE rejects animation observation"), !ResponseStatusIsOk(Stale)); if (UWorld* EditorWorld = Context->EditorWorld.Get()) { if (Context->EditorCharacter.IsValid()) EditorWorld->DestroyActor(Context->EditorCharacter.Get()); EditorWorld->GetOutermost()->SetDirtyFlag(Context->WasDirty); } PlayState = EPlayState::Stopped; PlaySessionId.Reset(); return true;
        }
        UWorld* World = PieWorld(); if (!World) { if (FPlatformTime::Seconds() < Context->Deadline) return false; Context->Test->AddError(TEXT("P1.5 runtime PIE did not start")); Context->Ending = true; return false; } ACharacter* Character = nullptr; for (TActorIterator<ACharacter> It(World); It; ++It) if (IsValid(*It) && ActorId(**It) == Context->CharacterId) { Character = *It; break; } if (!Character || !Character->GetMesh() || !Character->GetMesh()->GetAnimInstance()) { if (FPlatformTime::Seconds() < Context->Deadline) return false; Context->Test->AddError(TEXT("configured Character AnimInstance did not initialize in PIE")); GUnrealEd->RequestEndPlayMap(); Context->Ending = true; Context->Deadline = FPlatformTime::Seconds() + 15.0; return false; }
        TSharedRef<FJsonObject> Args = MakeShared<FJsonObject>(); Args->SetStringField(TEXT("sessionId"), Context->SessionId); Args->SetStringField(TEXT("characterId"), Context->CharacterId); Args->SetStringField(TEXT("animationBlueprintId"), Context->AnimationBlueprintId); Args->SetStringField(TEXT("stateMachineId"), Context->StateMachineId); const FString Response = ReadResponseOnGameThread(TEXT("p15-runtime-observe-") + FString::FromInt(Context->Phase), TEXT("play.animation_observe"), Args); TSharedPtr<FJsonObject> Envelope; const TSharedPtr<FJsonObject>* ResultField = nullptr; const bool Parsed = ResponseStatusIsOk(Response) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Response), Envelope) && Envelope && Envelope->TryGetObjectField(TEXT("result"), ResultField) && ResultField && ResultField->IsValid(); if (!Parsed) { if (!Context->Observed && FPlatformTime::Seconds() < Context->Deadline) return false; Context->Test->AddError(TEXT("runtime animation observation failed after initialization: ") + Response); GUnrealEd->RequestEndPlayMap(); Context->Ending = true; Context->Deadline = FPlatformTime::Seconds() + 15.0; return false; } Context->Observed = true; const TSharedPtr<FJsonObject> Observation = *ResultField; const TArray<TSharedPtr<FJsonValue>>* StateWeights = nullptr; const TSharedPtr<FJsonObject>* Transition = nullptr; Observation->TryGetArrayField(TEXT("stateWeights"), StateWeights); Observation->TryGetObjectField(TEXT("activeTransition"), Transition); const TSharedPtr<FJsonObject> IdleRow = StateWeights && StateWeights->Num() == 2 ? (*StateWeights)[0]->AsObject() : nullptr; const TSharedPtr<FJsonObject> MovingRow = StateWeights && StateWeights->Num() == 2 ? (*StateWeights)[1]->AsObject() : nullptr; const FString Active = Observation->GetStringField(TEXT("activeStateName")); const double Speed = Observation->GetNumberField(TEXT("speed")); const double OwnerSpeed = Observation->GetNumberField(TEXT("ownerPlanarSpeed")); const double IdleWeight = IdleRow ? IdleRow->GetNumberField(TEXT("weight")) : -1.0; const double MovingWeight = MovingRow ? MovingRow->GetNumberField(TEXT("weight")) : -1.0; UAnimInstance* LiveInstance = Character->GetMesh()->GetAnimInstance(); USkeletalMesh* LiveMesh = Character->GetMesh()->GetSkeletalMeshAsset(); const bool ExactObservation = MagiAxiValidateOutput(TEXT("play.animation_observe"), Observation.ToSharedRef()) && Observation->GetStringField(TEXT("sessionId")) == Context->SessionId && Observation->GetStringField(TEXT("characterId")) == Context->CharacterId && Observation->GetStringField(TEXT("meshComponentId")) == Context->CharacterId + TEXT("#component:") + Character->GetMesh()->GetName() && LiveMesh && Observation->GetStringField(TEXT("skeletalMeshId")) == LiveMesh->GetPathName() && LiveMesh->GetSkeleton() && Observation->GetStringField(TEXT("skeletonId")) == LiveMesh->GetSkeleton()->GetPathName() && Observation->GetStringField(TEXT("animationBlueprintId")) == Context->AnimationBlueprintId && LiveInstance && Observation->GetStringField(TEXT("animClass")) == LiveInstance->GetClass()->GetPathName() && Observation->GetStringField(TEXT("animationInstanceId")).StartsWith(Context->CharacterId + TEXT("#animation-instance:")) && Observation->GetStringField(TEXT("stateMachineId")) == Context->StateMachineId && IdleRow && IdleRow->GetStringField(TEXT("name")) == TEXT("idle") && IdleRow->GetStringField(TEXT("stateId")) == P15Identity(Context->AnimationBlueprintId, TEXT("state"), TEXT("idle")) && MovingRow && MovingRow->GetStringField(TEXT("name")) == TEXT("moving") && MovingRow->GetStringField(TEXT("stateId")) == P15Identity(Context->AnimationBlueprintId, TEXT("state"), TEXT("moving")) && Observation->GetStringField(TEXT("activeStateId")) == P15Identity(Context->AnimationBlueprintId, TEXT("state"), Active) && IsCanonicalSha256Revision(Observation->GetStringField(TEXT("revision"))); if (!ExactObservation) { Context->Test->AddError(TEXT("runtime animation observation identity/schema mismatch: ") + Serialize(Observation.ToSharedRef())); GUnrealEd->RequestEndPlayMap(); Context->Ending = true; Context->Deadline = FPlatformTime::Seconds() + 15.0; return false; }
        const FString WeightSelectedActive = MovingWeight > IdleWeight ? TEXT("moving") : TEXT("idle"); if (FMath::Abs((IdleWeight + MovingWeight) - 1.0) > 0.001 || Active != WeightSelectedActive) { Context->Test->AddError(TEXT("runtime animation active state does not match frozen weight-selection rule or weights do not sum to one")); GUnrealEd->RequestEndPlayMap(); Context->Ending = true; Context->Deadline = FPlatformTime::Seconds() + 15.0; return false; }
        const bool RevisionTransitionActive = Transition && Transition->IsValid() && (*Transition)->GetBoolField(TEXT("active")); FString RevisionFromName, RevisionToName; double RevisionTransitionFraction = 0.0; bool TransitionShapeValid = false; if (RevisionTransitionActive) { const FString FromId = (*Transition)->GetStringField(TEXT("fromStateId")); const FString ToId = (*Transition)->GetStringField(TEXT("toStateId")); RevisionFromName = FromId == P15Identity(Context->AnimationBlueprintId, TEXT("state"), TEXT("idle")) ? TEXT("idle") : FromId == P15Identity(Context->AnimationBlueprintId, TEXT("state"), TEXT("moving")) ? TEXT("moving") : FString(); RevisionToName = ToId == P15Identity(Context->AnimationBlueprintId, TEXT("state"), TEXT("idle")) ? TEXT("idle") : ToId == P15Identity(Context->AnimationBlueprintId, TEXT("state"), TEXT("moving")) ? TEXT("moving") : FString(); TransitionShapeValid = !RevisionFromName.IsEmpty() && !RevisionToName.IsEmpty() && RevisionFromName != RevisionToName && (*Transition)->TryGetNumberField(TEXT("elapsedFraction"), RevisionTransitionFraction); } else TransitionShapeValid = Transition && Transition->IsValid() && (*Transition)->HasTypedField<EJson::Null>(TEXT("transitionId")) && (*Transition)->HasTypedField<EJson::Null>(TEXT("fromStateId")) && (*Transition)->HasTypedField<EJson::Null>(TEXT("toStateId")) && (*Transition)->HasTypedField<EJson::Null>(TEXT("elapsedFraction")); const FString ExpectedRevision = Sha256(CanonicalRow({Context->SessionId, Context->CharacterId, LiveMesh->GetPathName(), LiveMesh->GetSkeleton()->GetPathName(), Context->AnimationBlueprintId, LiveInstance->GetClass()->GetPathName(), Context->StateMachineId, FString::SanitizeFloat(Speed), FString::SanitizeFloat(OwnerSpeed), FString::SanitizeFloat(IdleWeight), FString::SanitizeFloat(MovingWeight), Active, RevisionFromName, RevisionToName, RevisionTransitionActive ? FString::SanitizeFloat(RevisionTransitionFraction) : FString()})); if (!TransitionShapeValid || Observation->GetStringField(TEXT("animationInstanceId")) != Context->CharacterId + TEXT("#animation-instance:") + LiveInstance->GetName() || Observation->GetStringField(TEXT("revision")) != ExpectedRevision) { Context->Test->AddError(TEXT("runtime animation exact instance/transition/revision mismatch: ") + Serialize(Observation.ToSharedRef())); GUnrealEd->RequestEndPlayMap(); Context->Ending = true; Context->Deadline = FPlatformTime::Seconds() + 15.0; return false; }
        if (!Context->RejectionsChecked) { auto Rejected = [&](const FString& Suffix, const TSharedRef<FJsonObject>& BadArgs) { return !ResponseStatusIsOk(ReadResponseOnGameThread(TEXT("p15-runtime-reject-") + Suffix, TEXT("play.animation_observe"), BadArgs)); }; TSharedRef<FJsonObject> BadSession = MakeShared<FJsonObject>(*Args); BadSession->SetStringField(TEXT("sessionId"), Context->SessionId + TEXT("-stale")); TSharedRef<FJsonObject> BadCharacter = MakeShared<FJsonObject>(*Args); BadCharacter->SetStringField(TEXT("characterId"), Context->CharacterId + TEXT("-missing")); TSharedRef<FJsonObject> BadAnim = MakeShared<FJsonObject>(*Args); BadAnim->SetStringField(TEXT("animationBlueprintId"), TEXT("/Game/Missing.Missing")); TSharedRef<FJsonObject> BadMachine = MakeShared<FJsonObject>(*Args); BadMachine->SetStringField(TEXT("stateMachineId"), Context->StateMachineId + TEXT("-wrong")); USkeletalMesh* SavedMesh = Character->GetMesh()->GetSkeletalMeshAsset(); Character->GetMesh()->SetSkeletalMeshAsset(nullptr); const bool MeshRejected = Rejected(TEXT("mesh"), Args); Character->GetMesh()->SetSkeletalMeshAsset(SavedMesh); Context->Test->TestTrue(TEXT("runtime observation fails closed for wrong session, Character, AnimBP, state machine, and live mesh identity"), Rejected(TEXT("session"), BadSession) && Rejected(TEXT("character"), BadCharacter) && Rejected(TEXT("anim"), BadAnim) && Rejected(TEXT("machine"), BadMachine) && MeshRejected); Context->RejectionsChecked = true; return false; }
        const bool TransitionActive = Transition && Transition->IsValid() && (*Transition)->GetBoolField(TEXT("active")); if (TransitionActive) { const FString From = (*Transition)->GetStringField(TEXT("fromStateId")); const FString To = (*Transition)->GetStringField(TEXT("toStateId")); const bool IdleMoving = From == P15Identity(Context->AnimationBlueprintId, TEXT("state"), TEXT("idle")) && To == P15Identity(Context->AnimationBlueprintId, TEXT("state"), TEXT("moving")) && (*Transition)->GetStringField(TEXT("transitionId")) == P15Identity(Context->AnimationBlueprintId, TEXT("transition"), TEXT("idle->moving")); const bool MovingIdle = From == P15Identity(Context->AnimationBlueprintId, TEXT("state"), TEXT("moving")) && To == P15Identity(Context->AnimationBlueprintId, TEXT("state"), TEXT("idle")) && (*Transition)->GetStringField(TEXT("transitionId")) == P15Identity(Context->AnimationBlueprintId, TEXT("transition"), TEXT("moving->idle")); Context->SawIdleMoving |= IdleMoving; Context->SawMovingIdle |= MovingIdle; Context->SawIdleMovingBlend |= IdleMoving && IdleWeight > 0.0 && IdleWeight < 1.0 && MovingWeight > 0.0 && MovingWeight < 1.0; Context->SawMovingIdleBlend |= MovingIdle && IdleWeight > 0.0 && IdleWeight < 1.0 && MovingWeight > 0.0 && MovingWeight < 1.0; }
        if (Context->Phase == 0 && !TransitionActive && Active == TEXT("idle") && IdleWeight > 0.99 && MovingWeight < 0.01 && Speed <= 1.0 && OwnerSpeed <= 1.0) { Character->GetCharacterMovement()->Velocity = FVector(100.0, 0.0, 0.0); Context->Phase = 1; Context->Deadline = FPlatformTime::Seconds() + 15.0; return false; } if (Context->Phase == 1 && !TransitionActive && Active == TEXT("moving") && MovingWeight > 0.99 && IdleWeight < 0.01 && Context->SawIdleMoving && Context->SawIdleMovingBlend && Speed > 10.0 && FMath::IsNearlyEqual(Speed, OwnerSpeed, 1.0)) { Character->GetCharacterMovement()->Velocity = FVector::ZeroVector; Context->Phase = 2; Context->Deadline = FPlatformTime::Seconds() + 15.0; return false; } if (Context->Phase == 2 && !TransitionActive && Active == TEXT("idle") && IdleWeight > 0.99 && MovingWeight < 0.01 && Speed <= 1.0 && OwnerSpeed <= 1.0 && Context->SawMovingIdle && Context->SawMovingIdleBlend) { Context->Test->TestTrue(TEXT("runtime animation certifies stable idle, blended outbound, settled moving, blended return, and stable idle"), true); GUnrealEd->RequestEndPlayMap(); Context->Ending = true; Context->Deadline = FPlatformTime::Seconds() + 15.0; return false; } if (FPlatformTime::Seconds() < Context->Deadline) return false; Context->Test->AddError(FString::Printf(TEXT("runtime animation loop timed out in phase %d (active=%s speed=%f owner=%f weights=%f/%f outbound=%d/%d return=%d/%d)"), Context->Phase, *Active, Speed, OwnerSpeed, IdleWeight, MovingWeight, Context->SawIdleMoving, Context->SawIdleMovingBlend, Context->SawMovingIdle, Context->SawMovingIdleBlend)); GUnrealEd->RequestEndPlayMap(); Context->Ending = true; Context->Deadline = FPlatformTime::Seconds() + 15.0; return false;
    }
private:
    TSharedRef<FP15RuntimeObserveContext> Context;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMagiP15PublicRuntimeAnimationObserve, "MagiUnrealAXI.P15.PublicRuntimeAnimationObserve", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMagiP15PublicRuntimeAnimationObserve::RunTest(const FString&)
{
    constexpr TCHAR CharacterId[] = TEXT("/Game/MagiP15Character/BP_MagiP15Character.BP_MagiP15Character"); constexpr TCHAR AnimationBlueprintId[] = TEXT("/Game/MagiP15Character/ABP_MagiP15Character.ABP_MagiP15Character"); UBlueprint* CharacterBlueprint = P11LoadBlueprint(CharacterId); UAnimBlueprint* AnimationBlueprint = P15LoadAnimationBlueprint(AnimationBlueprintId); UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr; TestNotNull(TEXT("configured Character Blueprint exists for runtime observation"), CharacterBlueprint); TestNotNull(TEXT("complete Animation Blueprint exists for runtime observation"), AnimationBlueprint); TestNotNull(TEXT("editor world exists for runtime observation"), World); if (!CharacterBlueprint || !CharacterBlueprint->GeneratedClass || !AnimationBlueprint || !World || !World->GetOutermost()) return false; const bool WasDirty = World->GetOutermost()->IsDirty(); FActorSpawnParameters Spawn; Spawn.OverrideLevel = World->PersistentLevel; Spawn.ObjectFlags |= RF_Transactional; Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; ACharacter* Character = World->SpawnActor<ACharacter>(CharacterBlueprint->GeneratedClass, FVector(0.0, 0.0, 200.0), FRotator::ZeroRotator, Spawn); TestNotNull(TEXT("configured Character fixture spawns in editor world"), Character); if (!Character || !Character->GetActorGuid().IsValid()) { if (Character) World->DestroyActor(Character); World->GetOutermost()->SetDirtyFlag(WasDirty); return false; } TSharedRef<FJsonObject> StartArgs = MakeShared<FJsonObject>(); const FString StartResponse = ReadResponseOnGameThread(TEXT("p15-runtime-start"), TEXT("play.start"), StartArgs); TSharedPtr<FJsonObject> StartEnvelope; const TSharedPtr<FJsonObject>* StartResult = nullptr; const bool Started = ResponseStatusIsOk(StartResponse) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(StartResponse), StartEnvelope) && StartEnvelope && StartEnvelope->TryGetObjectField(TEXT("result"), StartResult) && StartResult && StartResult->IsValid(); TestTrue(TEXT("production play.start queues P1.5 runtime PIE"), Started); if (!Started) { World->DestroyActor(Character); World->GetOutermost()->SetDirtyFlag(WasDirty); return false; } TSharedRef<FP15RuntimeObserveContext> Context = MakeShared<FP15RuntimeObserveContext>(); Context->Test = this; Context->EditorWorld = World; Context->EditorCharacter = Character; Context->SessionId = (*StartResult)->GetStringField(TEXT("sessionId")); Context->CharacterId = ActorId(*Character); Context->AnimationBlueprintId = AnimationBlueprintId; Context->StateMachineId = P15Identity(AnimationBlueprintId, TEXT("state-machine"), TEXT("locomotion")); Context->WasDirty = WasDirty; Context->Deadline = FPlatformTime::Seconds() + 15.0; ADD_LATENT_AUTOMATION_COMMAND(FP15RuntimeObserveCommand(Context)); return true;
}


#endif
