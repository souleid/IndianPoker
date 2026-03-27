// Copyright Epic Games, Inc. All Rights Reserved.

#include "IndianPokerGameMode.h"
#include "IndianPokerCharacter.h"
#include "IndianPokerPlayerState.h"
#include "IndianPokerPlayerController.h"
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
	PlayerControllerClass = AIndianPokerPlayerController::StaticClass();
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

void AIndianPokerGameMode::CreateMatch(AIndianPokerCharacter* P1, AIndianPokerCharacter* P2)
{
	if (!P1 || !P2) return;

	// 기존에 매치가 있는지 확인 (중복 생성 방지)
	if (GetMatch(P1) || GetMatch(P2))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] 캐릭 중 하나가 이미 매치 중입니다."));
		return;
	}

	FPokerMatch NewMatch;
	NewMatch.MatchID = FGuid::NewGuid(); // 고유 식별자 발급
	NewMatch.Player1 = P1;
	NewMatch.Player2 = P2;
	NewMatch.AccumulatedPot = 0;
	
	// 덱 초기화 (20장)
	for (int32 i = 0; i < 2; ++i)
	{
		for (uint8 v = 1; v <= 10; ++v) NewMatch.Deck.Add(v);
	}
	// 셔플
	for (int32 i = NewMatch.Deck.Num() - 1; i > 0; --i)
	{
		int32 j = FMath::RandRange(0, i);
		NewMatch.Deck.Swap(i, j);
	}

	ActiveMatches.Add(NewMatch);
	UE_LOG(LogTemp, Log, TEXT("[GameMode] 새 매치 생성: %s vs %s"), *P1->NickName, *P2->NickName);
}

FPokerMatch* AIndianPokerGameMode::GetMatch(const AIndianPokerCharacter* Player)
{
	for (int32 i = 0; i < ActiveMatches.Num(); ++i)
	{
		if (ActiveMatches[i].IsInMatch(Player))
		{
			return &ActiveMatches[i];
		}
	}
	return nullptr;
}

FPokerMatch* AIndianPokerGameMode::GetMatchByID(FGuid MatchID)
{
	for (int32 i = 0; i < ActiveMatches.Num(); ++i)
	{
		if (ActiveMatches[i].MatchID == MatchID) return &ActiveMatches[i];
	}
	return nullptr;
}

void AIndianPokerGameMode::RemoveMatch(const AIndianPokerCharacter* Player)
{
	for (int32 i = 0; i < ActiveMatches.Num(); ++i)
	{
		if (ActiveMatches[i].IsInMatch(Player))
		{
			ActiveMatches.RemoveAt(i);
			UE_LOG(LogTemp, Log, TEXT("[GameMode] 매치 삭제 완료."));
			break;
		}
	}
}

void AIndianPokerGameMode::StartPokerMatchRound(AIndianPokerCharacter* Player)
{
	FPokerMatch* Match = GetMatch(Player);
	if (Match) StartPokerMatchRoundByID(Match->MatchID);
}

void AIndianPokerGameMode::StartPokerMatchRoundByID(FGuid MatchID)
{
	FPokerMatch* Match = GetMatchByID(MatchID);
	if (!Match || !Match->Player1 || !Match->Player2) return;

	AIndianPokerCharacter* P1 = Match->Player1;
	AIndianPokerCharacter* P2 = Match->Player2;

	AIndianPokerPlayerState* PS1 = Match->Player1->GetPlayerState<AIndianPokerPlayerState>();
	AIndianPokerPlayerState* PS2 = Match->Player2->GetPlayerState<AIndianPokerPlayerState>();

	if (PS1 && PS2)
	{
		// 1. 덱 소진 체크
		if (Match->Deck.Num() < 2)
		{
			UE_LOG(LogTemp, Warning, TEXT("[GameMode] 매치(%s vs %s) 덱 소진!"), *P1->NickName, *P2->NickName);
			// 덱 소진 시 승리 판정 로직 추가 가능
			return;
		}

		PS1->Chips -= 1;
		PS2->Chips -= 1;
		Match->AccumulatedPot += 2; // 증발 방지!

		PS1->CurrentBet = 0; // 이 라운드에서 순수하게 올린 베팅액
		PS2->CurrentBet = 0;
		Match->CurrentMaxBet = 0;

		Match->P1Card = Match->Deck.Pop();
		Match->P2Card = Match->Deck.Pop();

		// 4. 보안용 타겟 라이브 RPC 전송: 상대방 카드 정보만 전송
		P1->Client_ReceiveOpponentCard(Match->P2Card);
		P2->Client_ReceiveOpponentCard(Match->P1Card);

		// 5. 턴 결정 (라운드 횟수에 따라 교대 베팅 선점)
		Match->RoundCount++;
		bool bP1Starts = (Match->RoundCount % 2 != 0);
		PS1->bIsMyTurn = bP1Starts;
		PS2->bIsMyTurn = !bP1Starts;

		// 6. 시작 플레이어에게 알림
		if (bP1Starts) P1->Client_NotifyYourTurn(0);
		else P2->Client_NotifyYourTurn(0);

		UE_LOG(LogTemp, Log, TEXT("[GameMode] 매치 라운드 %d 시작! (P1:%d, P2:%d)"), 
			Match->RoundCount, Match->P1Card, Match->P2Card);
	}
}

