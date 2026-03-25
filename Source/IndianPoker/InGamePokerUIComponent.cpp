#include "InGamePokerUIComponent.h"
#include "IndianPokerPlayerController.h"
#include "Blueprint/UserWidget.h"

UInGamePokerUIComponent::UInGamePokerUIComponent()
{
	// UI 관리용이므로 틱은 필요 없습니다.
	PrimaryComponentTick.bCanEverTick = false;
}

void UInGamePokerUIComponent::BeginPlay()
{
	Super::BeginPlay();

	// 오너 컨트롤러 캐싱
	OwnerPC = Cast<AIndianPokerPlayerController>(GetOwner());
}

void UInGamePokerUIComponent::ShowPokerUI()
{
	if (!PokerWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("InGamePokerUIComponent: PokerWidgetClass가 설정되지 않았습니다."));
		return;
	}

	if (OwnerPC && OwnerPC->IsLocalController())
	{
		// 위젯이 아직 생성되지 않았다면 생성
		if (!PokerWidgetInstance)
		{
			PokerWidgetInstance = CreateWidget<UUserWidget>(OwnerPC, PokerWidgetClass);
		}

		// 화면에 띄우기
		if (PokerWidgetInstance && !PokerWidgetInstance->IsInViewport())
		{
			PokerWidgetInstance->AddToViewport();
		}
	}
}

void UInGamePokerUIComponent::HidePokerUI()
{
	if (PokerWidgetInstance && PokerWidgetInstance->IsInViewport())
	{
		PokerWidgetInstance->RemoveFromParent();
	}
}