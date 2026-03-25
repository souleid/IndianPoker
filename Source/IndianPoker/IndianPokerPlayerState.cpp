#include "IndianPokerPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "IndianPokerPlayerController.h"
#include "LobbyUIComponent.h"
#include "InGamePokerUIComponent.h"
#include "GameFramework/Pawn.h"

AIndianPokerPlayerState::AIndianPokerPlayerState()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	// PlayerState is automatically set to replicate by default, but ensuring it here is good practice.
	bReplicates = true;
	SetReplicatingMovement(false); // We don't need to replicate movement for PlayerState
	
	CurrentBattleState = EBattleState::Lobby;

	// 인디언 포커 초기값
	Chips = 20;
	CurrentBet = 0;
	bIsMyTurn = false;
}

void AIndianPokerPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AIndianPokerPlayerState, CurrentBattleState);
	DOREPLIFETIME(AIndianPokerPlayerState, Chips);
	DOREPLIFETIME(AIndianPokerPlayerState, CurrentBet);
	DOREPLIFETIME(AIndianPokerPlayerState, bIsMyTurn);
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

void AIndianPokerPlayerState::OnRep_Chips()
{
	// 로컬 플레이어 컨트롤러를 찾아서 UI 업데이트
	if (APlayerController* LocalPC = GetWorld()->GetFirstPlayerController())
	{
		if (AIndianPokerPlayerController* PC = Cast<AIndianPokerPlayerController>(LocalPC))
		{
			if (PC->PokerUIComp)
			{
				// 이 PlayerState가 내 것인지 상대방 것인지 판단
				if (PC->GetPlayerState<APlayerState>() == this)
				{
					PC->PokerUIComp->UpdateMyChipsUI(Chips);
				}
				else
				{
					//PC->PokerUIComp->UpdateOpponentChipsUI(Chips);
				}
			}
		}
	}
}

void AIndianPokerPlayerState::OnRep_CurrentBet()
{
	// 전적 판돈(Pot)은 보통 GameState에서 관리하는 게 좋으나, 
	// 여기서는 양쪽 PlayerState의 CurrentBet 합산으로 UI에 표시할 수 있음
	if (APlayerController* LocalPC = GetWorld()->GetFirstPlayerController())
	{
		if (AIndianPokerPlayerController* PC = Cast<AIndianPokerPlayerController>(LocalPC))
		{
			if (PC->PokerUIComp)
			{
				// 일단 개별 베팅액 업데이트 이벤트를 호출 (필요 시 UpdatePotUI 사용)
			}
		}
	}
}

void AIndianPokerPlayerState::OnRep_IsMyTurn()
{
	if (APlayerController* LocalPC = GetWorld()->GetFirstPlayerController())
	{
		if (AIndianPokerPlayerController* PC = Cast<AIndianPokerPlayerController>(LocalPC))
		{
			if (PC->PokerUIComp)
			{
				// 이 PlayerState가 내 것인데 bIsMyTurn이 true면 내 턴
				bool bIsActuallyMyTurn = (PC->GetPlayerState<APlayerState>() == this) ? bIsMyTurn : !bIsMyTurn;
				PC->PokerUIComp->UpdateTurnUI(bIsActuallyMyTurn);
			}
		}
	}
}
