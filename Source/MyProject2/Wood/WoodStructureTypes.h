// Faz 3 - Yapı ortak tipleri (yön, bağlantı türü, snap sonucu).
#pragma once

#include "CoreMinimal.h"
#include "WoodTypes.h"
#include "WoodStructureTypes.generated.h"

class UWoodPieceComponent;

/** Parçanın uzun ekseninin dünya yönü (bölüm D: işlev yönden türetilir). */
UENUM(BlueprintType)
enum class EWoodOrientation : uint8
{
	Vertical,
	Horizontal,
	Diagonal
};

/** İki parça arasındaki yapısal bağlantının türü. */
UENUM(BlueprintType)
enum class EWoodConnectionKind : uint8
{
	/** Uç ucuna değiyor (direk tepesi - kiriş ucu). */
	EndToEnd,
	/** Bir parçanın ucu diğerinin gövdesine değiyor. */
	EndToBody,
	/** İki gövde çaprazlanıyor ve en az birinde o noktada çentik var. Tek yönlü: üstteki alttakine oturur. */
	Notched,
	/** Döşeme/duvar/çatı tahtası bir taşıyıcının üstüne yaslanıyor (çentik gerekmez). Tek yönlü. */
	Resting
};

/** Yapı tanıma sonucu (mekanik 37-52): işlev modelden değil geometri ve bağlantıdan çıkarılır. */
UENUM(BlueprintType)
enum class EWoodFeatureKind : uint8
{
	HorizontalWall, // 39 üst üste kütük duvarı
	VerticalWall,   // 40 yan yana dikilmiş kütük duvarı
	StakeWall,      // 41 sivri kazık duvarı
	DefenseWall,    // 51 payandalı kazık duvarı
	Opening,        // 42-43 duvarda geçit/pencere boşluğu (kasasız)
	DoorFrame,      // 44 üstten lento ile çevrili geçit
	WindowFrame,    // 44 lento + denizlik ile çevrili pencere
	Platform,       // 37 direk + kiriş + döşeme
	RoofFrame,      // 45 sırt çatısı iskeleti
	RoofSurface,    // 46 çatı kaplaması
	Ramp,           // 47 yürünebilir eğik parça
	Stairs,         // 47 basamaklı eğik taşıyıcı
	Bridge,         // 48 iki zemin noktası arasında açıklık
	Railing,        // 50 platform üstünde direk + tırabzan
	Scaffold        // 49, 52 geçici iskele
};

USTRUCT(BlueprintType)
struct MYPROJECT2_API FWoodFeature
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wood")
	EWoodFeatureKind Kind = EWoodFeatureKind::Platform;

	UPROPERTY(BlueprintReadOnly, Category = "Wood")
	TArray<UWoodPieceComponent*> Pieces;

	UPROPERTY(BlueprintReadOnly, Category = "Wood")
	FVector Center = FVector::ZeroVector;

	/** Özelliğin en geniş boyutu (cm). Açıklıklarda boşluk genişliği. */
	UPROPERTY(BlueprintReadOnly, Category = "Wood")
	float SpanCm = 0.f;
};

/** FindSnap sonucu: hayalet önizleme ve InstallPiece bununla çalışır. */
USTRUCT(BlueprintType)
struct MYPROJECT2_API FWoodSnapResult
{
	GENERATED_BODY()

	/** Düzeltilmiş dünya dönüşümü. */
	UPROPERTY(BlueprintReadOnly, Category = "Wood")
	FTransform SnappedTransform;

	/** Başka bir parçaya yapıştı. */
	UPROPERTY(BlueprintReadOnly, Category = "Wood")
	bool bSnapped = false;

	/** Alt uç zemine oturuyor. */
	UPROPERTY(BlueprintReadOnly, Category = "Wood")
	bool bGrounded = false;

	/** Kurulabilir: snap veya zemin var. */
	UPROPERTY(BlueprintReadOnly, Category = "Wood")
	bool bPlaceable = false;

	/** Snap hedefi (bSnapped ise). */
	UPROPERTY(BlueprintReadOnly, Category = "Wood")
	UWoodPieceComponent* Target = nullptr;

	/** Bu bağlantı için önerilen çentik türü. */
	UPROPERTY(BlueprintReadOnly, Category = "Wood")
	EWoodJointType SuggestedJoint = EWoodJointType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wood")
	EWoodOrientation Orientation = EWoodOrientation::Horizontal;

	/** Yönden ve kesitten türetilen işlev (direk/kiriş/payanda/döşeme...). */
	UPROPERTY(BlueprintReadOnly, Category = "Wood")
	EWoodRole DerivedRole = EWoodRole::Unassigned;
};
