#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StoneTypes.h"
#include "StoneWorkingComponent.generated.h"
class AStoneActor;

UENUM(BlueprintType)
enum class EStoneWorkAction : uint8
{
    Crush UMETA(DisplayName = "Kırma"), Split UMETA(DisplayName = "Yarma"),
    Flatten UMETA(DisplayName = "Yassılaştırma"), Carve UMETA(DisplayName = "Yontma"),
    Corner UMETA(DisplayName = "Köşe Düzeltme"), Hollow UMETA(DisplayName = "Oyuk Açma"),
    Engrave UMETA(DisplayName = "Geçme Bağlantısı"), Smooth UMETA(DisplayName = "Yüzey Tesviyesi")
};

UCLASS(ClassGroup=(Stone), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStoneWorkingComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStoneWorkingComponent();
    UFUNCTION(BlueprintCallable) bool CrushBoulder(AStoneActor* Boulder, int32 Pieces);
    UFUNCTION(BlueprintCallable) bool ProduceBuildingStone(AStoneActor* Boulder);
    UFUNCTION(BlueprintCallable) bool ProduceFlatStone(AStoneActor* Stone);
    UFUNCTION(BlueprintCallable) bool CarveSurface(AStoneActor* Stone);
    UFUNCTION(BlueprintCallable) bool CutCorner(AStoneActor* Stone);
    UFUNCTION(BlueprintCallable) bool HollowGroove(AStoneActor* Stone);
    UFUNCTION(BlueprintCallable) bool EngraveJoint(AStoneActor* Stone);
    UFUNCTION(BlueprintCallable) bool SmoothSurface(AStoneActor* Stone);
    UFUNCTION(BlueprintCallable) bool ProduceGravel(AStoneActor* Stone, int32 Pieces);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 HitsPerAction = 6;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<AStoneActor> StoneClass;
};
