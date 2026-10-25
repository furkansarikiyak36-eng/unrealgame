// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LogShowcase.generated.h"

class ACarvableLog;
class AForestCharacter;
class UStaticMesh;

/**
 * Builds, when play starts, a row of stations that show what logs can be made into, each built piece by piece with the
 * player's own placement (AForestCharacter::PlaceForShowcase): the same choices, snaps, notches and support checks as
 * by hand. Nothing is a prefab. Stations run along the actor's forward axis, 14 m apart, each with a sign; what could
 * not be built is written on the sign and in Saved/LogShowcase.txt.
 *
 * Stations: cut chain, wall with door and window openings, cabin with corner joints, platform (posts, beams, braces,
 * floor), ramp, lean-to roof rafters, second storey, palisade, +-45 degree stack, steps, bench and table, carrying and
 * storage.
 */
UCLASS()
class MYPROJECT2_API ALogShowcase : public AActor
{
	GENERATED_BODY()

public:
	ALogShowcase();

	/** Builds every station now (also called by itself shortly after play starts). Returns the report. */
	UFUNCTION(BlueprintCallable, Category = "Showcase")
	FString BuildAll();

	/**
	 * Builds one station (0 = cut chain ... 13 = log house; see StationCount) and returns its report, "no ForestCharacter" if
	 * there is no player. The same station can be built again, each at its own place along the row. Used by the acceptance
	 * runs (Scripts/probe_showcase.sh) to measure a single station.
	 */
	UFUNCTION(BlueprintCallable, Category = "Showcase")
	FString BuildStation(int32 Index);

	/** How many stations there are. */
	UFUNCTION(BlueprintPure, Category = "Showcase")
	static int32 StationCount() { return 16; }

	UPROPERTY(EditAnywhere, Category = "Showcase")
	float StationSpacingCm = 1400.f;

	/**
	 * Build the stations when play starts. Off: the stations are baked into the level instead (Scripts/bake_log_showcase.sh
	 * builds them once in play with the real mechanics and places static copies in the map).
	 */
	UPROPERTY(EditAnywhere, Category = "Showcase")
	bool bBuildOnPlay = false;

protected:
	virtual void BeginPlay() override;

private:
	// One station: its origin and a running report.
	FVector O = FVector::ZeroVector;
	FVector F = FVector::ForwardVector;
	FVector R = FVector::RightVector;
	float FYaw = 0.f;
	FString Report;
	FString StationNotes;
	int32 Placed = 0;
	int32 Failed = 0;
	TWeakObjectPtr<AForestCharacter> Player;
	bool bBuilt = false;

	UStaticMesh* Mesh(int32 Length, const TCHAR* Profile = nullptr) const;
	ACarvableLog* Spawn(UStaticMesh* LogMesh) const;
	/** Ground point and the actor hit under P. */
	FVector Ground(const FVector& P, AActor*& OutActor) const;
	/** Records one placement: what was wanted, what the game chose. */
	bool Note(const TCHAR* What, uint8 Chosen, uint8 Wanted);

	ACarvableLog* LayAt(UStaticMesh* LogMesh, const FVector& P, float PieceYaw, const TCHAR* What);
	ACarvableLog* StandAt(UStaticMesh* LogMesh, const FVector& P, float Yaw, const TCHAR* What);
	/** A piece onto Lower, aimed Along cm from its centre along its axis, the player facing ViewYaw. */
	ACarvableLog* OnLog(UStaticMesh* LogMesh, ACarvableLog* Lower, float Along, float ViewYaw, uint8 Hold, float Slope,
		const TCHAR* What, uint8 Wanted);
	/** A piece onto the top end of a standing post, the player facing ViewYaw. */
	ACarvableLog* OnPostTop(UStaticMesh* LogMesh, ACarvableLog* Post, float ViewYaw, const TCHAR* What, uint8 Wanted);
	void Sign(const FString& Title);
	void BeginStation(int32 Index, const FString& Title);
	void EndStation();

	void StationCutChain();
	void StationWall();
	void StationCabin();
	void StationPlatform();
	void StationRoof();
	void StationPalisade();
	void StationDiagonal();
	void StationSteps();
	void StationFurniture();
	void StationHauling();
	void StationModuleOpening();
	void StationFaceSockets();
	void StationUnderSockets();
	void StationLogHouse();
	void StationPostBeams();
	void StationLengthMatrix();

	FVector StationSignSpot;
	FString StationTitle;
};
