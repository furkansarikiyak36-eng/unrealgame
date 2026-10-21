// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ScenarioSubsystem.generated.h"

/**
 * What the greybox world holds beyond the bare map, laid out by region when play begins (Frostpunk / The Long Dark: each
 * region offers hard but reachable materials):
 * - Kayalık (west): a cliff that needs a rope, building stones on top; colder.
 * - Orman (middle): the trees of the map; the burnt cabin of the people before.
 * - Nehir kıyısı (east): a river that is cold and slow to wade, wild grain, sheep and hens on the far bank.
 * And the notes those people left (Subnautica). Not laid out while the mechanics test runs (it has its own areas).
 */
UCLASS()
class MYPROJECT2_API UScenarioSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UScenarioSubsystem* Get(const UWorld* World);

	/** The notes, in story order. */
	static const TArray<TPair<FString, FString>>& StoryNotes();

	/** Lays the world out (once). */
	void LayOut();
	bool IsLaidOut() const { return bLaidOut; }

	/** Notes the player has read, by title, in the order read. */
	const TArray<FString>& GetJournal() const { return Journal; }
	/** Adds a note to the journal; false if it was read before. */
	bool RecordNote(const FString& Title);

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	bool bLaidOut = false;
	TArray<FString> Journal;
};
