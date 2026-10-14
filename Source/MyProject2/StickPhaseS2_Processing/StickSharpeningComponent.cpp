#include "StickSharpeningComponent.h"
#include "StickPhaseS1_Core/StickActor.h"
UStickSharpeningComponent::UStickSharpeningComponent(){PrimaryComponentTick.bCanEverTick=false;}
bool UStickSharpeningComponent::SharpenStick(AStickActor*S,ESharpeningMode M){if(!S)return false;S->ApplyShape(M==ESharpeningMode::DoubleEnd?EStickShape::DoublePoint:EStickShape::Sharpened);return true;}
