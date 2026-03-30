#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LobbyUIComponent.generated.h"

class AIndianPokerCharacter;

UCLASS( ClassGroup=(UI), Blueprintable, meta=(BlueprintSpawnableComponent) )
class INDIANPOKER_API ULobbyUIComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULobbyUIComponent();

	/** 로비 기본 HUD를 화면에 띄웁니다. */
	UFUNCTION(BlueprintCallable, Category = "UI|Lobby")
	void ShowLobbyUI();

	/** 로비 기본 HUD를 화면에서 숨깁니다. */
	UFUNCTION(BlueprintCallable, Category = "UI|Lobby")
	void HideLobbyUI();

	// -------------------------------------------------------
	// UI 이벤트 (블루프린트에서 구현)
	// -------------------------------------------------------

	/** 상대방에게 배틀 요청이 왔을 때 수락/거절 팝업을 띄우는 함수 */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Lobby")
	void ShowBattleRequestPopup(const FString& ChallengerName, AIndianPokerCharacter* Challenger);

	/** 내가 배틀을 걸어두고, 상대방의 대답을 기다릴 때 뜨는 팝업을 띄우는 함수 */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Lobby")
	void ShowWaitingPopup(AIndianPokerCharacter* Target);

	/** 팝업창을 강제로 닫아야 할 때 호출 (취소, 타임아웃 등) */
	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Lobby")
	void CloseAllLobbyPopups();

	// -------------------------------------------------------
	// RequestTimer - 배틀 요청 타임아웃 관리 (C++)
	// -------------------------------------------------------

	/**
	 * 배틀 요청 타이머를 시작합니다.
	 * @param Duration       타임아웃 시간 (초). 기본값 5초.
	 * @param InOpponent     요청 상대 캐릭터 (타임아웃 시 RPC 호출에 사용)
	 * @param bIsChallenger  내가 요청을 보낸 쪽(true)인지, 받은 쪽(false)인지
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|Lobby|RequestTimer")
	void StartRequestTimer(float Duration, AIndianPokerCharacter* InOpponent, bool bIsChallenger);

	/** 배틀 요청 타이머를 강제로 중단합니다. (수락/거절 시 반드시 호출) */
	UFUNCTION(BlueprintCallable, Category = "UI|Lobby|RequestTimer")
	void CancelRequestTimer();

	/**
	 * 타이머에 남은 시간(초)을 반환합니다.
	 * 위젯의 ProgressBar 갱신에 사용합니다.
	 * 타이머가 비활성 상태면 0.0을 반환합니다.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "UI|Lobby|RequestTimer")
	float GetRequestTimeRemaining() const;

	/** 요청 타이머의 총 지속 시간 (ProgressBar 비율 계산용) */
	UPROPERTY(BlueprintReadOnly, Category = "UI|Lobby|RequestTimer")
	float RequestTimerDuration = 5.0f;

private:
	/** 타이머 핸들 */
	FTimerHandle RequestTimerHandle;

	/** 타임아웃 시 RPC를 날릴 상대 캐릭터 */
	UPROPERTY()
	AIndianPokerCharacter* OpponentCharacter = nullptr;

	/** true면 내가 요청 보낸 쪽 → CancelBattleRequest / false면 받은 쪽 → DeclineBattle */
	bool bIAmChallenger = false;

	/** 타이머 만료 시 자동 호출되는 콜백 */
	void OnRequestTimerExpired();
};
