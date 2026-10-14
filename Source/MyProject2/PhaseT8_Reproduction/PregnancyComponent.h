#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PregnancyComponent.generated.h"
class UAnimalDefinition;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBirth,int32,OffspringCount);
UCLASS(ClassGroup=(Reproduction),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UPregnancyComponent:public UActorComponent{GENERATED_BODY()
public: UPregnancyComponent(); UFUNCTION(BlueprintCallable,Category="Pregnancy") void StartPregnancy(); UFUNCTION(BlueprintCallable,Category="Pregnancy") int32 UpdatePregnancy(float DeltaDays,int32 FreeCapacity); UFUNCTION(BlueprintPure,Category="Pregnancy") bool IsPregnant()const{return bIsPregnant;} UPROPERTY(BlueprintAssignable,Category="Events") FOnBirth OnBirth; UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<UAnimalDefinition> Definition=nullptr; UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 BaseOffspringCount=1;
protected: UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) bool bIsPregnant=false; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float DaysRemaining=0.f;};
