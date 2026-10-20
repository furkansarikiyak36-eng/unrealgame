#include "StickActor.h"
#include "StickCore/StickDefinition.h"
#include "StickCore/StickStateMachine.h"
#include "StickCore/StickConnectionGraph.h"
#include "StickPhaseS9_Extensions/StickDurabilityComponent.h"
#include "StickPhaseS9_Extensions/StickFlexComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
AStickActor::AStickActor()
{
    PrimaryActorTick.bCanEverTick = false;
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh")); RootComponent = MeshComponent;
    MeshComponent->SetCollisionProfileName(TEXT("PhysicsActor")); MeshComponent->SetSimulatePhysics(true); MeshComponent->SetMobility(EComponentMobility::Movable);
    StateMachine = CreateDefaultSubobject<UStickStateMachine>(TEXT("StateMachine"));
    ConnectionGraph = CreateDefaultSubobject<UStickConnectionGraph>(TEXT("ConnectionGraph"));
    DurabilityComponent = CreateDefaultSubobject<UStickDurabilityComponent>(TEXT("Durability"));
    FlexComponent = CreateDefaultSubobject<UStickFlexComponent>(TEXT("Flex"));
}
void AStickActor::InitializeFromDefinition(UStickDefinition* Def)
{
    if (!Def) return; Definition = Def; MaxHealth = Def->Durability; Health = MaxHealth;
    if (UStaticMesh* M = Def->Mesh.LoadSynchronous()) MeshComponent->SetStaticMesh(M);
    if (UMaterialInterface* Mat = Def->Material.LoadSynchronous()) MeshComponent->SetMaterial(0, Mat);
    MeshComponent->SetMassOverrideInKg(NAME_None, Def->Mass, true);
    if (DurabilityComponent) { DurabilityComponent->MaxHealth = MaxHealth; DurabilityComponent->Health = Health; }
    if (StateMachine) { StateMachine->OnStateChanged.AddDynamic(this, &AStickActor::HandleStateChanged); }
    SetStructuralState_Implementation(EStructuralNodeState::Loose);
}
EStructuralMaterialType AStickActor::GetMaterialType_Implementation() const { return EStructuralMaterialType::Wood; }
EStructuralNodeState AStickActor::GetStructuralState_Implementation() const { return StateMachine ? StateMachine->GetState() : EStructuralNodeState::Loose; }
bool AStickActor::SetStructuralState_Implementation(EStructuralNodeState S) { return StateMachine && StateMachine->TransitionTo(S); }
float AStickActor::GetStructuralMass_Implementation() const { return Definition ? Definition->Mass : 0.4f; }
float AStickActor::GetStructuralStrength_Implementation() const { return Definition ? Definition->BendingStrength : 15.f; }
bool AStickActor::IsGrounded_Implementation() const
{
    if (!GetWorld() || !MeshComponent) return false; FHitResult Hit; FCollisionQueryParams P; P.AddIgnoredActor(this);
    return GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), GetActorLocation()-FVector(0,0,30), ECC_WorldStatic, P);
}
void AStickActor::ApplyShape(EStickShape Shape)
{
    if (!Definition) return; UStickDefinition* D = DuplicateObject<UStickDefinition>(Definition, this); D->Shape = Shape; Definition = D;
}
void AStickActor::ApplyDamage(float Amount)
{
    if (Amount <= 0.f) return; Health = FMath::Max(0.f, Health-Amount);
    if (DurabilityComponent) DurabilityComponent->ApplyStress(Amount);
    if (Health <= 0.f) SetStructuralState_Implementation(EStructuralNodeState::Fractured);
}
void AStickActor::HandleStateChanged(AActor*, EStructuralNodeState State)
{
    if (!MeshComponent) return;
    switch(State) {
    case EStructuralNodeState::Loose: MeshComponent->SetMobility(EComponentMobility::Movable); MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); MeshComponent->SetSimulatePhysics(true); break;
    case EStructuralNodeState::Carried: case EStructuralNodeState::Dragging: MeshComponent->SetSimulatePhysics(false); MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly); break;
    case EStructuralNodeState::Installed: MeshComponent->SetSimulatePhysics(false); MeshComponent->SetMobility(EComponentMobility::Movable); MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); break;
    case EStructuralNodeState::Falling: MeshComponent->SetMobility(EComponentMobility::Movable); MeshComponent->SetSimulatePhysics(true); break;
    case EStructuralNodeState::Fractured: case EStructuralNodeState::Destroyed: Destroy(); break;
    default: break;
    }
}
