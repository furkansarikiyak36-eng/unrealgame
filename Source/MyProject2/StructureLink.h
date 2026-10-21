// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Wood/WoodStructureLogic.h"
#include "Wood/WoodStructureAnalysis.h"

class ACarvableLog;
class UWorld;

/**
 * Bridge between the carvable logs in the world and the structure graph of Wood/WoodStructureLogic.
 * Every anchored log (post, corner log, beam) is a node: its real ends, radius, length in modules and notches.
 * Support follows WoodStructure::ComputeSupport: a grounded piece holds itself, a horizontal carrier (beam) needs
 * two supported pieces under it, anything else one; support spreads from the ground, so a frame resting only on
 * itself does not stand. Pieces that lose their path to the ground are handed to physics and fall.
 */
namespace StructureLink
{
	/** The graph node for Log, as it is or as it would be at Transform. */
	WoodStructure::FNode MakeNode(const ACarvableLog* Log, int32 Id, const FTransform* Transform = nullptr);

	/** How far apart the supports of a beam may be, and how far it may reach past the last one (modules). */
	constexpr float MaxSupportGapModules = 4.5f;
	constexpr float MaxOverhangModules = 2.f;

	/**
	 * Support with the beam rules on top of the graph: a beam that is not lying on the ground holds only if its supports
	 * (posts, braces, the logs under it) are not further apart than MaxSupportGapModules and it does not reach past the
	 * outermost one by more than MaxOverhangModules. A beam that breaks the rule fails, and what stood on it with it.
	 * Failed nodes get Damage 1 (this is how the Wood logic marks a piece that cannot hold). Returns the supported ids.
	 * OutSnaps gets, for each beam that failed, how far from its A end it snaps: the middle of the gap or overhang that was too long.
	 */
	TSet<int32> Evaluate(TArray<WoodStructure::FNode>& Nodes, const TArray<WoodStructure::FEdge>& Edges,
		TMap<int32, float>* OutSnaps = nullptr, FString* OutNotes = nullptr);

	/** When set (the automatic mechanics test sets it), every piece that falls or snaps is logged with the structure graph at that moment. */
	extern bool bLogFalls;

	/** A walls, platform, door frame... the standing pieces make up (see WoodStructure::AnalyzeStructure). */
	struct FFeatureInfo
	{
		EWoodFeatureKind Kind = EWoodFeatureKind::Platform;
		TArray<TWeakObjectPtr<ACarvableLog>> Pieces;
		float SpanCm = 0.f;
		FVector Center = FVector::ZeroVector;
	};

	/** What the standing pieces make up. Worked out again only after the structure has changed (Settle, damage, new pieces). */
	const TArray<FFeatureInfo>& Features(UWorld* World);
	/** Turkish name of a kind of structure. */
	FString NameOf(EWoodFeatureKind Kind);
	/** What Log is part of, e.g. "Platform, Korkuluk" (empty if it stands alone). */
	FString DescribeFeaturesOf(UWorld* World, const ACarvableLog* Log);
	/** Something in the structure changed: the next Features() works it out afresh. */
	void MarkChanged();
	/** Monotonic version incremented whenever the structural graph is invalidated. */
	int32 Version();

	/** Every standing piece with its role, whether it rests on the ground, and what holds it: for reading what the graph sees. */
	FString Describe(UWorld* World);

	/** Lets every anchored piece that has no path to the ground fall. Returns how many fell. */
	int32 Settle(UWorld* World);

	/** Would Piece, anchored at Transform among the pieces already standing, be held up? */
	bool WouldBeSupported(UWorld* World, const ACarvableLog* Piece, const FTransform& Transform);
}
