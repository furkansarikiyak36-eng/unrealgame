// Faz 1 - Kütük durum makinesi (Loose / Carried / Installed) ve serbest kütük fiziği
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WoodTypes.h"
#include "WoodPieceComponent.generated.h"

class UPrimitiveComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FWoodStateChanged, UWoodPieceComponent*, Piece, EWoodState, OldState, EWoodState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FWoodImpact, UWoodPieceComponent*, Piece, AActor*, OtherActor, float, ImpulseStrength);

/**
 * Herhangi bir aktöre eklenir; o aktörün kök primitive'ini kütük olarak yönetir.
 * Mekanikler: 4 alma, 5 bırakma, 6 düşürme, 7 yuvarlama, 8 sürükleme, 9 itme, 10 çarpışma,
 * 56 yapıdan sökme, 74 fırlatma.
 */
UCLASS(ClassGroup = (Wood), meta = (BlueprintSpawnableComponent))
class MYPROJECT2_API UWoodPieceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWoodPieceComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood")
	FWoodPieceData PieceData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood")
	EWoodRole Role = EWoodRole::Unassigned;

	/** Geçici inşaat parçası (iskele, kalıp). Mekanik 49, 52: yapı bitince topluca sökülür. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood")
	bool bTemporary = false;

	/** Loose iken kullanılacak çarpışma modu. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood|Physics")
	TEnumAsByte<ECollisionEnabled::Type> LooseCollision = ECollisionEnabled::QueryAndPhysics;

	/** Bu değerin altındaki çarpışmalar OnImpact üretmez. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood|Physics")
	float MinImpactImpulse = 5000.f;

	/** Sürükleme ivmesi (cm/s^2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood|Physics")
	float DragAcceleration = 600.f;

	UPROPERTY(BlueprintAssignable, Category = "Wood")
	FWoodStateChanged OnStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Wood")
	FWoodImpact OnImpact;

	UFUNCTION(BlueprintPure, Category = "Wood")
	EWoodState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Wood")
	AActor* GetInstalledOn() const { return InstalledOn.Get(); }

	UFUNCTION(BlueprintPure, Category = "Wood")
	UPrimitiveComponent* GetPrimitive() const;

	/** Mekanik 4 */
	UFUNCTION(BlueprintCallable, Category = "Wood")
	bool PickUp(USceneComponent* AttachTo, FName Socket = NAME_None);

	/** Mekanik 5 */
	UFUNCTION(BlueprintCallable, Category = "Wood")
	bool Drop();

	/** Mekanik 6 / 74: hızla bırak ya da fırlat. */
	UFUNCTION(BlueprintCallable, Category = "Wood")
	bool ThrowPiece(FVector Velocity);

	/** Mekanik 7: yönde yuvarla. */
	UFUNCTION(BlueprintCallable, Category = "Wood")
	void Roll(FVector Direction, float Strength);

	/** Mekanik 8: hedefe doğru yerde çek. */
	UFUNCTION(BlueprintCallable, Category = "Wood")
	void DragToward(FVector TargetLocation);

	/** Mekanik 9: it. */
	UFUNCTION(BlueprintCallable, Category = "Wood")
	void Push(FVector Impulse);

	/** Yapıya bağla (Installed). */
	UFUNCTION(BlueprintCallable, Category = "Wood")
	bool Install(AActor* Structure, USceneComponent* AttachTo);

	/** Mekanik 56: yapıdan sök, serbest kütük yap. */
	UFUNCTION(BlueprintCallable, Category = "Wood")
	bool Uninstall();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	void ChangeState(EWoodState NewState);

	UPROPERTY(VisibleAnywhere, Category = "Wood")
	EWoodState State = EWoodState::Loose;

	TWeakObjectPtr<AActor> InstalledOn;
	mutable TWeakObjectPtr<UPrimitiveComponent> CachedPrimitive;
};
