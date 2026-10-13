// Faz 3 - Yapı mantığı (saf veri, dünyaya dokunmaz, otomasyon testiyle doğrulanır).
// Mekanik 12, 33-38, 53-58. Kütük fiziksel nesne olarak kalır; burada yalnızca yapısal grafik hesaplanır.
#pragma once

#include "CoreMinimal.h"
#include "WoodTypes.h"
#include "WoodStructureTypes.h"

namespace WoodStructure
{
	/** Bu hasarın üstünde çentik/uç bağlantıları gevşer: parça artık bağlantı üzerinden destek vermez/almaz (mekanik 63). Yaslanma etkilenmez. */
	constexpr float JointLooseDamage = 0.6f;

	/** Bu hasarda parça kopar: yapıdan ayrılır, kimseyi taşıyamaz (mekanik 59, 61, 62). */
	constexpr float FailureDamage = 0.85f;

	/** Yapı grafiğindeki bir parça: iki uç noktası olan kalın bir çubuk. */
	struct FNode
	{
		int32 Id = INDEX_NONE;
		FVector A = FVector::ZeroVector; // uzunluk ekseninin bir ucu (dünya)
		FVector B = FVector::ZeroVector; // diğer ucu
		float RadiusCm = 15.f;
		int32 LengthModules = 6;
		TArray<FWoodNotch> Notches;
		EWoodRole Role = EWoodRole::Unassigned;
		bool bGrounded = false; // alt ucu zemine oturuyor: kendi başına destekli sayılır
		float Damage = 0.f;     // 0 sağlam .. 1 yıkık (mekanik 61-63)
		bool bTemporary = false; // geçici inşaat parçası (mekanik 49, 52)

		FVector Axis() const { return (B - A).GetSafeNormal(); }
		double Length() const { return FVector::Dist(A, B); }
	};

	/**
	 * İki parça arasındaki bağlantı. Tek yönlüyse (bOneWay) A taşınan, B taşıyan parçadır.
	 * Çift yönlüyse (aynı yükseklikte uç uca) yön yoktur; ikisi birbirini destekleyebilir.
	 */
	struct FEdge
	{
		int32 A = INDEX_NONE;
		int32 B = INDEX_NONE;
		EWoodConnectionKind Kind = EWoodConnectionKind::EndToEnd;
		bool bOneWay = false;
	};

	/** Snap önerisi: Geom parçası Offset kadar kaydırılırsa hedefe oturur. */
	struct FSnap
	{
		bool bFound = false;
		int32 TargetId = INDEX_NONE;
		EWoodConnectionKind Kind = EWoodConnectionKind::EndToEnd;
		FVector Offset = FVector::ZeroVector;
		EWoodJointType SuggestedJoint = EWoodJointType::None;
	};

	/** Eksen dikeye (Z) veya yataya OrientTolDeg dereceden yakınsa dikey/yatay, değilse çapraz. Yön işareti önemsiz. */
	EWoodOrientation ClassifyOrientation(const FVector& Axis, float OrientTolDeg = 15.f);

	/** Kesit + yönden işlev: yuvarlak -> direk/kiriş/payanda (sivri dikey = kazık); düz yüzeyli (tahta, yarım) -> zemin/duvar/çatı. */
	EWoodRole DeriveRole(const FWoodPieceData& Data, EWoodOrientation Orientation);

	/**
	 * Yeni parçanın mevcut parçalarla gerçek bağlantılarını bulup OutEdges'e ekler.
	 * Sadece yan yana durmak bağlantı değildir: uç değmesi, çentikli geçme veya tahta yaslanması gerekir.
	 */
	void FindConnections(const FNode& NewNode, const TArray<FNode>& Existing, float ContactTolCm, TArray<FEdge>& OutEdges, float OrientTolDeg = 15.f);

	/**
	 * Desteklenen düğüm kümesi. Zemindeki düğümler destekli; diğerleri, destekli komşulardan yeterli sayıda destek alıyorsa
	 * (kiriş/döşeme/çatı en az 2, diğerleri en az 1). Döngüsel destek geçersizdir: zemine ulaşmayan çerçeve havada kalır.
	 */
	TSet<int32> ComputeSupport(const TArray<FNode>& Nodes, const TArray<FEdge>& Edges);

	/** RemovedId kalkarsa desteğini kaybedecek parçalar (kendisi hariç). */
	TArray<int32> FindDependentsOfRemoval(const TArray<FNode>& Nodes, const TArray<FEdge>& Edges, int32 RemovedId);

	/** Geom için en iyi snap: uç-uç (öncelikli) veya uç-gövde (modül çizgisine oturur). SnapRadiusCm içinde yoksa bFound=false. */
	FSnap FindBestSnap(const FNode& Geom, const TArray<FNode>& Others, float SnapRadiusCm, float OrientTolDeg = 15.f);

	/** Hasarı 0..1 arasında biriktirir (negatif miktar onarım). */
	float AccumulateDamage(float Current, float Amount);

	/** Geçici parçaların hepsi sökülürse desteğini kaybedecek KALICI parçalar (mekanik 52). */
	TArray<int32> FindTemporaryRemovalDependents(const TArray<FNode>& Nodes, const TArray<FEdge>& Edges);
}
