// Faz 4 - Yapı tanıma (saf mantık). Mekanik 37, 39-52.
// Her özellik ayrı model değil: aynı kütük grafiğine bakıp "bu bir duvar, bu bir platform" diye okunur (bölüm D).
#pragma once

#include "CoreMinimal.h"
#include "WoodStructureLogic.h"

namespace WoodStructure
{
	struct FFeature
	{
		EWoodFeatureKind Kind = EWoodFeatureKind::Platform;
		TArray<int32> NodeIds;
		FVector Center = FVector::ZeroVector;
		double SpanCm = 0.0; // açıklıklarda (Opening/DoorFrame/WindowFrame) boşluk genişliği, diğerlerinde en geniş boyut
	};

	/**
	 * Düğüm grafiğinden yapı özelliklerini çıkarır. Sezgiseldir ve bilerek temkinlidir: belirsizse özellik üretmez.
	 *
	 * Kurallar (özet):
	 * - Dikey duvar: yan yana değen en az 3 dikey direk/tahta. Yarısı sivriyse kazık duvarı; payandası varsa savunma duvarı.
	 * - Yatay duvar: üst üste oturan (paralel veya çentikli) yatay kütüklerden en az 3 katman.
	 * - Açıklık: dikey duvar parçaları arasında 70-400 cm boşluk; iki yanı bağlayan üst kiriş varsa kapı kasası, alt kiriş de varsa pencere kasası.
	 * - Platform: en az iki kirişe, her biri bir direğe oturan döşeme.
	 * - Çatı iskeleti: tepede buluşan iki karşılıklı çapraz payanda (15-60 derece). Kaplama: iskelete bağlı en az iki çatı tahtası.
	 * - Rampa: zemine oturan 35 dereceden yatık çapraz parça. Merdiven: çapraz taşıyıcıya bağlı en az 3 farklı yükseklikte yatay parça.
	 * - Köprü: 250 cm üzeri yatay parça, iki ucu zemindeki parçalara oturur, ortası boş.
	 * - Korkuluk: platforma oturan kısa direkleri bağlayan yatay kiriş.
	 * - İskele: geçici işaretli parçaların bağlı kümesi.
	 */
	TArray<FFeature> AnalyzeStructure(const TArray<FNode>& Nodes, const TArray<FEdge>& Edges, float ContactTolCm = 8.f, float OrientTolDeg = 15.f);
}
