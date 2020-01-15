#include "AALCharacter.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "Camera/CameraComponent.h"
#include "Components/MeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "ALPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Abilities/ALAbilityData.h"
#include "Components/ALExplosionComponent.h"
#include "Components/ALPushAbilityComponent.h"
#include "Components/ALFireProjectileAbilityComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "LogMacros.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "Components/ALSlipperyOil.h"
#include "Kismet/KismetMathLibrary.h"


AALCharacter::AALCharacter(const class FObjectInitializer& ObjectInitializer) :Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmAim = CreateDefaultSubobject<USpringArmComponent>(TEXT("AimSpringArm"));
	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapCollider"));

	//HealthComponent = CreateDefaultSubobject<UALHealthComponent>(TEXT("Health Component"));

	SphereCollision->SetCollisionProfileName("Trigger");
	SphereCollision->InitSphereRadius(20.f);
	SphereCollision->SetupAttachment(RootComponent);

	SphereCollision->bHiddenInGame = false;
	SphereCollision->OnComponentBeginOverlap.AddDynamic(this, &AALCharacter::BeginOverlap);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	GetCharacterMovement()->AirControl = 0.5;

	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 400.f;
	SpringArm->SetWorldRotation(FRotator(-15.f, 0.f, 0.f));
	SpringArm->bUsePawnControlRotation = true;


	SpringArmAim->SetupAttachment(RootComponent);
	SpringArmAim->TargetArmLength = 200.f;
	SpringArmAim->SetWorldRotation(FRotator(-15.f, 0.f, 0.f));
	SpringArmAim->bUsePawnControlRotation = false;

	PlayerCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	PlayerCamera->bUsePawnControlRotation = false;

	AutoPossessPlayer = EAutoReceiveInput::Player0;

	ShootRotation = UKismetMathLibrary::MakeRotFromX(PlayerCamera->GetForwardVector());
	CharDashDir = UKismetMathLibrary::MakeRotFromX(PlayerCamera->GetForwardVector());

	SprintSpeed = 1200.f;
	JumpCounter = 0.f;
	MaxJumpCounter = 1.f;
	LaunchVelocity = 500.f;
	CameraZoomAmmount = 20.f;
	DashVelocity = 10000.f;
	CharDashCounter = 0;
	CharDashMaxCounter = 1;
}

void AALCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitialWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	PlayerState = CastChecked<AALPlayerState>(GetWorld()->GetGameState()->PlayerArray[0]);

	ProjectileAbility = Cast<UALFireProjectileAbilityComponent>(GetComponentByClass(UALFireProjectileAbilityComponent::StaticClass()));
	PushAbility = Cast<UALPushAbilityComponent>(GetComponentByClass(UALPushAbilityComponent::StaticClass()));
	

	//RadioActiveAbility = Cast<UALRadioactiveAbilityComponent>(GetComponentByClass(UALRadioactiveAbilityComponent::StaticClass()));
	//MicrophoneAbility = Cast<UALMicrophoneAbilityComponent>(GetComponentByClass(UALMicrophoneAbilityComponent::StaticClass()));
}

void AALCharacter::Tick(float DeltaSeconds)
{ 
	Super::Tick(DeltaSeconds);

	if (IsOnOil)
	{
		Timer += DeltaSeconds;
		if (Timer >= OilTimer)
		{
			IsOnOil = false;
			Timer = 0;
		}
	}

	if (IsSprinting)
		BPOnSprint.Broadcast();
	if (!IsSprinting)
		BPOnStopSprint.Broadcast();

	Slideing();

	// Could be more made effective and easily managed by checking for a collision event instead of the collision itself with the ground object. Effectively 
	// that would mean a cheaper check in terms of cpu cycles as well as easier readability for future development. It would also open up the
	// possibility to use other checks in the future if the game design allowed it and not just ground collision.
	if (GetCharacterMovement()->IsMovingOnGround() == true)
	{
		JumpCounter = 0;
		IsJumping = false;
		IsDoubleJumping = false;
		IsDashing = false;
	}

	// Same as above. If done with a general collision check instead of checking if the character is moving on the ground these lines could've been applied
	// to that same check and would've saved up on CPU cycles as well as grouped every collision check together for easier readability.
	if ((GetCharacterMovement()->IsMovingOnGround() == true) && CharDashCounter >= CharDashMaxCounter)
		CharDashCounter = 0;
}

