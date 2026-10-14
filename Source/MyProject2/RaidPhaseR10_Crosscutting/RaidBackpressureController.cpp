#include "RaidBackpressureController.h"
bool URaidBackpressureController::CanAcceptRequest(int32 N)const{return !bOverloaded&&N<FMath::Max(0,MaxPendingRequests);}
void URaidBackpressureController::SignalOverload(){bOverloaded=true;}
void URaidBackpressureController::ClearOverload(){bOverloaded=false;}
