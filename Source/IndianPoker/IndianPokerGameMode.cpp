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

void AIndianPokerGameMode::NotifyPlayerReady(AIndianPokerCharacter* Player) {
	FPokerMatch* Match = GetMatch(Player);
	if (Match) {
		Match->ReadyPlayersCount++;
		if (Match->ReadyPlayersCount >= 2) {
			UE_LOG(LogTemp, Log, TEXT("[GameMode] 게임 레디 플레이 : %s") , *(Match->MatchID.ToString()));

			StartPokerMatchRoundByID(Match->MatchID);
			Match->ReadyPlayersCount = 0; // 초기화
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

	AIndianPokerPlayerController* PC1 = Cast<AIndianPokerPlayerController>(Match->Player1->GetController());
	AIndianPokerPlayerController* PC2 = Cast<AIndianPokerPlayerController>(Match->Player2->GetController());

	if (PS1 && PS2)
	{
		// 1. 배틀 종료 여부 체크 (파산 또는 덱 소진)
		if (CheckBattleEnd(Match))
		{
			return;
		}

		//기본 배팅액(입장료) 없음!

		PS1->Chips -= 1;
		PS2->Chips -= 1;
		Match->AccumulatedPot += 2; // 증발 방지!

		PS1->AccumulatedPot = Match->AccumulatedPot;
		PS2->AccumulatedPot = Match->AccumulatedPot;

		PS1->CurrentBet = 0; // 이 라운드에서 순수하게 올린 베팅액
		PS2->CurrentBet = 0;
		Match->CurrentMaxBet = 0;

		Match->P1Card = Match->Deck.Pop();
		Match->P2Card = Match->Deck.Pop();

		// 4. 보안용 타겟 라이브 RPC 전송: 상대방 카드 정보만 전송
		PC1->Client_ReceiveOpponentCard(Match->P2Card);
		PC2->Client_ReceiveOpponentCard(Match->P1Card);

		// 5. 턴 결정 (라운드 횟수에 따라 교대 베팅 선점)
		Match->RoundCount++;
		bool bP1Starts = (Match->RoundCount % 2 != 0);
		PS1->bIsMyTurn = bP1Starts;
		PS2->bIsMyTurn = !bP1Starts;

		// 6. 시작 플레이어에게 알림
		if (bP1Starts) PC1->Client_NotifyYourTurn(0);
		else PC2->Client_NotifyYourTurn(0);

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

	AIndianPokerPlayerController* PC1 = Cast<AIndianPokerPlayerController>(Match->Player1->GetController());
	AIndianPokerPlayerController* PC2 = Cast<AIndianPokerPlayerController>(Match->Player2->GetController());

	if (PS1 && PS2)
	{
		// 1. 승자 판정
		int32 Winner = CheckWinner(Match->P1Card, Match->P2Card);
		
		// 2. 칩 이동
		int32 TotalPot = Match->AccumulatedPot;
		if (Winner == 1) {
			PS1->Chips += TotalPot;
			PS1->CurrentBet = 0;
			Match->AccumulatedPot = 0;
		}
		else if (Winner == 2) {
			PS2->Chips += TotalPot;
			PS2->CurrentBet = 0;
			Match->AccumulatedPot = 0;
		}
		else { /* 무승부: 판돈 이월 (다음 라운드까지 대기) */ }

		// 3. 결과 공개 (RPC)
		PC1->Client_ShowRoundResult(Match->P1Card, Match->P2Card, Winner);
		PC2->Client_ShowRoundResult(Match->P2Card, Match->P1Card, (Winner == 0 ? 0 : (Winner == 1 ? 2 : 1)));

		// 4. 베팅액 초기화
		Match->CurrentMaxBet = 0;
		PS1->CurrentBet = 0;
		PS2->CurrentBet = 0;

		UE_LOG(LogTemp, Log, TEXT("[GameMode] 라운드 종료 처리 완료: 승자 %d, 판돈 %d"), Winner, TotalPot);

		// 안내 메세지 추가
		if (Winner == 0) {
			BroadcastMatchMessage(Match, TEXT("이번 라운드는 무승부"));
		} else {
			AIndianPokerCharacter* WinChar = (Winner == 1) ? Match->Player1 : Match->Player2;
			BroadcastMatchMessage(Match, FString::Printf(TEXT("%s님 라운드 승! (%d 수령)"), *WinChar->NickName, TotalPot));
		}
	}


	if (!CheckBattleEnd(Match)) {
		if (GetMatchByID(Match->MatchID))
		{
			FTimerHandle TimerHandle;
			FTimerDelegate TimerDel;
			TimerDel.BindUObject(this, &AIndianPokerGameMode::StartPokerMatchRoundByID, Match->MatchID);
			GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, 3.0f, false);
		}
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

		// 3. 결과 공개 (RPC 호출 추가)
		AIndianPokerPlayerController* PC1 = Cast<AIndianPokerPlayerController>(Match->Player1->GetController());
		AIndianPokerPlayerController* PC2 = Cast<AIndianPokerPlayerController>(Match->Player2->GetController());

		// 누가 이겼는지 판별 (P1이 죽었으면 2번 승리, P2가 죽었으면 1번 승리)
		int32 WinnerIndex = (FoldingPlayer == Match->Player1) ? 2 : 1;

		if (PC1)
		{
			PC1->Client_ShowRoundResult(Match->P1Card, Match->P2Card, WinnerIndex);
		}
		if (PC2)
		{
			// 상대방 시점에서는 내 카드가 OpponentCard이므로 인자 순서 주의
			int32 WinnerForP2 = (WinnerIndex == 2) ? 1 : 2;
			PC2->Client_ShowRoundResult(Match->P2Card, Match->P1Card, WinnerForP2);
		}

		Match->CurrentMaxBet = 0;
    FoldingPS->CurrentBet = 0;
    WinnerPS->CurrentBet = 0;
		Match->AccumulatedPot = 0;
		UE_LOG(LogTemp, Log, TEXT("[GameMode] 기권 종료: 승자 %d, 공개 카드 P1:%d, P2:%d"),
			WinnerIndex, Match->P1Card, Match->P2Card);

		BroadcastMatchMessage(Match, FString::Printf(TEXT("%s 다이(Fold)"), *FoldingPlayer->NickName));


		if (!CheckBattleEnd(Match)) {
			if (GetMatchByID(Match->MatchID))
			{
				FTimerHandle TimerHandle;
				FTimerDelegate TimerDel;
				TimerDel.BindUObject(this, &AIndianPokerGameMode::StartPokerMatchRoundByID, Match->MatchID);
				GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, 3.0f, false);
			}
		}
}

void AIndianPokerGameMode::ProcessBetAction(AIndianPokerCharacter* Player, EPokerBetAction Action, int32 Amount)
{
	FPokerMatch* Match = GetMatch(Player);
    if (!Match) return;

    AIndianPokerPlayerState* MyPS = Player->GetPlayerState<AIndianPokerPlayerState>();
    AIndianPokerCharacter* Opponent = (Player == Match->Player1) ? Match->Player2 : Match->Player1;
    AIndianPokerPlayerState* OppPS = Opponent->GetPlayerState<AIndianPokerPlayerState>();

    if (!MyPS || !OppPS || !MyPS->bIsMyTurn) return;

    switch (Action)
    {
    case EPokerBetAction::Race:
        {
            // [구조 수정] 비누적식 베팅: 이전에 소모한 칩과는 상관없이 현재 턴에 Amount만큼 새로 베팅
      			if (Amount <= 0) return;

						int32 ToCall = Match->CurrentMaxBet;
						int32 ActualAdd = FMath::Min(Amount, MyPS->Chips);

            if (ActualAdd > 0)
            {
                MyPS->Chips -= ActualAdd;
                MyPS->CurrentBet = ActualAdd; // 이번 턴에 낸 금액 저장
                Match->AccumulatedPot += ActualAdd; 
                Match->CurrentMaxBet = ActualAdd; // 다음 사람이 내야 할 금액으로 갱신

								MyPS->AccumulatedPot = Match->AccumulatedPot;
								OppPS->AccumulatedPot = Match->AccumulatedPot;

								FString AllInMsg = (MyPS->Chips <= 0) ? TEXT(" (올인!)") : TEXT("");

                // 턴 교체 전에 올인 상태 체크
                if (MyPS->Chips <= 0 || (OppPS && OppPS->Chips <= 0))
                {
                    // 비누적식에서는 내가 돈을 내면 상대가 무조건 대응해야 하므로, 
                    // 내가 올인했지만 상대보다 "새로 낸 금액"이 적거나 같을 때만 종료
                    if (ActualAdd <= ToCall)
                    {
                        BroadcastMatchMessage(Match, FString::Printf(TEXT("%s, %d개 레이스!%s"), *Player->NickName, Amount, *AllInMsg));
                        DetermineWinnerAndDistributeChips(Match);
                        return;
                    }
                }

                // 턴 교체
                MyPS->bIsMyTurn = false;
                OppPS->bIsMyTurn = true;

								if (AIndianPokerPlayerController* OppPC = Cast<AIndianPokerPlayerController>(Opponent->GetController()))
								{
									// 비누적식에서는 MaxBet 자체가 다음 사람의 콜 금액
									OppPC->Client_NotifyYourTurn(Match->CurrentMaxBet);
								}
								
								BroadcastMatchMessage(Match, FString::Printf(TEXT("%s, %d개 레이스!%s"), *Player->NickName, Amount, *AllInMsg));

								UE_LOG(LogTemp, Log, TEXT("[Race] %s님이 %d개 레이스. 총 판돈: %d"), *Player->NickName, ActualAdd, Match->AccumulatedPot);                
            }
        }
        break;

    case EPokerBetAction::Call:
        {
            // [구조 수정] 이전 사람이 낸 금액(CurrentMaxBet) 만큼 새로 칩 소모
            int32 ToCall = Match->CurrentMaxBet;
						int32 ActualAdd = FMath::Min(ToCall, MyPS->Chips);

						MyPS->Chips -= ActualAdd;
						MyPS->CurrentBet = ActualAdd;
						Match->AccumulatedPot += ActualAdd;

						MyPS->AccumulatedPot = Match->AccumulatedPot;
						OppPS->AccumulatedPot = Match->AccumulatedPot;

						FString AllInMsg = (MyPS->Chips <= 0) ? TEXT(" (올인!)") : TEXT("");
						BroadcastMatchMessage(Match, FString::Printf(TEXT("%s님이 콜!! %s"), *Player->NickName, *AllInMsg));

            DetermineWinnerAndDistributeChips(Match);
        }
        break;


    case EPokerBetAction::Die:
        {
            ProcessFold(Player);
        }
        break;
    }
}

bool AIndianPokerGameMode::CheckBattleEnd(FPokerMatch* Match)
{
	if (!Match || !Match->Player1 || !Match->Player2) return false;

	AIndianPokerPlayerState* PS1 = Match->Player1->GetPlayerState<AIndianPokerPlayerState>();
	AIndianPokerPlayerState* PS2 = Match->Player2->GetPlayerState<AIndianPokerPlayerState>();

	if (!PS1 || !PS2) return false;

	int32 FinalWinner = -1; // -1: 미종료, 0: 무승부, 1: P1 승리, 2: P2 승리

	// 1. 파산 여부 체크
	if (PS1->Chips <= 0 || PS2->Chips <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] 파산 발생! 배틀을 종료합니다."));
		FinalWinner = (PS1->Chips > 0) ? 1 : 2;
	}
	// 2. 덱 소진 체크
	else if (Match->Deck.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] 덱 소진! 칩 합계로 최종 승패 판정."));
		if (PS1->Chips > PS2->Chips) FinalWinner = 1;
		else if (PS2->Chips > PS1->Chips) FinalWinner = 2;
		else FinalWinner = 0; // 칩 개수까지 같으면 무승부
	}

	// 배틀 종료 처리
	if (FinalWinner != -1)
	{
		AIndianPokerPlayerController* PC1 = Cast<AIndianPokerPlayerController>(Match->Player1->GetController());
		AIndianPokerPlayerController* PC2 = Cast<AIndianPokerPlayerController>(Match->Player2->GetController());

		// 클라이언트들에게 결과 알림 (1: 승리, 2: 패배, 0: 무승부)
		if (PC1) PC1->Client_ShowBattleResult(FinalWinner == 0 ? 0 : (FinalWinner == 1 ? 1 : 2));
		if (PC2) PC2->Client_ShowBattleResult(FinalWinner == 0 ? 0 : (FinalWinner == 2 ? 1 : 2));

		// 최종 결과 안내 메세지
		if (FinalWinner == 0) {
			BroadcastMatchMessage(Match, TEXT("게임 종료. 최종 결과는 무승부입니다."));
		} else {
			AIndianPokerCharacter* FinalWinChar = (FinalWinner == 1) ? Match->Player1 : Match->Player2;
			BroadcastMatchMessage(Match, FString::Printf(TEXT("배틀 종료! %s 최종 승리"), *FinalWinChar->NickName));
		}

		Match->Player1->HandleEndBattle();

		return true;
	}

	return false;
}


void AIndianPokerGameMode::BroadcastMatchMessage(FPokerMatch* Match, const FString& Message)
{
	if (!Match) return;

	// 시스템 메세지의 발신자 이름 설정
	FString SystemName = TEXT("[System]");

	if (Match->Player1)
	{
		if (AIndianPokerPlayerController* PC1 = Cast<AIndianPokerPlayerController>(Match->Player1->GetController()))
		{
			PC1->Client_ReceiveMatchMessage(-1, SystemName, Message);
		}
	}
	if (Match->Player2)
	{
		if (AIndianPokerPlayerController* PC2 = Cast<AIndianPokerPlayerController>(Match->Player2->GetController()))
		{
			PC2->Client_ReceiveMatchMessage(-1, SystemName, Message);
		}
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
