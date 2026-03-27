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
