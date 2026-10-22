// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Shoring/ShoringTypes.h"

/** Pure planner: produces bounded geometry and explicit reciprocal links, without spawning actors. */
class MYPROJECT2_API FShoringLayoutPlanner
{
public:
    static bool BuildPlan(
        const FShoringLayoutRequest& Request,
        FShoringLayoutPlan& OutPlan,
        FString& OutError);
};