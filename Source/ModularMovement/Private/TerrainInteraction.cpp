// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainInteraction.h"

#include "CanvasTypes.h"
#include "ModularMovement.h"
#include "ModularMovementComponent.h"
#include "ModularWheel.h"
#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialParameterCollection.h"

// Sets default values for this component's properties
UTerrainInteraction::UTerrainInteraction(): RenderTarget2D(nullptr), CanvasRot(0), MaterialParameterCollection(nullptr),
                                            MaterialInterface(nullptr), OffsetMat(nullptr)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UTerrainInteraction::BeginPlay()
{
	Super::BeginPlay();

	// ...


	UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(),RenderTarget2D);
	
}


// Called every frame
void UTerrainInteraction::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UTerrainInteraction::Update(float DeltaTime, UModularMovementComponent* MC, const TArray<UModularWheel*>& Components)
{
	UCanvas* Canvas;
	FVector2D Size;
	FDrawToRenderTargetContext Context;

	
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(GetWorld(), RenderTarget2D, Canvas, Size, Context);
	PixelWorldSize= CanvasWorldSize/Size;

	
	const auto Transform=MC->GetMesh()->GetComponentTransform();
	const FVector2D NewLocation=FVector2D(Transform.GetLocation().X,Transform.GetLocation().Y);
	auto CanvasWorldLocation = FVector2D(floor((NewLocation / (PixelWorldSize * PixelSizeScale)).X),
										floor((NewLocation / (PixelWorldSize * PixelSizeScale)).Y)) * PixelWorldSize *PixelSizeScale;
	FVector2D Offset=CanvasWorldLocation-PreviousOffsetLocation;
	Offset=Offset/CanvasWorldSize;
	
	UKismetMaterialLibrary::SetVectorParameterValue(GetWorld(),MaterialParameterCollection,"Offset",FLinearColor(Offset.X,Offset.Y,0,0));
	

if((CanvasWorldLocation-PreviousDrawLocation).Size()>MinDrawDistance)
{
	PreviousDrawLocation =CanvasWorldLocation; 
	for (auto  Component : Components)
	{
		

		if(Component->AllowDrawInRenderTarget)
		{
			
			// Shift the entire contents of the source canvas by the specified offset
		
			

			// Get the wheel transform relative to the mesh transform
			FTransform WheelTransform = Component->GetComponentTransform().GetRelativeTransform(Transform);
			const FVector2D RelativeLocation=FVector2D(WheelTransform.GetLocation().X,WheelTransform.GetLocation().Y);
			// Adjust the wheel transform to align with the world's X-axis (forward)
			const FQuat WorldRotation = FQuat::Identity; // World's rotation is identity (no rotation)
			const FQuat WheelRotationDiff = WheelTransform.GetRotation().Inverse() * WorldRotation;
			WheelTransform.SetRotation(WheelRotationDiff);

			// Convert wheel relative transform to 2D render target canvas coordinates
			// Assuming the render target size is the same as the mesh's bounding box size
			const float Yaw=Transform.Rotator().Yaw ;
			FVector2D WheelPosition = UKismetMathLibrary::GetRotated2D((RelativeLocation) / CanvasWorldSize,
																Transform.Rotator().Yaw 	   ) + FVector2D(0.5, 0.5);
			const FVector2D WheelSize = BrushSize / CanvasWorldSize * Size;

		
			// Draw the box representing the wheel on the render target
			WheelPosition=WheelPosition*Size;
			
			
			Canvas->K2_DrawMaterial(MaterialInterface,WheelPosition - WheelSize * 0.5f, WheelSize,FVector2D(0,0),FVector2D::UnitVector,Transform.Rotator().Yaw +Component->WheelState.SteerAngle);
		
		}
	}
	
}

	PreviousOffsetLocation =CanvasWorldLocation; 
	Canvas->K2_DrawBox(FVector2D(0,0), Size, Size.Size()*0.1, FLinearColor::Black);
	Canvas->K2_DrawMaterial(OffsetMat, FVector2D(0, 0), Size, FVector2D(0.0f, 0.0f),FVector2D::UnitVector);
		

	
	
	
	
	UKismetMaterialLibrary::SetVectorParameterValue(GetWorld(),MaterialParameterCollection,"PosAndSize",FLinearColor(CanvasWorldLocation.X,CanvasWorldLocation.Y,CanvasWorldSize.X,CanvasWorldSize.Y));



	

	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);
}
