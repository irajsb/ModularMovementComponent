//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "VehicleWeaponComponent.h"




#include "Blueprint/WidgetLayoutLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"


// Sets default values for this component's properties
UVehicleWeaponComponent::UVehicleWeaponComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true;
	bWantsInitializeComponent = true;

	CurrentState = EWeaponState::Idle;

	CurrentAmmo = 0;
	CurrentAmmoInClip = 0;
	BurstCounter = 0;
	LastFireTime = 0.0f;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	SetIsReplicatedByDefault(true);
}

void UVehicleWeaponComponent::SetAimLocationOnScreen(FVector2D In)
{
	AimLocationOnScreen=In;

	if(GetOwnerRole()<ROLE_Authority)
	{
		ServerSetAimLocationOnScreen(In);
	}
}

void UVehicleWeaponComponent::ServerSetAimLocationOnScreen_Implementation(FVector2D In)
{
	SetAimLocationOnScreen(In);
}


// Called when the game starts
void UVehicleWeaponComponent::BeginPlay()
{
	Super::BeginPlay();


	CurrentTimeBetweenShots = WeaponConfig.InitialTimeBetweenShots;


	CurrentAmmoInClip = WeaponConfig.ClipSize;
	CurrentAmmo = WeaponConfig.AmmoCount;

	if (WeaponConfig.AimWidgetClass)
	{
		if (GetNetMode() == NM_Standalone || GetOwnerRole() == ROLE_AutonomousProxy)
		{
			if (const auto Comp = GetOwner()->AddComponentByClass(UWidgetComponent::StaticClass(), false, FTransform(),
			                                                      false))
			{
				WidgetComponent = Cast<UWidgetComponent>(Comp);
				WidgetComponent->SetWidgetClass(WeaponConfig.AimWidgetClass);
				WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
				WidgetComponent->SetDrawSize(WeaponConfig.AimWidgetSize);
			}
		}
	}
}


// Called every frame
void UVehicleWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                            FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CurrentHeat = FMath::Clamp<float>(CurrentHeat - WeaponConfig.HeatReduce * DeltaTime, 0, 1);

	// Calculate direction from the world
	if ((GetOwner()->GetLocalRole() == ROLE_Authority) || GetOwningPawn()->IsLocallyControlled())
	{
		if (const auto PC = Cast<APlayerController>(GetOwningPawn()->GetController()))
		{
			if (GetNetMode() == NM_Standalone || GetOwnerRole() < ROLE_Authority)
			{
				ServerSetControlRotation(GetWorld()->GetFirstPlayerController()->GetControlRotation().Vector());
			}
			SetAimDirection(CalculateAimDirection(PC));

			FHitResult HitResult;
			const FVector CamLoc = OverrideAimCamera
				                       ? OverrideAimCamera->GetComponentLocation()
				                       : PC->PlayerCameraManager->GetCameraLocation();
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(GetOwner());
			GetWorld()->LineTraceSingleByChannel(HitResult, CamLoc, CamLoc + AimDirection * 1000000.0,
			                                     WeaponConfig.Channel, Params);
			//Camera aim loc
			TargetLocation = HitResult.bBlockingHit ? HitResult.ImpactPoint : HitResult.TraceEnd;


			UpdateAnim(DeltaTime);
			CurrentWeaponRotationWorldSpace = GetOwner()->GetActorRotation().RotateVector(
				CurrentWeaponRotation.Quaternion().Vector()).Rotation();

			const FVector ComponentLoc = GetComponentLocation();
			GetWorld()->LineTraceSingleByChannel(HitResult, ComponentLoc,
			                                     ComponentLoc + CurrentWeaponRotationWorldSpace.Vector() * 1000000.0,
			                                     WeaponConfig.Channel, Params);
			TargetLocation = HitResult.bBlockingHit ? HitResult.ImpactPoint : HitResult.TraceEnd;

			if (WidgetComponent)
			{
				WidgetComponent->SetWorldLocation(TargetLocation);
			}
		}
	}
	else
	{
		CurrentWeaponRotationWorldSpace = GetOwner()->GetActorRotation().RotateVector(
			CurrentWeaponRotation.Quaternion().Vector()).Rotation();
	}
}

