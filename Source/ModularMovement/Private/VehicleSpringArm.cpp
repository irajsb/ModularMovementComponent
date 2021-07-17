// Fill out your copyright notice in the Description page of Project Settings.


#include "VehicleSpringArm.h"


#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "Kismet/KismetMathLibrary.h"




 
void UVehicleSpringArm::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{


	
	
	if(CoolDown<0.f)
	{
		if (APawn* OwningPawn = Cast<APawn>(GetOwner()))
		{
			const FRotator CurrentRot=bUsePawnControlRotation?	OwningPawn->GetViewRotation():GetComponentRotation();
			UMeshComponent* Mesh=Cast<UMeshComponent>(GetOwner()->GetRootComponent());
			if(Mesh)
			{
				const FVector Velocity=Mesh->GetPhysicsLinearVelocity()*FVector(1,1,0);
				if(Velocity.Size()>MinSpeedRange)
				{
					const float Interpolation=UKismetMathLibrary::MapRangeClamped(Velocity.Size(),MinSpeedRange,MaxSpeedRange,0,1)*InterpolationStrength;
					FRotator Result=UKismetMathLibrary::RInterpTo_Constant(CurrentRot,Velocity.Rotation(),DeltaTime,Interpolation);
					if(IgnorePitch)
					{
						Result.Pitch=CurrentRot.Pitch;
					}
					if(bUsePawnControlRotation)
					{
						OwningPawn->GetController()->SetControlRotation(Result);
					}else
					{
						SetWorldRotation(Result);
					}
			
				}
		
			}

		
		}
	}else
	{
		CoolDown-=DeltaTime;
	}
	
	
	
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UVehicleSpringArm::SetCooldown(float In)
{
	CoolDown=In;
}
