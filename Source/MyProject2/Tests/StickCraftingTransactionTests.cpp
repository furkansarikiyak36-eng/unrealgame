#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "StickPhaseS1_Core/StickInventoryComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStickCraftTransactionPreflightTest,
    "MyProject2.Inventory.StickCraftTransaction.Preflight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStickCraftTransactionPreflightTest::RunTest(const FString& Parameters)
{
    UStickInventoryComponent* Inventory = NewObject<UStickInventoryComponent>();
    TestNotNull(TEXT("Inventory component should be created"), Inventory);
    if (!Inventory)
    {
        return false;
    }

    Inventory->GlobalCapacity = 10;
    TestEqual(TEXT("Seed stick inventory"), Inventory->AddSticks(FName(TEXT("Twig")), 5), 5);

    FStickBundle FirstRequirement;
    FirstRequirement.StickType = FName(TEXT("Twig"));
    FirstRequirement.Count = 3;

    FStickBundle DuplicateRequirement;
    DuplicateRequirement.StickType = FName(TEXT("Twig"));
    DuplicateRequirement.Count = 3;

    TArray<FStickBundle> DuplicateInputs;
    DuplicateInputs.Add(FirstRequirement);
    DuplicateInputs.Add(DuplicateRequirement);

    TestFalse(TEXT("Duplicate ingredient rows must be aggregated before validation"),
        Inventory->CanApplyCraftTransaction(DuplicateInputs, FName(TEXT("Rope")), 1));
    TestFalse(TEXT("Invalid craft transaction must not apply"),
        Inventory->ApplyCraftTransaction(DuplicateInputs, FName(TEXT("Rope")), 1));
    TestEqual(TEXT("Failed preflight preserves all source sticks"),
        Inventory->GetCount(FName(TEXT("Twig"))), 5);
    TestEqual(TEXT("Failed preflight creates no output"),
        Inventory->GetCount(FName(TEXT("Rope"))), 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStickCraftTransactionCapacityTest,
    "MyProject2.Inventory.StickCraftTransaction.Capacity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStickCraftTransactionCapacityTest::RunTest(const FString& Parameters)
{
    UStickInventoryComponent* Inventory = NewObject<UStickInventoryComponent>();
    TestNotNull(TEXT("Inventory component should be created"), Inventory);
    if (!Inventory)
    {
        return false;
    }

    Inventory->GlobalCapacity = 5;
    TestEqual(TEXT("Fill inventory to capacity"), Inventory->AddSticks(FName(TEXT("Twig")), 5), 5);

    FStickBundle Requirement;
    Requirement.StickType = FName(TEXT("Twig"));
    Requirement.Count = 2;

    TArray<FStickBundle> Inputs;
    Inputs.Add(Requirement);

    TestFalse(TEXT("Output that exceeds post-consumption capacity is rejected"),
        Inventory->CanApplyCraftTransaction(Inputs, FName(TEXT("Rope")), 3));
    TestFalse(TEXT("Capacity failure must not consume ingredients"),
        Inventory->ApplyCraftTransaction(Inputs, FName(TEXT("Rope")), 3));
    TestEqual(TEXT("Failed capacity transaction preserves the source"),
        Inventory->GetCount(FName(TEXT("Twig"))), 5);
    TestEqual(TEXT("Failed capacity transaction creates no output"),
        Inventory->GetCount(FName(TEXT("Rope"))), 0);

    TestTrue(TEXT("Capacity freed by ingredients may be reused by output"),
        Inventory->ApplyCraftTransaction(Inputs, FName(TEXT("Rope")), 2));
    TestEqual(TEXT("Consumed twig count"), Inventory->GetCount(FName(TEXT("Twig"))), 3);
    TestEqual(TEXT("Produced rope count"), Inventory->GetCount(FName(TEXT("Rope"))), 2);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStickCraftTransactionSuccessTest,
    "MyProject2.Inventory.StickCraftTransaction.Success",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStickCraftTransactionSuccessTest::RunTest(const FString& Parameters)
{
    UStickInventoryComponent* Inventory = NewObject<UStickInventoryComponent>();
    TestNotNull(TEXT("Inventory component should be created"), Inventory);
    if (!Inventory)
    {
        return false;
    }

    Inventory->GlobalCapacity = 10;
    TestEqual(TEXT("Seed stick inventory"), Inventory->AddSticks(FName(TEXT("Twig")), 5), 5);

    FStickBundle Requirement;
    Requirement.StickType = FName(TEXT("Twig"));
    Requirement.Count = 3;

    TArray<FStickBundle> Inputs;
    Inputs.Add(Requirement);

    TestTrue(TEXT("Valid transaction should succeed"),
        Inventory->ApplyCraftTransaction(Inputs, FName(TEXT("Rope")), 2));
    TestEqual(TEXT("Remaining input count"), Inventory->GetCount(FName(TEXT("Twig"))), 2);
    TestEqual(TEXT("Output count"), Inventory->GetCount(FName(TEXT("Rope"))), 2);

    const TArray<FStickBundle> InvalidInputs;
    TestFalse(TEXT("Invalid output type is rejected"),
        Inventory->ApplyCraftTransaction(InvalidInputs, NAME_None, 1));
    TestEqual(TEXT("Invalid transaction preserves existing output"),
        Inventory->GetCount(FName(TEXT("Rope"))), 2);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
