#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ChunkSerializer.h"
#include "WorldPersistence.generated.h"
class UChunkStateStore; class UWorldStateSubsystem;
UCLASS()
class MYPROJECT2_API UWorldPersistence : public UObject
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable,Category="Persistence") bool SaveToFile(const FString& FilePath,UChunkStateStore* Store);
 UFUNCTION(BlueprintCallable,Category="Persistence") bool LoadFromFile(const FString& FilePath,UChunkStateStore* Store);
 UFUNCTION(BlueprintCallable,Category="Persistence") TArray<uint8> SerializeToBytes(UChunkStateStore* Store);
 UFUNCTION(BlueprintCallable,Category="Persistence") bool DeserializeFromBytes(const TArray<uint8>& Bytes,UChunkStateStore* Store);
 bool SaveWorldToFile(const FString& FilePath,UWorldStateSubsystem* State);
 bool LoadWorldFromFile(const FString& FilePath,UWorldStateSubsystem* State);
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bCompressSave=true;
protected:
 TArray<uint8> SerializeWorldToBytes(UWorldStateSubsystem* State);
 bool DeserializeWorldFromBytes(const TArray<uint8>& Bytes,UWorldStateSubsystem* State);
 TArray<uint8> WrapPayload(const TArray<uint8>& Payload) const;
 bool UnwrapPayload(const TArray<uint8>& Bytes,TArray<uint8>& OutPayload,int32* OutVersion=nullptr) const;
};
