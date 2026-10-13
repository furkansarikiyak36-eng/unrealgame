// Copyright Epic Games, Inc. All Rights Reserved.

#include "ForestOutline.h"
#include "Components/LineBatchComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
	/** World units; thin enough to read as drawn lines, thick enough to see at building distance. */
	constexpr float LineThickness = 1.5f;
	constexpr int32 RingSegments = 20;
	/** Lines running along a log, spaced evenly round it. */
	constexpr int32 LengthLines = 8;

	void Line(ULineBatchComponent* Lines, const FTransform& Transform, const FVector& A, const FVector& B, const FLinearColor& Color)
	{
		Lines->DrawLine(Transform.TransformPosition(A), Transform.TransformPosition(B), Color, SDPG_World, LineThickness);
	}

	/** Point on the ring round the axis at Angle, Along units from the centre. */
	FVector RingPoint(int32 Axis, const FVector& Centre, float Radius, float Along, float Angle)
	{
		const float S = FMath::Sin(Angle) * Radius;
		const float C = FMath::Cos(Angle) * Radius;
		return Axis == 0 ? Centre + FVector(Along, S, C) : Centre + FVector(C, S, Along);
	}

	bool IsRound(EForestItem Item)
	{
		switch (Item)
		{
		case EForestItem::Log:
		case EForestItem::TrunkLog:
		case EForestItem::BigLog:
		case EForestItem::ShortLog:
		case EForestItem::HollowLog:
		case EForestItem::Firewood:
			return true;
		default:
			return false;
		}
	}
}

FLinearColor ForestOutline::ValidColor()
{
	return FLinearColor(0.8f, 0.92f, 1.f);
}

FLinearColor ForestOutline::InvalidColor()
{
	return FLinearColor(1.f, 0.2f, 0.15f);
}

FLinearColor ForestOutline::CarveColor()
{
	return FLinearColor(0.35f, 0.7f, 1.f);
}

FLinearColor ForestOutline::CutColor()
{
	return FLinearColor(1.f, 0.05f, 0.04f);
}

void ForestOutline::DrawDashed(ULineBatchComponent* Lines, const FTransform& Transform, const TArray<FVector>& Points,
	bool bClosed, const FLinearColor& Color)
{
	// Dashes keep their world length whatever the polyline's spacing: 7 cm drawn, 5 cm gap.
	constexpr float Dash = 7.f;
	constexpr float Period = 12.f;
	constexpr float CutThickness = 2.f;
	if (!Lines || Points.Num() < 2)
	{
		return;
	}
	float Along = 0.f;
	const int32 Segments = bClosed ? Points.Num() : Points.Num() - 1;
	for (int32 Index = 0; Index < Segments; ++Index)
	{
		const FVector A = Transform.TransformPosition(Points[Index]);
		const FVector B = Transform.TransformPosition(Points[(Index + 1) % Points.Num()]);
		const float Length = FVector::Dist(A, B);
		float T = 0.f;
		while (T < Length)
		{
			const float Phase = FMath::Fmod(Along + T, Period);
			const bool bOn = Phase < Dash;
			const float Step = FMath::Min(Length - T, bOn ? Dash - Phase : Period - Phase);
			if (bOn && Length > 0.f)
			{
				Lines->DrawLine(FMath::Lerp(A, B, T / Length), FMath::Lerp(A, B, (T + Step) / Length), Color, SDPG_World, CutThickness);
			}
			T += FMath::Max(Step, 0.01f);
		}
		Along += Length;
	}
}

void ForestOutline::DrawCylinder(ULineBatchComponent* Lines, const FTransform& Transform, int32 Axis, const FVector& Centre,
	float Radius, float HalfLength, const FLinearColor& Color, int32 Rings)
{
	Rings = FMath::Max(2, Rings);
	for (int32 Ring = 0; Ring < Rings; ++Ring)
	{
		const float Along = FMath::Lerp(-HalfLength, HalfLength, static_cast<float>(Ring) / (Rings - 1));
		for (int32 Seg = 0; Seg < RingSegments; ++Seg)
		{
			const float A0 = 2.f * PI * Seg / RingSegments;
			const float A1 = 2.f * PI * (Seg + 1) / RingSegments;
			Line(Lines, Transform, RingPoint(Axis, Centre, Radius, Along, A0), RingPoint(Axis, Centre, Radius, Along, A1), Color);
		}
	}
	for (int32 Index = 0; Index < LengthLines; ++Index)
	{
		const float Angle = 2.f * PI * Index / LengthLines;
		Line(Lines, Transform, RingPoint(Axis, Centre, Radius, -HalfLength, Angle), RingPoint(Axis, Centre, Radius, HalfLength, Angle), Color);
	}
}

