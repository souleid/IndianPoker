// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "IndianPokerGameInstance.generated.h"

/**
 * 전역으로 유지되어야 할 플레이어 정보(닉네임, 세팅, 게임 진행 데이터 등)를 보관하는 C++ GameInstance 클래스입니다.
 */
UCLASS()
class INDIANPOKER_API UIndianPokerGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UIndianPokerGameInstance();

	// 플레이어가 로비에 접속하기 전 입력한 닉네임 보관용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Info")
	FString MyNickName;

	// 추후 서버 접속 IP 주소 관리나 인디언 포커 승수에 따른 전역 통계 등을 넣을 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Settings")
	FString TargetServerIP;
};
