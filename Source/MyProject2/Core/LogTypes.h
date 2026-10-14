#pragma once
#include "CoreMinimal.h"
#include "LogTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class ELogState : uint8 { Loose, Carried, Dragging, Installed, Falling, Burning, Destroyed };
UENUM(BlueprintType)
enum class EWoodShape : uint8 { Full, Half, Quarter, Plank, Tapered, Firewood };
UENUM(BlueprintType)
enum class EWoodSpecies : uint8 { Pine, Oak, Birch, Cedar };
UENUM(BlueprintType)
enum class EStructuralRole : uint8 { None, Foundation, Pillar, Beam, Brace, Floor, Wall, Roof, Stair, Railing, Bridge, Scaffold };

USTRUCT(BlueprintType)
struct FLogSocket
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName SocketName = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector LocalPosition = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FRotator LocalRotation = FRotator::ZeroRotator;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0")) float SocketRadius = 8.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bIsEndSocket = false;
    // Runtime occupancy is deliberately not exposed to Blueprint; this is a reference, not socket definition data.
    UPROPERTY(Transient) TWeakObjectPtr<AActor> Occupant;
};

USTRUCT(BlueprintType)
struct FLogConnection
{
    GENERATED_BODY()
    UPROPERTY() TWeakObjectPtr<AActor> ConnectedLog;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName MySocket = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName TheirSocket = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0")) float ConnectionStrength = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bLoadBearing = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLogStateChanged, AActor*, Log, ELogState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLogDestroyed, AActor*, Log);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLogInstalled, AActor*, Log);
