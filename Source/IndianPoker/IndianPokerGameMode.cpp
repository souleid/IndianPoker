// Copyright Epic Games, Inc. All Rights Reserved.

#include "IndianPokerGameMode.h"
#include "IndianPokerCharacter.h"
#include "IndianPokerPlayerState.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AIndianPokerGameMode::AIndianPokerGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	PlayerStateClass = AIndianPokerPlayerState::StaticClass();
}

FString AIndianPokerGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	FString InitialName = UGameplayStatics::ParseOption(Options, TEXT("Name"));
	
	UE_LOG(LogTemp, Warning, TEXT("====================================="));
	UE_LOG(LogTemp, Warning, TEXT("[IndianPokerGameMode] InitNewPlayer called!"));
	UE_LOG(LogTemp, Warning, TEXT("[IndianPokerGameMode] Raw Options String: %s"), *Options);
	UE_LOG(LogTemp, Warning, TEXT("[IndianPokerGameMode] Parsed Name: %s"), *InitialName);
	UE_LOG(LogTemp, Warning, TEXT("====================================="));

	FString ErrorMessage = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	// 강제로 PlayerState의 닉네임을 한 번 더 덮어씌워줍니다. (보험용)
	if (NewPlayerController && NewPlayerController->PlayerState && !InitialName.IsEmpty())
	{
		NewPlayerController->PlayerState->SetPlayerName(InitialName);
		UE_LOG(LogTemp, Warning, TEXT("[IndianPokerGameMode] Successfully forced PlayerName to: %s"), *InitialName);
	}

	return ErrorMessage;
}
