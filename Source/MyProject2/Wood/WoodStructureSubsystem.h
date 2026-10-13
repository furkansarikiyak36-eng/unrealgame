// Faz 3 - Yapı alt sistemi: kurulum, snap, destek yolu, söküm (mekanik 12, 33-38, 53-58).
// Kütük fiziksel nesne olarak kalır; yapısal bağlantılar burada ayrı bir grafikte tutulur.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WoodTypes.h"
#include "WoodStructureTypes.h"
#include "WoodStructureLogic.h"
#include "WoodStructureSubsystem.generated.h"

class UWoodPieceComponent;
class UWoodCarrierComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWoodStructureRejected, const FString&, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWoodSupportLost, const TArray<UWoodPieceComponent*>&, Pieces);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWoodStructureChanged);

/**
 * Dünya başına bir tane. Sadece Game / PIE dünyalarında oluşur.
 * Pivot varsayımı: parçanın pivotu uzunluğun ortasında, uzun eksen WoodCutterComponent::LocalLengthAxis (yoksa X).
 */
UCLASS()
class MYPROJECT2_API UWoodStructureSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Snap yarıçapı (cm). Önerilen parçanın ucu bu mesafedeki bir parçaya yapışır. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood|Structure")
	float SnapRadiusCm = 40.f;

	/** Bağlantı sayılması için değme toleransı (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood|Structure")
	float ContactToleranceCm = 8.f;

	/** Alt uç zemine bu kadar yakınsa (cm) parça zemine oturmuş sayılır. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood|Structure")
	float GroundToleranceCm = 15.f;

	/** Zemin izi, alt uçtan bu kadar yukarıdan başlar (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood|Structure")
	float GroundTraceUpCm = 30.f;

	/** Eksen dikeye/yataya bu açıdan (derece) yakınsa dikey/yatay sayılır. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood|Structure")
	float OrientationToleranceDeg = 15.f;

	/** Açıksa: yeterli destek bulamayan parça kurulamaz. Kapalıysa kurulur ve desteksiz işaretlenir. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood|Structure")
	bool bRequireSupportOnPlace = false;

	/** Açıksa: desteğini kaybeden parçalar serbest kütüğe dönüp düşer (mekanik 59'un ilk hâli). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood|Structure")
	bool bCollapseUnsupported = false;

	UPROPERTY(BlueprintAssignable, Category = "Wood|Structure")
	FWoodStructureRejected OnRejected;

	/** Desteğini kaybeden parçalar (mekanik 58). */
	UPROPERTY(BlueprintAssignable, Category = "Wood|Structure")
	FWoodSupportLost OnSupportLost;

	UPROPERTY(BlueprintAssignable, Category = "Wood|Structure")
	FWoodStructureChanged OnStructureChanged;

	/** Mekanik 53, 12: önerilen konumu snap + zemin ile düzeltir. Hayalet önizleme için her karede çağrılabilir. */
	UFUNCTION(BlueprintCallable, Category = "Wood|Structure")
	FWoodSnapResult FindSnap(UWoodPieceComponent* Piece, const FTransform& Proposed);

	/** Snap sonucuna göre kur. Structure verilirse parça o aktörün köküne bağlanır. */
	UFUNCTION(BlueprintCallable, Category = "Wood|Structure")
	bool InstallPiece(UWoodPieceComponent* Piece, const FWoodSnapResult& Snap, AActor* Structure = nullptr);

	/** Mekanik 12: omuzdaki kütüğü istenen konuma yerleştir (snap + bırak + kur). */
	UFUNCTION(BlueprintCallable, Category = "Wood|Structure")
	bool PlaceCarried(UWoodCarrierComponent* Carrier, UWoodPieceComponent* Piece, const FTransform& Proposed, AActor* Structure = nullptr);

	/** Mekanik 55-57: parçayı yapıdan sök. Taşıdığı parça varsa bForce olmadan reddeder. */
	UFUNCTION(BlueprintCallable, Category = "Wood|Structure")
	bool RemovePiece(UWoodPieceComponent* Piece, bool bForce = false);

	/** Mekanik 55, 58: bu parça sökülse hangi parçalar desteğini kaybeder (UI vurgusu için). */
	UFUNCTION(BlueprintPure, Category = "Wood|Structure")
	TArray<UWoodPieceComponent*> PreviewRemoval(const UWoodPieceComponent* Piece) const;

	/** Mekanik 54 */
	UFUNCTION(BlueprintPure, Category = "Wood|Structure")
	bool IsPieceSupported(const UWoodPieceComponent* Piece) const;

	UFUNCTION(BlueprintPure, Category = "Wood|Structure")
	bool IsPieceInstalledHere(const UWoodPieceComponent* Piece) const;

	UFUNCTION(BlueprintPure, Category = "Wood|Structure")
	TArray<UWoodPieceComponent*> GetConnectedPieces(const UWoodPieceComponent* Piece) const;

	UFUNCTION(BlueprintPure, Category = "Wood|Structure")
	TArray<UWoodPieceComponent*> GetUnsupportedPieces() const;

	UFUNCTION(BlueprintPure, Category = "Wood|Structure")
	int32 GetInstalledCount() const { return Records.Num(); }

	/** Parçanın mevcut dünya yönü. */
	UFUNCTION(BlueprintPure, Category = "Wood|Structure")
	EWoodOrientation GetPieceOrientation(const UWoodPieceComponent* Piece) const;

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;

protected:
	UFUNCTION()
	void HandlePieceStateChanged(UWoodPieceComponent* Piece, EWoodState OldState, EWoodState NewState);

private:
	struct FRecord
	{
		WoodStructure::FNode Node;
		TWeakObjectPtr<UWoodPieceComponent> Piece;
		const UWoodPieceComponent* Key = nullptr;
	};

	TMap<int32, FRecord> Records;
	TMap<const UWoodPieceComponent*, int32> IdByPiece;
	TArray<WoodStructure::FEdge> Edges;
	TSet<int32> Supported;
	int32 NextId = 1;

	FVector GetLocalLengthAxis(const UWoodPieceComponent* Piece) const;
	WoodStructure::FNode MakeNode(const UWoodPieceComponent* Piece, const FTransform& Xf, int32 Id) const;
	TArray<WoodStructure::FNode> CollectNodes(int32 ExcludeId = INDEX_NONE) const;
	bool TraceGround(const WoodStructure::FNode& Node, const AActor* IgnoreOwner, float& OutGapCm) const;

	void Reject(const FString& Reason);
	void PruneInvalid();
	void RecomputeSupport(TArray<UWoodPieceComponent*>* OutLost);

	/** Grafikten çıkarır (fiziksel durumu değiştirmez). Parça kayıtlıysa true. */
	bool UnregisterPiece(UWoodPieceComponent* Piece, TArray<UWoodPieceComponent*>* OutLost);
	void CollapsePieces(const TArray<UWoodPieceComponent*>& Pieces);
};
