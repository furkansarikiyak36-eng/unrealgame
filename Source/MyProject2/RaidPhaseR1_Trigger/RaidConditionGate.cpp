#include "RaidConditionGate.h"
URaidConditionGate::URaidConditionGate(){PrimaryComponentTick.bCanEverTick=false;}
bool URaidConditionGate::EvaluateGate(const FRaidContext& C) const {if(!bEnabled||C.Players.Num()<FMath::Max(0,MinPlayers)||(bRequireNight&&!C.World.bIsNight))return false;if(bRequireBase){for(const FPlayerStateSnapshot& P:C.Players)if(P.bIsAlive)return true;return false;}return true;}
