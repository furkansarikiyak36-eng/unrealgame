#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProductStorage.generated.h"
UCLASS() class MYPROJECT2_API AProductStorage:public AActor{GENERATED_BODY()
public: AProductStorage(); UFUNCTION(BlueprintCallable,Category="Storage") bool StoreProduct(FName ProductID,float Amount); UFUNCTION(BlueprintCallable,Category="Storage") float RetrieveProduct(FName ProductID,float Amount); UFUNCTION(BlueprintPure,Category="Storage") float GetTotalStored()const; UPROPERTY(EditAnywhere,BlueprintReadWrite) float MaxCapacity=1000.f; UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly) TMap<FName,float> Products;};
