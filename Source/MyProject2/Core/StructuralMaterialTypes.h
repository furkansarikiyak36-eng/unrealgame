#pragma once
#include "CoreMinimal.h"
#include "StructuralMaterialTypes.generated.h"

UENUM(BlueprintType)
enum class EStructuralMaterialType : uint8
{
    Wood UMETA(DisplayName = "Ahşap"),
    Stone UMETA(DisplayName = "Taş"),
    Metal UMETA(DisplayName = "Metal"),
    Rope UMETA(DisplayName = "Halat"),
    Mortar UMETA(DisplayName = "Harç")
};

UENUM(BlueprintType)
enum class EStructuralNodeState : uint8
{
    Loose UMETA(DisplayName = "Serbest"),
    Carried UMETA(DisplayName = "Taşınan"),
    Dragging UMETA(DisplayName = "Sürüklenen"),
    Installed UMETA(DisplayName = "Yapıya Bağlı"),
    Falling UMETA(DisplayName = "Düşen"),
    Burning UMETA(DisplayName = "Yanan"),
    Fractured UMETA(DisplayName = "Kırılmış"),
    Destroyed UMETA(DisplayName = "Yok Edilmiş")
};

UENUM(BlueprintType)
enum class EConnectionType : uint8
{
    None UMETA(DisplayName = "Yok"),
    ButtJoint UMETA(DisplayName = "Uç Uca"),
    Dovetail UMETA(DisplayName = "Kırlangıç Kuyruğu"),
    SaddleNotch UMETA(DisplayName = "Eyer Çentiği"),
    Mortared UMETA(DisplayName = "Harçlı"),
    Stacked UMETA(DisplayName = "İstiflenmiş"),
    ArchVoussoir UMETA(DisplayName = "Kemer Kaması"),
    Woven UMETA(DisplayName = "Örgülü"),
    Bound UMETA(DisplayName = "Bağlanmış"),
    Lashed UMETA(DisplayName = "Halatla Bağlı")
};

USTRUCT(BlueprintType)
struct FStructuralConnection
{
    GENERATED_BODY()

    UPROPERTY(Transient) TWeakObjectPtr<AActor> ConnectedNode;
    UPROPERTY(BlueprintReadWrite) EStructuralMaterialType MyMaterial = EStructuralMaterialType::Wood;
    UPROPERTY(BlueprintReadWrite) EStructuralMaterialType TheirMaterial = EStructuralMaterialType::Wood;
    UPROPERTY(BlueprintReadWrite) EConnectionType ConnectionType = EConnectionType::None;
    UPROPERTY(BlueprintReadWrite) FName MySocket;
    UPROPERTY(BlueprintReadWrite) FName TheirSocket;
    UPROPERTY(BlueprintReadWrite) float ConnectionStrength = 1.0f;
    UPROPERTY(BlueprintReadWrite) bool bLoadBearing = true;
    UPROPERTY(BlueprintReadWrite) bool bMortared = false;
};
