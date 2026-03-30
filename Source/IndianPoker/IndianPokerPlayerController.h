#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "IndianPokerPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;

struct FInputActionValue;

/**
 * Custom PlayerController to handle UI Input Modes during Indian Poker matches.
 */
UCLASS()
class INDIANPOKER_API AIndianPokerPlayerController : public APlayerController
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* MovementContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* SystemContext;

	/** Chat Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ChatAction;

public:
	AIndianPokerPlayerController();

	// 배틀(포커 게임) 모드로 전환: 마우스 커서 표시 및 UI 조작 활성화
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "IndianPoker|Input")
	void Client_TransitionToBattleMode();

	// 팝업(요청/대기창) 모드로 전환: 마우스 표시, 시야 회전만 잠금
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "IndianPoker|Input")
	void Client_TransitionToPopupMode();

	// 로비(이동) 모드로 전환: 마우스 커서 숨김 및 게임 조작 활성화
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "IndianPoker|Input")
	void Client_TransitionToLobbyMode();

	UFUNCTION(Client, Reliable)
	void Client_ShowInGamePokerUI();

	/** 매치 채팅 전송 (서버로) */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "IndianPoker|Chat")
	void Server_SendMatchMessage(const FString& Message);

	/** 매치 채팅 수신 (클라이언트로) */
	UFUNCTION(Client, Reliable, Category = "IndianPoker|Chat")
	void Client_ReceiveMatchMessage(int32 SenderID, const FString& Sender, const FString& Message);

public:

	// -------------------------------------------------------
	// Secure Card Distribution (Targeted RPCs)
	// -------------------------------------------------------

	/** 상대방의 카드 정보만 수신 (내 카드는 숨김) */
	UFUNCTION(Client, Reliable, Category = "Poker")
	void Client_ReceiveOpponentCard(uint8 CardValue);

	/** 라운드 종료 시 내 카드와 상대 카드를 모두 공개 */
	UFUNCTION(Client, Reliable, Category = "Poker")
	void Client_ShowRoundResult(uint8 MyCard, uint8 OpponentCard, int32 WinnerResult);

	/** 최종 배틀 결과 수신 (승자 정보 포함) */
	UFUNCTION(Client, Reliable, Category = "Poker")
	void Client_ShowBattleResult(int32 WinnerResult);


	/** 내 턴이 되었음을 알림 (콜에 필요한 칩 개수 포함) */
	UFUNCTION(Client, Reliable, Category = "Poker")
	void Client_NotifyYourTurn(int32 AmountToCall);

	// -------------------------------------------------------
	// Indian Poker Core Logic
	// -------------------------------------------------------

	/** 베팅하기: 이전 베팅보다 더 많은 칩을 걸 때 호출 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Poker")
	void Server_NetRace(int32 Amount);

	/** 콜: 상대 베팅액과 동일하게 맞추고 승패 판정 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Poker")
	void Server_NetCall();

	/** 다이: 기권하여 이번 판을 포기 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Poker")
	void Server_NetDie();

protected:
	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;

	void OnChatActionPressed(const FInputActionValue& Value);

public:
	/** 로비 전용 UI를 관장하는 액터 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Components")
	class ULobbyUIComponent* LobbyUIComp;

	/** 인게임 인디언포커 전용 UI를 관장하는 액터 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Components")
	class UInGamePokerUIComponent* PokerUIComp;
};
