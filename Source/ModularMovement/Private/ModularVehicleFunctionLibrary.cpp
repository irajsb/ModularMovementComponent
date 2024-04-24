//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "ModularVehicleFunctionLibrary.h"

#include "ModularMovementComponent.h"
#include  "ModularWheel.h"
#include "Engine/Engine.h"
#include "Misc/MessageDialog.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Engine/World.h"

#include "Logging/MessageLog.h" 
#include "Kismet/KismetMathLibrary.h"
#include "PhysicsEngine/PhysicsSettings.h"

float UModularVehicleFunctionLibrary::GetEngineRpm(UModularMovementComponent* MovementComponent)
{
	if (MovementComponent)
	{
		return MovementComponent->VehicleState.CurrentRpm;
	}
	return 0.f;
}

float UModularVehicleFunctionLibrary::GetEngineRpmRatio(UModularMovementComponent* MovementComponent)
{
	if (MovementComponent)
	{
		return MovementComponent->GetRPMRatio();
	}
	return 0.f;
}

UModularGearBox* UModularVehicleFunctionLibrary::GetGearBox(const UModularMovementComponent* MovementComponent)
{
	if (MovementComponent)
	{
		if(MovementComponent->GetSetup())
		{
			return MovementComponent->GetSetup()->GetGearBox();
		}
	}
	return nullptr;
}


void UModularVehicleFunctionLibrary::SetThrottleInputOnModularVehicle(APawn* Pawn, float Throttle)
{
	if (Pawn)
	{
		if (UModularMovementComponent* MC = Cast<UModularMovementComponent>(Pawn->GetMovementComponent()))
		{
			MC->SetThrottleInput(Throttle);
		}
		
	}
}

void UModularVehicleFunctionLibrary::SetSteerInputOnModularVehicle(APawn* Pawn, float Steer)
{
	if (Pawn)
	{
		if (UModularMovementComponent* MC = Cast<UModularMovementComponent>(Pawn->GetMovementComponent()))
		{
			MC->SetSteeringInput(Steer);
		}
		
	}
}

void UModularVehicleFunctionLibrary::SetHandBrakeInputOnModularVehicle(APawn* Pawn, bool Brake)
{
	if (Pawn)
	{
		if (UModularMovementComponent* MC = Cast<UModularMovementComponent>(Pawn->GetMovementComponent()))
		{
			MC->SetHandBrakeInput(Brake);
		}
		
	}
}


int UModularVehicleFunctionLibrary::GetForwardSpeedKMH(UModularMovementComponent* MovementComponent)
{
	return static_cast<int>(MovementComponent->VehicleState.ForwardSpeed * 0.036);
}

int UModularVehicleFunctionLibrary::GetForwardSpeedMPH(UModularMovementComponent* MovementComponent)
{
	return static_cast<int>(MovementComponent->VehicleState.ForwardSpeed * 0.0223694);
}

float UModularVehicleFunctionLibrary::GetForwardSpeedCMs(UModularMovementComponent* MovementComponent)
{
	return MovementComponent->VehicleState.ForwardSpeed;
}

int UModularVehicleFunctionLibrary::GetWheelsTouchingGround(UModularMovementComponent* MovementComponent)
{
	int WheelCount = 0;
	for (const UModularWheel* Wheel : MovementComponent->Components)
	{
		if (Wheel->WheelState.HitResult.bBlockingHit)
		{
			WheelCount++;
		}
	}
	return WheelCount;
}


float UModularVehicleFunctionLibrary::CalculateSuspensionRotationUsingPivot(UModularWheel* Wheel)
{
	//forms a triangle sum of angles =180
	if (!Wheel->GetWheelSetup())
	{
		return 0.0f;
	}
	float Result = 0;

	if (Wheel->GetWheelState()->WheelSetup->SuspensionPivot != 0)
	{
		const float PivotLen = Wheel->GetWheelState()->WheelSetup->SuspensionPivot;
		const float SuspLen = Wheel->GetWheelState()->WheelSetup->SuspensionLength;
		const float DeltaX = PivotLen;
		const float DeltaY = SuspLen;
		Result = 90 - FMath::RadiansToDegrees(FMath::Atan2(DeltaX, DeltaY));
	}


	return Result;
}

