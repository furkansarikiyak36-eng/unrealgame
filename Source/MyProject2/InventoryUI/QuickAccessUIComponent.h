#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "QuickAccessUIComponent.generated.h"
class UQuickAccessComponent;
UCLASS(ClassGroup=(Inventory),meta=(BlueprintSpawnableComponent))class MYPROJECT2_API UQuickAccessUIComponent:public UActorComponent
{GENERATED_BODY() public:UQuickAccessUIComponent();UFUNCTION(BlueprintCallable)void ShowRadialMenu();UFUNCTION(BlueprintCallable)void HideRadialMenu();UFUNCTION(BlueprintCallable)void SelectByIndex(int32 Index);UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly)bool bRadialMenuVisible=false;protected:UQuickAccessComponent*GetQuickAccess()const;};
