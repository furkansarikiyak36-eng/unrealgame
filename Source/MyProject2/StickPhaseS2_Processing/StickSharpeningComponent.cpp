#include "StickSharpeningComponent.h"

#include "StickPhaseS1_Core/StickActor.h"
#include "StickCore/StickDefinition.h"
#include "Core/StructuralMaterialTypes.h"

UStickSharpeningComponent::UStickSharpeningComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UStickSharpeningComponent::SharpenStick(AStickActor* Stick, ESharpeningMode Mode)
{
	if (!IsValid(Stick) || !Stick->GetDefinition())
	{
		return false;
	}

	const EStructuralNodeState State = Stick->GetStructuralState_Implementation();
	if (State != EStructuralNodeState::Loose && State != EStructuralNodeState::Carried)
	{
		return false;
	}

	const EStickShape TargetShape = Mode == ESharpeningMode::DoubleEnd
		? EStickShape::DoublePoint : EStickShape::Sharpened;
	if (Stick->GetDefinition()->Shape == TargetShape)
	{
		HitProgress.Remove(TWeakObjectPtr<AStickActor>(Stick));
		return false;
	}

	const TWeakObjectPtr<AStickActor> Key(Stick);
	FProgress& Progress = HitProgress.FindOrAdd(Key);
	if (Progress.Hits == 0 || Progress.Mode != Mode)
	{
		Progress.Mode = Mode;
		Progress.Hits = 0;
	}

	++Progress.Hits;
	const int32 RequiredHits = FMath::Max(1, HitsRequired);
	if (Progress.Hits >= RequiredHits)
	{
		Stick->ApplyShape(TargetShape);
		HitProgress.Remove(Key);
	}
	return true;
}

int32 UStickSharpeningComponent::GetRegisteredHits(const AStickActor* Stick) const
{
	if (!IsValid(Stick)) return 0;
	const FProgress* Progress = HitProgress.Find(TWeakObjectPtr<AStickActor>(const_cast<AStickActor*>(Stick)));
	return Progress ? Progress->Hits : 0;
}

float UStickSharpeningComponent::GetSharpenProgress(const AStickActor* Stick, ESharpeningMode Mode) const
{
	if (!IsValid(Stick) || !Stick->GetDefinition()) return 0.f;
	const EStickShape TargetShape = Mode == ESharpeningMode::DoubleEnd
		? EStickShape::DoublePoint : EStickShape::Sharpened;
	if (Stick->GetDefinition()->Shape == TargetShape) return 1.f;
	const FProgress* Progress = HitProgress.Find(TWeakObjectPtr<AStickActor>(const_cast<AStickActor*>(Stick)));
	if (!Progress || Progress->Mode != Mode) return 0.f;
	return FMath::Clamp(static_cast<float>(Progress->Hits) / FMath::Max(1, HitsRequired), 0.f, 1.f);
}
