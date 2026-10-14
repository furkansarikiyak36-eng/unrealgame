#include "RaidDebugHUD.h"
#include "Engine/Canvas.h"
void ARaidDebugHUD::DrawHUD(){Super::DrawHUD();if(bShowDebug&&Canvas)DrawText(DebugText,FLinearColor::Yellow,50.f,50.f,nullptr,1.2f);}
