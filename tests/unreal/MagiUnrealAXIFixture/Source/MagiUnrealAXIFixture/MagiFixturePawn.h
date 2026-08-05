#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "InputActionValue.h"
#include "MagiFixturePawn.generated.h"

UCLASS()
class MAGIUNREALAXIFIXTURE_API AMagiFixturePawn : public ACharacter
{
    GENERATED_BODY()
public:
    AMagiFixturePawn();
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    void Move(const FInputActionValue& Value);
    void InteractInput(const FInputActionValue& Value);
    UPROPERTY()
    TObjectPtr<class UInputMappingContext> MappingContext;
    UPROPERTY()
    TObjectPtr<class UInputAction> MoveAction;
    UPROPERTY()
    TObjectPtr<class UInputAction> InteractAction;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Magi Fixture")
    TObjectPtr<UStaticMeshComponent> FixtureMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Magi Fixture")
    TObjectPtr<USceneComponent> FixtureMarker;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Magi Fixture")
    TObjectPtr<UCameraComponent> FixtureCamera;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Magi Fixture")
    TObjectPtr<UPointLightComponent> FixtureLight;
    UPROPERTY(BlueprintReadOnly, Category="Magi Fixture")
    int32 InteractionCount = 0;
    UFUNCTION(BlueprintCallable, Category="Magi Fixture")
    void RecordInteraction();

};
UCLASS()
class MAGIUNREALAXIFIXTURE_API AMagiFixtureInteractable : public AActor
{
    GENERATED_BODY()
public:
    AMagiFixtureInteractable();
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Magi Fixture")
    TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(BlueprintReadOnly, Category="Magi Fixture")
    int32 InteractionCount = 0;
    UFUNCTION(BlueprintCallable, Category="Magi Fixture")
    void Interact(AMagiFixturePawn* Pawn);
};
