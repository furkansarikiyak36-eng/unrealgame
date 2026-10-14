#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LazyUpdateComponent.generated.h"
UCLASS(ClassGroup=(World),meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API ULazyUpdateComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    ULazyUpdateComponent();
    UFUNCTION(BlueprintCallable,Category="Lazy") float GetDecayedValue(float CachedValue,float RatePerDay,float CurrentDay,float Moisture=0.5f,float Temperature=20.0f);
    UFUNCTION(BlueprintCallable,Category="Lazy") void MarkDirty();
    UFUNCTION(BlueprintCallable,Category="Lazy") void MarkClean(float CurrentDay);
    UFUNCTION(BlueprintPure,Category="Lazy") bool IsDirty() const{return bIsDirty;}
    UFUNCTION(BlueprintPure,Category="Lazy") float GetLastUpdateDay() const{return LastUpdateDay;}
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float LastUpdateDay=1.0f;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) bool bIsDirty=false;
};
