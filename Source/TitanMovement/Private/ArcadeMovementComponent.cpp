// Fill out your copyright notice in the Description page of Project Settings.


#include "ArcadeMovementComponent.h"

#include "ArcadePawn.h"
#include "ArcadeWheelInterface.h"
#include "TitanMovement.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine.h"
#include "PhysicsEngine/PhysicsSettings.h"
FArcadeVehicleDebugParams GArcadeVehicleDebugParams;

DECLARE_CYCLE_STAT(TEXT("Arcade Tick Component"), STAT_ArcadeTickComponent, STATGROUP_MovementPhysics);
DECLARE_CYCLE_STAT(TEXT("Arcade Updage Engine"), STAT_ArcadeEngine, STATGROUP_MovementPhysics);\
DECLARE_CYCLE_STAT(TEXT("Arcade Updage Suspension"), STAT_ArcadeSuspension, STATGROUP_MovementPhysics);


static FAutoConsoleVariableRef CVarArcadeVehicleShowSuspensionDebug(
    TEXT("ArcadeVehicle.ShowSuspensionDebug"),
    GArcadeVehicleDebugParams.ShowSuspensionDebug,
    TEXT("Toggles Suspension Debugging visuals"));


#define LOCTEXT_NAMESPACE "ArcadeMovement"

UArcadeMovementComponent::UArcadeMovementComponent()
{


	SuspensionTraceTypeQuery= UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_Visibility);

	//reverse
	Gears.Add(FArcadeGearInfo(3));
	//idle
	Gears.Add(FArcadeGearInfo(0));
	//Forward
	Gears.Add(FArcadeGearInfo(3));
	Gears.Add(FArcadeGearInfo(2));
	Gears.Add(FArcadeGearInfo(1.55));
	Gears.Add(FArcadeGearInfo(1.33));
	Gears.Add(FArcadeGearInfo(1));

	AHUD::OnShowDebugInfo.AddUObject(this, &UArcadeMovementComponent::ShowDebugInfo);
}

UMeshComponent* UArcadeMovementComponent::GetMesh()
{
	return  Cast<UMeshComponent>(UpdatedComponent);
}

int UArcadeMovementComponent::GetNumberOfWheels()
{//TODo
	return 4;
}

FArcadeGearInfo UArcadeMovementComponent::GetGearInfo(int Index)
{
	if(Gears.IsValidIndex(Index))
	{
		return Gears[Index];
	}else
	{
		UE_LOG(LogArcadeVehicle,Error,TEXT("Wrong GearIndex"));
		FMessageLog("Blueprint").Warning(LOCTEXT("GearIndexNotValid", "Passed Gear Index was not valid"));
	}
const 	FArcadeGearInfo Gear(1);
	return  Gear;
}

void UArcadeMovementComponent::InitializeComponent()
{

	Components=	GetOwner()->GetComponentsByInterface(UArcadeWheelInterface::StaticClass());
	Super::InitializeComponent();
	
}

void UArcadeMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	ARCADE_CYCLE_COUNTER(STAT_ArcadeTickComponent)

	const float fDeltaTime=FMath::Min<float>(DeltaTime,0.0333);
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	UpdateEngine(fDeltaTime);
	UpdateSuspension(fDeltaTime);
	UpdateForces(fDeltaTime);
}

void UArcadeMovementComponent::UpdateEngine(float DeltaTime)
{

	ARCADE_CYCLE_COUNTER(STAT_ArcadeEngine)
	 CurrentRpm=(GetGearInfo(CurrentGear).GearRatio*DifferentialRatio*(GetMesh()->GetPhysicsLinearVelocity()*FVector(1,1,0)).Size()/20)*30/PI;
}

void UArcadeMovementComponent::UpdateSuspension(float DeltaTime)
{
	ARCADE_CYCLE_COUNTER(STAT_ArcadeSuspension)

	

	for(UActorComponent* Component: Components)
	{
		Cast<IArcadeWheelInterface>(Component)->UpdateSuspension(DeltaTime,this);
	}
}

void UArcadeMovementComponent::UpdateForces(float DeltaTime)
{

	//TODO Cycle stat




	for(UActorComponent* Component: Components)
	{
		Cast<IArcadeWheelInterface>(Component)->UpdateForces(DeltaTime,this);
	}

}


