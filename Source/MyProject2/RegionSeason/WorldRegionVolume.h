#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RegionSeasonTypes.h"
#include "WorldRegionVolume.generated.h"

class UBoxComponent;
class URegionSeasonProfile;
class URegionSeasonSubsystem;

/** Non-blocking spatial descriptor for a named region. */
UCLASS(Blueprintable)
class MYPROJECT2_API AWorldRegionVolume : public AActor
{
    GENERATED_BODY()
public:
    AWorldRegionVolume();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Region") TObjectPtr<UBoxComponent> RegionBounds;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Region") FName RegionId = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Region") FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Region") EWorldRegionType RegionType = EWorldRegionType::Unknown;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Region", meta=(ClampMin="-1000", ClampMax="1000")) int32 Priority = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Region|Climate") TObjectPtr<URegionSeasonProfile> ClimateProfile;
    UFUNCTION(BlueprintPure, Category="Region") bool ContainsWorldPoint(const FVector& WorldPoint) const;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    friend class URegionSeasonSubsystem;
};
