// Copyright Epic Games, Inc. All Rights Reserved.

#include "IndianPokerCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "IndianPokerPlayerState.h"
#include "IndianPokerPlayerController.h"
#include "LobbyUIComponent.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AIndianPokerCharacter

AIndianPokerCharacter::AIndianPokerCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	// Create Nameplate Widget Component
	NameplateWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("NameplateWidget"));
	NameplateWidget->SetupAttachment(RootComponent);
	NameplateWidget->SetWidgetSpace(EWidgetSpace::Screen); // Always face the camera
	NameplateWidget->SetDrawAtDesiredSize(true);
	NameplateWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 110.0f)); // Position it above the character's head

	// Create Interaction Radius
	InteractionRadius = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionRadius"));
	InteractionRadius->SetupAttachment(RootComponent);
	InteractionRadius->SetSphereRadius(300.f); // 외곽선 스캔 반경과 동일하게 확장
	InteractionRadius->SetCollisionProfileName(TEXT("Trigger"));

	PrimaryActorTick.bCanEverTick = true;
	CurrentTargetCharacter = nullptr;
}

void AIndianPokerCharacter::BeginPlay()
{
	Super::BeginPlay();
}

//////////////////////////////////////////////////////////////////////////
// Input

void AIndianPokerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AIndianPokerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AIndianPokerCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AIndianPokerCharacter::Look);
		
		// Interacting
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AIndianPokerCharacter::Interact);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AIndianPokerCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AIndianPokerCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AIndianPokerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AIndianPokerCharacter, NickName);
}

#include "Kismet/GameplayStatics.h"

void AIndianPokerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 로컬 플레이어만 매 프레임 정면 타겟을 스캔해서 외곽선을 그립니다.
	if (IsLocallyControlled())
	{
		UpdateOutlineTarget();
	}
}

void AIndianPokerCharacter::UpdateOutlineTarget()
{
	// 최적화: 맵 전체를 뒤지는 대신, 300반경 Sphere에 들어온 액터 1~3명만 가져와서 내적 계산 수행
	TArray<AActor*> FoundActors;
	if (InteractionRadius)
	{
		InteractionRadius->GetOverlappingActors(FoundActors, AIndianPokerCharacter::StaticClass());
	}

	float InteractionDistance = 300.0f; // 탐색 거리 (근처)
	float MaxAngle = 60.0f;             // 정면 기준 몇 도까지 허용할 것인지 (부채꼴 판정)

	AIndianPokerCharacter* BestTarget = nullptr;
	float ClosestDistance = InteractionDistance + 100.f;

	for (AActor* Actor : FoundActors)
	{
		AIndianPokerCharacter* OtherCharacter = Cast<AIndianPokerCharacter>(Actor);
		if (OtherCharacter && OtherCharacter != this)
		{
			FVector ToOther = OtherCharacter->GetActorLocation() - GetActorLocation();
			float Dist = ToOther.Size();
			
			// 1. 거리 체크
			if (Dist <= InteractionDistance)
			{
				// 2. 각도 체크 (내 정면에 있는지)
				ToOther.Z = 0; // 높이 차이 무시
				ToOther.Normalize();
				
				FVector MyForward = GetActorForwardVector();
				MyForward.Z = 0;
				MyForward.Normalize();
				
				// 내적(Dot Product)으로 두 벡터 사이의 각도 계산
				float DotProduct = FVector::DotProduct(MyForward, ToOther);
				float Angle = FMath::RadiansToDegrees(FMath::Acos(DotProduct));

				if (Angle <= MaxAngle && Dist < ClosestDistance)
				{
					BestTarget = OtherCharacter;
					ClosestDistance = Dist;
				}
			}
		}
	}

	// 타겟이 바뀌었을 경우 기존 타겟의 외곽선을 끕니다.
	if (BestTarget != CurrentTargetCharacter)
	{
		if (CurrentTargetCharacter && CurrentTargetCharacter->IsValidLowLevel())
		{
			SetCharacterOutline(CurrentTargetCharacter, false, 0);
		}
		CurrentTargetCharacter = BestTarget;
	}

	// 현재 타겟이 있으면 배틀 상태를 조회해서 외곽선 색상을 지속적으로 갱신합니다.
	if (CurrentTargetCharacter)
	{
		AIndianPokerPlayerState* TargetPS = CurrentTargetCharacter->GetPlayerState<AIndianPokerPlayerState>();
		int32 Stencil = 1; // 1번 스텐실: 초록색 (배틀 안하고 있는 Lobby 상태)
		
		if (TargetPS && TargetPS->CurrentBattleState != EBattleState::Lobby)
		{
			Stencil = 2; // 2번 스텐실: 빨간색 (이미 게임 중이거나 누군가 매치요청을 보낸 상태)
		}
		
		SetCharacterOutline(CurrentTargetCharacter, true, Stencil);
	}
}

