#include "MagiFixturePawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AMagiFixturePawn::AMagiFixturePawn()
{
    FixtureMarker = CreateDefaultSubobject<USceneComponent>(TEXT("FixtureMarker"));
    FixtureMarker->SetupAttachment(RootComponent);
    FixtureCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FixtureCamera"));
    FixtureCamera->SetupAttachment(RootComponent);
    FixtureCamera->SetRelativeLocation(FVector(-500.0, 0.0, 220.0));
    FixtureCamera->SetRelativeRotation(FRotator(-12.0, 0.0, 0.0));
    FixtureLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FixtureLight"));
    FixtureLight->SetupAttachment(RootComponent);
    FixtureLight->SetRelativeLocation(FVector(150.0, 0.0, 250.0));
    FixtureLight->SetIntensity(5000.0f);
    FixtureLight->SetAttenuationRadius(3000.0f);
    AutoPossessPlayer = EAutoReceiveInput::Player0;
    GetCharacterMovement()->GravityScale = 0.0f;
    GetCharacterMovement()->DefaultLandMovementMode = MOVE_Flying;
    GetCharacterMovement()->SetMovementMode(MOVE_Flying);
    GetCharacterMovement()->MaxFlySpeed = 600.0f;
}

void AMagiFixturePawn::BeginPlay()
{
    Super::BeginPlay();
    MappingContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/MagiM6/Input/IMC_MagiM6.IMC_MagiM6"));
    MoveAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/MagiM6/Input/IA_Move.IA_Move"));
    InteractAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/MagiM6/Input/IA_Interact.IA_Interact"));
    if (APlayerController* Controller = Cast<APlayerController>(GetController()))
    {
        if (ULocalPlayer* LocalPlayer = Controller->GetLocalPlayer())
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
                if (MappingContext) Subsystem->AddMappingContext(MappingContext, 0);
    }
}

void AMagiFixturePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    MappingContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/MagiM6/Input/IMC_MagiM6.IMC_MagiM6"));
    MoveAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/MagiM6/Input/IA_Move.IA_Move"));
    InteractAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/MagiM6/Input/IA_Interact.IA_Interact"));
    if (UEnhancedInputComponent* Enhanced = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (MoveAction) Enhanced->BindAction(MoveAction, ETriggerEvent::Started, this, &AMagiFixturePawn::Move);
        if (InteractAction) Enhanced->BindAction(InteractAction, ETriggerEvent::Started, this, &AMagiFixturePawn::InteractInput);
    }
}

void AMagiFixturePawn::Move(const FInputActionValue& Value)
{
    if (Value.Get<float>() > 0.0f)
        AddActorWorldOffset(FVector(100.0f, 0.0f, 0.0f), false);
}

void AMagiFixturePawn::InteractInput(const FInputActionValue& Value)
{
    if (!Value.Get<bool>()) return;
    for (TActorIterator<AMagiFixtureInteractable> It(GetWorld()); It; ++It)
    {
        if (AMagiFixtureInteractable* Target = *It) { Target->Interact(this); break; }
    }
}

void AMagiFixturePawn::RecordInteraction()
{
    ++InteractionCount;
}

AMagiFixtureInteractable::AMagiFixtureInteractable()
{
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) Mesh->SetStaticMesh(Cube.Object);
}

void AMagiFixtureInteractable::BeginPlay()
{
    Super::BeginPlay();
    InteractionCount = 0;
}

void AMagiFixtureInteractable::Interact(AMagiFixturePawn* Pawn)
{
    ++InteractionCount;
    Tags.AddUnique(FName(TEXT("MagiM6.Interacted")));
    AddActorWorldOffset(FVector(0, 0, 100));
    if (Pawn) Pawn->RecordInteraction();
}
