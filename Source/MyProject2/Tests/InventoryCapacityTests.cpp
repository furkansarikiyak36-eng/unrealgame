#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "InventoryCore/InventoryComponent.h"
#include "InventoryCore/InventoryItem.h"

namespace
{
    UWorld* CreateInventoryCapacityTestWorld()
    {
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
        if (!World) return nullptr;
        UWorld::InitializationValues Values;
        Values.AllowAudioPlayback(false);
        Values.RequiresHitProxies(false);
        Values.CreatePhysicsScene(false);
        Values.CreateNavigation(false);
        Values.CreateAISystem(false);
        Values.ShouldSimulatePhysics(false);
        World->InitializeNewWorld(Values);
        return World;
    }

    void DestroyInventoryCapacityTestWorld(UWorld* World)
    {
        if (World) World->DestroyWorld(false);
    }

    bool CreateInventoryFixture(UWorld* World, AActor*& Owner,
        UInventoryComponent*& Inventory, UInventoryItemData*& Existing, UInventoryItemData*& NewItem)
    {
        if (!World) return false;
        Owner = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator);
        if (!Owner) return false;
        Inventory = NewObject<UInventoryComponent>(Owner);
        if (!Inventory) return false;
        Inventory->MaxSlots = 4;
        Inventory->MaxWeight = 100.0f;
        Existing = NewObject<UInventoryItemData>(Inventory);
        NewItem = NewObject<UInventoryItemData>(Inventory);
        if (!Existing || !NewItem) return false;
        Existing->ItemID = FName(TEXT("ExistingTool"));
        Existing->DisplayName = FText::FromString(TEXT("Existing tool"));
        Existing->bStackable = false;
        Existing->MaxStack = 1;
        Existing->Weight = 0.1f;
        NewItem->ItemID = FName(TEXT("NewTool"));
        NewItem->DisplayName = FText::FromString(TEXT("New tool"));
        NewItem->bStackable = false;
        NewItem->MaxStack = 1;
        NewItem->Weight = 0.1f;
        Inventory->ItemDefinitions.Add(Existing);
        Inventory->ItemDefinitions.Add(NewItem);
        Owner->AddInstanceComponent(Inventory);
        Inventory->RegisterComponent();
        World->BeginPlay();
        return Inventory->GetSlots().Num() == 4;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FInventoryCapacityShrinkRollbackTest,
    "MyProject2.Inventory.Core.Capacity.ShrinkRollsBack",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryCapacityShrinkRollbackTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreateInventoryCapacityTestWorld();
    if (!TestNotNull(TEXT("Test world created"), World)) return false;
    AActor* Owner = nullptr;
    UInventoryComponent* Inventory = nullptr;
    UInventoryItemData* Existing = nullptr;
    UInventoryItemData* NewItem = nullptr;
    const bool bValid = CreateInventoryFixture(World, Owner, Inventory, Existing, NewItem);
    TestTrue(TEXT("Inventory fixture created and slots initialized"), bValid);
    if (!bValid) { DestroyInventoryCapacityTestWorld(World); return false; }

    TestTrue(TEXT("Seed first active slot"),
        Inventory->AddItem(Existing->ItemID, 1) == EInventoryResult::Success);
    Inventory->MaxSlots = 2;
    TestFalse(TEXT("Preflight sees only the two active slots"), Inventory->CanAddItem(NewItem->ItemID, 2));
    TestTrue(TEXT("Over-capacity add is rejected"),
        Inventory->AddItem(NewItem->ItemID, 2) == EInventoryResult::InventoryFull);
    TestEqual(TEXT("Failed add does not leave partial output"), Inventory->GetItemCount(NewItem->ItemID), 0);
    TestEqual(TEXT("Existing item is preserved"), Inventory->GetItemCount(Existing->ItemID), 1);
    TestEqual(TEXT("Used-slot count includes only active capacity"), Inventory->GetUsedSlotCount(), 1);
    TestEqual(TEXT("One active slot remains free"), Inventory->GetFreeSlotCount(), 1);
    DestroyInventoryCapacityTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FInventoryCapacityValidAddTest,
    "MyProject2.Inventory.Core.Capacity.ValidAdd",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryCapacityValidAddTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreateInventoryCapacityTestWorld();
    if (!TestNotNull(TEXT("Test world created"), World)) return false;
    AActor* Owner = nullptr;
    UInventoryComponent* Inventory = nullptr;
    UInventoryItemData* Existing = nullptr;
    UInventoryItemData* NewItem = nullptr;
    const bool bValid = CreateInventoryFixture(World, Owner, Inventory, Existing, NewItem);
    TestTrue(TEXT("Inventory fixture created and slots initialized"), bValid);
    if (!bValid) { DestroyInventoryCapacityTestWorld(World); return false; }

