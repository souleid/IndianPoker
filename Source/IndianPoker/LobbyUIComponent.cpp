#include "LobbyUIComponent.h"
#include "IndianPokerCharacter.h"
#include "IndianPokerPlayerController.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"


ULobbyUIComponent::ULobbyUIComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULobbyUIComponent::ShowLobbyUI()
{
}

void ULobbyUIComponent::HideLobbyUI()
{
}

void ULobbyUIComponent::StartRequestTimer(float Duration, AIndianPokerCharacter* InOpponent, bool bIsChallenger)
{
	// 혹시 이전 타이머가 살아있으면 먼저 정리
	CancelRequestTimer();

	OpponentCharacter = InOpponent;
	bIAmChallenger    = bIsChallenger;
	RequestTimerDuration = Duration;

	// 팝업이 뜨는 동시에 커서 표시 + 시야 회전 잠금
	if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
	{
		if (AIndianPokerPlayerController* IPC = Cast<AIndianPokerPlayerController>(PC))
		{
			IPC->Client_TransitionToPopupMode();
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RequestTimerHandle,
			this,
			&ULobbyUIComponent::OnRequestTimerExpired,
			Duration,
			false // 반복 없음
		);
		UE_LOG(LogTemp, Log, TEXT("[LobbyUIComponent] RequestTimer 시작 (%.1f초, bChallenger=%d)"), Duration, (int32)bIsChallenger);
	}
}


void ULobbyUIComponent::CancelRequestTimer()
{
	if (UWorld* World = GetWorld())
	{
		if (World->GetTimerManager().IsTimerActive(RequestTimerHandle))
		{
			World->GetTimerManager().ClearTimer(RequestTimerHandle);
			UE_LOG(LogTemp, Log, TEXT("[LobbyUIComponent] RequestTimer 취소됨."));
		}
	}
	OpponentCharacter = nullptr;
}

float ULobbyUIComponent::GetRequestTimeRemaining() const
{
	if (UWorld* World = GetWorld())
	{
		float Remaining = World->GetTimerManager().GetTimerRemaining(RequestTimerHandle);
		return FMath::Max(0.0f, Remaining);
	}
	return 0.0f;
}

void ULobbyUIComponent::OnRequestTimerExpired()
{
	UE_LOG(LogTemp, Warning, TEXT("[LobbyUIComponent] RequestTimer 만료! 상태 리셋 시작."));

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	AIndianPokerCharacter* OwnerChar = PC ? Cast<AIndianPokerCharacter>(PC->GetPawn()) : nullptr;

	UE_LOG(LogTemp, Warning, TEXT("[LobbyUIComponent] PC=%s / OwnerChar=%s / Opponent=%s"),
		PC        ? TEXT("OK") : TEXT("NULL"),
		OwnerChar ? TEXT("OK") : TEXT("NULL"),
		OpponentCharacter ? TEXT("OK") : TEXT("NULL"));

	if (OwnerChar && OpponentCharacter)
	{
		if (bIAmChallenger)
		{
			UE_LOG(LogTemp, Warning, TEXT("[LobbyUIComponent] → Server_CancelBattleRequest 호출"));
			OwnerChar->Server_CancelBattleRequest(OpponentCharacter);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[LobbyUIComponent] → Server_DeclineBattle 호출"));
			OwnerChar->Server_DeclineBattle(OpponentCharacter);
		}
	}

	// 클라이언트 UI 정리 (OnRep_BattleState가 닫겠지만 보험으로 한 번 더 호출)
	CloseAllLobbyPopups();
	OpponentCharacter = nullptr;
}

