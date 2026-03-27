#include "IndianPokerPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "IndianPokerGameMode.h"
#include "LobbyUIComponent.h"
#include "InGamePokerUIComponent.h"
#include "IndianPokerCharacter.h"
#include "InputActionValue.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

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

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(MovementContext, 0);
	}

	// 메인 메뉴(StartMenu) 등에서 넘어왔을 때 UI 전용 모드에 갇히는 것을 방지하고
	// 게임 플레이(캐릭터 조종) 상태로 확실하게 초기화해 줍니다.
	if (IsLocalPlayerController())
	{
		Client_TransitionToLobbyMode();
	}
}

void AIndianPokerPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Enhanced Input Component로 형변환해서 바인딩 시작!
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// "ChatAction이 눌리면(Triggered), 내(this) OnChatActionPressed 함수를 실행해라"
		EnhancedInputComponent->BindAction(ChatAction, ETriggerEvent::Started, this, &AIndianPokerPlayerController::OnChatActionPressed);
	}
}

void AIndianPokerPlayerController::OnChatActionPressed(const FInputActionValue& Value)
{
	if (PokerUIComp)
	{
		// 블루프린트에서 구현할 수 있도록 이벤트 호출 (이미 헤더에 선언되어 있는 함수 활용 가능)
		// 혹은 직접 위젯을 찾아 Focus 노드를 실행하도록 짜도 됩니다.
		UE_LOG(LogTemp, Warning, TEXT("채팅 키(Enter) 눌림!"));
		PokerUIComp->K2_FocusChatInput();
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

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->RemoveMappingContext(SystemContext);
		Subsystem->AddMappingContext(MovementContext, 0);
	}

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

	// 2. [심플 핵심] 이동 권한 박탈!
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// 3인칭 템플릿의 이동/점프가 담긴 IMC는 제거
		Subsystem->RemoveMappingContext(MovementContext);

		// 채팅(Enter) 등이 담긴 시스템 IMC만 추가 (혹은 유지)
		Subsystem->AddMappingContext(SystemContext, 0);
	}

	// 3. 마우스 커서 켜기
	SetShowMouseCursor(true);
	FInputModeGameAndUI InputMode;
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

		FInputModeGameAndUI InputMode;
		SetInputMode(InputMode);
}

void AIndianPokerPlayerController::Client_ReceiveMatchMessage_Implementation(int32 SenderID, const FString& Sender, const FString& Message)
{
	bool isMine = false;
	// UI 컴포넌트에게 채팅창에 글 쓰라고 명령
	APlayerState* MyPS = GetPlayerState<APlayerState>();
	if (MyPS)
	{
		// 서버가 보내준 SenderID와 내 로컬 ID가 같으면 내가 보낸 것!
		isMine = (SenderID == MyPS->GetPlayerId());
	}

	if (PokerUIComp)
	{
		PokerUIComp->ShowOpponentAction(Sender, Message, isMine);
	}
}
