// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ForestTypes.h"

class ULineBatchComponent;
class UStaticMesh;

/**
 * Line drawings for build plans and previews, in the style of Sons of the Forest blueprints:
 * round pieces get rings at both ends plus lines along their length, square pieces their twelve edges.
 * Everything is drawn in world space into a ULineBatchComponent that is flushed and redrawn by its owner.
 */
namespace ForestOutline
{
	/** Plan colour for a spot that can be built on, and for one that cannot. */
	MYPROJECT2_API FLinearColor ValidColor();
	MYPROJECT2_API FLinearColor InvalidColor();
	/** Colour of carving previews. */
	MYPROJECT2_API FLinearColor CarveColor();
	/** Red of the dashed cut guide on trees and logs. */
	MYPROJECT2_API FLinearColor CutColor();

	/** A dashed line through local-space Points (closed back to the first point if bClosed), like a cut marked on bark. */
	MYPROJECT2_API void DrawDashed(ULineBatchComponent* Lines, const FTransform& Transform, const TArray<FVector>& Points,
		bool bClosed, const FLinearColor& Color);

	/** Outline of a piece given its local box (a carvable log): round items as a cylinder along X, others as a box. */
	MYPROJECT2_API void DrawPieceBox(ULineBatchComponent* Lines, EForestItem Item, const FBox& Box, const FTransform& Transform, const FLinearColor& Color);

	/** Outline of a build piece (log, plank, stone...) placed with Transform (scale included). */
	MYPROJECT2_API void DrawPiece(ULineBatchComponent* Lines, EForestItem Item, const UStaticMesh* Mesh, const FTransform& Transform, const FLinearColor& Color);

	/** A cylinder whose axis is local X (Axis = 0) or local Z (Axis = 2), centred on Centre in local space. */
	MYPROJECT2_API void DrawCylinder(ULineBatchComponent* Lines, const FTransform& Transform, int32 Axis, const FVector& Centre,
		float Radius, float HalfLength, const FLinearColor& Color, int32 Rings = 2);

	/** The twelve edges of a local-space box. */
	MYPROJECT2_API void DrawBox(ULineBatchComponent* Lines, const FTransform& Transform, const FBox& Box, const FLinearColor& Color);
}
