// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "Engine/DataTable.h"
#include "VehicleWeaponComponent.generated.h"

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
struct FWeaponDataRow : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()


	UPROPERTY(EditAnywhere)
	int AmmoCount;
	UPROPERTY(EditAnywhere)
	float ReloadTime;
	UPROPERTY(EditAnywhere)
	TSubclassOf<UDamageType> DamageType;
	UPROPERTY(EditAnywhere)
	float InitialTimeBetweenShots;
	UPROPERTY(EditAnywhere)
	float TargetTimeBetweenShots;
	UPROPERTY(EditAnywhere)
	float InterpolationSpeed;
	UPROPERTY(EditAnywhere)
	bool bInfiniteAmmo;
	UPROPERTY(EditAnywhere)
	bool bInfiniteClip;
	UPROPERTY(EditAnywhere)
	float RotInterpolationSpeed;
	UPROPERTY(EditAnywhere)
	float HeatPerShot;
	/*Multiplied by delatime */
	UPROPERTY(EditAnywhere)
	float HeatReduce;

	//AIHearing
	UPROPERTY(EditAnywhere)
	float Loudness;
	//AIHearing
	UPROPERTY(EditAnywhere)
	float MaxRange;
	//AIHearing
	UPROPERTY(EditAnywhere)
	FName Tag;

	/*Increases cooldowns reloads etc!*/
	UPROPERTY(EditAnywhere)
	float AITimerMultiplier;
	/*Negative To Disable*/
	UPROPERTY(EditAnywhere)
	float WeaponRecoil;

	bool IsOwnedByAI;
	UPROPERTY(EditAnywhere)
	bool InstantRotation;
	UPROPERTY(EditAnywhere)
	float MinPitch=-5;
	UPROPERTY(EditAnywhere)
	float MaxPitch=30;
	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> AimWidgetClass;
	UPROPERTY(EditAnywhere)
	FVector2D AimWidgetSize=FVector2D(50,50);
	UPROPERTY(EditAnywhere)
	TEnumAsByte<ECollisionChannel> Channel;

	FWeaponDataRow(): HeatPerShot(0), HeatReduce(0), Loudness(0), MaxRange(0), Channel(ECC_Visibility)
	{
		InterpolationSpeed = 1;

		InitialTimeBetweenShots = TargetTimeBetweenShots = 1;
		ReloadTime = 1;
		AmmoCount = 32;
		bInfiniteClip = false;
		bInfiniteAmmo = true;

		RotInterpolationSpeed = 20;
		WeaponRecoil = 0;
		IsOwnedByAI = false;
		InstantRotation = false;
		AITimerMultiplier = 1;
	}
};

//////////////////
///class
UCLASS( Blueprintable , meta=(BlueprintSpawnableComponent))
class MODULARMOVEMENT_API UVehicleWeaponComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UVehicleWeaponComponent();
	FTimerHandle TimerHandle_HandleFiring;
	FTimerHandle TimerHandle_StopReload;
	FTimerHandle TimerHandle_ReloadWeapon;

	///////////////////////////////////////////
	/** [local + server] start weapon fire */
	UFUNCTION(BlueprintCallable)
	virtual void StartFire(bool bAutoFire);

	/** [local + server] stop weapon fire */
	UFUNCTION(BlueprintCallable)
	virtual void StopFire(bool bAutoFire);
	/** [local + server] firing started */
	virtual void OnBurstStarted();
	/** [local + server] handle weapon refire, compensating for slack time if the timer can't sample fast enough */
	void HandleReFiring();

	/** Whether to allow automatic weapons to catch up with shorter refire cycles */
	UPROPERTY(Config)
	bool bAllowAutomaticWeaponCatchup = true;
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

	float CurrentTimeBetweenShots;
	/** Called in network play to do the cosmetic fx for firing */
	virtual void SimulateWeaponFire();
	


	/*IsAnimPlaying*/
	UPROPERTY(BlueprintReadOnly)
	bool bPlayingFireAnim;


	/** Called in network play to stop cosmetic fx (e.g. for a looping shot). */
	virtual void StopSimulatingWeaponFire();

	/** [local] weapon specific fire implementation */
	virtual void FireWeapon() PURE_VIRTUAL(UWeaponComponent::FireWeapon,);
	
	/** consume a bullet */
	void UseAmmo();
	
	UPROPERTY()
	APlayerController* PlayerController;

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
	FOnFire OnFire;
	UFUNCTION(reliable, server, WithValidation)
	void ServerStartFire();

	UFUNCTION(reliable, server, WithValidation)
	void ServerStopFire();
	/** check if weapon can fire */
	bool CanFire() const;

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

	/** get the originating location for camera damage */
	FVector GetCameraDamageStartLocation(const FVector& AimDir) const;
	/** Get the aim of the weapon, allowing for adjustments to be made by the weapon */
	
	

