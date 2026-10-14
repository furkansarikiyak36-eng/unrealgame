#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AnimalNutritionComponent.generated.h"
class UAnimalDefinition;
USTRUCT(BlueprintType) struct FFeedItem{GENERATED_BODY() UPROPERTY(EditAnywhere,BlueprintReadWrite) FName FeedID; UPROPERTY(EditAnywhere,BlueprintReadWrite) float DryMatter=.85f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MetabolizableEnergy=10.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float CrudeProtein=.15f;};
UCLASS(ClassGroup=(Animal),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UAnimalNutritionComponent:public UActorComponent{GENERATED_BODY()
public: UAnimalNutritionComponent(); UFUNCTION(BlueprintPure,Category="Nutrition") float ComputeIntakeCapacity(float Weight,float TempFactor,float HealthFactor)const; UFUNCTION(BlueprintPure,Category="Nutrition") float ComputeActualIntake(float IMax,float AvailableFeed)const; UFUNCTION(BlueprintPure,Category="Nutrition") float ComputeEnergyIntake(float FeedAmount,const FFeedItem& Feed)const; UFUNCTION(BlueprintPure,Category="Nutrition") float ComputeProteinIntake(float FeedAmount,const FFeedItem& Feed)const; UFUNCTION(BlueprintPure,Category="Nutrition") float ComputeSuitability(float Intake,float Requirement)const; UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<UAnimalDefinition> Definition=nullptr;};
