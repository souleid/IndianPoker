// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "IndianPokerGameMode.generated.h"

UENUM(BlueprintType)
enum class EPokerBetAction : uint8
{
	Race,
	Call,
	Die
};

USTRUCT(BlueprintType)
struct FPokerMatch
{
	GENERATED_BODY()

    UPROPERTY()
    FGuid MatchID;

	UPROPERTY()
	class AIndianPokerCharacter* Player1 = nullptr;

	UPROPERTY()
	class AIndianPokerCharacter* Player2 = nullptr;

	UPROPERTY()
	TArray<uint8> Deck;

	UPROPERTY()
	int32 RoundCount = 0;

	/** 이번 라운드 최고 베팅액 (콜 기준점) */
	UPROPERTY()
	int32 CurrentMaxBet = 1;

	UPROPERTY()
    int32 AccumulatedPot = 0;

	// 서버 전용 카드 값 (보안상 리플리케이션 안 함)
	uint8 P1Card = 0;
	uint8 P2Card = 0;

	bool IsInMatch(const class AIndianPokerCharacter* Player) const
    {
        return Player != nullptr && (Player == Player1 || Player == Player2);
    }
};

UCLASS(minimalapi)
class AIndianPokerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AIndianPokerGameMode();

	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;

public:
	/** 새로운 매치 생성 */
	void CreateMatch(class AIndianPokerCharacter* P1, class AIndianPokerCharacter* P2);

	/** 특정 플레이어가 속한 매치 데이터 가져오기 */
	FPokerMatch* GetMatch(const class AIndianPokerCharacter* Player);

	FPokerMatch* GetMatchByID(FGuid MatchID);

	/** 매치 종료 및 삭제 */
	void RemoveMatch(const class AIndianPokerCharacter* Player);

	/** 인디언 포커: 라운드 시작 진행 (서버 전용) */
	void StartPokerMatchRound(class AIndianPokerCharacter* Player);

	UFUNCTION()
    void StartPokerMatchRoundByID(FGuid MatchID);

	/** 승패 판정 (1 vs 10 룰 포함) */
	int32 CheckWinner(uint8 MyCard, uint8 OpponentCard);

	/** 승자 결정 및 칩 분배 처리 (서버 전용) */
	void DetermineWinnerAndDistributeChips(struct FPokerMatch* Match);

	/** 기권(Fold/Die) 처리 (서버 전용) */
	void ProcessFold(class AIndianPokerCharacter* Player);

	/** 모든 베팅 액션 통합 처리 (서버 전용) */
	void ProcessBetAction(class AIndianPokerCharacter* Player, EPokerBetAction Action, int32 Amount = 0);

	void RouteMatchChat(class AIndianPokerCharacter* Sender, const FString& Message);
private:
	/** 진행 중인 모든 매치 리스트 */
	UPROPERTY()
	TArray<FPokerMatch> ActiveMatches;
};



