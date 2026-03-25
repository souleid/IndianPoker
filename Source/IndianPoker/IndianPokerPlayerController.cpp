#include "IndianPokerPlayerController.h"
#include "IndianPokerGameMode.h"
#include "LobbyUIComponent.h"
#include "InGamePokerUIComponent.h"
#include "IndianPokerCharacter.h"
#include "GameFramework/Pawn.h"

AIndianPokerPlayerController::AIndianPokerPlayerController()
{
	// 로비 UI 전담 컴포넌트 부착
	LobbyUIComp = CreateDefaultSubobject<ULobbyUIComponent>(TEXT("LobbyUIComp"));
	
	// 인게임 UI 전담 컴포넌트 부착
	PokerUIComp = CreateDefaultSubobject<UInGamePokerUIComponent>(TEXT("PokerUIComp"));
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

void AIndianPokerPlayerController::Client_TransitionToPopupMode_Implementation()
{
	// 1. 마우스 커서 표시 + GameAndUI (WASD 이동은 살리되, UI 클릭 가능)
	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	// 2. 시야 회전(카메라 Look) 잠금: IgnoreLookInput = true
	//    캐릭터 이동(WASD)은 여전히 가능하지만 마우스로 카메라를 돌릴 수는 없습니다.
	SetIgnoreLookInput(true);
}

void AIndianPokerPlayerController::Client_TransitionToLobbyMode_Implementation()
{
	// 1. 마우스 커서 숨기기
	bShowMouseCursor = false;

	// 2. 조작 모드를 게임 플레이(캐릭터 이동) 모드로 변경
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	// 3. 팝업 모드에서 잠궜던 시야 회전 복구
	SetIgnoreLookInput(false);
}

void AIndianPokerPlayerController::Client_ShowInGamePokerUI_Implementation()
{
	// 1. 기존 로비 UI 숨기기
	if (LobbyUIComp)
	{
		LobbyUIComp->HideLobbyUI();
	}

	// 2. 새로운 인게임 UI 컴포넌트 활성화
	if (PokerUIComp)
	{
		PokerUIComp->ShowPokerUI(); // 위젯을 화면에 AddToViewport
	}

	// 3. 입력 모드를 UI 위주로 변경하고 마우스 커서 켜기
	SetShowMouseCursor(true);
	FInputModeUIOnly InputMode;
	SetInputMode(InputMode);
}

void AIndianPokerPlayerController::Server_SendMatchMessage_Implementation(const FString& Message)
{
	// 1. 발신자 캐릭터 가져오기
    AIndianPokerCharacter* MyChar = Cast<AIndianPokerCharacter>(GetPawn());
    if (!MyChar) return;

    // 2. 현재 방장(GameMode) 찾아오기
    AIndianPokerGameMode* GM = Cast<AIndianPokerGameMode>(GetWorld()->GetAuthGameMode());
    if (GM)
    {
        // 3. GameMode에게 "나 이 채팅 쳤으니까, 우리 테이블(Match) 사람들에게 싹 다 뿌려줘!" 라고 위임
        GM->RouteMatchChat(MyChar, Message);
    }
}

void AIndianPokerPlayerController::Client_ReceiveMatchMessage_Implementation(const FString& Sender, const FString& Message)
{
	// UI 컴포넌트에게 채팅창에 글 쓰라고 명령
	if (PokerUIComp)
	{
		PokerUIComp->ShowOpponentAction(FString::Printf(TEXT("%s: %s"), *Sender, *Message));
	}
}