void ForestOutline::DrawBox(ULineBatchComponent* Lines, const FTransform& Transform, const FBox& Box, const FLinearColor& Color)
{
	const FVector& Lo = Box.Min;
	const FVector& Hi = Box.Max;
	const FVector Corners[8] = {
		FVector(Lo.X, Lo.Y, Lo.Z), FVector(Hi.X, Lo.Y, Lo.Z), FVector(Hi.X, Hi.Y, Lo.Z), FVector(Lo.X, Hi.Y, Lo.Z),
		FVector(Lo.X, Lo.Y, Hi.Z), FVector(Hi.X, Lo.Y, Hi.Z), FVector(Hi.X, Hi.Y, Hi.Z), FVector(Lo.X, Hi.Y, Hi.Z) };
	for (int32 Index = 0; Index < 4; ++Index)
	{
		Line(Lines, Transform, Corners[Index], Corners[(Index + 1) % 4], Color);
		Line(Lines, Transform, Corners[Index + 4], Corners[(Index + 1) % 4 + 4], Color);
		Line(Lines, Transform, Corners[Index], Corners[Index + 4], Color);
	}
}

void ForestOutline::DrawPiece(ULineBatchComponent* Lines, EForestItem Item, const UStaticMesh* Mesh, const FTransform& Transform, const FLinearColor& Color)
{
	if (Mesh)
	{
		DrawPieceBox(Lines, Item, Mesh->GetBoundingBox(), Transform, Color);
	}
}

void ForestOutline::DrawPieceBox(ULineBatchComponent* Lines, EForestItem Item, const FBox& Box, const FTransform& Transform, const FLinearColor& Color)
{
	if (!Lines)
	{
		return;
	}
	const FVector Centre = Box.GetCenter();
	const FVector Extent = Box.GetExtent();

	if (IsRound(Item))
	{
		// Long logs get a middle ring too, so a wall plan reads as stacked logs, not tubes.
		const float Length = Extent.X * 2.f * Transform.GetScale3D().X;
		DrawCylinder(Lines, Transform, 0, Centre, (Extent.Y + Extent.Z) * 0.5f, Extent.X, Color, Length > 300.f ? 3 : 2);
		return;
	}
	if (Item == EForestItem::HalfLog)
	{
		// Half round below the flat face: the flat top is the box's upper face.
		const float Radius = Extent.Y;
		const float Top = Box.Max.Z;
		const FVector Axis(Centre.X, Centre.Y, Top);
		for (const float End : { Box.Min.X, Box.Max.X })
		{
			for (int32 Seg = 0; Seg < RingSegments / 2; ++Seg)
			{
				const float A0 = PI * 0.5f + PI * Seg / (RingSegments / 2);
				const float A1 = PI * 0.5f + PI * (Seg + 1) / (RingSegments / 2);
				Line(Lines, Transform, FVector(End, Axis.Y + FMath::Sin(A0) * Radius, Top + FMath::Cos(A0) * Radius),
					FVector(End, Axis.Y + FMath::Sin(A1) * Radius, Top + FMath::Cos(A1) * Radius), Color);
			}
			Line(Lines, Transform, FVector(End, Box.Min.Y, Top), FVector(End, Box.Max.Y, Top), Color);
		}
		for (int32 Index = 0; Index <= 4; ++Index)
		{
			const float A = PI * 0.5f + PI * Index / 4.f;
			const FVector Offset(0.f, FMath::Sin(A) * Radius, FMath::Cos(A) * Radius);
			Line(Lines, Transform, FVector(Box.Min.X, Axis.Y, Top) + Offset, FVector(Box.Max.X, Axis.Y, Top) + Offset, Color);
		}
		return;
	}
	DrawBox(Lines, Transform, Box, Color);
}
