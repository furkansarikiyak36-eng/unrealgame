#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RepairToolComponent.generated.h"
class ALogActor;class AStoneActor;class AStickActor;
UCLASS(ClassGroup=(Inventory),meta=(BlueprintSpawnableComponent))class MYPROJECT2_API URepairToolComponent:public UActorComponent
{GENERATED_BODY()public:URepairToolComponent();UFUNCTION(BlueprintCallable)bool RepairStructure(AActor*Structure,float Amount);UFUNCTION(BlueprintCallable)bool RepairLog(ALogActor*Log,float Amount);UFUNCTION(BlueprintCallable)bool RepairStone(AStoneActor*Stone,float Amount);UFUNCTION(BlueprintCallable)bool RepairStick(AStickActor*Stick,float Amount);UPROPERTY(EditAnywhere,BlueprintReadWrite)float RepairPower=25;UPROPERTY(EditAnywhere,BlueprintReadWrite)float RepairRange=200;};
