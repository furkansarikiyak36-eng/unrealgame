#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SparseCellStorage.generated.h"
USTRUCT(BlueprintType) struct FSparseCellRecord { GENERATED_BODY() UPROPERTY(BlueprintReadWrite) int32 LocalX=0; UPROPERTY(BlueprintReadWrite) int32 LocalY=0; UPROPERTY(BlueprintReadWrite) float Value=0; UPROPERTY(BlueprintReadWrite) bool bHasData=false; };
UCLASS()
class MYPROJECT2_API USparseCellStorage : public UObject
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable,Category="Sparse") void SetCellValue(int32 LocalX,int32 LocalY,float Value,float DefaultValue);
 UFUNCTION(BlueprintPure,Category="Sparse") float GetCellValue(int32 LocalX,int32 LocalY,float DefaultValue) const;
 UFUNCTION(BlueprintPure,Category="Sparse") bool HasCustomValue(int32 LocalX,int32 LocalY) const;
 UFUNCTION(BlueprintCallable,Category="Sparse") void RemoveCell(int32 LocalX,int32 LocalY);
 UFUNCTION(BlueprintCallable,Category="Sparse") void Clear(){Records.Empty();}
 UFUNCTION(BlueprintPure,Category="Sparse") int32 GetCustomCellCount() const{return Records.Num();}
protected:
 static uint64 MakeKey(int32 X,int32 Y){return (uint64(uint32(Y))<<32)|uint64(uint32(X));}
 UPROPERTY() TMap<uint64,FSparseCellRecord> Records;
};