int32 AIndianPokerGameMode::CheckWinner(uint8 MyCard, uint8 OpponentCard)
{
	if (MyCard == OpponentCard) return 0; // 무승부

	// 1 vs 10 특수 룰: 1은 10을 이김
	if (MyCard == 1 && OpponentCard == 10) return 1; // 내 승리
	if (MyCard == 10 && OpponentCard == 1) return 2; // 상대 승리

	// 일반 룰: 높은 숫자 승리
	return (MyCard > OpponentCard) ? 1 : 2;
}

void AIndianPokerGameMode::DetermineWinnerAndDistributeChips(FPokerMatch* Match)
{
	if (!Match || !Match->Player1 || !Match->Player2) return;

	AIndianPokerPlayerState* PS1 = Match->Player1->GetPlayerState<AIndianPokerPlayerState>();
	AIndianPokerPlayerState* PS2 = Match->Player2->GetPlayerState<AIndianPokerPlayerState>();

	if (PS1 && PS2)
	{
		// 1. 승자 판정
		int32 Winner = CheckWinner(Match->P1Card, Match->P2Card);
		
		// 2. 칩 이동
		int32 TotalPot = PS1->CurrentBet + PS2->CurrentBet;
		if (Winner == 1) PS1->Chips += TotalPot;
		else if (Winner == 2) PS2->Chips += TotalPot;
		else { /* 무승부: 판돈 이월 (다음 라운드까지 대기) */ }

		// 3. 결과 공개 (RPC)
		Match->Player1->Client_ShowRoundResult(Match->P1Card, Match->P2Card, Winner);
		Match->Player2->Client_ShowRoundResult(Match->P2Card, Match->P1Card, (Winner == 0 ? 0 : (Winner == 1 ? 2 : 1)));

		// 4. 베팅액 초기화
		PS1->CurrentBet = 0;
		PS2->CurrentBet = 0;

		UE_LOG(LogTemp, Log, TEXT("[GameMode] 라운드 종료 처리 완료: 승자 %d, 판돈 %d"), Winner, TotalPot);
	}
}

void AIndianPokerGameMode::ProcessFold(AIndianPokerCharacter* FoldingPlayer)
{
	FPokerMatch* Match = GetMatch(FoldingPlayer);
    if (!Match) return;

    AIndianPokerCharacter* P1 = Match->Player1;
    AIndianPokerCharacter* P2 = Match->Player2;
    AIndianPokerCharacter* WinnerPlayer = (FoldingPlayer == P1) ? P2 : P1;

    AIndianPokerPlayerState* FoldingPS = FoldingPlayer->GetPlayerState<AIndianPokerPlayerState>();
    AIndianPokerPlayerState* WinnerPS = WinnerPlayer->GetPlayerState<AIndianPokerPlayerState>();

    // 1. 다이 선언: 승자에게 중앙 금고(Pot) 몰아주기
    WinnerPS->Chips += Match->AccumulatedPot;
    Match->AccumulatedPot = 0;

    // 2. 10-Fold 페널티 체크
    uint8 FoldingCard = (FoldingPlayer == P1) ? Match->P1Card : Match->P2Card;
    if (FoldingCard == 10)
    {
        int32 Penalty = FMath::Min(FoldingPS->Chips, 10);
        FoldingPS->Chips -= Penalty;
        WinnerPS->Chips += Penalty;
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] 10을 들고 다이! 칩 %d개 강제 압수!"), Penalty);
    }

    FoldingPS->CurrentBet = 0;
    WinnerPS->CurrentBet = 0;
}

