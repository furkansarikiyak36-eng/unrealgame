// Faz 1 - Kütük sistemi ortak veri modeli (MyProject2)
#pragma once

#include "CoreMinimal.h"
#include "WoodTypes.generated.h"

/** Kütüğün yaşam döngüsü durumu (mekanik 4-10, 56). */
UENUM(BlueprintType)
enum class EWoodState : uint8
{
	Loose      UMETA(DisplayName = "Loose (Yerde serbest)"),
	Carried    UMETA(DisplayName = "Carried (Taşınıyor)"),
	Installed  UMETA(DisplayName = "Installed (Yapıya bağlı)")
};

/** Kesit biçimi (mekanik 19-22). */
UENUM(BlueprintType)
enum class EWoodCrossSection : uint8
{
	Round,
	Half,
	Quarter,
	Plank
};

/** Uç biçimi (mekanik 23, 30, 31). */
UENUM(BlueprintType)
enum class EWoodEndShape : uint8
{
	Flat,
	Pointed,
	Chamfered
};

/** Bağlantı / çentik türü (mekanik 24-29). */
UENUM(BlueprintType)
enum class EWoodJointType : uint8
{
	None,
	Corner,
	Saddle,
	Oval,
	FlatLap,
	Groove,
	Dovetail
};

/** Parçanın yapıdaki işlevi; geometri ve yönden türetilir (bölüm D). */
UENUM(BlueprintType)
enum class EWoodRole : uint8
{
	Unassigned,
	Post,
	Beam,
	Brace,
	Floor,
	Wall,
	Roof,
	Stake,
	Firewood,
	Furniture
};

/** Modül çizgisi üzerindeki çentik. ModuleLine 0..6 (0 = A ucu, 6 = tam boy). */
USTRUCT(BlueprintType)
struct MYPROJECT2_API FWoodNotch
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood")
	int32 ModuleLine = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood")
	EWoodJointType JointType = EWoodJointType::None;
};

/** Her kütük parçasının taşıdığı veri (bölüm 2). */
USTRUCT(BlueprintType)
struct MYPROJECT2_API FWoodPieceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood")
	FName SourceSpecies = NAME_None;

	/** Kaynak ağacın yüksekliği (cm). Plan: şu an sabit 15.6 m. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood")
	float SourceTreeHeightCm = 1560.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood")
	float DiameterCm = 40.f;

	/** Tam kütük boyu (cm). Modül = bunun 1/6'sı (390 cm -> 65 cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood", meta = (ClampMin = "10"))
	float FullLogLengthCm = 450.f;

	/** Parçanın modül sayısı, 1..6 (mekanik 16-18). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood", meta = (ClampMin = "1", ClampMax = "6"))
	int32 LengthModules = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood")
	EWoodCrossSection CrossSection = EWoodCrossSection::Round;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood")
	EWoodEndShape EndA = EWoodEndShape::Flat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood")
	EWoodEndShape EndB = EWoodEndShape::Flat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood")
	bool bHollow = false;

	/** Boyuna oluk açıldı mı (mekanik 28). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood")
	bool bLengthGroove = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood")
	TArray<FWoodNotch> Notches;

	/** 0 = sağlam, 1 = tamamen hasarlı (mekanik 61-63). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood", meta = (ClampMin = "0", ClampMax = "1"))
	float DamageFraction = 0.f;

	float GetModuleLengthCm() const;
	float GetLengthCm() const;
	/** Yaklaşık hacim (litre); taşıma kapasitesi bununla hesaplanır. */
	float GetVolumeLiters() const;

	/** Kesit boyutları. Kural: Yatay (Lateral) eksen önce yarılır, Dikey (Vertical) eksen sonra. */
	float GetLateralCm() const;
	float GetVerticalCm() const;
};

/**
 * Bir kesim çıktısının, kesimden önceki parçanın kutusu içindeki yeri.
 * S: uzunluk boyunca 0..1 (A ucundan B ucuna). L ve V: -0.5..0.5 (merkezden).
 * Varsayılan değer = parçanın tamamı.
 */
USTRUCT(BlueprintType)
struct MYPROJECT2_API FWoodSlice
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood") float S0 = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood") float S1 = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood") float L0 = -0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood") float L1 = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood") float V0 = -0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood") float V1 = 0.5f;
};
