#include "IndianPokerCharacter.h"
#include "IndianPokerGameMode.h"
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
	DOREPLIFETIME(AIndianPokerCharacter, BattleOpponent);
	DOREPLIFETIME(AIndianPokerCharacter, bBattleTransitioning);
}

void AIndianPokerCharacter::OnRep_BattleTransitioning()
{
	if (bBattleTransitioning)
	{
		// 전환 시작 시: 현재 Yaw를 기준점으로 저장, Alpha 리셋
		SavedActorYaw         = GetActorRotation().Yaw;
		BattleTransitionAlpha = 0.f;
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
	else
	{
		// 전환 종료 시: 이동방향 자동회전 복구
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}
}

#include "Kismet/GameplayStatics.h"

void AIndianPokerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bBattleTransitioning && BattleOpponent && BattleOpponent->IsValidLowLevel())
	{
		// 카메라 전환은 로컬 플레이어만
		if (IsLocallyControlled())
		{
			TickBattleTransition(DeltaTime);
		}
		else
		{
			// Non-owning 클라이언트: 회전만 독립적으로 실행
			BattleTransitionAlpha = FMath::Clamp(BattleTransitionAlpha + DeltaTime / BattleTransitionDuration, 0.f, 1.f);
			const float EasedAlpha = FMath::InterpEaseInOut(0.f, 1.f, BattleTransitionAlpha, 2.f);
			const FVector ToOpponent = BattleOpponent->GetActorLocation() - GetActorLocation();
			const float   TargetYaw  = FRotationMatrix::MakeFromX(ToOpponent).Rotator().Yaw;
			SetActorRotation(FRotator(0.f, FMath::LerpStable(SavedActorYaw, TargetYaw, EasedAlpha), 0.f));
		}
		return; // 전환 중에는 외곽선 스캔 스킵
	}

	// 외곽선 스캔: 로컬 플레이어이고 Lobby 상태일 때만
	if (IsLocallyControlled())
	{
		AIndianPokerPlayerState* MyPS = GetPlayerState<AIndianPokerPlayerState>();
		if (MyPS && MyPS->CurrentBattleState == EBattleState::Lobby)
		{
			UpdateOutlineTarget();
		}
		else if (CurrentTargetCharacter) // InGame 상태: 외곽선 제거
		{
			SetCharacterOutline(CurrentTargetCharacter, false, 0);
			CurrentTargetCharacter = nullptr;
		}
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
	if (!Challenger) return;

	AIndianPokerPlayerState* MyPS = GetPlayerState<AIndianPokerPlayerState>();
	AIndianPokerPlayerState* ChallengerPS = Challenger->GetPlayerState<AIndianPokerPlayerState>();

	if (MyPS && ChallengerPS)
	{
		MyPS->CurrentBattleState = EBattleState::InGame;
		ChallengerPS->CurrentBattleState = EBattleState::InGame;
		
		AIndianPokerPlayerController* MyPC = Cast<AIndianPokerPlayerController>(GetController());
		AIndianPokerPlayerController* ChallengerPC = Cast<AIndianPokerPlayerController>(Challenger->GetController());

		// 1. 양쪽 타이머 정리 + 팝업 닫기
		if (MyPC && MyPC->LobbyUIComp)
		{
			MyPC->LobbyUIComp->CancelRequestTimer();
			MyPC->LobbyUIComp->CloseAllLobbyPopups();
		}
		if (ChallengerPC && ChallengerPC->LobbyUIComp)
		{
			ChallengerPC->LobbyUIComp->CancelRequestTimer();
			ChallengerPC->LobbyUIComp->CloseAllLobbyPopups();
		}

		// 2. 서버에서 Match 생성 + BattleOpponent 세팅
		if (AIndianPokerGameMode* GM = GetWorld()->GetAuthGameMode<AIndianPokerGameMode>())
		{
			GM->CreateMatch(this, Challenger);
		}

		BattleOpponent           = Challenger;
		bBattleTransitioning     = true;
		Challenger->BattleOpponent       = this;
		Challenger->bBattleTransitioning = true;

		// 3. 로컬 클라이언트에게 카메라 전환 + InputMode 명령 (Client RPC)
		Client_StartBattleTransition(Challenger);
		Challenger->Client_StartBattleTransition(this);
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

// -------------------------------------------------------
// Battle Transition
// -------------------------------------------------------

void AIndianPokerCharacter::Client_StartBattleTransition_Implementation(AIndianPokerCharacter* Opponent)
{
	if (!IsLocallyControlled() || !Opponent || !CameraBoom) return;

	UE_LOG(LogTemplateCharacter, Log, TEXT("[Client_StartBattleTransition] 상대: %s"), *Opponent->NickName);

	// 0. 클라이언트 사이드 RequestTimer 강제 정리 + 팝업 닫기
	//    (Server_AcceptBattle에서 서버쪽을 정리했지만 클라이언트 타이머는 따로 정리해야 함)
	if (AIndianPokerPlayerController* IPC = Cast<AIndianPokerPlayerController>(GetController()))
	{
		if (IPC->LobbyUIComp)
		{
			IPC->LobbyUIComp->CancelRequestTimer();
			IPC->LobbyUIComp->CloseAllLobbyPopups();
		}
	}

	// 1. 상대 + 초기 회전값 저장
	BattleOpponent = Opponent;
	SavedActorYaw  = GetActorRotation().Yaw;

	// 2. 현재 카메라 붐 상태 저장 (배틀 종료 시 복원용)
	SavedArmLength        = CameraBoom->TargetArmLength;
	SavedBoomRotation     = CameraBoom->GetRelativeRotation();
	SavedBoomSocketOffset = CameraBoom->SocketOffset;

	// 3. 이동 중 자동 회전 비활성화
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// 4. 카메라 붐이 컨트롤러 회전을 따르지 않도록 고정
	CameraBoom->bUsePawnControlRotation = false;

	// 5. 전환 시작
	BattleTransitionAlpha = 0.f;
	bBattleTransitioning  = true;

	// 6. 입력 모드: UI Only (마우스 O, WASD 이동 완전 차단)
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeUIOnly());
	}
}

