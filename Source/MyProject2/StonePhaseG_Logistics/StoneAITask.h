#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StoneAITask.generated.h"
UENUM(BlueprintType)
enum class EStoneAITaskType : uint8
{
    Gather UMETA(DisplayName = "Taş Topla"), Deliver UMETA(DisplayName = "Depoya Taşı"),
    BuildWall UMETA(DisplayName = "Duvar Ör"), MineRock UMETA(DisplayName = "Kaya Kaz"),
    Idle UMETA(DisplayName = "Bekle")
};
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class MYPROJECT2_API UStoneAITask : public UActorComponent
{
    GENERATED_BODY()
public:
    UStoneAITask();
    UFUNCTION(BlueprintCallable) void AssignTask(EStoneAITaskType TaskType, AActor* Target);
    UFUNCTION(BlueprintCallable) void CancelTask();
    UFUNCTION(BlueprintPure) EStoneAITaskType GetCurrentTask() const { return CurrentTask; }
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) EStoneAITaskType CurrentTask = EStoneAITaskType::Idle;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly) TObjectPtr<AActor> TaskTarget = nullptr;
};
