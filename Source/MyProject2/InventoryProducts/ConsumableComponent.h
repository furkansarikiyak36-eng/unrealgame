#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ConsumableComponent.generated.h"
UENUM(BlueprintType)enum class EConsumableEffect:uint8{RestoreHealth,RestoreStamina,RestoreHunger,RestoreThirst,RestoreWarmth,CurePoison,BoostStrength,BoostSpeed};
USTRUCT(BlueprintType)struct FConsumableEffectData{GENERATED_BODY() UPROPERTY(EditAnywhere,BlueprintReadWrite)EConsumableEffect Effect=EConsumableEffect::RestoreHealth;UPROPERTY(EditAnywhere,BlueprintReadWrite)float Amount=20;UPROPERTY(EditAnywhere,BlueprintReadWrite)float Duration=0;};
USTRUCT(BlueprintType)struct FConsumableData{GENERATED_BODY() UPROPERTY(EditAnywhere,BlueprintReadWrite)TArray<FConsumableEffectData>Effects;UPROPERTY(EditAnywhere,BlueprintReadWrite)float Cooldown=2;UPROPERTY(EditAnywhere,BlueprintReadWrite)FName ConsumedSoundTag=NAME_None;};
UCLASS(ClassGroup=(Inventory),meta=(BlueprintSpawnableComponent))class MYPROJECT2_API UConsumableComponent:public UActorComponent
{GENERATED_BODY() public:UConsumableComponent();UFUNCTION(BlueprintCallable)bool Consume(FName ItemID);UFUNCTION(BlueprintPure)TArray<FConsumableEffectData>GetEffectsFor(FName ItemID)const;UPROPERTY(EditAnywhere,BlueprintReadWrite)TMap<FName,FConsumableData>ConsumableDatabase;};
