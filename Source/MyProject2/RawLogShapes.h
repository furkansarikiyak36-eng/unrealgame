// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class ACarvableLog;
class UStaticMesh;
struct FHitResult;

/**
 * Working the raw UnrealExport logs with the axe, as finished shapes rather than free carving. Every log is a length
 * (600 / 300 / 150 / 75 cm, the 75 cm module) and a profile, and each has its own mesh /Game/UnrealExport/SM_RawLog_<L>[_<Profile>]:
 *
 *   Raw (round) -> Half -> Quarter                         split lengthwise (hit an end face)
 *   Raw -> Hewn1 (one side flat) -> Hewn2 (two) -> Kalas   hew (hit a side away from the middle)
 *   Kalas -> 4 Planks                                      split lengthwise
 *   Raw -> Gutter -> Trough                                channel (R switches to it, then hit the top)
 *   any profile, L -> 2 x L/2                              cut across (hit the middle); 75 cm cannot be cut shorter
 *
 * Laying one log across another notches both by themselves (flat notch top and bottom, like SM_RawLog_75_FlatNotch_TopBottom).
 */
namespace RawLogShapes
{
	enum class EProfile : uint8 { Raw, Half, Quarter, Hewn1, Hewn2, Kalas, Plank, Gutter, Trough, FlatNotch, Unknown };
	enum class EAction : uint8 { CrossCut, Split, Hew, Groove };

	struct FShape
	{
		int32 Length = 0;
		EProfile Profile = EProfile::Unknown;
		bool IsValid() const { return Length > 0 && Profile != EProfile::Unknown; }
	};

	constexpr int32 ShortestCm = 75;

	/** The shape of a raw-log mesh from its name (SM_RawLog_300_Half...); invalid for anything else. */
	MYPROJECT2_API FShape FromMesh(const UStaticMesh* Mesh);
	/** The shape of a building piece made from a raw-log mesh. */
	MYPROJECT2_API FShape FromLog(const ACarvableLog* Log);
	MYPROJECT2_API UStaticMesh* LoadMesh(int32 Length, EProfile Profile);

	/** What a hit at Hit does: an end face splits, the middle cuts across, the sides are hewn; bChannelMode: channel. */
	MYPROJECT2_API EAction ClassifyHit(const ACarvableLog* Log, const FHitResult& Hit, bool bChannelMode);
	/** Whether Action can be done to Shape; when not, OutWhy says why (shown to the player). */
	MYPROJECT2_API bool CanApply(const FShape& Shape, EAction Action, FString& OutWhy);
	/** Axe hits it takes. */
	MYPROJECT2_API int32 HitsFor(EAction Action);
	/** Replaces Log by the pieces Action makes, where it lies; OutMessage describes the result. False if it cannot. */
	MYPROJECT2_API bool Apply(ACarvableLog* Log, EAction Action, FString& OutMessage);

	MYPROJECT2_API FString ProfileName(EProfile Profile);
	/** "300 cm yarım kütük". */
	MYPROJECT2_API FString Describe(const FShape& Shape);
	/** "Enine kesiliyor". */
	MYPROJECT2_API FString ActionName(EAction Action);
}
