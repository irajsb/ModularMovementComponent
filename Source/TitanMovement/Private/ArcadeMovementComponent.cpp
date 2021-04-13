// Fill out your copyright notice in the Description page of Project Settings.


#include "ArcadeMovementComponent.h"

#include "ArcadePawn.h"
#include "ArcadeWheelInterface.h"
#include "TitanMovement.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine.h"
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
	Super::InitializeComponent();
	if(bDebugMode)
	{
	
	}
}

void UArcadeMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	ARCADE_CYCLE_COUNTER(STAT_ArcadeTickComponent)

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	UpdateEngine(DeltaTime);
	UpdateSuspension(DeltaTime);
}

void UArcadeMovementComponent::UpdateEngine(float DeltaTime)
{

	ARCADE_CYCLE_COUNTER(STAT_ArcadeEngine)
	 CurrentRpm=(GetGearInfo(CurrentGear).GearRatio*DifferentialRatio*(GetMesh()->GetPhysicsLinearVelocity()*FVector(1,1,0)).Size()/20)*30/PI;
}

void UArcadeMovementComponent::UpdateSuspension(float DeltaTime)
{
	ARCADE_CYCLE_COUNTER(STAT_ArcadeSuspension)

TArray<UActorComponent*> Components=	GetOwner()->GetComponentsByInterface(UArcadeWheelInterface::StaticClass());

	for(UActorComponent* Component: Components)
	{
		Cast<IArcadeWheelInterface>(Component)->UpdateSuspension(DeltaTime,this);
	}
}







void UArcadeMovementComponent::WheelTrace(UWorld* World,
                                          FArcadeWheelInfo& WheelInfo,float DeltaTime,USceneComponent* ArcadeWheel)
{

	
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());
	const FVector ComponentLocation=ArcadeWheel->GetComponentLocation()+WheelInfo.TraceStartOffset;
	FHitResult TraceResult;
	UKismetSystemLibrary::SphereTraceSingle(GetWorld(),ComponentLocation,ComponentLocation+(ArcadeWheel->GetUpVector()*-1*WheelInfo.SuspensionLength),WheelInfo.WheelRadius,SuspensionTraceTypeQuery,true,ActorsToIgnore,GArcadeVehicleDebugParams.ShowSuspensionDebug? EDrawDebugTrace::ForOneFrame:EDrawDebugTrace::None,TraceResult,true);

	const float CurrentLen=1-TraceResult.Time;
	const float DampingCorrection=((CurrentLen-WheelInfo.WheelState.PreviousLen)*DampingCorrectionMultiplier*WheelInfo.Stiffness);
	if(TraceResult.bBlockingHit)
	{
	
	GetMesh()->AddImpulseAtLocation(DeltaTime*60*(TraceResult.ImpactNormal*(WheelInfo.Stiffness+DampingCorrection)*(CurrentLen)),TraceResult.TraceStart);
	}
	
	WheelInfo.WheelState.PreviousLen=CurrentLen;
	//Debug
	#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	if(GArcadeVehicleDebugParams.ShowSuspensionDebug)
	{
		DrawDebugLine(GetWorld(),TraceResult.TraceStart,TraceResult.TraceStart+(DeltaTime*60*(TraceResult.ImpactNormal*(WheelInfo.Stiffness)*(CurrentLen)))/20,FColor::Red,false,-1,0,5);
		DrawDebugLine(GetWorld(),TraceResult.TraceStart+FVector(0,20,0),TraceResult.TraceStart+FVector(0,20,0)+(DeltaTime*60*(TraceResult.ImpactNormal*(WheelInfo.Stiffness+DampingCorrection)*(CurrentLen)))/20,FColor::Green,false,-1,0,5);
		DrawDebugLine(GetWorld(),TraceResult.TraceStart+FVector(0,-20,0),TraceResult.TraceStart+FVector(0,-20,0)+FVector(0,0,1)*DampingCorrection/20,FColor::Blue,false,-1,0,5);
		DrawDebugString(GetWorld(),TraceResult.TraceStart-FVector(-50,-50,0),TEXT("OrginalForce"),0,FColor::Red,0);
		DrawDebugString(GetWorld(),TraceResult.TraceStart-FVector(-50,-50,-25),TEXT("CorrectedForce"),0,FColor::Green,0);
		DrawDebugString(GetWorld(),TraceResult.TraceStart-FVector(-50,-50,-50),TEXT("DampingCorrection"),0,FColor::Blue,0);

	}


	#endif

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