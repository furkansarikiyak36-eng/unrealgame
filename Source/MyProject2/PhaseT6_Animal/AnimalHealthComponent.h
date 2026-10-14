#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AnimalHealthComponent.generated.h"
UCLASS(ClassGroup=(Animal),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UAnimalHealthComponent:public UActorComponent{GENERATED_BODY()
public: UAnimalHealthComponent(); UFUNCTION(BlueprintCallable,Category="Health") void UpdateHealth(float Recovery,float Stress,float Disease,float DeltaDays); UFUNCTION(BlueprintCallable,Category="Health") void ApplyTreatment(float Amount); UFUNCTION(BlueprintPure,Category="Health") float GetHealth()const{return Health;} UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxHealth=100.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float ComfortTemp=20.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float TempTolerance=15.f;
protected: UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float Health=100.f;};
