#include "IndianPokerPlayerController.h"

AIndianPokerPlayerController::AIndianPokerPlayerController()
{
	// Initialization if needed
}

void AIndianPokerPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 메인 메뉴(StartMenu) 등에서 넘어왔을 때 UI 전용 모드에 갇히는 것을 방지하고
	// 게임 플레이(캐릭터 조종) 상태로 확실하게 초기화해 줍니다.
	if (IsLocalPlayerController())
	{
		Client_TransitionToLobbyMode();
	}
}

void AIndianPokerPlayerController::Client_TransitionToBattleMode_Implementation()
{
	// 1. 마우스 커서 보이기
	bShowMouseCursor = true;

	// 2. 조작 모드를 UI를 클릭할 수 있는 Game And UI 모드로 변경
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	
	SetInputMode(InputMode);
}

void AIndianPokerPlayerController::Client_TransitionToLobbyMode_Implementation()
{
	// 1. 마우스 커서 숨기기
	bShowMouseCursor = false;

	// 2. 조작 모드를 게임 플레이(캐릭터 이동) 모드로 변경
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}
