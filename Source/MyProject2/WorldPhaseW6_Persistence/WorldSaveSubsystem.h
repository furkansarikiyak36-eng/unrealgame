#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "WorldSaveSubsystem.generated.h"
class UWorldPersistence;
UCLASS()
class MYPROJECT2_API UWorldSaveSubsystem : public UGameInstanceSubsystem
{
 GENERATED_BODY()
public:
 virtual void Initialize(FSubsystemCollectionBase& Collection) override;
 virtual void Deinitialize() override;
 UFUNCTION(BlueprintCallable,Category="Save") bool SaveWorld(const FString& SlotName);
 UFUNCTION(BlueprintCallable,Category="Save") bool LoadWorld(const FString& SlotName);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float AutoSaveIntervalSeconds=300.0f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bAutoSaveEnabled=true;
protected:
 UPROPERTY() TObjectPtr<UWorldPersistence> Persistence;
 double LastAutoSaveTime=0.0;
 FTSTicker::FDelegateHandle AutoSaveTickerHandle;
 bool TickAutoSave(float DeltaTime);
 FString GetSlotPath(const FString& SlotName) const;
};