FHitResult UVehicleWeaponComponent::WeaponTrace(const FVector& TraceFrom, const FVector& TraceTo) const
{
	FCollisionQueryParams TraceParams(TEXT("WeaponTrace"), true, GetOwner());
	TraceParams.bReturnPhysicalMaterial = true;

	FHitResult Hit(ForceInit);
	GetWorld()->LineTraceSingleByChannel(Hit, TraceFrom, TraceTo, ECC_Visibility, TraceParams);

	return Hit;
}


void UVehicleWeaponComponent::StartFire(bool bAutoFire)
{
	if (GetOwnerRole() < ROLE_Authority)
	{
		ServerStartFire();
	}

	if (!bWantsToFire)
	{
		bWantsToFire = true;
		DetermineWeaponState();
	}
}

void UVehicleWeaponComponent::StopFire(bool bAutoFire)
{
	if (bManualShooting && bAutoFire)
	{
		return;
	}
	//if(!bManualShootStopped&&bAutoFire==true)
	//return;
	if ((GetOwner()->GetLocalRole() < ROLE_Authority) && GetOwner() && GetOwningPawn()->IsLocallyControlled())
	{
		ServerStopFire();
	}

	if (bWantsToFire)
	{
		bWantsToFire = false;
		DetermineWeaponState();
	}
	bAutoShooting = false;
	bManualShooting = false;
}

bool UVehicleWeaponComponent::ServerStartFire_Validate()
{
	return true;
}

void UVehicleWeaponComponent::ServerStartFire_Implementation()
{
	StartFire(false);
}

bool UVehicleWeaponComponent::ServerStopFire_Validate()
{
	return true;
}

void UVehicleWeaponComponent::ServerStopFire_Implementation()
{
	StopFire(false);
}

void UVehicleWeaponComponent::DetermineWeaponState()
{
	EWeaponState::Type NewState = EWeaponState::Idle;


	if (bPendingReload)
	{
		if (CanReload() == false)
		{
			NewState = CurrentState;
		}
		else
		{
			NewState = EWeaponState::Reloading;
		}
	}
	else if ((bPendingReload == false) && (bWantsToFire == true) && (CanFire() == true))
	{
		NewState = EWeaponState::Firing;
	}


	SetWeaponState(NewState);
}

bool UVehicleWeaponComponent::CanReload() const
{
	return CurrentState != EWeaponState::Reloading;
}

bool UVehicleWeaponComponent::CanFire()
{
	return true;
}

void UVehicleWeaponComponent::SetWeaponState(EWeaponState::Type NewState)
{
	const EWeaponState::Type PrevState = CurrentState;

	if (PrevState == EWeaponState::Firing && NewState != EWeaponState::Firing)
	{
		OnBurstFinished();
	}

	CurrentState = NewState;

	if (PrevState != EWeaponState::Firing && NewState == EWeaponState::Firing)
	{
		OnBurstStarted();
	}
}

void UVehicleWeaponComponent::OnBurstStarted()
{
	/// start firing, can be delayed to satisfy TimeBetweenShots
	const float GameTime = GetWorld()->GetTimeSeconds();
	if (LastFireTime > 0 && WeaponConfig.InitialTimeBetweenShots > 0.0f &&
		LastFireTime + WeaponConfig.InitialTimeBetweenShots > GameTime)
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_HandleFiring, this, &UVehicleWeaponComponent::HandleFiring,
		                                       LastFireTime + WeaponConfig.InitialTimeBetweenShots - GameTime, false);
	}
	else
	{
	
		HandleFiring();
	}
}

