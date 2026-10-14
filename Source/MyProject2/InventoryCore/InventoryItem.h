#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InventoryTypes.h"
#include "InventoryItem.generated.h"

class UTexture2D;
class UStaticMesh;

UCLASS(BlueprintType)
class MYPROJECT2_API UInventoryItemData : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity") FName ItemID;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity") FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity") FText Description;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity") EInventoryCategory Category = EInventoryCategory::Resource;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Identity") EItemRarity Rarity = EItemRarity::Common;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats") int32 MaxStack = 20;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats") float Weight = 0.1f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats") bool bConsumable = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats") bool bQuestItem = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats") bool bCanBeDropped = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats") bool bStackable = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual") TSoftObjectPtr<UTexture2D> Icon;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual") TSoftObjectPtr<UStaticMesh> WorldMesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting") bool bCanBeIngredient = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting") bool bIsTool = false;
    UFUNCTION(BlueprintCallable, Category="Validation") bool ValidateData(FString& OutError) const;
    UFUNCTION(BlueprintPure, Category="Stack") FInventoryItem ToInventoryItem(int32 InCount = 1) const;
};

UCLASS()
class MYPROJECT2_API UInventoryItemLibrary : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Library") static UInventoryItemData* FindItemData(FName ItemID);
    UFUNCTION(BlueprintCallable, Category="Library") static void RegisterItemData(UInventoryItemData* ItemData);
    UFUNCTION(BlueprintCallable, Category="Library") static TArray<UInventoryItemData*> GetAllItems();
private:
    static TMap<FName, TWeakObjectPtr<UInventoryItemData>> ItemRegistry;
};
