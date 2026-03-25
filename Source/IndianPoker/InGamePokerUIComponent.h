#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Blueprint/UserWidget.h"
#include "InGamePokerUIComponent.generated.h"

class AIndianPokerPlayerController;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class INDIANPOKER_API UInGamePokerUIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInGamePokerUIComponent();

protected:
	virtual void BeginPlay() override;

public:
	// ==========================================
	// UI 제어 함수
	// ==========================================
	UFUNCTION(BlueprintCallable, Category = "Poker UI")
	void ShowPokerUI();

	UFUNCTION(BlueprintCallable, Category = "Poker UI")
	void HidePokerUI();

	// ==========================================
	// 게임 로직 연동용 함수 (예시)
	// ==========================================
	// 판돈이 바뀌었을 때 UI를 업데이트하는 함수
	UFUNCTION(BlueprintImplementableEvent, Category = "Poker UI")
	void UpdatePotUI(int32 CurrentPotAmount);

	// 내 턴 여부에 따라 UI를 업데이트하는 함수
	UFUNCTION(BlueprintImplementableEvent, Category = "Poker")
	void UpdateTurnUI(bool bIsMyTurn);

	/** 이번 턴에 콜(Call)을 하기 위해 필요한 칩 개수 알림 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Poker")
	void NotifyRequiredCallAmount(int32 Amount);

	/** 라운드 결과 공개 (보안 연동) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Poker")
	void ShowRoundResult(uint8 MyCard, uint8 OpponentCard, int32 WinnerResult);

	// 내 칩 개수를 업데이트하는 함수
	UFUNCTION(BlueprintImplementableEvent, Category = "Poker UI")
	void UpdateMyChipsUI(int32 MyChips);

	// 상대방이 베팅/다이 했을 때 알림을 띄워주는 함수
	UFUNCTION(BlueprintImplementableEvent, Category = "Poker UI")
	void ShowOpponentAction(const FString& ActionMessage);

protected:
	// 띄워줄 UMG 위젯 클래스 (블루프린트에서 할당)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Poker UI")
	TSubclassOf<UUserWidget> PokerWidgetClass;

	// 실제 화면에 띄워진 위젯 인스턴스
	UPROPERTY()
	UUserWidget* PokerWidgetInstance;

	// 컨트롤러 캐싱용
	UPROPERTY()
	AIndianPokerPlayerController* OwnerPC;
};