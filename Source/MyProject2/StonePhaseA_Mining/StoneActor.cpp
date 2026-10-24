#include "StoneActor.h"
#include "StoneCore/StoneDefinition.h"
#include "StoneCore/StoneStateMachine.h"
#include "StoneCore/StoneConnectionGraph.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

AStoneActor::AStoneActor()
{
    PrimaryActorTick.bCanEverTick = false;
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = MeshComponent;
    MeshComponent->SetCollisionProfileName(TEXT("PhysicsActor"));
    MeshComponent->SetSimulatePhysics(true);
    MeshComponent->SetMobility(EComponentMobility::Movable);
    StateMachine = CreateDefaultSubobject<UStoneStateMachine>(TEXT("StateMachine"));
    ConnectionGraph = CreateDefaultSubobject<UStoneConnectionGraph>(TEXT("ConnectionGraph"));
}

void AStoneActor::InitializeFromDefinition(UStoneDefinition* Def)
{
    if (!Def) return;
    BaseDefinition = Def;
    Definition = Def;
    MaxHealth = FMath::Max(1.0f, Def->CompressiveStrength * 0.5f);
    Health = MaxHealth;
    if (UStaticMesh* LoadedMesh = Def->Mesh.LoadSynchronous()) MeshComponent->SetStaticMesh(LoadedMesh);
    if (UMaterialInterface* Mat = Def->Material.LoadSynchronous()) MeshComponent->SetMaterial(0, Mat);
    MeshComponent->SetMassOverrideInKg(NAME_None, Def->Mass, true);
    StateMachine->OnStateChanged.AddUniqueDynamic(this, &AStoneActor::HandleStateChanged);
    SetStructuralState_Implementation(EStructuralNodeState::Loose);
}

EMaterialType AStoneActor::GetMaterialType_Implementation() const { return EMaterialType::Stone; }
EStructuralNodeState AStoneActor::GetStructuralState_Implementation() const
{ return StateMachine ? StateMachine->GetState() : EStructuralNodeState::Loose; }
bool AStoneActor::SetStructuralState_Implementation(EStructuralNodeState NewState)
{ return StateMachine && StateMachine->TransitionTo(NewState); }
float AStoneActor::GetStructuralMass_Implementation() const { return Definition ? Definition->Mass : 25.0f; }
float AStoneActor::GetStructuralStrength_Implementation() const { return Definition ? Definition->CompressiveStrength : 500.0f; }

bool AStoneActor::IsGrounded_Implementation() const
{
    if (!MeshComponent || !GetWorld()) return false;
    FHitResult Hit;
    const FVector Start = GetActorLocation();
    const FVector End = Start - FVector(0, 0, 40.0f);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(StoneGroundCheck), false, this);
    return GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params);
}

void AStoneActor::ApplyShape(EStoneShape NewShape, FVector NewDimensions)
{
    UStoneDefinition* ShapeBase = BaseDefinition ? BaseDefinition : Definition;
    if (!ShapeBase || NewDimensions.X <= 0 || NewDimensions.Y <= 0 || NewDimensions.Z <= 0) return;
    if (!BaseDefinition)
    {
        BaseDefinition = ShapeBase;
    }
    UStoneDefinition* NewDef = DuplicateObject<UStoneDefinition>(ShapeBase, this);
    NewDef->Shape = NewShape;
    NewDef->Dimensions = NewDimensions;
    NewDef->Volume = NewDef->GetVolumeFromDimensions();
    const float ScaleX = NewDimensions.X / 30.0f;
    const float ScaleY = NewDimensions.Y / 20.0f;
    const float ScaleZ = NewDimensions.Z / 20.0f;
    Definition = NewDef;
    MeshComponent->SetRelativeScale3D(FVector(ScaleX, ScaleY, ScaleZ));
}

bool AStoneActor::RestorePersistentState(UStoneDefinition* SavedBaseDefinition, EStoneShape SavedShape,
    const FVector& SavedDimensions, float SavedHealth, EStructuralNodeState SavedState, FString& OutError)
{
    OutError.Reset();
    if (!SavedBaseDefinition || !FMath::IsFinite(SavedHealth) || SavedHealth <= 0.0f
        || SavedHealth > FMath::Max(1.0f, SavedBaseDefinition->CompressiveStrength * 0.5f)
        || !FMath::IsFinite(SavedDimensions.X) || !FMath::IsFinite(SavedDimensions.Y) || !FMath::IsFinite(SavedDimensions.Z)
        || SavedDimensions.X <= 0.0f || SavedDimensions.Y <= 0.0f || SavedDimensions.Z <= 0.0f
        || (SavedState != EStructuralNodeState::Loose && SavedState != EStructuralNodeState::Installed))
    {
        OutError = TEXT("Stone persistence record has invalid definition, health, dimensions or state.");
        return false;
    }

    InitializeFromDefinition(SavedBaseDefinition);
    ApplyShape(SavedShape, SavedDimensions);
    Health = SavedHealth;

    if (!StateMachine)
    {
        OutError = TEXT("Stone has no state machine after initialization.");
        return false;
    }
    const EStructuralNodeState CurrentState = StateMachine->GetState();
    if (CurrentState != SavedState && !SetStructuralState_Implementation(SavedState))
    {
        OutError = TEXT("Stone state machine rejected the saved structural state.");
        return false;
    }
    return true;
}

void AStoneActor::ApplyDamage(float Amount)
{
    if (Amount <= 0.0f || Health <= 0.0f) return;
    Health = FMath::Max(0.0f, Health - Amount);
    if (Health <= 0.0f) SetStructuralState_Implementation(EStructuralNodeState::Fractured);
}

void AStoneActor::HandleStateChanged(AActor* Stone, EStructuralNodeState NewState)
{
    if (!MeshComponent) return;
    switch (NewState)
    {
    case EStructuralNodeState::Loose:
        MeshComponent->SetMobility(EComponentMobility::Movable);
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        MeshComponent->SetSimulatePhysics(true);
        break;
    case EStructuralNodeState::Carried:
    case EStructuralNodeState::Dragging:
        MeshComponent->SetSimulatePhysics(false);
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        break;
    case EStructuralNodeState::Installed:
        MeshComponent->SetSimulatePhysics(false);
        MeshComponent->SetMobility(EComponentMobility::Movable);
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        break;
    case EStructuralNodeState::Falling:
        MeshComponent->SetMobility(EComponentMobility::Movable);
        MeshComponent->SetSimulatePhysics(true);
        MeshComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
        break;
    case EStructuralNodeState::Fractured:
        // The fracture component owns rubble spawning; avoid spawning unconfigured stones here.
        Destroy();
        break;
    case EStructuralNodeState::Destroyed:
        Destroy();
        break;
    default: break;
    }
}
