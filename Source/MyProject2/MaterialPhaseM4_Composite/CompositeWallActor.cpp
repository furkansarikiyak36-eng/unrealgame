// CompositeWallActor.cpp
#include "CompositeWallActor.h"
#include "CompositeStructureComponent.h"
#include "StructureLayerComponent.h"
#include "LayerDamageResolver.h"
#include "MaterialPhaseM3_Instance/MaterialInstanceStateComponent.h"
#include "MaterialPhaseM2_Interaction/MaterialInteractionSubsystem.h"
#include "MaterialCore/MaterialRegistry.h"
#include "MaterialCore/MaterialDefinition.h"
#include "MaterialPhaseM1_Definition/MaterialPropertyMapper.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

namespace
{
    UMaterialDefinition* ResolveMaterial(ACompositeWallActor* Owner, FName MaterialID)
    {
        if (!Owner) return nullptr;
        if (UWorld* World = Owner->GetWorld())
        {
            if (UGameInstance* GI = World->GetGameInstance())
            {
                if (UMaterialRegistry* Registry = GI->GetSubsystem<UMaterialRegistry>())
                {
                    if (UMaterialDefinition* Existing = Registry->FindMaterial(MaterialID)) return Existing;
                }
            }
        }

        // Data Asset registry bağlı değilse etkileşim testi için güvenli temel profil kullan.
        UMaterialDefinition* Fallback = NewObject<UMaterialDefinition>(Owner);
        if (!Fallback) return nullptr;
        Fallback->MaterialID = MaterialID;
        if (MaterialID.ToString().StartsWith(TEXT("Stone")))
        {
            Fallback->MaterialClass = EMaterialClass::Stone;
            Fallback->Properties = UMaterialPropertyMapper::MakeStoneProfile();
        }
        else if (MaterialID.ToString().StartsWith(TEXT("Plaster")))
        {
            Fallback->MaterialClass = EMaterialClass::Plaster;
            Fallback->Properties = UMaterialPropertyMapper::MakePlasterProfile();
        }
        else if (MaterialID.ToString().StartsWith(TEXT("Metal")))
        {
            Fallback->MaterialClass = EMaterialClass::Metal;
            Fallback->Properties = UMaterialPropertyMapper::MakeMetalProfile();
        }
        else
        {
            Fallback->MaterialClass = EMaterialClass::Wood;
            Fallback->Properties = UMaterialPropertyMapper::MakeWoodProfile();
        }
        return Fallback;
    }

    void ApplyInteractionToLayer(ACompositeWallActor* Owner, UStructureLayerComponent* Layer,
        EInteractionType Type, const FMaterialInteractionContext& Context, EMaterialReactionType FallbackReaction)
    {
        if (!Owner || !Layer) return;
        UWorld* World = Owner->GetWorld();
        UMaterialInteractionSubsystem* Interaction = World ? World->GetSubsystem<UMaterialInteractionSubsystem>() : nullptr;
        if (!Interaction) return;
        UMaterialDefinition* Material = ResolveMaterial(Owner, Layer->GetLayer().MaterialID);
        if (!Material) return;
        const float Damage = Interaction->ApplyInteraction(Owner, Material, Type, Context);
        if (Damage <= 0.0f) return;
        EMaterialReactionType Reaction = Interaction->PredictReaction(Material, Type, Context);
        if (Reaction == EMaterialReactionType::None) Reaction = FallbackReaction;
        Layer->ApplyDamageToLayer(Damage, Reaction);
    }
}

ACompositeWallActor::ACompositeWallActor()
{
    PrimaryActorTick.bCanEverTick = false;
    WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wall"));
    RootComponent = WallMesh;
    StructureComponent = CreateDefaultSubobject<UCompositeStructureComponent>(TEXT("Structure"));
    InstanceState = CreateDefaultSubobject<UMaterialInstanceStateComponent>(TEXT("Instance"));
}

void ACompositeWallActor::BeginPlay()
{
    Super::BeginPlay();
    InitializeDefaultWall();
}

