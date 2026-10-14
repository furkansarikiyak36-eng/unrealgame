#include "WaveExceptionHandler.h"
bool UWaveExceptionHandler::HandleStuckCapacity(float D,float T)const{return D>FMath::Max(0.f,T);}
float UWaveExceptionHandler::HandleNoSpawnPosition(float R,float M)const{return FMath::Min(FMath::Max(R*1.25f,R+100.f),FMath::Max(R,M));}
bool UWaveExceptionHandler::HandlePlayerDeath(int32 ID)const{return ID>=0;}
void UWaveExceptionHandler::HandleServerPause(){}
bool UWaveExceptionHandler::HandleCorruptWaveDefinition(FName ID)const{return !ID.IsNone();}
