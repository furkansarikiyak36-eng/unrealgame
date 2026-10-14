#include "StickBreakingComponent.h"
#include "StickPhaseS1_Core/StickActor.h"
#include "StickCore/StickDefinition.h"
#include "Engine/World.h"

UStickBreakingComponent::UStickBreakingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

bool UStickBreakingComponent::BeginBreak(AStickActor* Stick)
{
    if (!IsValid(Stick) || bIsBreaking)
    {
        return false;
    }

    BreakingStick = Stick;
    bIsBreaking = true;
    BreakProgress = 0.0f;
    return true;
}

bool UStickBreakingComponent::CompleteBreak()
{
    if (!IsValid(BreakingStick) || !GetWorld())
    {
        return false;
    }

    AStickActor* Source = BreakingStick;
    const FVector Origin = Source->GetActorLocation();
    const FRotator Rotation = Source->GetActorRotation();
    const int32 ExpectedPieces = FMath::Max(1, PiecesPerBreak);
    TArray<AStickActor*> SpawnedPieces;
    SpawnedPieces.Reserve(ExpectedPieces);

    for (int32 Index = 0; Index < ExpectedPieces; ++Index)
    {
        AStickActor* Piece = GetWorld()->SpawnActor<AStickActor>(
            Source->GetClass(),
            Origin + FVector(Index * 5.0f, 0.0f, 0.0f),
            Rotation);

        if (!Piece)
        {
            for (AStickActor* Spawned : SpawnedPieces)
            {
                if (IsValid(Spawned))
                {
                    Spawned->Destroy();
                }
            }

            // Keep the source stick. Reset this attempt so Tick does not retry every frame.
            BreakingStick = nullptr;
            bIsBreaking = false;
            BreakProgress = 0.0f;
            return false;
        }

        if (UStickDefinition* Definition = Source->GetDefinition())
        {
            Piece->InitializeFromDefinition(Definition);
        }
        Piece->ApplyShape(EStickShape::Broken);
        SpawnedPieces.Add(Piece);
    }

    if (SpawnedPieces.Num() != ExpectedPieces || !Source->Destroy())
    {
        for (AStickActor* Spawned : SpawnedPieces)
        {
            if (IsValid(Spawned))
            {
                Spawned->Destroy();
            }
        }

        BreakingStick = nullptr;
        bIsBreaking = false;
        BreakProgress = 0.0f;
        return false;
    }

    BreakingStick = nullptr;
    bIsBreaking = false;
    BreakProgress = 0.0f;
    return true;
}

bool UStickBreakingComponent::CancelBreak()
{
    if (!bIsBreaking) return false;
    bIsBreaking = false;
    BreakingStick = nullptr;
    BreakProgress = 0.0f;
    return true;
}

void UStickBreakingComponent::TickComponent(
    float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bIsBreaking || !IsValid(BreakingStick)) return;
    BreakProgress += DeltaTime;
    if (BreakProgress >= FMath::Max(BreakDuration, 0.01f))
    {
        CompleteBreak();
    }
}
