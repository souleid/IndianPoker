// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "IndianPokerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class UWidgetComponent;
class USphereComponent;
class UCharacterMovementComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class AIndianPokerCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** Nameplate Widget Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = UI, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* NameplateWidget;

	/** Interaction Radius */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Interaction, meta = (AllowPrivateAccess = "true"))
	class USphereComponent* InteractionRadius;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Interact Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* InteractAction;

	
public:
	AIndianPokerCharacter();
	

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for interaction input */
	void Interact(const FInputActionValue& Value);
			
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void UpdateOutlineTarget();
	void SetCharacterOutline(AIndianPokerCharacter* Target, bool bEnable, int32 StencilValue);

	// 현재 시야반경 내에서 가장 가까운 타겟 (외곽선 띄우는 대상)
	UPROPERTY(Transient)
	AIndianPokerCharacter* CurrentTargetCharacter;

	/** 배틀 대기 상태(로비)일 때 띄울 초록색 외곽선 매터리얼 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Outline")
	class UMaterialInterface* OutlineGreenMat;

	/** 배틀 중일 때 띄울 빨간색 외곽선 매터리얼 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Outline")
	class UMaterialInterface* OutlineRedMat;

public:
	// 직관적인 닉네임 관리를 위해 Character에 직접 리플리케이트 변수 추가
	UPROPERTY(ReplicatedUsing = OnRep_NickName, BlueprintReadWrite, Category = "Player Info")
	FString NickName;

	UFUNCTION()
	void OnRep_NickName();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Player Info")
	void Server_SetNickName(const FString& InName);

	// Battle Request System
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Battle")
	void Server_RequestBattle(AIndianPokerCharacter* TargetCharacter);

	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "Battle")
	void Client_ReceiveBattleRequest(const FString& ChallengerName, AIndianPokerCharacter* Challenger);
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Battle")
	void Server_AcceptBattle(AIndianPokerCharacter* Challenger);
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Battle")
	void Server_DeclineBattle(AIndianPokerCharacter* Challenger);

	/** 신청자가 자신의 매칭 요청을 취소할 때 호출 (클라이언트 -> 서버) */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Battle")
	void Server_CancelBattleRequest(AIndianPokerCharacter* Target);

	/** 신청자에게 대기 중 UI를 띄우라고 서버가 내리는 클라이언트 명령 */
	UFUNCTION(Client, Reliable, Category = "Battle")
	void Client_ShowWaitingUI(AIndianPokerCharacter* Target);

	// -------------------------------------------------------
	// Battle Transition (수락 후 연출)
	// -------------------------------------------------------

	/** 수락 확정 시 서버 → 양쪽 클라이언트에게 배틀 연출 시작 명령 */
	UFUNCTION(Client, Reliable, Category = "Battle")
	void Client_StartBattleTransition(AIndianPokerCharacter* Opponent);

	/** 배틀 종료 시 카메라/회전 원복 명령 */
	UFUNCTION(Client, Reliable, Category = "Battle")
	void Client_EndBattleTransition();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Poker")
	void Server_ReportTransitionFinished();

	/** 라운드 시작 (서버 전용) */
	void StartPokerRound();

	/** 배틀 종료 실질 로직 (서버/클라이언트 공용 호출 가능하도록 설계) */
	void HandleEndBattle();

	/** 배틀 기권 또는 최종 종료 시 호출 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Poker")
	void Server_EndBattle();

	/** 배틀 카메라 목표 ArmLength (에디터에서 조절 가능) */
	UPROPERTY(EditAnywhere, Category = "Battle|Camera")
	float BattleArmLength = 400.f;

	UPROPERTY(EditAnywhere, Category = "Battle|Camera")
	FRotator BattleBoomRotation = FRotator(-60.f, 0.f, 0.f);

	/** 배틀 전환 총 소요 시간 (초) */
	UPROPERTY(EditAnywhere, Category = "Battle|Camera")
	float BattleTransitionDuration = 1.2f;

	/** [추가] 배틀 종료 후 로비로 돌아오는 시간 (더 천천히) */
	UPROPERTY(EditAnywhere, Category = "Battle|Camera")
	float BattleReturnDuration = 3.0f; // 3초 정도로 넉넉하게 설정

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	// -------------------------------------------------------
	// Battle Transition 내부 상태
	// -------------------------------------------------------

	/** 마주볼 배틀 상대 - Replicated로 모든 클라이언트에서 회전 방향 계산 가능 */
	UPROPERTY(Replicated, Transient)
	AIndianPokerCharacter* BattleOpponent = nullptr;

	/** 전환 진행 중 여부 - 모든 클라이언트에 복제하여 상대방 화면에서도 회전 */
	UPROPERTY(ReplicatedUsing = OnRep_BattleTransitioning)
	bool bBattleTransitioning = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsTransitionForward)
	bool bIsTransitionForward = true;

	UPROPERTY(Replicated)
	float SavedActorYaw = 0.f;

	UFUNCTION()
	void OnRep_IsTransitionForward();

	UFUNCTION()
	void OnRep_BattleTransitioning();

	/** 전환 진행도 (0→1) */
	float BattleTransitionAlpha = 0.f;

	/** 저장된 원래 카메라 Arm 값 (배틀 종료 시 복원용) */
	float   SavedArmLength = 400.f;
	FRotator SavedBoomRotation = FRotator(-15.f, 0.f, 0.f);
	FVector  SavedBoomSocketOffset = FVector::ZeroVector;

	/** Tick에서 호출되는 전환 처리 함수 */
	void TickBattleTransition(float DeltaTime);

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
