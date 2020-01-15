#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AALCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAbsoringObject);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FJump);

UCLASS()
class AALCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	AALCharacter(const class FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent);

public:

	UPROPERTY(BlueprintReadOnly)
	bool IsOnOil = false;
	UPROPERTY(BlueprintReadOnly)
	bool IsSprinting = false;
	UPROPERTY(BlueprintReadOnly)
	bool IsDashing = false;
	UPROPERTY(BlueprintReadOnly)
	bool IsJumping = false;
	UPROPERTY(BlueprintReadOnly)
	bool IsDoubleJumping = false;
	UPROPERTY(BlueprintReadOnly)
	float JumpCounter;

	class AALPlayerState * PlayerState;

	UPROPERTY(VisibleDefaultsOnly)
	class USphereComponent* SphereCollision;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* PlayerCamera;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	class USpringArmComponent* SpringArm;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	class USpringArmComponent* SpringArmAim;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float CameraZoomAmmount;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float BaseTurnRate;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Camera")
	float BaseLookUpRate;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character Movement: Walking")
	float SprintSpeed = 1200.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character Movement: Walking")
	float SneakSpeed = 200.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character Movement: Walking")
	float SlowSpeed = 100;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character Movement: Sliding")
	float GlidSpeed = 2000.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character Movement: Sliding", meta = (ToolTip = "The value divided by 'Move Right Value'.\nMake sure it isn't less then 1"))
	float SlideValue = 2;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character Movement: Sliding", meta = (ToolTip = "Timer the player can gilde, while holding down 'sprint/slide' button"))
	float SlideTimer = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character Movement: Jumping")
	float MaxJumpCounter;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character Movement: Jumping")
	float LaunchVelocity;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character Movement: Dashing")
	float DashVelocity;

	// All test functions and variables should've been removed before the build of the final version.
	/// TEST
	UPROPERTY(EditDefaultsOnly, Category = "Pushing")
	float ForceStrength = 10000.f;

	UFUNCTION(BlueprintCallable)
	bool ReturnIfSprinting();

	UFUNCTION(BlueprintCallable)
	bool ReturnIfSneaking();

	UFUNCTION(BlueprintCallable)
	bool ReturnIfDashing();

	UFUNCTION(BlueprintCallable)
	bool ReturnIfJumping();

	UFUNCTION(BlueprintCallable)
	bool ReturnIfDoubleJumping();

	void CallTakeDamage(int32 Damage,bool IsGuard);

	void StartSlow();
	void EndSlow();

	UPROPERTY(BlueprintAssignable, meta = (ToolTip = "Happens while trying to absorb something"))
	FOnAbsoringObject OnAbsorbingRequest;
	UPROPERTY(BlueprintAssignable, meta = (ToolTip = "Happens when successfully absorb an ability"))
	FOnAbsoringObject OnAbsorbingSuccessRequest;
	UPROPERTY(BlueprintAssignable, meta = (ToolTip = "Happens when player fail to absorb ability"))
	FOnAbsoringObject OnAbsorbingFailureRequest;

	// Clean up the code before building the final version
// 	UPROPERTY(BlueprintAssignable, meta = (ToolTip = "Happens when player is gliding on oil"))
// 	FOnGliding OnGlidingRequest;

 	UPROPERTY(BlueprintAssignable)
 	FJump OnJump;

 	UPROPERTY(BlueprintAssignable)
 	FJump BPOnDoubleJump;

	UPROPERTY(BlueprintAssignable)
	FJump BPOnSprint;

	UPROPERTY(BlueprintAssignable)
	FJump BPOnStopSprint;

	UPROPERTY(BlueprintAssignable)
	FJump BPOnDash;

	UPROPERTY(BlueprintAssignable)
	FJump BPOnSlide;

	UPROPERTY(BLueprintAssignable)
	FJump BPOnLanding;

	// Clean up the code before building the final version
// 	UFUNCTION(BlueprintImplementableEvent, meta = (ToolTip = "Called when the character dash action is performed"))
// 	void BPApplyDashEffects();

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsGliding() { return IsOnOil && IsSprinting; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsRunning() { return !IsOnOil && IsSprinting; }


private:
	// Bunch up the UFUNCTIONs together instead of splitting them up in different parts of the header. Same
	// goes for the variables and other values.
	
	UFUNCTION(BlueprintCallable)
	void ApplyCamerShake();

	UFUNCTION()
	void ApplyCamerFOV();

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TSubclassOf<class UCameraShake> CamerShake;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TSubclassOf<class UCameraShake> CamerFov;

	class UALFireProjectileAbilityComponent* ProjectileAbility;
	class UALPushAbilityComponent* PushAbility;
	//class UALRadioactiveAbilityComponent* RadioActiveAbility;
	//class UALMicrophoneAbilityComponent* MicrophoneAbility;
	//class UALHealthComponent* HealthComponent;

	UPROPERTY(EditAnywhere, Category = "Absorb Ability", meta = (ToolTip = "The time the player have to hold down key to get ability"))
	float AbsorbTimer = 1.f;

	// True if player holds down absorbing button
	bool IsAbsorbing;
	bool StartAbsorbing = false;

	bool IsSneaking = false;
	bool IsAiming = false;
	FRotator ShootRotation;

	float InitialWalkSpeed;

	float CharDashCounter;
	float CharDashMaxCounter;
	FRotator CharDashDir;

	void MoveForward(float Val);
	void MoveRight(float Val);
	void TurnAtRate(float Rate);
	void HandleLookHorizontal(float Val);
	void HandleLookVertical(float Val);
	void HandleCameraZoomIn();
	void HandleCameraZoomOut();
	void HandleSprintPressed();
	void HandleSprintReleased();
	void HandleJump();
	void HandleDoubleJump();
	void HandleSneakPressed();
	void HandleSneakReleased();

	void HandleFireProjectile();

	void HandleAbsorbAbilityPressed();
	void HandleAbsorbAbilityReleased();

	// TEST Aim
	void HandleAimPressed();
	void HandleAimReleased();

	// TEST FirePush
	void HandlePush();

	UFUNCTION()
	void BeginOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	void Slideing();

	UPROPERTY(EditDefaultsOnly, Category = "Character Movement: Sliding", meta = (ToolTip = "The time player is 'slippery' after walking on oil"))
	float OilTimer = 1.5f;

	
	float Timer = 0;

	bool IsCallingOut = false;

};
