#include "WorldSaveSubsystem.h"
#include "WorldPersistence.h"
#include "WorldCore/WorldStateSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Misc/Paths.h"
void UWorldSaveSubsystem::Initialize(FSubsystemCollectionBase& C){Super::Initialize(C);Persistence=NewObject<UWorldPersistence>(this);LastAutoSaveTime=FPlatformTime::Seconds();AutoSaveTickerHandle=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this,&UWorldSaveSubsystem::TickAutoSave),1.0f);}
void UWorldSaveSubsystem::Deinitialize(){if(AutoSaveTickerHandle.IsValid())FTSTicker::GetCoreTicker().RemoveTicker(AutoSaveTickerHandle);Super::Deinitialize();}
FString UWorldSaveSubsystem::GetSlotPath(const FString& Slot) const{const FString Safe=FPaths::MakeValidFileName(Slot);return FPaths::ProjectSavedDir()/TEXT("World")/(Safe+TEXT(".bin"));}
bool UWorldSaveSubsystem::TickAutoSave(float){if(!bAutoSaveEnabled)return true;const double Now=FPlatformTime::Seconds();if(Now-LastAutoSaveTime>=FMath::Max(10.0f,AutoSaveIntervalSeconds)){SaveWorld(TEXT("Autosave"));LastAutoSaveTime=Now;}return true;}
bool UWorldSaveSubsystem::SaveWorld(const FString& Slot){if(!Persistence||!GetGameInstance())return false;UWorld* W=GetGameInstance()->GetWorld();UWorldStateSubsystem* S=W?W->GetSubsystem<UWorldStateSubsystem>():nullptr;return S&&Persistence->SaveWorldToFile(GetSlotPath(Slot),S);}
bool UWorldSaveSubsystem::LoadWorld(const FString& Slot){if(!Persistence||!GetGameInstance())return false;UWorld* W=GetGameInstance()->GetWorld();UWorldStateSubsystem* S=W?W->GetSubsystem<UWorldStateSubsystem>():nullptr;return S&&Persistence->LoadWorldFromFile(GetSlotPath(Slot),S);}