void AIndianPokerCharacter::Client_EndBattleTransition_Implementation()
{
	if (!IsLocallyControlled() || !CameraBoom) return;

	UE_LOG(LogTemplateCharacter, Log, TEXT("[Client_EndBattleTransition] 카메라/회전 원복 시작."));

	// 전환 역방향 재생 (현재 alpha에서 0으로)
	// 간단하게 저장값 즉시 복원 후 역방향 Lerp 시작
	BattleOpponent       = nullptr;
	BattleTransitionAlpha = 1.f;   // 역방향으로 재생하려면 1에서 시작
	bBattleTransitioning  = true;  // Tick에서 감소 방향으로 처리

	// 이동 방향 자동 회전 즉시 복구 (배틀 끝났으니 다시 이동 방향으로 돌아가도 됨)
	GetCharacterMovement()->bOrientRotationToMovement = true;
	CameraBoom->bUsePawnControlRotation = true;

	// 카메라 즉시 복원 (부드럽게 하려면 alpha 역Lerp 추가 가능)
	CameraBoom->TargetArmLength   = SavedArmLength;
	CameraBoom->SetRelativeRotation(SavedBoomRotation);
	CameraBoom->SocketOffset       = SavedBoomSocketOffset;
	bBattleTransitioning           = false;

	AIndianPokerPlayerController* PC = Cast<AIndianPokerPlayerController>(GetController());
	if (PC) PC->Client_TransitionToLobbyMode();
}

void AIndianPokerCharacter::TickBattleTransition(float DeltaTime)
{
	if (!CameraBoom) return;

	// Alpha 0→1 진행 (전체 Duration으로)
	BattleTransitionAlpha = FMath::Clamp(BattleTransitionAlpha + DeltaTime / BattleTransitionDuration, 0.f, 1.f);

	// ■ Phase 1 (Alpha 0→0.5): 쪭러쯐 회전만
	// ■ Phase 2 (Alpha 0.5→1): 카메라 전환만
	const float RotPhaseMax = 0.5f;
	const float CamPhaseMin = 0.5f;

	// --- Phase 1: 액터 Yaw 회전 ---
	if (BattleOpponent && BattleOpponent->IsValidLowLevel())
	{
		// 0→RotPhaseMax 구간을 0→1로 정규화
		const float RotAlpha    = FMath::Clamp(BattleTransitionAlpha / RotPhaseMax, 0.f, 1.f);
		const float EasedRot    = FMath::InterpEaseInOut(0.f, 1.f, RotAlpha, 2.f);
		const FVector  ToOpponent = BattleOpponent->GetActorLocation() - GetActorLocation();
		const float    TargetYaw  = FRotationMatrix::MakeFromX(ToOpponent).Rotator().Yaw;
		SetActorRotation(FRotator(0.f, FMath::LerpStable(SavedActorYaw, TargetYaw, EasedRot), 0.f));
	}

	// --- Phase 2: 카메라 붐 전환 ---
	if (BattleTransitionAlpha >= CamPhaseMin)
	{
		// CamPhaseMin→1 구간을 0→1로 정규화
		const float CamAlpha  = FMath::Clamp((BattleTransitionAlpha - CamPhaseMin) / (1.f - CamPhaseMin), 0.f, 1.f);
		const float EasedCam  = FMath::InterpEaseInOut(0.f, 1.f, CamAlpha, 2.f);
		CameraBoom->TargetArmLength = FMath::Lerp(SavedArmLength, BattleArmLength, EasedCam);
		CameraBoom->SetRelativeRotation(FMath::Lerp(SavedBoomRotation, BattleBoomRotation, EasedCam));
	}

	// --- 전환 완료 ---
	if (BattleTransitionAlpha >= 1.f)
	{
		bBattleTransitioning = false;
		UE_LOG(LogTemplateCharacter, Log, TEXT("[TickBattleTransition] 전환 완료."));

		if (IsLocallyControlled())
		{
			if (AIndianPokerPlayerController* PC = Cast<AIndianPokerPlayerController>(GetController()))
			{
				// 컨트롤러에게 인게임 UI 띄우라고 지시
				PC->Client_ShowInGamePokerUI();
			}
		}

		// 서버 사이드: 라운드 시작 (한 명만 호출해도 되도록 HasAuthority 체크)
		if (HasAuthority())
		{
			StartPokerRound();
		}
	}
}