void AALCharacter::SetupPlayerInputComponent(UInputComponent* InputComponent)
{
	Super::SetupPlayerInputComponent(InputComponent);

	InputComponent->BindAxis("MoveForward", this, &AALCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AALCharacter::MoveRight);
	InputComponent->BindAxis("LookHorizontal", this, &AALCharacter::HandleLookHorizontal);
	InputComponent->BindAxis("LookVertical", this, &AALCharacter::HandleLookVertical);
	InputComponent->BindAxis("TurnRate", this, &AALCharacter::TurnAtRate);

	InputComponent->BindAction("Jump", IE_Pressed, this, &AALCharacter::HandleJump);
	InputComponent->BindAction("Jump", IE_Pressed, this, &AALCharacter::HandleDoubleJump);
	InputComponent->BindAction("Sprint", IE_Pressed, this, &AALCharacter::HandleSprintPressed);
	InputComponent->BindAction("Sprint", IE_Released, this, &AALCharacter::HandleSprintReleased);
	InputComponent->BindAction("ZoomIn", IE_Pressed, this, &AALCharacter::HandleCameraZoomIn);
	InputComponent->BindAction("ZoomOut", IE_Released, this, &AALCharacter::HandleCameraZoomOut);
	InputComponent->BindAction("Sneak", IE_Pressed, this, &AALCharacter::HandleSneakPressed);
	InputComponent->BindAction("Sneak", IE_Released, this, &AALCharacter::HandleSneakReleased);

	InputComponent->BindAction("Projectile", IE_Pressed, this, &AALCharacter::HandleFireProjectile);

	// ---------
	// Should've been removed since it's not in use of the final version. Cleaning the code is important to avoid misstakes when revisiting
	// the code.
	
	/*Old - ability system*/
	//InputComponent->BindAction("Absorb", IE_Pressed, this, &AALCharacter::HandleAbsorbAbilityPressed);
	//InputComponent->BindAction("Absorb", IE_Released, this, &AALCharacter::HandleAbsorbAbilityReleased);
	//InputComponent->BindAction("FireAbility", IE_Pressed, this, &AALCharacter::HandleFireAbility);
	// ---------
	
	// TEST aim
	InputComponent->BindAction("Aim", IE_Pressed, this, &AALCharacter::HandleAimPressed);
	InputComponent->BindAction("Aim", IE_Released, this, &AALCharacter::HandleAimReleased);

	// TEST FirePush
	InputComponent->BindAction("FirePush", IE_Pressed, this, &AALCharacter::HandlePush);
}

void AALCharacter::ApplyCamerShake()
{
	// A variable for the ammount of CameraFOV would've been good in case a menu option for it would've been planned in the future. It's
	// something that some people can experience as a initiator for motion sickness as they play games and an option for the ammount or any at all
	// would've been forward thinking with the player in mind. 
	if (CamerShake != nullptr)
		GetWorld()->GetFirstPlayerController()->PlayerCameraManager->PlayCameraShake(CamerShake, 1.0f);
}

void AALCharacter::ApplyCamerFOV()
{
	// A variable for the ammount of CameraFOV would've been good in case a menu option for it would've been planned in the future.
	// It's an easy quality of life thing that players appreciate and would've been easy to apply to the game. Also it's a good
	// easy-to-make accessability option for those who easily get motion sickness while playing games.
	if (CamerFov != nullptr)
		GetWorld()->GetFirstPlayerController()->PlayerCameraManager->PlayCameraShake(CamerFov, 1.0f);
}

void AALCharacter::MoveForward(float Val)
{
	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);
	const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	AddMovementInput(Direction, Val);
}

void AALCharacter::MoveRight(float Val)
{
	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);
	const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	
	if (IsOnOil && IsSprinting)
		AddMovementInput(Direction, Val / 2);
	
	else
		AddMovementInput(Direction, Val);

}

void AALCharacter::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AALCharacter::HandleLookHorizontal(float Val)
{
	AddControllerYawInput(Val);
}

void AALCharacter::HandleLookVertical(float Val)
{
	AddControllerPitchInput(Val);
}

// -----------
// splitting up the camera settings and functionality from the character into it's own component would've been a good option
// for keeping the character code clean, but also to split it's own properties from the character since it doesn't exactly have anything to do
// with the character itself. It also would've been a good option to have a public min and max zoom ammount as floats for the designers to be able
// to adress/change their values on their own from inside the client itself.

void AALCharacter::HandleCameraZoomIn()
{
	if (SpringArm->TargetArmLength > 150.f)
		SpringArm->TargetArmLength -= CameraZoomAmmount;
	else
		SpringArm->TargetArmLength++;
}

void AALCharacter::HandleCameraZoomOut()
{
	if (SpringArm->TargetArmLength < 400.f)
		SpringArm->TargetArmLength += CameraZoomAmmount;
	else
		SpringArm->TargetArmLength--;
}

// -----------

