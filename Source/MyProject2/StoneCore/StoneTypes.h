#pragma once
#include "CoreMinimal.h"
#include "MaterialCore/MaterialTypes.h"
#include "StoneTypes.generated.h"

UENUM(BlueprintType)
enum class EStoneShape : uint8
{
    Small UMETA(DisplayName = "Küçük Taş"),
    Building UMETA(DisplayName = "Yapı Taşı"),
    Flat UMETA(DisplayName = "Yassı Taş"),
    Carved UMETA(DisplayName = "Yontulmuş"),
    Boulder UMETA(DisplayName = "Büyük Kaya"),
    Rubble UMETA(DisplayName = "Moloz"),
    Gravel UMETA(DisplayName = "Çakıl"),
    Voussoir UMETA(DisplayName = "Kemer Kaması"),
    Keystone UMETA(DisplayName = "Kilit Taşı")
};

UENUM(BlueprintType)
enum class EStoneType : uint8
{
    Granite UMETA(DisplayName = "Granit"),
    Limestone UMETA(DisplayName = "Kireçtaşı"),
    Sandstone UMETA(DisplayName = "Kumtaşı"),
    Slate UMETA(DisplayName = "Kayağantaşı"),
    Obsidian UMETA(DisplayName = "Obsidyen")
};

UENUM(BlueprintType)
enum class EStoneRole : uint8
{
    None UMETA(DisplayName = "Yok"),
    Foundation UMETA(DisplayName = "Temel"),
    Pillar UMETA(DisplayName = "Kolon"),
    Beam UMETA(DisplayName = "Kiriş"),
    Wall UMETA(DisplayName = "Duvar"),
    Floor UMETA(DisplayName = "Döşeme"),
    Arch UMETA(DisplayName = "Kemer"),
    Stair UMETA(DisplayName = "Merdiven"),
    Fireplace UMETA(DisplayName = "Şömine"),
    Path UMETA(DisplayName = "Yol"),
    Trap UMETA(DisplayName = "Tuzak")
};

USTRUCT(BlueprintType)
struct FStoneFace
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName FaceName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector LocalNormal = FVector::UpVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float FaceArea = 100.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Roughness = 0.5f;
    UPROPERTY(Transient) TWeakObjectPtr<AActor> OccupiedBy;
};

USTRUCT(BlueprintType)
struct FStoneSocket
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName SocketName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector LocalPosition;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FRotator LocalRotation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float SocketRadius = 10.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsVertical = false;
    UPROPERTY(Transient) TWeakObjectPtr<AActor> Occupant;
};