// -------------------------------------------------------
// Indian Poker Core Logic Implementations
// -------------------------------------------------------

void AIndianPokerCharacter::StartPokerRound()
{
	if (!HasAuthority()) return;

	if (AIndianPokerGameMode* GM = GetWorld()->GetAuthGameMode<AIndianPokerGameMode>())
	{
		GM->StartPokerMatchRound(this);
	}
}

void AIndianPokerCharacter::Server_NetRace_Implementation(int32 Amount)
{
	if (AIndianPokerGameMode* GM = GetWorld()->GetAuthGameMode<AIndianPokerGameMode>())
	{
		GM->ProcessBetAction(this, EPokerBetAction::Race, Amount);
	}
}

void AIndianPokerCharacter::Server_NetCall_Implementation()
{
	if (AIndianPokerGameMode* GM = GetWorld()->GetAuthGameMode<AIndianPokerGameMode>())
	{
		GM->ProcessBetAction(this, EPokerBetAction::Call);
	}
}

void AIndianPokerCharacter::Server_NetDie_Implementation()
{
	if (AIndianPokerGameMode* GM = GetWorld()->GetAuthGameMode<AIndianPokerGameMode>())
	{
		GM->ProcessBetAction(this, EPokerBetAction::Die);
	}
}

void AIndianPokerCharacter::Server_EndBattle_Implementation()
{
	if (!HasAuthority()) return;

	// 1. 게임모드에서 매치 데이터 삭제
	if (AIndianPokerGameMode* GM = GetWorld()->GetAuthGameMode<AIndianPokerGameMode>())
	{
		GM->RemoveMatch(this);
	}

	// 2. 상태 초기화 (본인 및 상대)
	AIndianPokerPlayerState* MyPS = GetPlayerState<AIndianPokerPlayerState>();
	if (MyPS) MyPS->CurrentBattleState = EBattleState::Lobby;

	if (BattleOpponent)
	{
		AIndianPokerPlayerState* OppPS = BattleOpponent->GetPlayerState<AIndianPokerPlayerState>();
		if (OppPS) OppPS->CurrentBattleState = EBattleState::Lobby;

		// 상대방에게도 종료 연출 명령
		BattleOpponent->Client_EndBattleTransition();
		BattleOpponent->BattleOpponent = nullptr;
		BattleOpponent->bBattleTransitioning = false;
	}

	// 3. 내 연출 원복 및 포인터 정리
	Client_EndBattleTransition();
	BattleOpponent = nullptr;
	bBattleTransitioning = false;
}

void AIndianPokerCharacter::Client_ReceiveOpponentCard_Implementation(uint8 CardValue)
{
	if (AIndianPokerPlayerController* PC = Cast<AIndianPokerPlayerController>(GetController()))
	{
		if (PC->PokerUIComp)
		{
			//PC->PokerUIComp->UpdateOpponentCardUI(CardValue);
		}
	}
}

void AIndianPokerCharacter::Client_ShowRoundResult_Implementation(uint8 MyCard, uint8 OpponentCard, int32 WinnerResult)
{
	if (AIndianPokerPlayerController* PC = Cast<AIndianPokerPlayerController>(GetController()))
	{
		if (PC->PokerUIComp)
		{
			//PC->PokerUIComp->ShowRoundResult(MyCard, OpponentCard, WinnerResult);
		}
	}
}

void AIndianPokerCharacter::Client_NotifyYourTurn_Implementation(int32 AmountToCall)
{
	if (AIndianPokerPlayerController* PC = Cast<AIndianPokerPlayerController>(GetController()))
	{
		if (PC->PokerUIComp)
		{
			// UI 컴포넌트에 턴 시작 알림 (필요한 콜 금액 전달)
			//PC->PokerUIComp->UpdateTurnUI(true);
			//PC->PokerUIComp->NotifyRequiredCallAmount(AmountToCall);
		}
	}
}


