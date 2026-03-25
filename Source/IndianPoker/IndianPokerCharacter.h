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

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

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

protected:

	virtual void NotifyControllerChanged() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