void AIndianPokerCharacter::SetCharacterOutline(AIndianPokerCharacter* Target, bool bEnable, int32 StencilValue)
{
	if (Target && Target->GetMesh())
	{
		if (bEnable)
		{
			UMaterialInterface* OutlineMat = (StencilValue == 1) ? Target->OutlineGreenMat : Target->OutlineRedMat;
			
			// Overlay Material 메커니즘을 Inverted Hull 머티리얼과 결합!
			Target->GetMesh()->SetOverlayMaterial(OutlineMat);
		}
		else
		{
			Target->GetMesh()->SetOverlayMaterial(nullptr);
		}
	}
}

void AIndianPokerCharacter::Interact(const FInputActionValue& Value)
{
	// 에임 스캔으로 찾은 타겟이 있을 때만 클릭이 먹히도록 
	if (IsLocallyControlled() && CurrentTargetCharacter)
	{
		AIndianPokerPlayerState* MyPS = GetPlayerState<AIndianPokerPlayerState>();
		AIndianPokerPlayerState* TargetPS = CurrentTargetCharacter->GetPlayerState<AIndianPokerPlayerState>();
		
		// 나도 로비 상태이고, 상대방도 로비 상태(초록 피두리)일 때만 배틀을 걸 수 있음!
		if (MyPS && TargetPS && 
			MyPS->CurrentBattleState == EBattleState::Lobby && 
			TargetPS->CurrentBattleState == EBattleState::Lobby)
		{
			Server_RequestBattle(CurrentTargetCharacter);
		}
		else
		{
			if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("현재 나 또는 상대가 배틀 중이거나 매칭 대기 중입니다!"));
		}
	}
}

void AIndianPokerCharacter::Server_RequestBattle_Implementation(AIndianPokerCharacter* TargetCharacter)
{
	if (!TargetCharacter) return;
	
	AIndianPokerPlayerState* MyPS = GetPlayerState<AIndianPokerPlayerState>();
	AIndianPokerPlayerState* TargetPS = TargetCharacter->GetPlayerState<AIndianPokerPlayerState>();

	UE_LOG(LogTemplateCharacter, Log, TEXT("[Server_RequestBattle] 신청자: %s / 대상: %s"), 
		MyPS ? *MyPS->GetPlayerName() : TEXT("Unknown"),
		TargetPS ? *TargetPS->GetPlayerName() : TEXT("Unknown"));
	
	// 서버에서도 보안 및 레이스 컨디션 방지를 위해 한 번 더블 체크
	if (MyPS && TargetPS && 
		MyPS->CurrentBattleState == EBattleState::Lobby && 
		TargetPS->CurrentBattleState == EBattleState::Lobby)
	{
		// 1. 서버가 허가함 -> 두 캐릭터의 상태를 즉시 'MatchRequested'으로 잠금
		MyPS->CurrentBattleState = EBattleState::MatchRequested;
		TargetPS->CurrentBattleState = EBattleState::MatchRequested;

		UE_LOG(LogTemplateCharacter, Log, TEXT("[Server_RequestBattle] 두 상태 잠금 성공. Target한테 Client RPC 발송..."));
		
		// 2. 상대방(Target)에게 수락/거절 팝업창을 띄우도록 Client RPC 명령
		TargetCharacter->Client_ReceiveBattleRequest(MyPS->GetPlayerName(), this);

		// 3. 나(신청자) 본인에게도 대기 중 창을 띄우라고 Client RPC 명령
		Client_ShowWaitingUI(TargetCharacter);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("[Server_RequestBattle] 선실: 상태 코드 문제로 데이터 차단. MyState=%d, TargetState=%d"),
			MyPS ? (int32)MyPS->CurrentBattleState : -1,
			TargetPS ? (int32)TargetPS->CurrentBattleState : -1);
	}
}

void AIndianPokerCharacter::Client_ShowWaitingUI_Implementation(AIndianPokerCharacter* Target)
{
	UE_LOG(LogTemplateCharacter, Log, TEXT("[Client_ShowWaitingUI] 컨트롤러 LobbyUIComp 탐색 중..."));

	AIndianPokerPlayerController* PC = Cast<AIndianPokerPlayerController>(GetController());
	if (PC && PC->LobbyUIComp)
	{
		UE_LOG(LogTemplateCharacter, Log, TEXT("[Client_ShowWaitingUI] LobbyUIComp 발견! ShowWaitingPopup 호출."));

		PC->LobbyUIComp->ShowWaitingPopup(Target);
		// 요청 보낸 쪽(Challenger)으로 타이머 시작
		PC->LobbyUIComp->StartRequestTimer(5.0f, Target, true);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("[Client_ShowWaitingUI] 실패! PC=%s / LobbyUIComp=%s"),
			PC ? TEXT("OK") : TEXT("NULL"),
			(PC && PC->LobbyUIComp) ? TEXT("OK") : TEXT("NULL"));
	}
}

