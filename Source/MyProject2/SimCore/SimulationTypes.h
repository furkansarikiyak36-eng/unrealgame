#pragma once
#include "CoreMinimal.h"
#include "SimulationTypes.generated.h"

UENUM(BlueprintType) enum class ESimTickRate : uint8 { Frame, Hourly, Daily, Weekly };
UENUM(BlueprintType) enum class EGrowthStage : uint8 { Seed, Germination, Seedling, Vegetative, Flowering, Fruiting, Maturity, Senescence };
UENUM(BlueprintType) enum class ESoilTexture : uint8 { Sandy, Loamy, Clay, Silty, Organic };
UENUM(BlueprintType) enum class EAnimalSpecies : uint8 { Cow, Sheep, Chicken, Goat, Pig };
UENUM(BlueprintType) enum class EAnimalSex : uint8 { Male, Female };
UENUM(BlueprintType) enum class EAnimalPhysiologicalState : uint8 { Growing, Adult, Pregnant, Lactating, Sick, Elderly };

USTRUCT(BlueprintType) struct FSoilState { GENERATED_BODY()
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float Water=20.f; // mm
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float WaterMax=100.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float WaterWilting=10.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float WaterCritical=40.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float Nitrogen=50.f; // kg/ha
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float Phosphorus=30.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float Potassium=40.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float pH=6.5f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float OrganicMatter=2.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) ESoilTexture Texture=ESoilTexture::Loamy;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float WeedBiomass=0.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float DiseasePressure=0.f;
};
USTRUCT(BlueprintType) struct FPlantState { GENERATED_BODY()
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float AccumulatedGDD=0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EGrowthStage Stage=EGrowthStage::Seed;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float StageProgress=0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Biomass=0.f; // kg/ha
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float WaterStress=1.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float NutrientStress=1.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float WeedPressure=0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 SeedCount=0;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bIsAlive=true;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bIsHarvestable=false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float DaysSincePlanting=0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float CumulativeSevereStressDays=0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float FrostDamage=0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float HeatDamage=0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float DiseaseSeverity=0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float YieldPotential=0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float QualityIndex=1.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float WaterUptakeTodayMm=0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float NitrogenUptakeToday=0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float PhosphorusUptakeToday=0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float PotassiumUptakeToday=0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bHasFailed=false;
};
USTRUCT(BlueprintType) struct FAnimalState { GENERATED_BODY()
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float AgeDays=0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float BodyWeight=40.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float BCS=3.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Health=100.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float EnergyBalance=0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EAnimalPhysiologicalState PhysioState=EAnimalPhysiologicalState::Growing;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EAnimalSex Sex=EAnimalSex::Female;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bIsAlive=true;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bIsPregnant=false;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float PregnancyDays=0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float LactationDays=0.f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float ReproductionProgress=0.f;
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSimulationDayTick, int32, DayNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSimulationHourTick, int32, HourNumber);