public:
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	FWeaponDataRow WeaponConfig;
	UFUNCTION(BlueprintCallable)
	virtual void UpdateAnim(float DeltaTime);
	UPROPERTY(BlueprintReadOnly)
	FRotator CurrentWeaponRotation;


	FVector TargetLocation;

	FRotator TargetCosmeticRotation;
	UFUNCTION()
	void OnRep_BurstCounter();

	UFUNCTION()
	void OnRep_Reload();


	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	virtual void BeginPlay() override;

	FHitResult WeaponTrace(const FVector& TraceFrom, const FVector& TraceTo) const;

	//fx
	/*Disables Pooling And Spawns Particle in world*/

	/** FX for muzzle flash */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	UParticleSystem* MuzzleFX;
	/** FX for muzzle flash in world space*/
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	UParticleSystem* MuzzleWorldSpaceFX;
	/** spawned component for muzzle FX */
	UPROPERTY(Transient)
	UParticleSystemComponent* MuzzlePSC;


	/** camera shake on firing */
	//UPROPERTY(EditDefaultsOnly, Category=Effects)
	//TSubclassOf<UCameraShakeBase> FireCameraShake;

	/** force feedback effect to play when the weapon is fired */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	UForceFeedbackEffect* FireForceFeedback;

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

	/** is fire sound looped? */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	uint32 bLoopedFireSound : 1;
	/** firing audio (bLoopedFireSound set) */
	UPROPERTY(Transient)
	UAudioComponent* FireAC;

	/** play weapon sounds */
	UAudioComponent* PlayWeaponSound(USoundCue* Sound);
	/*Support pooling */
	UAudioComponent* PlayWeaponSound(USoundCue* Sound, UAudioComponent* ACin);

	bool ShouldDealDamage(AActor* InActor) const;
	bool ClientShouldDealDamage(AActor* InActor) const;
	UPROPERTY()
	AActor* TargetActor;


	UFUNCTION(BlueprintCallable)
	void UpdateMainWeaponHUD(bool bIsReloading = false);
	/*for auto fire*/
	bool bStableAim;
	bool bAutoShooting;
	bool bManualShooting;
	


	//AI
	//Called by AI WeaponManager
public:
	bool bIsAIAllowedToFire;
	void UpdateAI();
	//did we increase timers(Reload ETC)?
	bool TimersUpdated = false;
	void RegisterIsOwnedByAI();

	
	APawn* GetOwningPawn();
	UMeshComponent* GetMesh();
	UPROPERTY(BlueprintReadOnly)
	float CurrentHeat;

	UPROPERTY(EditAnywhere,Category=Setup)
	FVector2D AimLocationOnScreen=FVector2D(2,3);
	FVector CalculateAimDirection() const;

	UPROPERTY(Replicated)
	FVector AimDirection;

	
	void SetAimDirection(FVector In);
	UFUNCTION(Server,Reliable)
	void ServerSetAimDirection(FVector In);

private:
	UPROPERTY()
	UWidgetComponent* WidgetComponent;
};
