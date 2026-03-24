#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "IndianPokerPlayerController.generated.h"

/**
 * Custom PlayerController to handle UI Input Modes during Indian Poker matches.
 */
UCLASS()
class INDIANPOKER_API AIndianPokerPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AIndianPokerPlayerController();

	// 배틀(포커 게임) 모드로 전환: 마우스 커서 표시 및 UI 조작 활성화
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "IndianPoker|Input")
	void Client_TransitionToBattleMode();

	// 로비(이동) 모드로 전환: 마우스 커서 숨김 및 게임 조작 활성화
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "IndianPoker|Input")
	void Client_TransitionToLobbyMode();

protected:
	virtual void BeginPlay() override;
};
