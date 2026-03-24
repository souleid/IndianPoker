// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "IndianPokerPlayerState.generated.h"

/**
 * Custom PlayerState to handle multiplayer properties like Nickname, Chips, and Battle State.
 */
UCLASS()
class INDIANPOKER_API AIndianPokerPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	AIndianPokerPlayerState();

};
