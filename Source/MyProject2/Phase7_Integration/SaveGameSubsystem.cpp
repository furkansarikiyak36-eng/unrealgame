#include "Phase7_Integration/SaveGameSubsystem.h"
#include "Phase1_Physical/LogActor.h"
#include "Core/LogDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/World.h"
bool USaveGameSubsystem::SaveWorld(const FString& SlotName)
{
    UWorld* World=GetGameInstance()?GetGameInstance()->GetWorld():nullptr;if(!World||SlotName.IsEmpty())return false;CachedLogs.Reset();
    for(TActorIterator<ALogActor> It(World);It;++It){ALogActor* L=*It;if(!L||L->GetLogState()==ELogState::Destroyed)continue;FLogSaveData D;D.Transform=L->GetActorTransform();D.State=(uint8)L->GetLogState();D.CurrentLength=L->GetCurrentLength();D.Shape=(uint8)L->GetCurrentShape();D.Definition=L->GetDefinition();D.Health=L->GetHealth();D.InfrastructureLinkKey=L->InfrastructureLinkKey;CachedLogs.Add(D);}
    ULogSaveGame* Save=Cast<ULogSaveGame>(UGameplayStatics::CreateSaveGameObject(ULogSaveGame::StaticClass()));if(!Save)return false;Save->Logs=CachedLogs;return UGameplayStatics::SaveGameToSlot(Save,SlotName,0);
}
bool USaveGameSubsystem::LoadWorld(const FString& SlotName)
{
    UWorld* World=GetGameInstance()?GetGameInstance()->GetWorld():nullptr;if(!World||SlotName.IsEmpty()||!UGameplayStatics::DoesSaveGameExist(SlotName,0))return false;ULogSaveGame* Save=Cast<ULogSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName,0));if(!Save)return false;CachedLogs=Save->Logs;
    for(TActorIterator<ALogActor> It(World);It;++It)It->Destroy();
    for(const FLogSaveData& D:CachedLogs){UClass* SpawnClass=LogClass.Get();if(!SpawnClass)SpawnClass=ALogActor::StaticClass();ALogActor* L=World->SpawnActor<ALogActor>(SpawnClass,D.Transform);if(!L)continue;if(D.Definition)L->InitializeFromDefinition(D.Definition);L->SetActorTransform(D.Transform);L->ApplyCut((EWoodShape)D.Shape,D.CurrentLength);L->SetHealth(D.Health);L->InfrastructureLinkKey=D.InfrastructureLinkKey;L->SetLogState((ELogState)D.State);}
    return true;
}
