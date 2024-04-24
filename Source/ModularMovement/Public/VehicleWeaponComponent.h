//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/DamageType.h"
#include "Blueprint/UserWidget.h"
#include "Engine/EngineTypes.h"
#include "Engine/EngineBaseTypes.h"
#include "Components/SceneComponent.h"
#include "Runtime/Launch/Resources/Version.h"
#if  ENGINE_MINOR_VERSION >1
#include "Engine/TimerHandle.h"
#endif
#include "GameFramework/Pawn.h"

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraShakeBase.h"
#include  "Sound/SoundCue.h"
#include "Sound/SoundConcurrency.h"
#include "Components/MeshComponent.h"
#include "Camera/CameraShakeBase.h"
#include  "Components/AudioComponent.h"
#include "VehicleWeaponComponent.generated.h"
class UCameraComponent;
class UWidgetComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFireAnimationStateChanged, bool, bPlay);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReloadAnimationStateChanged, bool, bPlay);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFire);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNotifyAmmo, int, Ammo, int, ClipSize);


namespace EWeaponState
{
	enum Type
	{
		Idle,
		Firing,
		Reloading,
		Equipping,
	};
}


USTRUCT(BlueprintType)
struct FWeaponDataRow 
{
	GENERATED_USTRUCT_BODY()

	// Setup Category


	/** Ammo count */
	UPROPERTY(EditAnywhere, Category = "Setup")
	int ClipSize;

	/** Ammo count */
	UPROPERTY(EditAnywhere, Category = "Setup")
	int AmmoCount;

	/** Time to reload after ammo finishes */
	UPROPERTY(EditAnywhere, Category = "Setup")
	float ReloadTime;

	/** Damage type to be registered */
	UPROPERTY(EditAnywhere, Category = "Setup")
	TSubclassOf<UDamageType> DamageType;

	/** Initial time between shots for weapons that need to speed up before shooting, such as gatling guns */
	UPROPERTY(EditAnywhere, Category = "Setup")
	float InitialTimeBetweenShots;




	/** Infinite ammo (infinite reloads) */
	UPROPERTY(EditAnywhere, Category = "Setup")
	bool bInfiniteAmmo;

	/** Infinite current mag */
	UPROPERTY(EditAnywhere, Category = "Setup")
	bool bInfiniteClip;

	/** Speed for animating aim rotation of weapon */
	UPROPERTY(EditAnywhere, Category = "Setup")
	float RotInterpolationSpeed;




	/** Heat generated per shot */
	UPROPERTY(EditAnywhere, Category = "Setup")
	float HeatPerShot;

	/** Heat reduction rate per second */
	UPROPERTY(EditAnywhere, Category = "Setup")
	float HeatReduce;

	/** AITimer Multiplier */
	UPROPERTY(EditAnywhere, Category = "Setup")
	float AITimerMultiplier;

	/** Weapon Recoil (Negative To Disable) */
	UPROPERTY(EditAnywhere, Category = "Setup",DisplayName="Physical Shock Recoil")
	float WeaponRecoil;

	// Recoil for animating barrels
	UPROPERTY(EditAnywhere, Category = "Setup")
	float MaxAnimationRecoil=-30.f;

	UPROPERTY(EditAnywhere, Category = "Setup")
	float RecoilInterpolationSpeed;

	/** Flag indicating if owned by AI */
	UPROPERTY(EditAnywhere, Category = "Setup")
	bool IsOwnedByAI;

	/** Flag indicating instant rotation */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Setup")
	bool InstantRotation;

	/** Minimum pitch for aim */
	UPROPERTY(EditAnywhere, Category = "Setup")
	float MinPitch;

	/** Maximum pitch for aim */
	UPROPERTY(EditAnywhere, Category = "Setup")
	float MaxPitch;

	/** Widget to be shown at where aim is */
	UPROPERTY(EditAnywhere, Category = "Setup")
	TSubclassOf<UUserWidget> AimWidgetClass;

	UPROPERTY(EditAnywhere, Category = "Setup")
	bool AimDirectlyForward=false;
	/** Size of aim widget */
	UPROPERTY(EditAnywhere, Category = "Setup")
	FVector2D AimWidgetSize;

	

	/** Collision channel */
	UPROPERTY(EditAnywhere, Category = "Setup")
	TEnumAsByte<ECollisionChannel> Channel;

