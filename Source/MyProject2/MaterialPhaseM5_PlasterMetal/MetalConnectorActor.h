// MetalConnectorActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MetalDefinition.h"
#include "MetalConnectorActor.generated.h"

class UStaticMeshComponent;
class UCorrosionComponent;

UCLASS()
class MYPROJECT2_API AMetalConnectorActor : public AActor
{
    GENERATED_BODY()

public:
    AMetalConnectorActor();

    UFUNCTION(BlueprintCallable, Category = "Connector")
    void InitializeConnector(UMetalDefinition* Def);

    UFUNCTION(BlueprintCallable, Category = "Connector")
    void UpdateCorrosion(float Moisture, float DeltaDays);

    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ConnectorMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCorrosionComponent> CorrosionComponent;
};
