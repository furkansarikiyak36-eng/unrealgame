// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Shoring/ShoringTypes.h"

/**
 * Pure deterministic gameplay support model. Capacity/load values are abstract
 * tuning units, not real geotechnical or structural engineering measurements.
 */
class MYPROJECT2_API FShoringSupportResolver
{
public:
    static bool Evaluate(
        const TArray<FShoringSupportNode>& Nodes,
        TArray<FShoringSupportAssessment>& OutAssessments,
        FString& OutError);

    static float GetNominalCapacity(EShoringPieceKind Kind);
};