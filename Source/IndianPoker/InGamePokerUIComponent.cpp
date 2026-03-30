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
    // [핵심 수정] 재활용하지 않고 무조건 새로 만듭니다.
    // 기존에 잔상이 남았을 경우를 대비해 확실히 제거
    if (PokerWidgetInstance)
    {
      PokerWidgetInstance->RemoveFromParent();
      PokerWidgetInstance = nullptr;
    }

    // 새로운 도화지 생성
    PokerWidgetInstance = CreateWidget<UUserWidget>(OwnerPC, PokerWidgetClass);

    if (PokerWidgetInstance)
    {
      PokerWidgetInstance->AddToViewport();
      UE_LOG(LogTemp, Log, TEXT("InGamePokerUIComponent: 새 위젯 생성 완료."));
    }
  }
}

void UInGamePokerUIComponent::HidePokerUI()
{
  // [핵심 수정] 단순히 화면에서 지우는 게 아니라 포인터를 밀어줍니다.
  if (PokerWidgetInstance)
  {
    PokerWidgetInstance->RemoveFromParent();
    PokerWidgetInstance = nullptr; // 다음 배틀 때 새로 생성되도록 보장
    UE_LOG(LogTemp, Log, TEXT("InGamePokerUIComponent: 위젯 제거 및 메모리 정리 완료."));
  }
}

void UInGamePokerUIComponent::UpdateSpringLogic(float DeltaTime, float Stiffness, float Damping, FSpringAnimationState& SpringState)
{
	// 1. 가속도 계산: F = -k(x - target) - cv
	float Displacement = SpringState.CurrentValue - SpringState.TargetValue;
	float SpringForce = -Stiffness * Displacement;
	float DampingForce = Damping * SpringState.Velocity;
	float Acceleration = SpringForce - DampingForce;

	// 2. 속도 및 위치 업데이트 (Euler Integration)
	SpringState.Velocity += Acceleration * DeltaTime;
	SpringState.CurrentValue += SpringState.Velocity * DeltaTime;
}