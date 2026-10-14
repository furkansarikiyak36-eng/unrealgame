#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LogCarryComponent.generated.h"
class ALogActor;
UCLASS(ClassGroup=(Log),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API ULogCarryComponent:public UActorComponent
{GENERATED_BODY() public: ULogCarryComponent(); UFUNCTION(BlueprintCallable) bool PickUp(ALogActor* Log,bool bOnShoulder=true); UFUNCTION(BlueprintCallable) bool Drop(); UFUNCTION(BlueprintCallable) bool Throw(float Charge); UFUNCTION(BlueprintPure) ALogActor* GetCarriedLog()const{return CarriedLog;} UFUNCTION(BlueprintPure) bool IsCarrying()const{return CarriedLog!=nullptr;} UPROPERTY(EditAnywhere,BlueprintReadWrite) FName ShoulderSocket=TEXT("shoulder_r"); UPROPERTY(EditAnywhere,BlueprintReadWrite) FName HandSocket=TEXT("hand_r"); UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxCarryMass=120.0f; protected: UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TObjectPtr<ALogActor> CarriedLog=nullptr; };
