#include "UndergroundSupportActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "UndergroundHazardSubsystem.h"

AUndergroundSupportActor::AUndergroundSupportActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SupportMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UndergroundSupportMesh"));
    SetRootComponent(SupportMesh);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultSupportCube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (DefaultSupportCube.Succeeded())
    {
        SupportMesh->SetStaticMesh(DefaultSupportCube.Object);
    }
    SupportMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    SupportMesh->SetCollisionObjectType(ECC_WorldStatic);
    SupportMesh->SetCollisionResponseToAllChannels(ECR_Block);

    SelfWeight = 35.0f;
    CompressiveCapacityKN = 40.0f;
    StructuralHealth = 100.0f;
    MaximumIntegrity = 100.0f;
    CurrentIntegrity = 100.0f;
}

void AUndergroundSupportActor::ApplySupportProfile()
{
    const float SafeMaximum = FMath::IsFinite(MaximumIntegrity)
        ? FMath::Clamp(MaximumIntegrity, 1.0f, 100000.0f)
        : 100.0f;
    MaximumIntegrity = SafeMaximum;
    CurrentIntegrity = FMath::Clamp(
        FMath::IsFinite(CurrentIntegrity) ? CurrentIntegrity : SafeMaximum,
        0.0f,
        SafeMaximum);

    switch (SupportType)
    {
        case EUndergroundSupportType::TimberPost:
            SelfWeight = 35.0f;
            CompressiveCapacityKN = 40.0f;
            MoistureDamagePerM3 = FMath::Max(0.0f, MoistureDamagePerM3);
            break;
        case EUndergroundSupportType::TimberBrace:
            SelfWeight = 25.0f;
            CompressiveCapacityKN = 28.0f;
            MoistureDamagePerM3 = FMath::Max(0.0f, MoistureDamagePerM3);
            break;
        case EUndergroundSupportType::StonePillar:
            SelfWeight = 220.0f;
            CompressiveCapacityKN = 180.0f;
            MoistureDamagePerM3 = FMath::Min(FMath::Max(0.0f, MoistureDamagePerM3), 1.0f);
            break;
        case EUndergroundSupportType::ReinforcedBeam:
            SelfWeight = 160.0f;
            CompressiveCapacityKN = 240.0f;
            MoistureDamagePerM3 = FMath::Min(FMath::Max(0.0f, MoistureDamagePerM3), 0.5f);
            break;
        default:
            SupportType = EUndergroundSupportType::TimberPost;
            SelfWeight = 35.0f;
            CompressiveCapacityKN = 40.0f;
            break;
    }

    if (SupportMesh)
    {
        switch (SupportType)
        {
            case EUndergroundSupportType::TimberPost:
                SupportMesh->SetRelativeScale3D(FVector(0.18f, 0.18f, 1.5f));
                SupportMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 75.0f));
                break;
            case EUndergroundSupportType::TimberBrace:
                SupportMesh->SetRelativeScale3D(FVector(1.5f, 0.18f, 0.18f));
                SupportMesh->SetRelativeLocation(FVector::ZeroVector);
                break;
            case EUndergroundSupportType::StonePillar:
                SupportMesh->SetRelativeScale3D(FVector(0.42f, 0.42f, 1.5f));
                SupportMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 75.0f));
                break;
            case EUndergroundSupportType::ReinforcedBeam:
                SupportMesh->SetRelativeScale3D(FVector(1.5f, 0.22f, 0.25f));
                SupportMesh->SetRelativeLocation(FVector::ZeroVector);
                break;
            default:
                break;
        }
    }

    StructuralHealth = CurrentIntegrity;
}

void AUndergroundSupportActor::BeginPlay()
{
    if (!SupportId.IsValid())
    {
        SupportId = FGuid::NewGuid();
    }
    ApplySupportProfile();
    Super::BeginPlay();

    if (UWorld* World = GetWorld())
    {
        if (UUndergroundHazardSubsystem* Hazards = World->GetSubsystem<UUndergroundHazardSubsystem>())
        {
            Hazards->RegisterSupportActor(this);
        }
    }
}

void AUndergroundSupportActor::ConfigureSupport(
    const FGuid& InSupportId,
    EUndergroundSupportType InType,
    float InIntegrity)
{
    SupportId = InSupportId.IsValid() ? InSupportId : FGuid::NewGuid();
    SupportType = InType;
    CurrentIntegrity = FMath::IsFinite(InIntegrity)
        ? FMath::Clamp(InIntegrity, 0.0f, FMath::Max(1.0f, MaximumIntegrity))
        : FMath::Max(1.0f, MaximumIntegrity);
    ApplySupportProfile();
}

float AUndergroundSupportActor::ApplyDamage(float Amount, const FVector& Direction)
{
    if (IsCollapsed() || !FMath::IsFinite(Amount) || Amount <= 0.0f)
    {
        return CurrentIntegrity;
    }

    const float Remaining = Super::ApplyDamage(Amount, Direction);
    CurrentIntegrity = FMath::Clamp(Remaining, 0.0f, MaximumIntegrity);
    if (UWorld* World = GetWorld())
    {
        if (UUndergroundHazardSubsystem* Hazards = World->GetSubsystem<UUndergroundHazardSubsystem>())
        {
            Hazards->NotifySupportActorChanged(this);
        }
    }
    return CurrentIntegrity;
}

void AUndergroundSupportActor::ApplyMoistureExposure(float WaterVolumeM3)
{
    if (!IsSupportOperational() || !FMath::IsFinite(WaterVolumeM3) || WaterVolumeM3 <= 0.0f)
    {
        return;
    }

    const float Damage = WaterVolumeM3 * FMath::Max(0.0f, MoistureDamagePerM3);
    if (Damage <= 0.0f)
    {
        return;
    }

    CurrentIntegrity = FMath::Clamp(CurrentIntegrity - Damage, 0.0f, MaximumIntegrity);
    StructuralHealth = CurrentIntegrity;
    if (CurrentIntegrity <= 0.0f)
    {
        TriggerChaosCollapse(FVector(0.0f, 0.0f, -80.0f));
    }
    else if (UWorld* World = GetWorld())
    {
        if (UUndergroundHazardSubsystem* Hazards = World->GetSubsystem<UUndergroundHazardSubsystem>())
        {
            Hazards->NotifySupportActorChanged(this);
        }
    }
}

void AUndergroundSupportActor::TriggerChaosCollapse(const FVector& Impulse)
{
    if (IsCollapsed())
    {
        return;
    }

    Super::TriggerChaosCollapse(Impulse);
    if (SupportMesh)
    {
        SupportMesh->SetMobility(EComponentMobility::Movable);
        SupportMesh->SetSimulatePhysics(true);
        if (!Impulse.IsNearlyZero())
        {
            SupportMesh->AddImpulse(Impulse, NAME_None, true);
        }
    }

    if (UWorld* World = GetWorld())
    {
        if (UUndergroundHazardSubsystem* Hazards = World->GetSubsystem<UUndergroundHazardSubsystem>())
        {
            Hazards->UnregisterSupportActor(this);
        }
    }
}