void UArcadeMovementComponent::WheelTrace(
                                          FWheelState& WheelState,float DeltaTime,USceneComponent* ArcadeWheel)
{

	//logic
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());
	const FVector ComponentLocation=ArcadeWheel->GetComponentLocation()+WheelState.WheelSetup.TraceStartOffset;
	FHitResult TraceResult;
	UKismetSystemLibrary::SphereTraceSingle(GetWorld(),ComponentLocation,ComponentLocation+(ArcadeWheel->GetUpVector()*-1*WheelState.WheelSetup.SuspensionLength),WheelState.WheelSetup.WheelRadius,SuspensionTraceTypeQuery,true,ActorsToIgnore,GArcadeVehicleDebugParams.ShowSuspensionDebug? EDrawDebugTrace::ForOneFrame:EDrawDebugTrace::None,TraceResult,true);

	const float CurrentLen=1-TraceResult.Time;
	const float DampingCorrection=((CurrentLen-WheelState.PreviousLen)*DampingCorrectionMultiplier*WheelState.WheelSetup.Stiffness);
	if(TraceResult.bBlockingHit)
	{
	WheelState.WheelLoad=DeltaTime*(FVector(0,0,1)*(WheelState.WheelSetup.Stiffness+DampingCorrection)*(CurrentLen));
	GetMesh()->AddForceAtLocation(WheelState.WheelLoad,TraceResult.TraceStart);
	
	}
	
	WheelState.PreviousLen=CurrentLen;
	WheelState.HitResult=TraceResult;
	//logic
	//Debug
	#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	if(GArcadeVehicleDebugParams.ShowSuspensionDebug)
	{
		DrawDebugLine(GetWorld(),TraceResult.TraceStart,TraceResult.TraceStart+(DeltaTime*60*(TraceResult.ImpactNormal*(WheelState.WheelSetup.Stiffness)*(CurrentLen)))/20,FColor::Red,false,-1,0,5);
		DrawDebugLine(GetWorld(),TraceResult.TraceStart+FVector(0,20,0),TraceResult.TraceStart+FVector(0,20,0)+(DeltaTime*60*(TraceResult.ImpactNormal*(WheelState.WheelSetup.Stiffness+DampingCorrection)*(CurrentLen)))/20,FColor::Green,false,-1,0,5);
		DrawDebugLine(GetWorld(),TraceResult.TraceStart+FVector(0,-20,0),TraceResult.TraceStart+FVector(0,-20,0)+FVector(0,0,1)*DampingCorrection/20,FColor::Blue,false,-1,0,5);
		DrawDebugString(GetWorld(),TraceResult.TraceStart-FVector(-50,-50,0),TEXT("OrginalForce"),0,FColor::Red,0);
		DrawDebugString(GetWorld(),TraceResult.TraceStart-FVector(-50,-50,-25),TEXT("CorrectedForce"),0,FColor::Green,0);
		DrawDebugString(GetWorld(),TraceResult.TraceStart-FVector(-50,-50,-50),TEXT("DampingCorrection"),0,FColor::Blue,0);

	}


	#endif

}

