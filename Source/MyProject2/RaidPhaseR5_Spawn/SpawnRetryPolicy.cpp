#include "SpawnRetryPolicy.h"
bool USpawnRetryPolicy::ShouldRetry(int32 N) const{return N<FMath::Max(0,Policy.MaxAttempts);}
float USpawnRetryPolicy::GetNextAttemptDelay(int32 N) const{return FMath::Max(0.f,Policy.AttemptInterval)*FMath::Pow(FMath::Max(1.f,Policy.BackoffMultiplier),(float)FMath::Max(0,N));}
