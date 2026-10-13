// Faz 2 - Kesimi dünyada uygular: veriyi günceller, gerekirse yeni parça aktörü üretir.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WoodTypes.h"
#include "WoodCutLibrary.h"
#include "WoodCutterComponent.generated.h"

class UWoodPieceComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWoodCutDone, const FString&, OperationName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWoodCutFailed, const FString&, Reason);

/**
 * UWoodPieceComponent ile aynı aktöre eklenir.
 * Kesim kuralları UWoodCutLibrary'de; burada sadece sonucu dünyaya uygular.
 *
 * Varsayılan görsel: parçanın ölçeği kesim oranında değiştirilir (yaklaşık).
 * Gerçek yarım silindir / sivri uç / çentik geometrisi için ReshapeVisual olayını
 * (Blueprint veya C++) DynamicMesh ile doldurman gerekir.
 */
UCLASS(ClassGroup = (Wood), meta = (BlueprintSpawnableComponent))
class MYPROJECT2_API UWoodCutterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWoodCutterComponent();

	/** Kütüğün uzun ekseni (bileşen yerel uzayı). Varsayılan X. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood|Cut")
	FVector LocalLengthAxis = FVector(1.f, 0.f, 0.f);

	/** Yatay eksen: yuvarlak kütük önce bu eksende yarılır. Varsayılan Y. Dikey eksen = Length x Lateral. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood|Cut")
	FVector LocalLateralAxis = FVector(0.f, 1.f, 0.f);

	/** Kesimden sonra yeni parçalar bu kadar saniye uyumasın diye fizik açılır. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood|Cut")
	bool bCopySimulationToNewPieces = true;

	UPROPERTY(BlueprintAssignable, Category = "Wood|Cut")
	FWoodCutDone OnCutDone;

	UPROPERTY(BlueprintAssignable, Category = "Wood|Cut")
	FWoodCutFailed OnCutFailed;

	/** Tüm modül çizgilerinin (1..N-1) dünya konumları: kesim çizgilerini göstermek için. */
	UFUNCTION(BlueprintPure, Category = "Wood|Cut")
	TArray<FVector> GetCutLineWorldPositions() const;

	/** Mekanik 17: dünya noktasına en yakın modül çizgisi. */
	UFUNCTION(BlueprintPure, Category = "Wood|Cut")
	int32 FindNearestCutLine(FVector WorldPoint) const;

	UFUNCTION(BlueprintCallable, Category = "Wood|Cut") bool CutAtModule(int32 ModuleLine);
	UFUNCTION(BlueprintCallable, Category = "Wood|Cut") bool CutAtWorldPoint(FVector WorldPoint);
	UFUNCTION(BlueprintCallable, Category = "Wood|Cut") bool SplitLengthwise();
	UFUNCTION(BlueprintCallable, Category = "Wood|Cut") bool SplitQuarter();
	UFUNCTION(BlueprintCallable, Category = "Wood|Cut") bool MakePlank();
	UFUNCTION(BlueprintCallable, Category = "Wood|Cut") bool TrimEnd(bool bEndA);
	UFUNCTION(BlueprintCallable, Category = "Wood|Cut") bool AddNotch(int32 ModuleLine, EWoodJointType JointType);
	UFUNCTION(BlueprintCallable, Category = "Wood|Cut") bool CutGroove();
	UFUNCTION(BlueprintCallable, Category = "Wood|Cut") bool SharpenEnd(bool bEndA);
	UFUNCTION(BlueprintCallable, Category = "Wood|Cut") bool ChamferEnd(bool bEndA);
	UFUNCTION(BlueprintCallable, Category = "Wood|Cut") bool MakeFirewood();

	/**
	 * Her sonuç parçası için çağrılır (kesilen parçanın kendisi dahil).
	 * Gerçek geometriyi burada üret; varsayılan implementasyon boştur.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Wood|Cut")
	void ReshapeVisual(UWoodPieceComponent* Piece, const FWoodPieceData& NewData, const FWoodSlice& Slice);

protected:
	virtual void ReshapeVisual_Implementation(UWoodPieceComponent* Piece, const FWoodPieceData& NewData, const FWoodSlice& Slice);

private:
	UWoodPieceComponent* GetPiece() const;
	FVector GetVerticalAxis() const;

	FTransform MakeSliceTransform(const FWoodPieceData& OldData, const FTransform& OldXf, const FWoodSlice& Slice) const;

	bool Finish(const FWoodCutResult& Result, const TArray<FWoodSlice>& Slices, const TCHAR* OperationName);
	bool FinishInPlace(const FWoodCutResult& Result, const TCHAR* OperationName);
};
