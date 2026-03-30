// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "IndianPokerPlayerState.generated.h"

/**
 * Custom PlayerState to handle multiplayer properties like Nickname, Chips, and Battle State.
 */
UENUM(BlueprintType)
enum class EBattleState : uint8
{
	Lobby UMETA(DisplayName = "Lobby"),
	MatchRequested UMETA(DisplayName = "Match Requested"),
	InGame UMETA(DisplayName = "In Game")
};

UCLASS()
class INDIANPOKER_API AIndianPokerPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	AIndianPokerPlayerState();

	UPROPERTY(ReplicatedUsing = OnRep_BattleState, BlueprintReadOnly, Category = "Battle")
	EBattleState CurrentBattleState;

	/** 인디언 포커: 소지 칩 개수 */
	UPROPERTY(ReplicatedUsing = OnRep_Chips, BlueprintReadOnly, Category = "Poker")
	int32 Chips;

	/** 인디언 포커: 이번 라운드에 승자가 갈져갈 칩 개수 */
	UPROPERTY(ReplicatedUsing = OnRep_AccumulatedPot, BlueprintReadOnly, Category = "Poker")
	int32 AccumulatedPot;

	/** 인디언 포커: 현재 판에 베팅한 칩 개수 */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentBet, BlueprintReadOnly, Category = "Poker")
	int32 CurrentBet;

	UPROPERTY(ReplicatedUsing = OnRep_IsMyTurn, BlueprintReadOnly, Category = "Poker")
	bool bIsMyTurn;

	UFUNCTION()
	void OnRep_BattleState();

	UFUNCTION()
	void OnRep_Chips();

	UFUNCTION()
	void OnRep_CurrentBet();

	UFUNCTION()
	void OnRep_IsMyTurn();

	UFUNCTION()
	void OnRep_AccumulatedPot();
};