void ACompositeWallActor::InitializeDefaultWall()
{
    if (!StructureComponent || StructureComponent->Layers.Num() > 0) return;
    UStructureLayerComponent* Foundation = StructureComponent->AddLayer(EStructureLayerRole::Foundation, TEXT("Stone"), 0.4f);
    UStructureLayerComponent* Skeleton = StructureComponent->AddLayer(EStructureLayerRole::Skeleton, TEXT("Wood"), 0.15f);
    UStructureLayerComponent* Cladding = StructureComponent->AddLayer(EStructureLayerRole::Cladding, TEXT("Plaster"), 0.05f);
    UStructureLayerComponent* Roof = StructureComponent->AddLayer(EStructureLayerRole::Roof, TEXT("Wood"), 0.1f);
    if (Foundation) Foundation->OnLayerConditionChanged.AddUniqueDynamic(this, &ACompositeWallActor::HandleLayerConditionChanged);
    if (Skeleton) Skeleton->OnLayerConditionChanged.AddUniqueDynamic(this, &ACompositeWallActor::HandleLayerConditionChanged);
    if (Cladding) Cladding->OnLayerConditionChanged.AddUniqueDynamic(this, &ACompositeWallActor::HandleLayerConditionChanged);
    if (Roof) Roof->OnLayerConditionChanged.AddUniqueDynamic(this, &ACompositeWallActor::HandleLayerConditionChanged);
}

void ACompositeWallActor::ApplyDamageToLayer(EStructureLayerRole LayerRole, float Amount, EMaterialReactionType Reaction)
{
    if (StructureComponent) StructureComponent->ApplyDamageToLayerRole(LayerRole, Amount, Reaction);
}

void ACompositeWallActor::ApplyFireDamage(float Intensity, float DurationHours)
{
    FMaterialInteractionContext Context;
    Context.Magnitude = FMath::Max(0.0f, Intensity);
    Context.DurationHours = FMath::Max(0.0f, DurationHours);
    Context.AmbientTemperature = 400.0f;
    if (!StructureComponent) return;
    ApplyInteractionToLayer(this, StructureComponent->FindLayer(EStructureLayerRole::Skeleton), EInteractionType::Fire, Context, EMaterialReactionType::Burn);
    ApplyInteractionToLayer(this, StructureComponent->FindLayer(EStructureLayerRole::Roof), EInteractionType::Fire, Context, EMaterialReactionType::Burn);
}

void ACompositeWallActor::ApplyWaterDamage(float Intensity, float DurationHours)
{
    FMaterialInteractionContext Context;
    Context.Moisture = FMath::Clamp(Intensity, 0.0f, 1.0f);
    Context.DurationHours = FMath::Max(0.0f, DurationHours);
    if (!StructureComponent) return;
    ApplyInteractionToLayer(this, StructureComponent->FindLayer(EStructureLayerRole::Cladding), EInteractionType::Water, Context, EMaterialReactionType::Dissolve);
}

void ACompositeWallActor::ApplyImpactDamage(float Magnitude)
{
    FMaterialInteractionContext Context;
    Context.Magnitude = FMath::Max(0.0f, Magnitude);
    if (!StructureComponent) return;
    ApplyInteractionToLayer(this, StructureComponent->FindLayer(EStructureLayerRole::Cladding), EInteractionType::Impact, Context, EMaterialReactionType::Shatter);
}

bool ACompositeWallActor::IsStructurallyStable() const
{
    return StructureComponent ? StructureComponent->IsStructureStable() : false;
}

EStructureLayerCondition ACompositeWallActor::GetOverallCondition() const
{
    return StructureComponent ? StructureComponent->GetWorstCondition() : EStructureLayerCondition::Pristine;
}

void ACompositeWallActor::HandleLayerConditionChanged(EStructureLayerRole LayerRole, EStructureLayerCondition NewCondition)
{
    if (!StructureComponent || NewCondition != EStructureLayerCondition::Collapsed) return;
    if (LayerRole == EStructureLayerRole::Cladding && ULayerDamageResolver::ShouldDamagePropagateToSkeleton(NewCondition))
        StructureComponent->ApplyDamageToLayerRole(EStructureLayerRole::Skeleton, 0.25f, EMaterialReactionType::Shatter);
    else if (LayerRole == EStructureLayerRole::Skeleton)
        StructureComponent->ApplyDamageToLayerRole(EStructureLayerRole::Roof, 1.0f, EMaterialReactionType::Shatter);
}
