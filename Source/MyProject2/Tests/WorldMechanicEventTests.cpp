#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "WorldCore/Events/WorldMechanicEventSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldMechanicEventRejectsInvalidPayload,
    "MyProject2.WorldEvents.RejectsInvalidPayload",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldMechanicEventRejectsInvalidPayload::RunTest(const FString& Parameters)
{
    FWorldMechanicEvent Event;
    TestFalse(TEXT("None event type is invalid"), Event.IsValid());

    Event.Type = EWorldMechanicEventType::WaterStateChanged;
    TestTrue(TEXT("Finite typed event is valid"), Event.IsValid());

    Event.ScalarValue = TNumericLimits<float>::QuietNaN();
    TestFalse(TEXT("NaN payload is invalid"), Event.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldMechanicEventQueueBoundsAndPrioritizes,
    "MyProject2.WorldEvents.QueueIsBoundedAndPrioritized",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldMechanicEventQueueBoundsAndPrioritizes::RunTest(const FString& Parameters)
{
    UWorldMechanicEventSubsystem* Events = NewObject<UWorldMechanicEventSubsystem>(GetTransientPackage());
    TestNotNull(TEXT("Event subsystem object created"), Events);
    if (!Events) return false;

    Events->MaxQueuedEvents = 64;
    for (int32 Index = 0; Index < 64; ++Index)
    {
        FWorldMechanicEvent Event;
        Event.Type = EWorldMechanicEventType::WaterStateChanged;
        Event.ScalarValue = static_cast<float>(Index);
        TestTrue(TEXT("Queue accepts normal event"), Events->EnqueueEvent(Event));
    }
    TestEqual(TEXT("Queue reached configured bound"), Events->GetPendingEventCount(), 64);

    const uint32 VersionBeforeReject = Events->GetWorldVersion();
    FWorldMechanicEvent NormalOverflow;
    NormalOverflow.Type = EWorldMechanicEventType::WaterStateChanged;
    TestFalse(TEXT("Normal overflow is rejected"), Events->EnqueueEvent(NormalOverflow));
    TestEqual(TEXT("Rejected event does not advance world version"), Events->GetWorldVersion(), VersionBeforeReject);
    TestEqual(TEXT("Queue remains bounded after rejection"), Events->GetPendingEventCount(), 64);

    FWorldMechanicEvent Critical;
    Critical.Type = EWorldMechanicEventType::VoxelEditCommitted;
    Critical.bHighPriority = true;
    TestTrue(TEXT("Critical event displaces a normal event"), Events->EnqueueEvent(Critical));
    TestEqual(TEXT("Queue remains bounded after displacement"), Events->GetPendingEventCount(), 64);
    TestEqual(TEXT("Accepted voxel edit advances world version"), Events->GetWorldVersion(), VersionBeforeReject + 1u);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
