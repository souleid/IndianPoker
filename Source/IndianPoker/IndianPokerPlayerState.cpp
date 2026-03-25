// Fill out your copyright notice in the Description page of Project Settings.


#include "IndianPokerPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "IndianPokerPlayerController.h"
#include "LobbyUIComponent.h"
#include "GameFramework/Pawn.h"

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
	// 상태가 다시 Lobby로 돌아왔을 때 = 취소/거절/타임아웃 모든 경우 처리
	if (CurrentBattleState == EBattleState::Lobby)
	{
		APawn* Pawn = GetPawn();
		if (Pawn)
		{
			AIndianPokerPlayerController* PC = Cast<AIndianPokerPlayerController>(Pawn->GetController());
			if (PC && PC->LobbyUIComp)
			{
				UE_LOG(LogTemp, Log, TEXT("[OnRep_BattleState] 상태가 Lobby로 복귀. 타이머 취소 + 팝업 닫는 중..."));
				// 타이머가 아직 살아있다면 먼저 정리 (취소 버튼 누른 쪽의 타이머 중복 발화 방지)
				PC->LobbyUIComp->CancelRequestTimer();
				PC->LobbyUIComp->CloseAllLobbyPopups();
				// 팝업 모드에서 잠궜던 커서 / 시야 회전 복구
				PC->Client_TransitionToLobbyMode();
			}
		}
	}
}
