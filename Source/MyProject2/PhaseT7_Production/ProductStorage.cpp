#include "ProductStorage.h"
AProductStorage::AProductStorage(){PrimaryActorTick.bCanEverTick=false;}
float AProductStorage::GetTotalStored()const{float T=0.f;for(const auto& P:Products)T+=P.Value;return T;}
bool AProductStorage::StoreProduct(FName ID,float A){if(ID.IsNone()||A<0.f||GetTotalStored()+A>FMath::Max(0.f,MaxCapacity))return false;Products.FindOrAdd(ID)+=A;return true;}
float AProductStorage::RetrieveProduct(FName ID,float A){if(A<=0.f)return 0.f;if(float* P=Products.Find(ID)){const float X=FMath::Min(*P,A);*P-=X;if(*P<=.01f)Products.Remove(ID);return X;}return 0.f;}