    Inventory->MaxSlots = 2;
    TestTrue(TEXT("Two individual items fit in two active slots"),
        Inventory->AddItem(NewItem->ItemID, 2) == EInventoryResult::Success);
    TestEqual(TEXT("Both items were added"), Inventory->GetItemCount(NewItem->ItemID), 2);
    TestEqual(TEXT("Two active slots are used"), Inventory->GetUsedSlotCount(), 2);
    TestEqual(TEXT("No active slot remains free"), Inventory->GetFreeSlotCount(), 0);
    DestroyInventoryCapacityTestWorld(World);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FInventoryRestoreFailurePreservesCurrentTest,
    "MyProject2.Inventory.Core.Capacity.RestoreFailurePreservesCurrent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryRestoreFailurePreservesCurrentTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreateInventoryCapacityTestWorld();
    if (!TestNotNull(TEXT("Test world created"), World)) return false;
    AActor* Owner = nullptr;
    UInventoryComponent* Inventory = nullptr;
    UInventoryItemData* Existing = nullptr;
    UInventoryItemData* NewItem = nullptr;
    const bool bValid = CreateInventoryFixture(World, Owner, Inventory, Existing, NewItem);
    TestTrue(TEXT("Inventory fixture created and slots initialized"), bValid);
    if (!bValid) { DestroyInventoryCapacityTestWorld(World); return false; }

    TestTrue(TEXT("Current inventory is seeded"),
        Inventory->AddItem(Existing->ItemID, 1) == EInventoryResult::Success);
    Inventory->MaxSlots = 2;

    TArray<FInventoryItem> TooLargeSave;
    for (int32 i = 0; i < 3; ++i)
    {
        FInventoryItem Saved;
        Saved.ItemID = FName(*FString::Printf(TEXT("SavedItem%d"), i));
        Saved.Count = 1;
        Saved.MaxStack = 1;
        Saved.Weight = 0.1f;
        TooLargeSave.Add(Saved);
    }

    TestFalse(TEXT("Oversized saved inventory is rejected"),
        Inventory->RestoreFromSavedItems(TooLargeSave));
    TestEqual(TEXT("Failed restore preserves previous item"),
        Inventory->GetItemCount(Existing->ItemID), 1);
    TestEqual(TEXT("Failed restore inserts no partial saved item"),
        Inventory->GetItemCount(FName(TEXT("SavedItem0"))), 0);
    DestroyInventoryCapacityTestWorld(World);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FInventoryRestoreInvalidEntryPreservesCurrentTest,
    "MyProject2.Inventory.Core.Capacity.RestoreInvalidEntryPreservesCurrent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryRestoreInvalidEntryPreservesCurrentTest::RunTest(const FString& Parameters)
{
    UWorld* World = CreateInventoryCapacityTestWorld();
    if (!TestNotNull(TEXT("Test world created"), World)) return false;
    AActor* Owner = nullptr;
    UInventoryComponent* Inventory = nullptr;
    UInventoryItemData* Existing = nullptr;
    UInventoryItemData* NewItem = nullptr;
    const bool bValid = CreateInventoryFixture(World, Owner, Inventory, Existing, NewItem);
    TestTrue(TEXT("Inventory fixture created and slots initialized"), bValid);
    if (!bValid) { DestroyInventoryCapacityTestWorld(World); return false; }

    TestTrue(TEXT("Current inventory is seeded"),
        Inventory->AddItem(Existing->ItemID, 1) == EInventoryResult::Success);

    FInventoryItem InvalidSavedItem;
    InvalidSavedItem.ItemID = FName(TEXT("CorruptItem"));
    InvalidSavedItem.Count = -1;
    InvalidSavedItem.MaxStack = 1;
    InvalidSavedItem.Weight = 0.1f;
    TArray<FInventoryItem> InvalidSave;
    InvalidSave.Add(InvalidSavedItem);

    TestFalse(TEXT("Negative saved-item count is rejected"),
        Inventory->RestoreFromSavedItems(InvalidSave));
    TestEqual(TEXT("Rejected corrupt save preserves current inventory"),
        Inventory->GetItemCount(Existing->ItemID), 1);
    TestEqual(TEXT("Corrupt saved item was not inserted"),
        Inventory->GetItemCount(InvalidSavedItem.ItemID), 0);

    DestroyInventoryCapacityTestWorld(World);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
