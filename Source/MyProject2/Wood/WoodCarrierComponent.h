// Faz 1 - Oyuncu taşıma bileşeni: hacme dayalı omuz kapasitesi (mekanik 4-6, 11)
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WoodCarrierComponent.generated.h"

class UWoodPieceComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWoodCarrierChanged);

UCLASS(ClassGroup = (Wood), meta = (BlueprintSpawnableComponent))
class MYPROJECT2_API UWoodCarrierComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWoodCarrierComponent();

	/** Toplam taşıma kapasitesi (litre). 390 cm x 40 cm tam kütük ~490 L. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood|Carry")
	float MaxCarryLiters = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood|Carry")
	FName CarrySocket = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood|Carry")
	FVector CarryOffset = FVector(0.f, 0.f, 0.f);

	/** Her ek parça için ofsete eklenen kayma. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood|Carry")
	FVector StackStep = FVector(0.f, 0.f, 12.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wood|Carry")
	FRotator CarryRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintAssignable, Category = "Wood|Carry")
	FWoodCarrierChanged OnCarriedChanged;

	/** Taşıma noktasını ayarla (ör. karakter mesh'i veya omuz bileşeni). */
	UFUNCTION(BlueprintCallable, Category = "Wood|Carry")
	void SetCarryAttachComponent(USceneComponent* InAttach) { CarryAttach = InAttach; }

	UFUNCTION(BlueprintPure, Category = "Wood|Carry")
	float GetCarriedLiters() const;

	UFUNCTION(BlueprintPure, Category = "Wood|Carry")
	bool CanCarry(const UWoodPieceComponent* Piece) const;

	UFUNCTION(BlueprintPure, Category = "Wood|Carry")
	TArray<UWoodPieceComponent*> GetCarried() const { return Carried; }

	/** Bakış yönünde küre taramasıyla kütük bul. */
	UFUNCTION(BlueprintCallable, Category = "Wood|Carry")
	UWoodPieceComponent* FindPieceInView(FVector Start, FVector Direction, float Distance = 300.f, float Radius = 15.f) const;

	UFUNCTION(BlueprintCallable, Category = "Wood|Carry")
	bool TryPickUp(UWoodPieceComponent* Piece);

	UFUNCTION(BlueprintCallable, Category = "Wood|Carry")
	bool DropPiece(UWoodPieceComponent* Piece);

	/** En son alınanı bırak. */
	UFUNCTION(BlueprintCallable, Category = "Wood|Carry")
	bool DropLast();

	UFUNCTION(BlueprintCallable, Category = "Wood|Carry")
	void DropAll();

	UFUNCTION(BlueprintCallable, Category = "Wood|Carry")
	bool ThrowLast(FVector Velocity);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> CarryAttach;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWoodPieceComponent>> CarriedStorage;

	// Blueprint'e açılan görünüm (CarriedStorage ile aynı içerik)
	TArray<UWoodPieceComponent*> Carried;
};