void UVehicleWeaponComponent::OnBurstFinished()
{
	// stop firing FX on remote clients
	BurstCounter = 0;

	// stop firing FX locally, unless it's a dedicated server
	//if (GetNetMode() != NM_DedicatedServer)
	//{
	StopSimulatingWeaponFire();
	//}

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_HandleFiring);
	bRefiring = false;

	// reset firing interval adjustment
	TimerIntervalAdjustment = 0.0f;
}


void UVehicleWeaponComponent::HandleFiring()
{
	if ((CurrentAmmoInClip > 0 || HasInfiniteClip() || HasInfiniteAmmo()) && CanFire())
	{
		if (GetNetMode() != NM_DedicatedServer)
		{
			SimulateWeaponFire();
		}

		if (GetOwner())
		{
			HandleFire.Broadcast();
			if(  GetOwningPawn()->IsLocallyControlled())
			{
				ApplyRecoil();
				UseAmmo();
			}
			
			// update firing FX on remote clients if function was called on server
			BurstCounter++;
		}
	}
	else if (CanReload())
	{
		StartReload();
	}
	else if (GetOwner() && GetOwningPawn()->IsLocallyControlled())
	{
		if (GetCurrentAmmo() == 0 && !bRefiring)
		{
			//Use here as play sounds for out of ammo 
		}

		// stop weapon fire FX, but stay in Firing state
		if (BurstCounter > 0)
		{
			OnBurstFinished();
		}
	}
	else
	{
		OnBurstFinished();
	}

	if (GetOwner() && GetOwningPawn()->IsLocallyControlled())
	{
		// local client will notify server
		if (GetOwner()->GetLocalRole() < ROLE_Authority)
		{
			ServerHandleFiring();
		}

		// reload after firing last round
		if (CurrentAmmoInClip <= 0 && CanReload())
		{
			StartReload();
		}

		// setup refire timer
		
		bRefiring = (CurrentState == EWeaponState::Firing && WeaponConfig.InitialTimeBetweenShots > 0.0f);
		if (bRefiring)
		{
			GetWorld()->GetTimerManager().SetTimer(TimerHandle_HandleFiring, this, &UVehicleWeaponComponent::HandleReFiring, FMath::Max<float>(WeaponConfig.InitialTimeBetweenShots + TimerIntervalAdjustment, SMALL_NUMBER), false);
			TimerIntervalAdjustment = 0.f;
		}
	}

	LastFireTime = GetWorld()->GetTimeSeconds();
}

bool UVehicleWeaponComponent::HasInfiniteAmmo() const
{
	return WeaponConfig.bInfiniteAmmo;
}

bool UVehicleWeaponComponent::HasInfiniteClip() const
{
	return WeaponConfig.bInfiniteClip;
}


void UVehicleWeaponComponent::SimulateWeaponFire()
{
	if (GetOwner()->GetLocalRole() == ROLE_Authority && CurrentState != EWeaponState::Firing)
	{
		return;
	}

	CurrentHeat = CurrentHeat = FMath::Clamp<float>(CurrentHeat + WeaponConfig.HeatPerShot, 0, 1);
	if (MuzzleFX)
	{
		if (/*!bLoopedMuzzleFX ||*/ MuzzlePSC == nullptr)
		{
			if ((GetOwner() != nullptr))
			{
				MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, this,TEXT("Muzzle"));
				MuzzlePSC->bAutoDestroy = false;
			}
		}
		else
		{
			MuzzlePSC->Activate(true);
		}
	}
	if (MuzzleWorldSpaceFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleWorldSpaceFX, GetComponentLocation(),
		                                         GetComponentRotation(), WorldSpaceParticleScale);
	}
	if (!bPlayingFireAnim)
	{
		bPlayingFireAnim = true;
		FireStateChange.Broadcast(true);
	}

	if (bLoopedFireSound)
	{
		if (!FireAC)
		{
			FireAC = PlayWeaponSound(FireLoopSound);
		}
		else if (!FireAC->IsPlaying())
		{
			PlayWeaponSound(FireLoopSound, FireAC);
		}
	}
	else
	{
		FireAC = PlayWeaponSound(FireSound, FireAC);
	}


	//Play camera shake

	if (APlayerController* PC = Cast<APlayerController>(GetOwningPawn()->GetController()))
	{
		PC->ClientStartCameraShake(Shake);
		
	}
	
}

