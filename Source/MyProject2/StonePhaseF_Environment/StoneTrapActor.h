#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoneTrapActor.generated.h"
class UBoxComponent;
class UStaticMeshComponent;
UENUM(BlueprintType)
enum class EStoneTrapType : uint8
{
    RollingBoulder UMETA(DisplayName = "Yuvarlanan Kaya"),
    FallingStone UMETA(DisplayName = "Düşen Taş"),
    SpikeStone UMETA(DisplayName = "Taş Kazık")
};
UCLASS()
class MYPROJECT2_API AStoneTrapActor : public AActor
{
    GENERATED_BODY()
public:
    AStoneTrapActor();
    UFUNCTION(BlueprintCallable) void Trigger(AActor* Victim);
    UFUNCTION(BlueprintCallable) void Rearm();
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> TriggerVolume;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> TrapMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EStoneTrapType TrapType = EStoneTrapType::RollingBoulder;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Damage = 80.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Cooldown = 8.0f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) bool bArmed = true;
protected:
    float LastTriggerTime = -100.0f;
};
