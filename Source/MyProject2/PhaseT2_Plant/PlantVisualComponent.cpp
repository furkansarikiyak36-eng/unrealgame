#include "PlantVisualComponent.h"
#include "PlantDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
UPlantVisualComponent::UPlantVisualComponent(){PrimaryComponentTick.bCanEverTick=false;}
void UPlantVisualComponent::SetDefinition(UPlantDefinition* InDefinition){Definition=InDefinition;}
void UPlantVisualComponent::UpdateVisual(EGrowthStage Stage,float StageProgress){AActor* Owner=GetOwner();if(!Owner)return;if(!TargetMesh)TargetMesh=Owner->FindComponentByClass<UStaticMeshComponent>();if(!TargetMesh)return;const int32 Index=FMath::Clamp(int32(Stage),0,7);if(Definition&&Definition->StageMeshes.IsValidIndex(Index)&&!Definition->StageMeshes[Index].IsNull()){if(UStaticMesh* Mesh=Definition->StageMeshes[Index].LoadSynchronous())TargetMesh->SetStaticMesh(Mesh);}const float T=FMath::Clamp((float(Index)+FMath::Clamp(StageProgress,0.f,1.f))/7.f,0.f,1.f);TargetMesh->SetRelativeScale3D(FVector(FMath::Lerp(.05f,1.f,T)));}