void UVehicleWeaponComponent::StopSimulatingWeaponFire()
{
	if (bLoopedMuzzleFX)
	{
		if (MuzzlePSC != nullptr)
		{
			MuzzlePSC->DeactivateSystem();
			//MuzzlePSC = NULL;
		}
	}

	if (bPlayingFireAnim)
	{
		bPlayingFireAnim = false;
		FireStateChange.Broadcast(false);
	}

	if (FireAC)
	{
		if (bLoopedFireSound)
		{
			FireAC->FadeOut(0.1f, 0.0f);
			FireAC->Stop();
		}
		//FireAC = NULL;

		PlayWeaponSound(FireFinishSound);
	}
}


void UVehicleWeaponComponent::UseAmmo()
{
	if (!HasInfiniteAmmo())
	{
		CurrentAmmoInClip--;
	}

	if (!HasInfiniteAmmo() && !HasInfiniteClip())
	{
		CurrentAmmo--;
	}

	
	UpdateMainWeaponHUD();
}


void UVehicleWeaponComponent::StartReload(bool bFromReplication)
{
	
	if (!bFromReplication && GetOwner()->GetLocalRole() < ROLE_Authority)
	{
		ServerStartReload();
	}

	if (bFromReplication || CanReload())
	{
		bPendingReload = true;
		DetermineWeaponState();

		//play Reload Anim
		ReloadStateChange.Broadcast(true);

		GetWorld()->GetTimerManager().SetTimer(TimerHandle_StopReload, this, &UVehicleWeaponComponent::StopReload,
		                                       WeaponConfig.ReloadTime, false);
		if (GetOwner()->GetLocalRole() == ROLE_Authority)
		{
			GetWorld()->GetTimerManager().SetTimer(TimerHandle_ReloadWeapon, this,
			                                       &UVehicleWeaponComponent::ReloadWeapon,
			                                       FMath::Max(0.3f, WeaponConfig.ReloadTime - 0.3f), false);
		}

		if (GetOwner() && GetOwningPawn()->IsLocallyControlled())
		{
			PlayWeaponSound(ReloadSound);

			UpdateMainWeaponHUD(true);
		}
	}
}


void UVehicleWeaponComponent::StopReload()
{
	UpdateMainWeaponHUD();
	if (CurrentState == EWeaponState::Reloading)
	{
		bPendingReload = false;
		DetermineWeaponState();
		ReloadStateChange.Broadcast(false);
	}
}

void UVehicleWeaponComponent::ReloadWeapon()
{
	CurrentAmmoInClip = WeaponConfig.ClipSize;
	CurrentAmmo -= WeaponConfig.ClipSize;
}


bool UVehicleWeaponComponent::ServerStartReload_Validate()
{
	return true;
}

void UVehicleWeaponComponent::ServerStartReload_Implementation()
{
	StartReload();
}

bool UVehicleWeaponComponent::ServerHandleFiring_Validate()
{
	return true;
}

void UVehicleWeaponComponent::ServerHandleFiring_Implementation()
{
	//HandleFiring();

	if ((CurrentAmmoInClip > 0 && CanFire() || WeaponConfig.bInfiniteClip))
	{
		// update ammo
		UseAmmo();

		// update firing FX on remote clients
		BurstCounter++;
	}
}


