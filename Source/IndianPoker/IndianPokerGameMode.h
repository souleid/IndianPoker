// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "IndianPokerGameMode.generated.h"

UCLASS(minimalapi)
class AIndianPokerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AIndianPokerGameMode();

	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal = TEXT("")) override;
};