void UArcadeMovementComponent::ApplyWheelForces(FWheelState& WheelState, float DeltaTime,
    USceneComponent* ArcadeWheel)
{
	if(!WheelState.HitResult.bBlockingHit)
		return;

	const FTransform WorldTransform = GetMesh()->GetBodyInstance()->GetUnrealWorldTransform();
	const float SteerAngleDegrees = WheelState.SteerAngle; // temp
	const FRotator SteeringRotator(0.f, SteerAngleDegrees, 0.f);
	const FVector	LocalWheelVelocity = WorldTransform.InverseTransformVector(GetMesh()->GetPhysicsLinearVelocityAtPoint(ArcadeWheel->GetComponentLocation()));
	const FVector GroundVelocityVector = SteeringRotator.UnrotateVector(LocalWheelVelocity);
	const float SlipAngle = FMath::Atan2(GroundVelocityVector.Y, GroundVelocityVector.X);
	float FinalLongitudinalForce = 0.f;
	FVector ForceFromFriction;
	//EffectiveRadius
	const float Re=WheelState.WheelSetup.WheelRadius;
	const float MassPerWheel=(GetMesh()->GetMass()/GetNumberOfWheels());
	float 	AppliedLinearDriveForce = WheelState.DriveTorque / CmToM(Re);
	float AppliedLinearBrakeForce = 50000000000/*WheelState.BrakeTorque*/ / CmToM(Re);

		// currently just letting the brake override the throttle
		bool Braking = true;//WheelState.DriveTorque > FMath::Abs(WheelState.BrakeTorque);
		float BrakeFactor = 1.0f;
		float K = 1;//0.4f;
		
		// are we actually touching the ground
		if (WheelState.HitResult.bBlockingHit)
		{
		float	LongitudinalAdhesiveLimit = WheelState.WheelLoad.Size() * WheelState.HitResult.PhysMaterial.Get()->Friction * WheelState.WheelSetup.LongitudinalFrictionMultiplier;
		float	LateralAdhesiveLimit = WheelState.WheelLoad.Size() * WheelState.HitResult.PhysMaterial.Get()->Friction * WheelState.WheelSetup.LateralFrictionMultiplier;

			if (Braking)
			{

				// whether the velocity is +ve or -ve when we brake we are slowing the vehicle down
				// so force is opposing current direction of travel.
				float ForceRequiredToBringToStop = MassPerWheel * K* (GroundVelocityVector.X) / DeltaTime;
				FinalLongitudinalForce = AppliedLinearBrakeForce;

				// check we are not applying more force than required so we end up overshooting 
				// and accelerating in the opposite direction
				if (FinalLongitudinalForce > FMath::Abs(ForceRequiredToBringToStop))
				{
					FinalLongitudinalForce = FMath::Abs(ForceRequiredToBringToStop);
					UE_LOG(LogTemp,Error,TEXT("VelocityReplaced"));
				}

				// ensure the brake opposes current direction of travel
				if (GroundVelocityVector.X > 0.0f)
				{
					FinalLongitudinalForce = -FinalLongitudinalForce;
				}

			}
			else
			{
				FinalLongitudinalForce = AppliedLinearDriveForce;
			}

			// lateral grip
			float FinalLateralForce = -(MassPerWheel * K * GroundVelocityVector.Y) / DeltaTime;

			UE_LOG(LogTemp,Log,TEXT("%f"),FinalLongitudinalForce);
			ForceFromFriction.X = FinalLongitudinalForce;
			//UE_LOG(LogTemp,Error,TEXT("%s"),*ForceFromFriction.ToString())
			float DynamicFrictionLongitudialScaling = 0.75f;
			float TractionControlAndABSScaling = 0.98f;	// how close to perfection is the system working

			const float	SideSlipModifier = 1.0f;
				bool Locked = false;
			 WheelState.Spinning = false;
			
			// we can only obtain as much accel/decel force as the friction will allow
			if (FMath::Abs(FinalLongitudinalForce) > LongitudinalAdhesiveLimit)
			{
				if (Braking)
				{
					BrakeFactor = FMath::Clamp(LongitudinalAdhesiveLimit / FMath::Abs(FinalLongitudinalForce), 0.6f, 1.0f);
				}

				if ((Braking && WheelState.WheelSetup.ABSEnabled) || (!Braking && WheelState.WheelSetup.TractionControlEnabled))
				{
					WheelState.Spin = 0.0f;
					ForceFromFriction.X = LongitudinalAdhesiveLimit * TractionControlAndABSScaling;
				}
				else
				{
					if (!Braking)
					{
						WheelState.Spinning = true;
						WheelState.Spin += 0.5f * DeltaTime;
						WheelState.Spin = FMath::Clamp(WheelState.Spin, -2.f, 2.f);
					}
					else
					{
						Locked = true;
					}
					ForceFromFriction.X = LongitudinalAdhesiveLimit * DynamicFrictionLongitudialScaling;
					
				}
			}
			else
			{
				WheelState.Spin = 0.0f;
			}

			if (FinalLongitudinalForce < -LongitudinalAdhesiveLimit)
			{
				ForceFromFriction.X = -ForceFromFriction.X;
			}

			static float DynamicFrictionLateralScaling = 0.75f;
			if (Locked || WheelState.Spinning)
			{
				//SideSlipModifier *=1; //TODO Check thisWheelState.WheelSetup.SideSlipModifier;
			}

			// Lateral needs more grip to feel right!
			LateralAdhesiveLimit *= 1.0f * SideSlipModifier;
			ForceFromFriction.Y = FinalLateralForce;
			if (FMath::Abs(FinalLateralForce) > LateralAdhesiveLimit)
			{
				ForceFromFriction.Y = LateralAdhesiveLimit * DynamicFrictionLateralScaling;
			}

			if (FinalLateralForce < -LateralAdhesiveLimit)
			{
				ForceFromFriction.Y = -ForceFromFriction.Y;
			}


			// wheel rolling - just match the ground speed exactly
			if (BrakeFactor < 1.0f)
			{
				WheelState.Omega *= BrakeFactor;
			}
			else if (WheelState.Spin > 0.1f)
			{
				WheelState.Omega += WheelState.Spin;
			}
			else
			{
				float GroundOmega = GroundVelocityVector.X / Re;
				WheelState.Omega += (GroundOmega - WheelState.Omega);
			}

		}
		// Wheel angular position
		WheelState.AngularPosition += WheelState.Omega * DeltaTime;

		while (WheelState.AngularPosition >= PI * 2.f)
		{
			WheelState.AngularPosition -= PI * 2.f;
		}
		while (WheelState.AngularPosition <= -PI * 2.f)
		{
			WheelState.AngularPosition += PI * 2.f;
		}

		if (!WheelState.HitResult.bBlockingHit)
		{
			ForceFromFriction = FVector::ZeroVector;
			return;
		}

		
	


	float RotationAngle = FMath::RadiansToDegrees(WheelState.AngularPosition);
	FVector FrictionForceLocal = ForceFromFriction;
    FrictionForceLocal = SteeringRotator.RotateVector(FrictionForceLocal);

	FVector GroundZVector = WheelState.HitResult.ImpactNormal;
	FVector GroundXVector = FVector::CrossProduct(GetMesh()->GetRightVector(), GroundZVector);
	FVector GroundYVector = FVector::CrossProduct(GroundZVector, GroundXVector);
	FMatrix Mat(GroundXVector, GroundYVector, GroundZVector, GetMesh()->GetComponentLocation());
	FVector FrictionForceVector = Mat.TransformVector(FrictionForceLocal);

	GetMesh()->AddForceAtLocation(FrictionForceVector,ArcadeWheel->GetComponentLocation());
	UE_LOG(LogTemp,Error,TEXT("Friction %s"),*FrictionForceLocal.ToString());
	DrawDebugLine(GetWorld(),WheelState.HitResult.ImpactPoint,WheelState.HitResult.ImpactPoint+GroundVelocityVector,FColor::Red,false,-1,0,15);
	DrawDebugLine(GetWorld(),WheelState.HitResult.ImpactPoint,WheelState.HitResult.ImpactPoint+FrictionForceLocal,FColor::Green,false,-1,0,15);

}