	FWeaponDataRow() : HeatPerShot(0), HeatReduce(0), RecoilInterpolationSpeed(200), Channel(ECC_Visibility)
	{
		ClipSize = 1;

		InitialTimeBetweenShots = 1;
		ReloadTime = 1;
		AmmoCount = 32;
		bInfiniteClip = false;
		bInfiniteAmmo = true;
		RotInterpolationSpeed = 20;
		WeaponRecoil = 0;
		IsOwnedByAI = false;
		InstantRotation = false;
		AITimerMultiplier = 1;
		MinPitch = -10;
		MaxPitch = 30;
		AimWidgetSize = FVector2D(50, 50);
	}
};

//////////////////
///class
UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent))
class MODULARMOVEMENT_API UVehicleWeaponComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UVehicleWeaponComponent();

	//-----------------------
	//-----Props------------
	//--------------------


	/*IsAnimPlaying*/
	UPROPERTY(BlueprintReadOnly, Category=State)
	bool bPlayingFireAnim;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category=Setup)
	FWeaponDataRow WeaponConfig;
	UPROPERTY(EditAnywhere, Category=Setup)
	FVector2D AimLocationOnScreen = FVector2D(2, 3);

	UFUNCTION(BlueprintCallable, Category=Weapon)
	void SetAimLocationOnScreen(FVector2D In);
	UFUNCTION(Server, Reliable)
	void ServerSetAimLocationOnScreen(FVector2D In);
	/** Whether to allow automatic weapons to catch up with shorter refire cycles */
	UPROPERTY(Config)
	bool bAllowAutomaticWeaponCatchup = true;

	/** is weapon fire active? */
	uint32 bWantsToFire : 1;
	/** is reload animation playing? */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_Reload)
	uint32 bPendingReload : 1;
	/** weapon is refiring */
	uint32 bRefiring;

	/** current weapon state */
	EWeaponState::Type CurrentState;

	/** time of last successful weapon fire */
	float LastFireTime;

	/** burst counter, used for replicating fire events to remote clients */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_BurstCounter)
	int32 BurstCounter;


	/** Adjustment to handle frame rate affecting actual timer interval. */
	UPROPERTY(Transient)
	float TimerIntervalAdjustment;

	/** current total ammo */
	UPROPERTY(Transient, Replicated)
	int32 CurrentAmmo;

	/** current ammo - inside clip */
	UPROPERTY(Transient, Replicated)
	int32 CurrentAmmoInClip;

	bool bIsAIAllowedToFire;

	UPROPERTY(BlueprintReadOnly, Category=Weapon)
	float CurrentHeat;

	//did we increase timers(Reload ETC)?
	bool TimersUpdated = false;

	//Weapon rotation for animating
	UPROPERTY(BlueprintReadOnly, Replicated, Category=Weapon)
	FRotator CurrentWeaponRotation;
	//Weapon rotation for animating
	UPROPERTY(BlueprintReadOnly, Category=Weapon)
	FRotator CurrentWeaponRotationWorldSpace;

	FVector TargetLocation;

	/** FX for muzzle flash */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	UParticleSystem* MuzzleFX;
	/** FX for muzzle flash in world space*/
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	UParticleSystem* MuzzleWorldSpaceFX;
	UPROPERTY(EditAnywhere, Category=Effects)
	FVector WorldSpaceParticleScale = FVector(1);
	/** spawned component for muzzle FX */
	UPROPERTY(Transient)
	UParticleSystemComponent* MuzzlePSC;


	/** single fire sound (bLoopedFireSound not set) */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* FireSound;

	/** looped fire sound (bLoopedFireSound set) */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* FireLoopSound;

	/** finished burst sound (bLoopedFireSound set) */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* FireFinishSound;

	/** out of ammo sound */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* OutOfAmmoSound;

	/** reload sound */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	USoundCue* ReloadSound;
	/** is muzzle FX looped? */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	uint32 bLoopedMuzzleFX : 1;

	UPROPERTY(EditDefaultsOnly, Category=Effects)
	TSubclassOf< UCameraShakeBase> Shake;
	/** is fire sound looped? */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	uint32 bLoopedFireSound : 1;

	//-----------------------
	//-----Funcs------------
	//--------------------


	/** [local + server] start weapon fire auto fire is for weapons that are not controlled by user and want to shoot on proximity */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	virtual void StartFire(bool bAutoFire);

	/** [local + server] stop weapon fire */
	UFUNCTION(BlueprintCallable, Category=Weapon)
	virtual void StopFire(bool bAutoFire);

	/** [local + server] firing started */
	virtual void OnBurstStarted();

	/** [local + server] handle weapon refire, compensating for slack time if the timer can't sample fast enough */
	void HandleReFiring();

	UFUNCTION(BlueprintCallable, Category=Weapon)
	void SetInstantRotation(bool Input);

	/** get current ammo amount (total) */
	int32 GetCurrentAmmo() const;

	/** [local + server] firing finished */
	virtual void OnBurstFinished();

	/** [local + server] interrupt weapon reload */
	virtual void StopReload();

	/** [server] performs actual reload */
	virtual void ReloadWeapon();

	UFUNCTION(reliable, server, WithValidation)
	void ServerStartReload();

	UFUNCTION(reliable, server, WithValidation)
	void ServerHandleFiring();

	/** Called in network play to stop cosmetic fx (e.g. for a looping shot). */
	virtual void StopSimulatingWeaponFire();

	/** Called in network play to do the cosmetic fx for firing */
	virtual void SimulateWeaponFire();


	/** consume a bullet */
	void UseAmmo();


	UFUNCTION(reliable, server, WithValidation)
	void ServerStartFire();

	UFUNCTION(reliable, server, WithValidation)
	void ServerStopFire();
	/** check if weapon can fire */
	static bool CanFire();

	/** check if weapon can be reloaded */
	bool CanReload() const;
	/** [local + server] handle weapon fire */
	void HandleFiring();
	/** check if weapon has infinite ammo (include owner's cheats) */
	bool HasInfiniteAmmo() const;

	/** check if weapon has infinite clip (include owner's cheats) */
	bool HasInfiniteClip() const;
	/** [all] start weapon reload */
	virtual void StartReload(bool bFromReplication = false);


	/** update weapon state */
	void SetWeaponState(EWeaponState::Type NewState);

	/** determine current weapon state */
	void DetermineWeaponState();

	UFUNCTION(BlueprintCallable, Category=Weapon)
	virtual void UpdateAnim(float DeltaTime);

	UFUNCTION()
	void OnRep_BurstCounter();

	UFUNCTION()
	void OnRep_Reload();


	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	virtual void BeginPlay() override;




	/** firing audio (bLoopedFireSound set) */


	/** play weapon sounds */
	UAudioComponent* PlayWeaponSound(USoundCue* Sound);
	/*Support pooling */
	UAudioComponent* PlayWeaponSound(USoundCue* Sound, UAudioComponent* ACin);

	bool ShouldDealDamage(const AActor* InActor) const;
	bool ClientShouldDealDamage(const AActor* InActor) const;
	UPROPERTY()
	AActor* TargetActor;


	UFUNCTION(BlueprintCallable, Category=Weapon)
	void UpdateMainWeaponHUD(bool bIsReloading = false);


	//AI
	//Called by AI WeaponManager
