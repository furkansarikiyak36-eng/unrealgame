// Faz 2 - Kesim kuralları (saf veri). Mekanik 16-32.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WoodTypes.h"
#include "WoodCutLibrary.generated.h"

USTRUCT(BlueprintType)
struct MYPROJECT2_API FWoodCutResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wood")
	bool bSuccess = false;

	/** Başarısızlık nedeni (oyuncuya gösterilebilir). */
	UPROPERTY(BlueprintReadOnly, Category = "Wood")
	FString Reason;

	/** Sonuç parçaları. Yerinde düzenlemelerde tek eleman, kesimlerde birden fazla. */
	UPROPERTY(BlueprintReadOnly, Category = "Wood")
	TArray<FWoodPieceData> Pieces;
};

UCLASS()
class MYPROJECT2_API UWoodCutLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Mekanik 17: A ucundan mesafeyi en yakın modül çizgisine yuvarlar (1..N-1). */
	UFUNCTION(BlueprintPure, Category = "Wood|Cut")
	static int32 SnapToModuleLine(const FWoodPieceData& Data, float DistanceFromEndACm);

	/** Mekanik 16, 18: enine kesim. */
	UFUNCTION(BlueprintCallable, Category = "Wood|Cut")
	static FWoodCutResult CrossCut(const FWoodPieceData& Data, int32 ModuleLine);

	/** Mekanik 19, 20: yuvarlak kütüğü iki yarım kütüğe ayır. */
	UFUNCTION(BlueprintCallable, Category = "Wood|Cut")
	static FWoodCutResult SplitLengthwise(const FWoodPieceData& Data);

	/** Mekanik 22: yarım kütüğü iki çeyrek parçaya ayır. */
	UFUNCTION(BlueprintCallable, Category = "Wood|Cut")
	static FWoodCutResult SplitQuarter(const FWoodPieceData& Data);

	/** Mekanik 21: yarım veya çeyrek parçayı düz tahtaya çevir. */
	UFUNCTION(BlueprintCallable, Category = "Wood|Cut")
	static FWoodCutResult MakePlank(const FWoodPieceData& Data);

	/** Mekanik 23: sivri/pahlı ucu düzelt. */
	UFUNCTION(BlueprintCallable, Category = "Wood|Cut")
	static FWoodCutResult TrimEnd(const FWoodPieceData& Data, bool bEndA);

	/** Mekanik 24-27, 29: çentik aç. ModuleLine 0..N. */
	UFUNCTION(BlueprintCallable, Category = "Wood|Cut")
	static FWoodCutResult AddNotch(const FWoodPieceData& Data, int32 ModuleLine, EWoodJointType JointType);

	/** Mekanik 28: boyuna oluk. */
	UFUNCTION(BlueprintCallable, Category = "Wood|Cut")
	static FWoodCutResult CutGroove(const FWoodPieceData& Data);

	/** Mekanik 30: ucu sivriltme. */
	UFUNCTION(BlueprintCallable, Category = "Wood|Cut")
	static FWoodCutResult SharpenEnd(const FWoodPieceData& Data, bool bEndA);

	/** Mekanik 31: pah kırma. */
	UFUNCTION(BlueprintCallable, Category = "Wood|Cut")
	static FWoodCutResult ChamferEnd(const FWoodPieceData& Data, bool bEndA);

	/** Mekanik 32: ateşlik odun. Her modül için kesite göre 4 / 2 / 1 çeyrek parça üretir. */
	UFUNCTION(BlueprintCallable, Category = "Wood|Cut")
	static FWoodCutResult MakeFirewood(const FWoodPieceData& Data);
};
