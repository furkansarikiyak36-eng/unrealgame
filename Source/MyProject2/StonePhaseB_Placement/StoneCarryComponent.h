#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StoneCarryComponent.generated.h"

class AStoneActor;

UENUM(BlueprintType)
enum class EStoneCarryMode : uint8
{
    Stacked UMETA(DisplayName = "Envanterde İstif"),
    SingleHand UMETA(DisplayName = "Tek Elle"),
    TwoHands UMETA(DisplayName = "İki Elle"),
    Shouldered UMETA(DisplayName = "Omuzda"),
    Dragging UMETA(DisplayName = "Sürükleyerek")
};

UCLASS(ClassGroup=(Stone), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStoneCarryComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStoneCarryComponent();
    UFUNCTION(BlueprintCallable) bool PickUp(AStoneActor* Stone, EStoneCarryMode Mode = EStoneCarryMode::SingleHand);
    UFUNCTION(BlueprintCallable) bool Drop();
    UFUNCTION(BlueprintCallable) bool ReleaseForPlacement(AStoneActor* Stone);
    UFUNCTION(BlueprintCallable) int32 AddToStack(AStoneActor* SmallStone);
    UFUNCTION(BlueprintCallable) AStoneActor* TakeFromStack();
    UFUNCTION(BlueprintPure) AStoneActor* GetCarried() const { return CarriedStone; }
    UFUNCTION(BlueprintPure) bool IsCarrying() const { return CarriedStone != nullptr; }
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName HandSocket = TEXT("hand_r");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName LeftHandSocket = TEXT("hand_l");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ShoulderSocket = TEXT("shoulder_r");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 MaxStackSize = 20;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxTwoHandMass = 80.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxSingleHandMass = 15.0f;
protected:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) TObjectPtr<AStoneActor> CarriedStone = nullptr;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) EStoneCarryMode CurrentMode = EStoneCarryMode::SingleHand;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) TArray<TObjectPtr<AStoneActor>> StackedStones;
};