void AIndianPokerGameMode::ProcessBetAction(AIndianPokerCharacter* Player, EPokerBetAction Action, int32 Amount)
{
	FPokerMatch* Match = GetMatch(Player);
    if (!Match) return;

    AIndianPokerPlayerState* MyPS = Player->GetPlayerState<AIndianPokerPlayerState>();
    AIndianPokerCharacter* Opponent = (Player == Match->Player1) ? Match->Player2 : Match->Player1;
    AIndianPokerPlayerState* OppPS = Opponent->GetPlayerState<AIndianPokerPlayerState>();

    if (!MyPS || !OppPS || !MyPS->bIsMyTurn) return;

    bool bRoundEnded = false;

    switch (Action)
    {
    case EPokerBetAction::Race:
        {
            // 내가 콜 해야 할 금액 + 추가 레이즈 금액
      			if (Amount <= 0) return;

            int32 ToCall = Match->CurrentMaxBet - MyPS->CurrentBet;
            int32 TotalToAdd = ToCall + Amount;

            if (MyPS->Chips >= TotalToAdd)
            {
                MyPS->Chips -= TotalToAdd;
                MyPS->CurrentBet += TotalToAdd;
                Match->AccumulatedPot += TotalToAdd; // 판돈은 바로바로 금고에!
                Match->CurrentMaxBet = MyPS->CurrentBet;

                // 턴 교체
                MyPS->bIsMyTurn = false;
                OppPS->bIsMyTurn = true;
                // Opponent->Client_NotifyYourTurn(Match->CurrentMaxBet - OppPS->CurrentBet);
            }
        }
        break;

    case EPokerBetAction::Call:
        {
            int32 ToAdd = Match->CurrentMaxBet - MyPS->CurrentBet;
            if (MyPS->Chips >= ToAdd)
            {
                MyPS->Chips -= ToAdd;
                MyPS->CurrentBet += ToAdd;
                Match->AccumulatedPot += ToAdd;

                DetermineWinnerAndDistributeChips(Match);
                bRoundEnded = true;
            }
        }
        break;

    case EPokerBetAction::Die:
        {
            ProcessFold(Player);
            bRoundEnded = true;
        }
        break;
    }

    if (bRoundEnded)
    {
        // 안전한 Delegate를 이용한 3초 뒤 라운드 시작
        FTimerHandle TimerHandle;
        FTimerDelegate TimerDel;
        TimerDel.BindUObject(this, &AIndianPokerGameMode::StartPokerMatchRoundByID, Match->MatchID);
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, 3.0f, false);
    }
}

void AIndianPokerGameMode::RouteMatchChat(AIndianPokerCharacter* Sender, const FString& Message)
{
   if (!Sender) return;

    FPokerMatch* Match = GetMatch(Sender);
		
    if (Match)
    {
			  APlayerState* PS = Sender->GetPlayerState();
				if (!PS) return;
        int32 SenderID = PS->GetPlayerId();
        // 1. 매치에 속한 양쪽 플레이어의 컨트롤러를 찾아 클라이언트 함수(RPC) 호출
        if (Match->Player1) 
        {
            if (AIndianPokerPlayerController* PC1 = Cast<AIndianPokerPlayerController>(Match->Player1->GetController()))
            {
                PC1->Client_ReceiveMatchMessage(SenderID, Sender->NickName, Message);
            }
        }
        
        if (Match->Player2) 
        {
            if (AIndianPokerPlayerController* PC2 = Cast<AIndianPokerPlayerController>(Match->Player2->GetController()))
            {
                PC2->Client_ReceiveMatchMessage(SenderID, Sender->NickName, Message);
            }
        }
        
        UE_LOG(LogTemp, Log, TEXT("[Private Chat / MatchID: %s] %s: %s"), *Match->MatchID.ToString(), *Sender->NickName, *Message);
    }
}