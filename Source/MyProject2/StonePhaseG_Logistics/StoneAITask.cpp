#include "StoneAITask.h"
UStoneAITask::UStoneAITask() { PrimaryComponentTick.bCanEverTick = false; }
void UStoneAITask::AssignTask(EStoneAITaskType TaskType, AActor* Target)
{
    CurrentTask = TaskType; TaskTarget = TaskType == EStoneAITaskType::Idle ? nullptr : Target;
}
void UStoneAITask::CancelTask() { CurrentTask = EStoneAITaskType::Idle; TaskTarget = nullptr; }