public:
	void UpdateAI();

	void RegisterIsOwnedByAI();
	void ApplyRecoil() const;

	APawn* GetOwningPawn() const;
	UMeshComponent* GetMesh() const;


	FVector CalculateAimDirection(const APlayerController* PC) const;
	UPROPERTY()
	FVector AimDirection;
	void SetAimDirection(const FVector& In);
	UFUNCTION(Server, Reliable)
	void ServerSetControlRotation(FVector In);

	//-----------------------
	//      Events
	//-----------------------
	/*Anim Notifier*/
	UPROPERTY(BlueprintAssignable)
	FOnFireAnimationStateChanged FireStateChange;
	UPROPERTY(BlueprintAssignable)
	FOnReloadAnimationStateChanged ReloadStateChange;
	UPROPERTY(BlueprintAssignable)
	FOnReload OnReload;
	UPROPERTY(BlueprintAssignable)
	FNotifyAmmo NotifyAmmo;
	UPROPERTY(BlueprintAssignable)
	FOnFire HandleFire;

private:
	UPROPERTY()
	UWidgetComponent* WidgetComponent;
	FTimerHandle TimerHandle_HandleFiring;
	FTimerHandle TimerHandle_StopReload;
	FTimerHandle TimerHandle_ReloadWeapon;
	float CurrentTimeBetweenShots;

	UPROPERTY(Transient)
	UAudioComponent* FireAC;

	/*for auto fire*/
	bool bStableAim;
	bool bAutoShooting;
	bool bManualShooting;

public:
	//Custom camera for aiming
	UPROPERTY(BlueprintReadWrite, Category=Weapon)
	UCameraComponent* OverrideAimCamera = nullptr;
	UPROPERTY(EditAnywhere, Category=Setup)
	bool ReplicateControlRotation = false;

	float LastDirectionChange;
	UPROPERTY(BlueprintReadOnly,Category=Recoil)
	float CurrentAnimationRecoil;
	float TargetAnimationRecoil;
	FRotator LastTargetRotation;
};
