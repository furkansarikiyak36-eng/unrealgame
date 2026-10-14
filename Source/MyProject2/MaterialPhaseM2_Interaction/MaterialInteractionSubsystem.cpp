// MaterialInteractionSubsystem.cpp
#include "MaterialInteractionSubsystem.h"
#include "MaterialInteractionMatrix.h"
#include "FireMaterialRule.h"
#include "WaterMaterialRule.h"
#include "ImpactMaterialRule.h"
#include "TimeMaterialRule.h"
#include "Engine/World.h"

namespace
{
    bool MatchesInteractionType(EInteractionType Type, const UInteractionRule* Rule)
    {
        if (!Rule) return false;
        switch (Type)
        {
            case EInteractionType::Fire:   return Rule->IsA<UFireMaterialRule>();
            case EInteractionType::Water:  return Rule->IsA<UWaterMaterialRule>();
            case EInteractionType::Impact: return Rule->IsA<UImpactMaterialRule>();
            case EInteractionType::Time:   return Rule->IsA<UTimeMaterialRule>();
            default: return false;
        }
    }

    UInteractionRule* ResolveRule(EInteractionType Type, const UMaterialDefinition* Material,
        UMaterialInteractionMatrix* Matrix, UFireMaterialRule* Fire,
        UWaterMaterialRule* Water, UImpactMaterialRule* Impact, UTimeMaterialRule* Time)
    {
        if (!Material) return nullptr;
        if (Matrix)
        {
            for (UInteractionRule* Candidate : Matrix->GetRulesFor(Material->MaterialClass))
            {
                if (Candidate->bIsEnabled && MatchesInteractionType(Type, Candidate) && Candidate->CanApplyTo(Material))
                    return Candidate;
            }
            UInteractionRule* MatrixDefault = nullptr;
            switch (Type)
            {
                case EInteractionType::Fire:   MatrixDefault = Matrix->DefaultFireRule; break;
                case EInteractionType::Water:  MatrixDefault = Matrix->DefaultWaterRule; break;
                case EInteractionType::Impact: MatrixDefault = Matrix->DefaultImpactRule; break;
                case EInteractionType::Time:   MatrixDefault = Matrix->DefaultTimeRule; break;
                default: break;
            }
            if (MatrixDefault && MatrixDefault->bIsEnabled && MatchesInteractionType(Type, MatrixDefault) && MatrixDefault->CanApplyTo(Material))
                return MatrixDefault;
        }
        UInteractionRule* Fallback = nullptr;
        switch (Type)
        {
            case EInteractionType::Fire:   Fallback = Fire; break;
            case EInteractionType::Water:  Fallback = Water; break;
            case EInteractionType::Impact: Fallback = Impact; break;
            case EInteractionType::Time:   Fallback = Time; break;
            default: break;
        }
        return Fallback && Fallback->bIsEnabled && Fallback->CanApplyTo(Material) ? Fallback : nullptr;
    }
}

void UMaterialInteractionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    FireRule = NewObject<UFireMaterialRule>(this);
    WaterRule = NewObject<UWaterMaterialRule>(this);
    ImpactRule = NewObject<UImpactMaterialRule>(this);
    TimeRule = NewObject<UTimeMaterialRule>(this);
}

float UMaterialInteractionSubsystem::ApplyInteraction(AActor* Target, UMaterialDefinition* Material,
    EInteractionType Type, const FMaterialInteractionContext& Context)
{
    UInteractionRule* Rule = ResolveRule(Type, Material, Matrix, FireRule, WaterRule, ImpactRule, TimeRule);
    if (!Rule) return 0.0f;

    const float Damage = FMath::Max(0.0f, Rule->EvaluateDamage(Material, Context));
    if (Damage <= 0.0f) return 0.0f;

    const EMaterialReactionType Reaction = Rule->DetermineReaction(Material, Context);
    if (Target && OnMaterialDamage.IsBound())
        OnMaterialDamage.Broadcast(Target, Damage, Reaction);
    return Damage;
}

EMaterialReactionType UMaterialInteractionSubsystem::PredictReaction(const UMaterialDefinition* Material,
    EInteractionType Type, const FMaterialInteractionContext& Context) const
{
    UInteractionRule* Rule = ResolveRule(Type, Material, Matrix, FireRule, WaterRule, ImpactRule, TimeRule);
    if (!Rule || Rule->EvaluateDamage(Material, Context) <= 0.0f) return EMaterialReactionType::None;
    return Rule->DetermineReaction(Material, Context);
}
