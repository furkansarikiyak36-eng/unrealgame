#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SimulationTypes.h"
#include "PlantVisualComponent.generated.h"
class UStaticMeshComponent; class UPlantDefinition;
UCLASS(ClassGroup=(Plant),meta=(BlueprintSpawnableComponent)) class MYPROJECT2_API UPlantVisualComponent:public UActorComponent{GENERATED_BODY()
public: UPlantVisualComponent(); UFUNCTION(BlueprintCallable,Category="Visual") void UpdateVisual(EGrowthStage Stage,float StageProgress); UFUNCTION(BlueprintCallable,Category="Visual") void SetDefinition(UPlantDefinition* InDefinition); UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Config") FName AttachSocketName=TEXT("plant_socket");
protected: UPROPERTY() TObjectPtr<UStaticMeshComponent> TargetMesh=nullptr; UPROPERTY() TObjectPtr<UPlantDefinition> Definition=nullptr;};