void AALCharacter::HandleSprintPressed()
{
	IsSprinting = true;

	if (GetCharacterMovement()->MaxWalkSpeed != SlowSpeed && GetCharacterMovement()->IsMovingOnGround() == true && !IsOnOil)
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;

	if (GetCharacterMovement()->IsMovingOnGround() == false && CharDashCounter < CharDashMaxCounter && !IsOnOil)
	{
		// Should've been removed from the final version in order to keep the code clean for future development
		
//		BPApplyDashEffects();
		LaunchCharacter(PlayerCamera->GetForwardVector() * DashVelocity, false, true);
		SetActorRotation(CharDashDir);
		ApplyCamerFOV();

		CharDashCounter++;
		IsDashing = true;
		BPOnDash.Broadcast();
	}
}

void AALCharacter::HandleSprintReleased()
{
	IsSprinting = false;

	if (GetCharacterMovement()->MaxWalkSpeed != SlowSpeed)
		GetCharacterMovement()->MaxWalkSpeed = InitialWalkSpeed;
}

void AALCharacter::HandleJump()
{
	Jump();
	if (GetCharacterMovement()->IsMovingOnGround() == true)
		OnJump.Broadcast();
	IsJumping = true;
}

void AALCharacter::HandleDoubleJump()
{
	// Since the check for ground movement of the character is done here it isn't necessary for it in the tick function and the event can be
	// broadcasted instead to save up on CPU cycles.
	if (GetCharacterMovement()->IsMovingOnGround() == false && JumpCounter < MaxJumpCounter)
	{
		UE_LOG(LogTemp, Warning, TEXT("double jumped"))
		LaunchCharacter(FVector(0, 0, 1) * LaunchVelocity, false, true);
		JumpCounter++;
		BPOnDoubleJump.Broadcast();
		IsDoubleJumping = true;
	}
}

void AALCharacter::HandleSneakPressed()
{
	IsSneaking = true;

	if (GetCharacterMovement()->MaxWalkSpeed != SlowSpeed)
		GetCharacterMovement()->MaxWalkSpeed = SneakSpeed;
}

void AALCharacter::HandleSneakReleased()
{
	IsSneaking = false;

	if (GetCharacterMovement()->MaxWalkSpeed != SlowSpeed)
		GetCharacterMovement()->MaxWalkSpeed = InitialWalkSpeed;
}

void AALCharacter::StartSlow()
{
	GetCharacterMovement()->MaxWalkSpeed = SlowSpeed;
}

void AALCharacter::EndSlow()
{
	GetCharacterMovement()->MaxWalkSpeed = InitialWalkSpeed;
}

//TEST AIM
void AALCharacter::HandleAimPressed()
{
	// Aiming isn't used in the final version of the game and therefor should've been removed in it's entirety 
	if (IsAiming == false)
		PlayerCamera->SetupAttachment(SpringArmAim, USpringArmComponent::SocketName);
		
	IsAiming = true;
}

//TEST AIM
void AALCharacter::HandleAimReleased()
{
	// Aiming isn't used in the final version of the game and therefor should've been removed in it's entirety 
	if (IsAiming == true)
		PlayerCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	IsAiming = false;
}

//TEST PUSH
void AALCharacter::HandlePush()
{
	// Pushing isn't present in the final version of the game and therefor shouldn't be in the code of the character. If it were to be used however
	// it should be it's own component that's applied onto the character instead of being part of it.
	PushAbility->Push();
}

void AALCharacter::BeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
	// Overlapping with oil
	if (OtherActor->GetComponentByClass(UALSlipperyOil::StaticClass()))
		IsOnOil = true;	
}

void AALCharacter::Slideing()
{
	if (!IsOnOil && IsSprinting)
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;

	if (IsOnOil && IsSprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed = GlidSpeed;
		BPOnSlide.Broadcast();
	}

}

void AALCharacter::HandleFireProjectile()
{
/*	SetActorRotation(ShootRotation);*/
	ProjectileAbility->ThrowProjectile();
}

void AALCharacter::HandleAbsorbAbilityPressed()
{
	IsAbsorbing = true;
}

void AALCharacter::HandleAbsorbAbilityReleased()
{
	IsAbsorbing = false;
}

bool AALCharacter::ReturnIfSneaking()
{
	return IsSneaking;
}

bool AALCharacter::ReturnIfDashing()
{
	return IsDashing;
}

bool AALCharacter::ReturnIfJumping()
{
	return IsJumping;
}

bool AALCharacter::ReturnIfDoubleJumping()
{
	return IsDoubleJumping;
}

void AALCharacter::CallTakeDamage(int32 Damage, bool IsGuard)
{
	//HealthComponent->TakeDamage(Damage, IsGuard);
}

bool AALCharacter::ReturnIfSprinting()
{
	return IsSprinting;
}
