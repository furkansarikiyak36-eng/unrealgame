#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DamageComponent.generated.h"
UCLASS(ClassGroup=(Damage),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UDamageComponent:public UActorComponent
{GENERATED_BODY() public: UDamageComponent(); virtual void BeginPlay() override; UFUNCTION(BlueprintCallable) void ApplyDamage(float Amount); UFUNCTION(BlueprintPure) float GetHealth()const{return Health;} UFUNCTION(BlueprintPure) bool IsWeakened()const{return Health<MaxHealth*0.3f;} UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="1.0")) float MaxHealth=100.0f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float Health=100.0f; };