void UVehicleWeaponComponent::HandleReFiring()
{
	// Update TimerIntervalAdjustment
	const UWorld* MyWorld = GetWorld();

	const float SlackTimeThisFrame = FMath::Max(0.0f, (MyWorld->TimeSeconds - LastFireTime) - CurrentTimeBetweenShots);

	if (bAllowAutomaticWeaponCatchup)
	{
		TimerIntervalAdjustment -= SlackTimeThisFrame;
	}
	
	HandleFiring();
}

void UVehicleWeaponComponent::SetInstantRotation(bool Input)
{
	WeaponConfig.InstantRotation = Input;
}

int32 UVehicleWeaponComponent::GetCurrentAmmo() const
{
	return CurrentAmmo;
}

void UVehicleWeaponComponent::UpdateAnim(float DeltaTime)
{
	if (!GetOwner())
	{
		return;
	}

	FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(GetComponentLocation(), TargetLocation);
	TargetRotation = GetOwner()->GetActorRotation().UnrotateVector(TargetRotation.Vector()).Rotation();
	TargetRotation.Pitch = FMath::ClampAngle(TargetRotation.Pitch, WeaponConfig.MinPitch, WeaponConfig.MaxPitch);

	if (WeaponConfig.InstantRotation)
	{
		CurrentWeaponRotation = TargetRotation;
		bStableAim = true;
	}
	else
	{
		CurrentWeaponRotation = UKismetMathLibrary::RInterpTo_Constant(CurrentWeaponRotation, TargetRotation, DeltaTime,
		                                                               WeaponConfig.RotInterpolationSpeed);
		if (UKismetMathLibrary::EqualEqual_RotatorRotator(TargetRotation, CurrentWeaponRotation, 1))
		{
			bStableAim = true;
		}
		else
		{
			bStableAim = false;
		}
	}
}

void UVehicleWeaponComponent::OnRep_BurstCounter()
{
	
	if (BurstCounter > 0)
	{
		SimulateWeaponFire();
	}
	else
	{
		StopSimulatingWeaponFire();
	}
}

void UVehicleWeaponComponent::OnRep_Reload()
{
	if (bPendingReload)
	{
		StartReload(true);
	}
	else
	{
		StopReload();
	}
}


void UVehicleWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME( UVehicleWeaponComponent, MyPawn );

	DOREPLIFETIME_CONDITION(UVehicleWeaponComponent, CurrentAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UVehicleWeaponComponent, CurrentAmmoInClip, COND_OwnerOnly);

	DOREPLIFETIME_CONDITION(UVehicleWeaponComponent, BurstCounter, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(UVehicleWeaponComponent, CurrentWeaponRotation, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(UVehicleWeaponComponent, bPendingReload, COND_SkipOwner);
}


UAudioComponent* UVehicleWeaponComponent::PlayWeaponSound(USoundCue* Sound)
{
	//todo create spawn sound at world for AI 
	if (bLoopedFireSound)
	{
		UAudioComponent* AC = nullptr;
		if (Sound && GetOwner())
		{
			AC = UGameplayStatics::SpawnSoundAttached(Sound, this,TEXT("Muzzle"));
		}

		return AC;
	}
	if (UAudioComponent* AC = UGameplayStatics::SpawnSoundAttached(Sound, this))
	{
		AC->SetWorldLocation(GetComponentLocation());
	}
	return nullptr;
}

UAudioComponent* UVehicleWeaponComponent::PlayWeaponSound(USoundCue* Sound, UAudioComponent* ACin)
{
	if (!bLoopedFireSound)
	{
		return PlayWeaponSound(Sound);
	}
	if (!ACin)
	{
		if (Sound && GetOwner())
		{
			ACin = UGameplayStatics::SpawnSoundAttached(Sound, this,TEXT("Muzzle"));
			ACin->bAutoDestroy = false;
		}

		return ACin;
	}
	ACin->Play();
	return ACin;
}

bool UVehicleWeaponComponent::ShouldDealDamage(const AActor* InActor) const
{
	//TODO Implement Gamemode DealDamage check
	// if we're an actor on the server, or the actor's role is authoritative, we should register damage
	if (InActor == GetOwner())
	{
		return false;
	}
	if (InActor)
	{
		if (GetNetMode() != NM_Client ||
			InActor->GetLocalRole() == ROLE_Authority ||
			InActor->GetTearOff())
		{
			return true;
		}
	}

	return false;
}


bool UVehicleWeaponComponent::ClientShouldDealDamage(const AActor* InActor) const
{
	if (InActor == GetOwner())
	{
		return false;
	}
	//TODO Implement Gamemode DealDamage check
	return true;
}


void UVehicleWeaponComponent::UpdateMainWeaponHUD(bool bIsReloading)
{
	if (GetOwnerRole() == ROLE_AutonomousProxy || GetNetMode() == NM_Standalone)
	{
		if (bIsReloading)
		{
			OnReload.Broadcast();
		}


		if (!WeaponConfig.bInfiniteAmmo)
		{
			NotifyAmmo.Broadcast(CurrentAmmoInClip, WeaponConfig.ClipSize);
		}
		else
		{
			NotifyAmmo.Broadcast(CurrentAmmoInClip, 0);
		}
	}
}


void UVehicleWeaponComponent::UpdateAI()
{
	if (bIsAIAllowedToFire)
	{
		if (bStableAim)
		{
			StartFire(false);
		}
		else
		{
			StopFire(false);
		}
	}
	else
	{
		bStableAim = false;
		StopFire(false);
	}
}

void UVehicleWeaponComponent::RegisterIsOwnedByAI()
{
	WeaponConfig.IsOwnedByAI = true;
	if (!TimersUpdated)
	{
		WeaponConfig.ReloadTime = WeaponConfig.ReloadTime * WeaponConfig.AITimerMultiplier;
		WeaponConfig.InitialTimeBetweenShots = WeaponConfig.InitialTimeBetweenShots * WeaponConfig.AITimerMultiplier;
		WeaponConfig.InitialTimeBetweenShots = WeaponConfig.AITimerMultiplier;
		TimersUpdated = true;
	}
}

void UVehicleWeaponComponent::ApplyRecoil() const
{
	if (WeaponConfig.WeaponRecoil > 0)
	{
		const FVector ImpulseVector = UKismetMathLibrary::Vector_SlerpVectorToDirection(
			CurrentWeaponRotationWorldSpace.Vector(), FVector::DownVector, 0.5);
		GetMesh()->AddImpulseAtLocation(
			ImpulseVector * -1 * WeaponConfig.WeaponRecoil, GetComponentLocation());
	}
}

APawn* UVehicleWeaponComponent::GetOwningPawn() const
{
	APawn* Result = Cast<APawn>(GetOwner());
	return Result;
}

UMeshComponent* UVehicleWeaponComponent::GetMesh() const
{
	return Cast<UMeshComponent>(GetOwner()->GetRootComponent());
}

FVector UVehicleWeaponComponent::CalculateAimDirection(const APlayerController* PC) const
{
	FVector WorldDirection;
	if (OverrideAimCamera)
	{
	
	
		WorldDirection = OverrideAimCamera->GetForwardVector();
	}
	else
	{
		FVector WorldPos;
		
		UGameplayStatics::DeprojectScreenToWorld(PC,UWidgetLayoutLibrary::GetViewportSize(GetWorld())/AimLocationOnScreen,WorldPos,WorldDirection);
		
	}
	return WorldDirection;
}

void UVehicleWeaponComponent::SetAimDirection(const FVector& In)
{
	AimDirection = In;
}



void UVehicleWeaponComponent::ServerSetControlRotation_Implementation(FVector In)
{
	if (GetOwningPawn())
	{
		if (APlayerController* PC = Cast<APlayerController>(GetOwningPawn()->GetController()))
		{
			PC->SetControlRotation(In.Rotation());
		}
	}
}
