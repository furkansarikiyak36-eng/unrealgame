#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ThrownWeaponActor.generated.h"
class UStaticMeshComponent;class USphereComponent;class UParticleSystemComponent;
UENUM(BlueprintType)enum class EThrownWeaponType:uint8{Molotov,Grenade,SmokeBomb,FlashBang};
UCLASS()class MYPROJECT2_API AThrownWeaponActor:public AActor
{GENERATED_BODY()public:AThrownWeaponActor();UFUNCTION(BlueprintCallable)void Throw(const FVector&Direction,float Force);UFUNCTION(BlueprintCallable)void Detonate();UFUNCTION(BlueprintCallable)void SetFuseTime(float Seconds);UPROPERTY(VisibleAnywhere)TObjectPtr<UStaticMeshComponent>Mesh;UPROPERTY(VisibleAnywhere)TObjectPtr<USphereComponent>ExplosionSphere;UPROPERTY(VisibleAnywhere)TObjectPtr<UParticleSystemComponent>ExplosionFX;UPROPERTY(EditAnywhere,BlueprintReadWrite)EThrownWeaponType WeaponType=EThrownWeaponType::Molotov;UPROPERTY(EditAnywhere,BlueprintReadWrite)float FuseTime=3;UPROPERTY(EditAnywhere,BlueprintReadWrite)float ExplosionRadius=400;UPROPERTY(EditAnywhere,BlueprintReadWrite)float Damage=60;protected:virtual void Tick(float DeltaTime)override;float ElapsedTime=0;bool bDetonated=false;};
