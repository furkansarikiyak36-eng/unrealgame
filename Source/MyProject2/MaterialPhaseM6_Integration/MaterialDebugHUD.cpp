// MaterialDebugHUD.cpp
#include "MaterialDebugHUD.h"
#include "MaterialPhaseM4_Composite/CompositeStructureComponent.h"
#include "MaterialPhaseM4_Composite/StructureLayerComponent.h"
#include "Engine/Canvas.h"

void AMaterialDebugHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!bShowDebug || !Canvas || !DebugTarget.IsValid()) return;

    AActor* Target = DebugTarget.Get();
    UCompositeStructureComponent* Structure = Target->FindComponentByClass<UCompositeStructureComponent>();
    if (!Structure) return;

    FString Text = FString::Printf(TEXT("=== Material Debug: %s ===\n"), *Target->GetName());

    for (UStructureLayerComponent* L : Structure->Layers)
    {
        if (!L) continue;
        const FStructuralLayer& Layer = L->GetLayer();
        Text += FString::Printf(TEXT("Layer: %d | Material: %s | Condition: %d | LoadBearing: %.2f\n"),
            (int32)Layer.Role, *Layer.MaterialID.ToString(), (int32)Layer.Condition, Layer.LoadBearing);
    }

    Text += FString::Printf(TEXT("Stable: %s | Overall: %d\n"),
        Structure->IsStructureStable() ? TEXT("YES") : TEXT("NO"),
        (int32)Structure->GetWorstCondition());

    DrawText(Text, FLinearColor::Yellow, 50.0f, 50.0f, nullptr, 1.2f);
}

void AMaterialDebugHUD::SetDebugTarget(AActor* Target)
{
    DebugTarget = Target;
}
