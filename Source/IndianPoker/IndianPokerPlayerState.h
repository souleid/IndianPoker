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

	UFUNCTION()
	void OnRep_BattleState();
};