bool UModularVehicleFunctionLibrary::CapsuleTraceSingleWithRotation(UObject* WorldContextObject, const FVector Start,
                                                                    const FVector End, FRotator Rot, float Radius,
                                                                    float HalfHeight, ETraceTypeQuery TraceChannel,
                                                                    bool bTraceComplex,
                                                                    const TArray<AActor*>& ActorsToIgnore,
                                                                    EDrawDebugTrace::Type DrawDebugType,
                                                                    FHitResult& OutHit, bool bIgnoreSelf,
                                                                    FLinearColor TraceColor, FLinearColor TraceHitColor,
                                                                    float DrawTime)
{
	const ECollisionChannel CollisionChannel = UEngineTypes::ConvertToCollisionChannel(TraceChannel);

	static const FName CapsuleTraceSingleName(TEXT("CapsuleTraceSingle"));
	const FCollisionQueryParams Params = ConfigureCollisionParams(CapsuleTraceSingleName, bTraceComplex, ActorsToIgnore,
	                                                              bIgnoreSelf, WorldContextObject);

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	const bool bHit = World
		                  ? World->SweepSingleByChannel(OutHit, Start, End, Rot.Quaternion(), CollisionChannel,
		                                                FCollisionShape::MakeCapsule(Radius, HalfHeight), Params)
		                  : false;

#if ENABLE_DRAW_DEBUG
	//DrawDebugCapsuleTraceSingle(World, Start, End, Radius, HalfHeight, DrawDebugType, bHit, OutHit, TraceColor, TraceHitColor, DrawTime);
#endif

	return bHit;
}

FCollisionQueryParams UModularVehicleFunctionLibrary::ConfigureCollisionParams(FName TraceTag, bool bTraceComplex,
                                                                               const TArray<AActor*>& ActorsToIgnore,
                                                                               bool bIgnoreSelf,
                                                                               UObject* WorldContextObject)
{
	FCollisionQueryParams Params(TraceTag, SCENE_QUERY_STAT_ONLY(KismetTraceUtils), bTraceComplex);
	Params.bReturnPhysicalMaterial = true;
	Params.bReturnFaceIndex = !UPhysicsSettings::Get()->bSuppressFaceRemapTable;
	// Ask for face index, as long as we didn't disable globally
	Params.AddIgnoredActors(ActorsToIgnore);
	if (bIgnoreSelf)
	{
		if (const AActor* IgnoreActor = Cast<AActor>(WorldContextObject))
		{
			Params.AddIgnoredActor(IgnoreActor);
		}
		else
		{
			// find owner
			UObject* CurrentObject = WorldContextObject;
			while (CurrentObject)
			{
				CurrentObject = CurrentObject->GetOuter();
				IgnoreActor = Cast<AActor>(CurrentObject);
				if (IgnoreActor)
				{
					Params.AddIgnoredActor(IgnoreActor);
					break;
				}
			}
		}
	}

	return Params;
}

void UModularVehicleFunctionLibrary::UpdateWheelState(UModularWheel* Wheel, FWheelState NewWheelState)
{
	Wheel->UpdateWheelState(NewWheelState);
}




void UModularVehicleFunctionLibrary::DrawDebugSphereWithDepth(UObject* Context, FVector Location, float Size, int Depth)
{
	DrawDebugSphere(Context->GetWorld(), Location, Size, 20, FColor::White, false, -1.0f, Depth, 1.0f);
}

