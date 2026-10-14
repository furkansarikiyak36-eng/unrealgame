#include "Phase1_Physical/LogActor.h"
#include "Core/LogDefinition.h"
#include "Core/LogStateMachine.h"
#include "Core/LogConnectionGraph.h"
#include "Core/StructuralGraph.h"
#include "Phase1_Physical/LogPhysicsComponent.h"
#include "Components/StaticMeshComponent.h"

ALogActor::ALogActor()
{
    PrimaryActorTick.bCanEverTick = false;
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh")); RootComponent = MeshComponent;
    MeshComponent->SetCollisionProfileName(TEXT("PhysicsActor")); MeshComponent->SetSimulatePhysics(true); MeshComponent->SetMobility(EComponentMobility::Movable);
    StateMachine = CreateDefaultSubobject<ULogStateMachine>(TEXT("StateMachine"));
    ConnectionGraph = CreateDefaultSubobject<ULogConnectionGraph>(TEXT("ConnectionGraph"));
    StructuralGraph = CreateDefaultSubobject<UStructuralGraph>(TEXT("StructuralGraph"));
    PhysicsComponent = CreateDefaultSubobject<ULogPhysicsComponent>(TEXT("PhysicsComponent"));
}

void ALogActor::BeginPlay()
{
    Super::BeginPlay();
    if (StateMachine && !bStateDelegateBound) { StateMachine->OnStateChanged.AddDynamic(this, &ALogActor::HandleStateChanged); bStateDelegateBound = true; }
    if (Definition && !MeshComponent->GetStaticMesh()) InitializeFromDefinition(Definition);
    HandleStateChanged(this, GetLogState());
}

void ALogActor::InitializeFromDefinition(ULogDefinition* Def)
{
    if (!Def) return; FString Error;
    if (!Def->ValidateDefinition(Error)) { UE_LOG(LogTemp, Warning, TEXT("Invalid log definition %s: %s"), *GetNameSafe(Def), *Error); return; }
    Definition = Def; CurrentLength = Def->Length; CurrentShape = Def->Shape;
    if (UStaticMesh* LoadedMesh = Def->Mesh.LoadSynchronous()) MeshComponent->SetStaticMesh(LoadedMesh);
    if (UMaterialInterface* Mat = Def->Material.LoadSynchronous()) MeshComponent->SetMaterial(0, Mat);
    BaseMeshScale = MeshComponent->GetRelativeScale3D();
    MeshComponent->SetMassOverrideInKg(NAME_None, Def->Mass, true);
    if (StateMachine && HasActorBegunPlay() && !bStateDelegateBound) { StateMachine->OnStateChanged.AddDynamic(this, &ALogActor::HandleStateChanged); bStateDelegateBound = true; }
}

bool ALogActor::IsLogGrounded() const
{
    if (!GetWorld()) return false;
    FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(LogGroundCheck), false, this);
    const FVector Start = GetActorLocation(); const FVector End = Start - FVector(0,0,60.0f);
    return GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params);
}
ELogState ALogActor::GetLogState() const { return StateMachine ? StateMachine->GetState() : ELogState::Loose; }
bool ALogActor::SetLogState(ELogState NewState) { return StateMachine && StateMachine->TransitionTo(NewState); }
FVector ALogActor::GetCylinderAxis() const { return GetActorForwardVector(); }

void ALogActor::ApplyCut(EWoodShape NewShape, float NewLength)
{
    if (!Definition || NewLength <= 0.0f) return;
    CurrentShape = NewShape; CurrentLength = FMath::Clamp(NewLength, 1.0f, Definition->Length);
    // Assumes the configured mesh's longitudinal axis is local X; content must follow this convention.
    const float BaseLength = FMath::Max(Definition->Length, 1.0f);
    FVector Scale = BaseMeshScale; Scale.X *= CurrentLength / BaseLength; MeshComponent->SetRelativeScale3D(Scale);
    const float VolumeRatio = CurrentLength / BaseLength;
    MeshComponent->SetMassOverrideInKg(NAME_None, FMath::Max(0.05f, Definition->Mass * VolumeRatio), true);
}

void ALogActor::SetHealth(float NewHealth) { Health = FMath::Clamp(NewHealth, 0.0f, 100.0f); }

void ALogActor::HandleStateChanged(AActor*, ELogState NewState)
{
    if (!MeshComponent) return;
    switch (NewState)
    {
    case ELogState::Loose: MeshComponent->SetMobility(EComponentMobility::Movable); MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); MeshComponent->SetSimulatePhysics(true); break;
    case ELogState::Carried: case ELogState::Dragging: MeshComponent->SetMobility(EComponentMobility::Movable); MeshComponent->SetSimulatePhysics(false); MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly); break;
    case ELogState::Installed: MeshComponent->SetMobility(EComponentMobility::Movable); MeshComponent->SetSimulatePhysics(false); MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); break;
    case ELogState::Falling: MeshComponent->SetMobility(EComponentMobility::Movable); MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); MeshComponent->SetSimulatePhysics(true); break;
    case ELogState::Burning: BurnTimer = Definition ? Definition->BurnTime : 120.0f; MeshComponent->SetMobility(EComponentMobility::Movable); break;
    case ELogState::Destroyed: Destroy(); break;
    default: break;
    }
}


EMaterialType ALogActor::GetMaterialType_Implementation() const
{
    return EMaterialType::Wood;
}

EStructuralNodeState ALogActor::GetStructuralState_Implementation() const
{
    switch (GetLogState())
    {
    case ELogState::Carried: return EStructuralNodeState::Carried;
    case ELogState::Dragging: return EStructuralNodeState::Dragging;
    case ELogState::Installed: return EStructuralNodeState::Installed;
    case ELogState::Falling: return EStructuralNodeState::Falling;
    case ELogState::Burning: return EStructuralNodeState::Burning;
    case ELogState::Destroyed: return EStructuralNodeState::Destroyed;
    default: return EStructuralNodeState::Loose;
    }
}

bool ALogActor::SetStructuralState_Implementation(EStructuralNodeState NewState)
{
    switch (NewState)
    {
    case EStructuralNodeState::Loose: return SetLogState(ELogState::Loose);
    case EStructuralNodeState::Carried: return SetLogState(ELogState::Carried);
    case EStructuralNodeState::Dragging: return SetLogState(ELogState::Dragging);
    case EStructuralNodeState::Installed: return SetLogState(ELogState::Installed);
    case EStructuralNodeState::Falling: return SetLogState(ELogState::Falling);
    case EStructuralNodeState::Burning: return SetLogState(ELogState::Burning);
    case EStructuralNodeState::Destroyed: return SetLogState(ELogState::Destroyed);
    case EStructuralNodeState::Fractured: return SetLogState(ELogState::Falling);
    default: return false;
    }
}

float ALogActor::GetStructuralMass_Implementation() const
{
    if (!Definition) return 40.0f;
    const float Ratio = Definition->Length > 0.0f ? CurrentLength / Definition->Length : 1.0f;
    return Definition->Mass * FMath::Clamp(Ratio, 0.01f, 1.0f);
}

float ALogActor::GetStructuralStrength_Implementation() const
{
    const float BaseStrength = Definition ? Definition->StructuralStrength : 100.0f;
    return BaseStrength * FMath::Clamp(Health / 100.0f, 0.0f, 1.0f);
}

bool ALogActor::IsGrounded_Implementation() const
{
    return IsLogGrounded();
}