float UArcadeMovementComponent::CmToM(float In)
{
	return  In*100;
}

//debug
void UArcadeMovementComponent::ShowDebugInfo(AHUD* HUD, UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo,
    float& YL, float& YPos)
{
	
	if (Canvas && HUD)
	{
		static FName NAME_Vehicle = FName(TEXT("Vehicle"));
		
		if(GetPawnOwner())
		{
			if(APlayerController* PlayerController= Cast<APlayerController>(GetPawnOwner()->GetController()))
			{
				
				if ( HUD->ShouldDisplayDebug(NAME_Vehicle))
				{
					
					DrawDebug(Canvas, YL, YPos);
				}
			}
			
		}
		
		
	}
}

void UArcadeMovementComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	UFont* RenderFont = GEngine->GetLargeFont();
	float X, Y;
	Canvas->GetCenter(X, Y);
	float YLine = Y * 2.f - 50.f;
	float Scaling = 2.f;
	//Canvas->DrawText(RenderFont, FString::Printf(TEXT("%d mph"), (int)ForwardSpeedMPH), X-100, YLine, Scaling, Scaling);
	Canvas->DrawText(RenderFont, FString::Printf(TEXT("[%d]"), (int)CurrentGear), X, YLine, Scaling, Scaling);
	Canvas->DrawText(RenderFont, FString::Printf(TEXT("%d rpm"), (int)CurrentRpm), X+50, YLine, Scaling, Scaling);
	
}



#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

float UArcadeMovementComponent::CalcDialAngle(float CurrentValue, float MaxValue)
{
	return (CurrentValue / MaxValue) * 3.f / 2.f * PI - (PI * 0.25f);
}

void UArcadeMovementComponent::DrawDial(UCanvas* Canvas, FVector2D Pos, float Radius, float CurrentValue, float MaxValue)
{
	float Angle = CalcDialAngle(CurrentValue, MaxValue);
	FVector2D PtEnd(Pos.X - FMath::Cos(Angle) * Radius, Pos.Y - FMath::Sin(Angle) * Radius);
	DrawLine2D(Canvas, Pos, PtEnd, FColor::White, 3.f);

	for (float I = 0; I < MaxValue; I += 1000.0f)
	{
		Angle = CalcDialAngle(I, MaxValue);
		PtEnd.Set(-FMath::Cos(Angle) * Radius, -FMath::Sin(Angle) * Radius);
		FVector2D PtStart = PtEnd * 0.8f;
		DrawLine2D(Canvas, Pos + PtStart, Pos + PtEnd, FColor::White, 2.f);
	}

	// the last checkmark
	Angle = CalcDialAngle(MaxValue, MaxValue);
	PtEnd.Set(-FMath::Cos(Angle) * Radius, -FMath::Sin(Angle) * Radius);
	FVector2D PtStart = PtEnd * 0.8f;
	DrawLine2D(Canvas, Pos+PtStart, Pos+PtEnd, FColor::Red, 2.f);

}


void UArcadeMovementComponent::DrawLine2D(UCanvas* Canvas, const FVector2D& StartPos, const FVector2D& EndPos, FColor Color, float Thickness)
{
	if (Canvas)
	{
		FCanvasLineItem LineItem(StartPos, EndPos);
		LineItem.SetColor(Color);
		LineItem.LineThickness = Thickness;
		Canvas->DrawItem(LineItem);
	}
}

#endif




#undef LOCTEXT_NAMESPACE