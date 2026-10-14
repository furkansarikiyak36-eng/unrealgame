// Copyright Epic Games, Inc. All Rights Reserved.

#include "BurnSubsystem.h"
#include "CarvableLog.h"
#include "Campfire.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Wood/WoodStructureGeometry.h"

UBurnSubsystem* UBurnSubsystem::Get(UWorld* World)
{
	return World ? World->GetSubsystem<UBurnSubsystem>() : nullptr;
}

void UBurnSubsystem::Light(ACarvableLog* Piece)
{
	Burning.AddUnique(Piece);
	Warming.Remove(Piece);
}

void UBurnSubsystem::Warm(ACarvableLog* Piece)
{
	Warming.AddUnique(Piece);
}

int32 UBurnSubsystem::GetBurningCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<ACarvableLog>& Piece : Burning)
	{
		Count += Piece.IsValid() && Piece->IsBurning() ? 1 : 0;
	}
	return Count;
}

bool UBurnSubsystem::IsTickable() const
{
	return Burning.Num() > 0 || Warming.Num() > 0;
}

void UBurnSubsystem::Tick(float DeltaSeconds)
{
	Advance(DeltaSeconds);
}

void UBurnSubsystem::Advance(float Seconds)
{
	constexpr float StepSeconds = 0.25f;
	while (Seconds > 0.f)
	{
		const float Dt = FMath::Min(Seconds, StepSeconds);
		Step(Dt);
		Seconds -= Dt;
	}
}

void UBurnSubsystem::Step(float Seconds)
{
	Burning.RemoveAll([](const TWeakObjectPtr<ACarvableLog>& P) { return !P.IsValid() || !P->IsBurning(); });
	Warming.RemoveAll([Seconds](const TWeakObjectPtr<ACarvableLog>& P) { return !P.IsValid() || !P->CoolStep(Seconds); });

	UWorld* World = GetWorld();
	TArray<ACarvableLog*> Sources;
	for (const TWeakObjectPtr<ACarvableLog>& Piece : Burning)
	{
		Sources.Add(Piece.Get());
	}
	for (ACarvableLog* Source : Sources)
	{
		FVector Centre, Axis;
		float Radius, Half;
		if (!ACarvableLog::GetLogAxis(Source, Centre, Axis, Radius, Half))
		{
			continue;
		}
		// Samples along the burning piece: the flames are all along it.
		TArray<FVector> Flames;
		for (int32 Index = 0; Index < 5; ++Index)
		{
			Flames.Add(Centre + Axis * Half * (Index / 2.f - 1.f));
		}
		const auto GapTo = [&Flames, Radius](const FVector& A, const FVector& B, float OtherRadius)
		{
			double Best = TNumericLimits<double>::Max();
			for (const FVector& Flame : Flames)
			{
				double T = 0.0;
				Best = FMath::Min(Best, WoodStructure::Geo::DistToSegment(Flame, A, B, T));
			}
			return static_cast<float>(Best) - Radius - OtherRadius;
		};
		const auto Rate = [](float Gap) { return FMath::Clamp((SpreadReachCm - Gap) / (SpreadReachCm - TouchGapCm), 0.f, 1.f); };

		for (TActorIterator<ACarvableLog> It(World); It; ++It)
		{
			FVector OtherCentre, OtherAxis;
			float OtherRadius, OtherHalf;
			if (*It == Source || It->IsBurning() || It->IsCharred() || !ACarvableLog::GetLogAxis(*It, OtherCentre, OtherAxis, OtherRadius, OtherHalf))
			{
				continue;
			}
			const float Heat = Rate(GapTo(OtherCentre - OtherAxis * OtherHalf, OtherCentre + OtherAxis * OtherHalf, OtherRadius));
			if (Heat > 0.f)
			{
				It->ApplyHeat(Heat * Seconds);
			}
		}
		for (TActorIterator<ACampfire> It(World); It; ++It)
		{
			if (!It->IsBurning() && Rate(GapTo(It->GetActorLocation(), It->GetActorLocation() + FVector(0.f, 0.f, 1.f), 40.f)) > 0.f)
			{
				It->Ignite();
			}
		}
	}
	for (ACarvableLog* Source : Sources)
	{
		Source->StepBurn(Seconds);
	}
}
