// Fill out your copyright notice in the Description page of Project Settings.


#include "IndianPokerPlayerState.h"
#include "Net/UnrealNetwork.h"

AIndianPokerPlayerState::AIndianPokerPlayerState()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	// PlayerState is automatically set to replicate by default, but ensuring it here is good practice.
	bReplicates = true;
	SetReplicatingMovement(false); // We don't need to replicate movement for PlayerState
	
	CurrentBattleState = EBattleState::Lobby;
}

void AIndianPokerPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AIndianPokerPlayerState, CurrentBattleState);
}

void AIndianPokerPlayerState::OnRep_BattleState()
{
}