void UModularVehicleFunctionLibrary::GetWheelAnimationData(UModularWheel* Wheel,
                                                           FVector& Location, FRotator& Rotation, float DeltaTime)
{
	//TODO make sure to execute  this once per frame

	if (!Wheel)
	{
		return;
	}

	
	FWheelState& WheelState = *Wheel->GetWheelState();
	if (Wheel->GetWorld()->IsGameWorld() && WheelState.WheelSetup)
	{
		const FVector SuspensionTraceLocation = WheelState.HitResult.bBlockingHit
			                                        ? WheelState.HitResult.Location
			                                        : WheelState.HitResult.TraceEnd;


		
		const FTransform WheelTransform = Wheel->GetWheelTransform();


		float Sin, Cos;


		const FVector ContactPointPosition = WheelTransform.InverseTransformPosition(SuspensionTraceLocation);


		FVector ResultPosition = FVector::ZeroVector;
		ResultPosition.Z = ContactPointPosition.Z;


		if (WheelState.SuspAngle != 0.0f)
		{
			const float PivotAngle = FMath::Atan2(ResultPosition.Z, WheelState.WheelSetup->SuspensionPivot);
			FMath::SinCos(&Sin, &Cos, PivotAngle);
			ResultPosition.Y = FMath::Abs(Sin) * WheelState.WheelSetup->SuspensionPivot * FMath::Sign(
				WheelState.InitialLocalLocation.Y) * -0.5;
		}

		if (WheelState.WheelSetup->AnimSpeed != 0.0f)
		{
			ResultPosition = UKismetMathLibrary::VInterpTo_Constant(WheelState.PreviousLocation, ResultPosition,
			                                                        DeltaTime, WheelState.WheelSetup->AnimSpeed);
		}


		Location = ResultPosition;
		
		WheelState.PreviousLocation = ResultPosition;

		const float Steer = WheelState.SteerAngle;


		Rotation = FRotator(FMath::RadiansToDegrees(-1 * WheelState.AngularPosition), Steer, 0);


		if (WheelState.SuspAngle != 0.0f)
		{
			const float CurrentAngle = UKismetMathLibrary::MapRangeClamped(
				ResultPosition.Z, WheelState.WheelSetup->SuspensionLength, -WheelState.WheelSetup->SuspensionLength,
				-WheelState.SuspAngle, WheelState.SuspAngle);
			Rotation = UKismetMathLibrary::ComposeRotators(Rotation, FRotator(0.00f, 0.00f, -CurrentAngle));

			WheelState.CurrentPivotAngle = CurrentAngle;
		}
	}
}

void UModularVehicleFunctionLibrary::SetupWheelLocationFromBone(const USkeletalMeshComponent* Mesh, TArray<UTrackableComponent* >Wheels,
                                                                const FString& BoneNamePrefix)
{
	if(!Mesh)
	{
#if WITH_EDITOR
		FMessageDialog::Open(EAppMsgType::Ok,FText::FromString(TEXT("Mesh was not found function SetupWheelLocationFromBone")));
#endif
		
		return;
	}

	for (const auto Wheel:Wheels)
	{
		const FName BoneName=FName(BoneNamePrefix+Wheel->GetName());
		Wheel->SetWorldLocation(	Mesh->GetSocketLocation(BoneName));

		if(const auto WheelCast=Cast<UModularWheel>(Wheel))
		{
			WheelCast->OptionalBoneName=BoneName;
		}
	}
}

void UModularVehicleFunctionLibrary::NotifyError(FString Error)
{
	FMessageLog PIELogger = FMessageLog(FName("PIE"));
	PIELogger.Error(FText::FromString(Error));
}

void UModularVehicleFunctionLibrary::ChangeCollisionOnPhysicsBody(USkeletalMeshComponent* skeletalMesh, FName boneName,
	ECollisionEnabled::Type CollisionType)
{
	if(skeletalMesh)
	{
		if(const auto BI=skeletalMesh->GetBodyInstance(boneName))
		{
			BI->SetShapeCollisionEnabled(0, CollisionType);
		}
	}
}
