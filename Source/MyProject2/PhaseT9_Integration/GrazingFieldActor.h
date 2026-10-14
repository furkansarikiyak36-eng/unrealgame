#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrazingFieldActor.generated.h"
class UStaticMeshComponent;
UCLASS() class MYPROJECT2_API AGrazingFieldActor:public AActor{GENERATED_BODY()
public: AGrazingFieldActor(); UFUNCTION(BlueprintCallable,Category="Grazing") void UpdateDaily(float GrassGrowth,float Consumption); UFUNCTION(BlueprintCallable,Category="Grazing") float ConsumeGrass(float Amount); UFUNCTION(BlueprintPure,Category="Grazing") float GetGrassBiomass()const{return GrassBiomass;} UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FieldMesh; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) float GrassBiomass=500.f; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxGrass=1000.f;};