void AIndianPokerCharacter::Client_ReceiveBattleRequest_Implementation(const FString& ChallengerName, AIndianPokerCharacter* Challenger)
{	
	UE_LOG(LogTemplateCharacter, Log, TEXT("[Client_ReceiveBattleRequest] 컨트롤러 LobbyUIComp 탐색 중. 신청자: %s"), *ChallengerName);

	AIndianPokerPlayerController* PC = Cast<AIndianPokerPlayerController>(GetController());
	if (PC && PC->LobbyUIComp)
	{
		UE_LOG(LogTemplateCharacter, Log, TEXT("[Client_ReceiveBattleRequest] LobbyUIComp 발견! ShowBattleRequestPopup 호출."));
		PC->LobbyUIComp->ShowBattleRequestPopup(Challenger->NickName, Challenger);
		// 요청 받은 쪽(Receiver)으로 타이머 시작
		PC->LobbyUIComp->StartRequestTimer(5.0f, Challenger, false);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("[Client_ReceiveBattleRequest] 실패! PC=%s / LobbyUIComp=%s"),
			PC ? TEXT("OK") : TEXT("NULL"),
			(PC && PC->LobbyUIComp) ? TEXT("OK") : TEXT("NULL"));
	}
}

void AIndianPokerCharacter::Server_AcceptBattle_Implementation(AIndianPokerCharacter* Challenger)
{
	AIndianPokerPlayerState* MyPS = GetPlayerState<AIndianPokerPlayerState>();
	AIndianPokerPlayerState* ChallengerPS = Challenger ? Challenger->GetPlayerState<AIndianPokerPlayerState>() : nullptr;

	if (MyPS && ChallengerPS)
	{
		MyPS->CurrentBattleState = EBattleState::InGame;
		ChallengerPS->CurrentBattleState = EBattleState::InGame;
		
		// Input Mode를 UI 전용 모드로 변경
		AIndianPokerPlayerController* MyPC = Cast<AIndianPokerPlayerController>(GetController());
		AIndianPokerPlayerController* ChallengerPC = Cast<AIndianPokerPlayerController>(Challenger->GetController());
		
		if (MyPC) MyPC->Client_TransitionToBattleMode();
		if (ChallengerPC) ChallengerPC->Client_TransitionToBattleMode();
	}
}

void AIndianPokerCharacter::Server_DeclineBattle_Implementation(AIndianPokerCharacter* Challenger)
{
	AIndianPokerPlayerState* MyPS = GetPlayerState<AIndianPokerPlayerState>();
	AIndianPokerPlayerState* ChallengerPS = Challenger ? Challenger->GetPlayerState<AIndianPokerPlayerState>() : nullptr;

	UE_LOG(LogTemplateCharacter, Warning, TEXT("[Server_DeclineBattle] 거절자(Me)PS=%s / 신청자(Challenger)=%s / ChallengerPS=%s"),
		MyPS ? *MyPS->GetPlayerName() : TEXT("NULL"),
		Challenger ? *Challenger->GetName() : TEXT("NULL"),
		ChallengerPS ? *ChallengerPS->GetPlayerName() : TEXT("NULL"));

	if (MyPS)
	{
		MyPS->CurrentBattleState = EBattleState::Lobby;
		UE_LOG(LogTemplateCharacter, Warning, TEXT("[Server_DeclineBattle] 거절자 상태 Lobby 전환 완료."));
	}

	if (ChallengerPS)
	{
		ChallengerPS->CurrentBattleState = EBattleState::Lobby;
		UE_LOG(LogTemplateCharacter, Warning, TEXT("[Server_DeclineBattle] 신청자 상태 Lobby 전환 완료. OnRep이 신청자 클라이언트에 전파되어야 합니다."));
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("[Server_DeclineBattle] !! ChallengerPS가 NULL입니다. Challenger 참조 문제 또는 PlayerState 아직 미할당!"));
	}
}

void AIndianPokerCharacter::Server_CancelBattleRequest_Implementation(AIndianPokerCharacter* Target)
{
	AIndianPokerPlayerState* MyPS = GetPlayerState<AIndianPokerPlayerState>();

	if (MyPS) MyPS->CurrentBattleState = EBattleState::Lobby;
	if (Target) 
	{
		AIndianPokerPlayerState* TargetPS = Target->GetPlayerState<AIndianPokerPlayerState>();
		if (TargetPS) TargetPS->CurrentBattleState = EBattleState::Lobby;
	}

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("배틀 요청이 취소되었습니다. (둘 다 로비로 복귀)"));
}

void AIndianPokerCharacter::Server_SetNickName_Implementation(const FString& InName)
{
	NickName = InName;
	// 서버에서도 즉시 UI를 갱신하고 싶다면 OnRep_NickName()을 수동으로 한 번 호출해주셔도 좋습니다.
}

void AIndianPokerCharacter::OnRep_NickName()
{
	// 닉네임이 리플리케이트 되어 들어왔을 때 실행됩니다.
	// (블루프린트 위젯 바인딩에서 매 틱마다 NickName을 읽어오게 설정하셨다면 이 함수는 비워두셔도 완벽하게 작동합니다!)
}

