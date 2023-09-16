//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "PacejkaTireModel.h"
#include "Widgets/SCanvas.h"

/**
 * 
 */

class MODULARMOVEMENTEDITOR_API SPacejkaGraph : public SCompoundWidget
{
public:
 SLATE_BEGIN_ARGS(SPacejkaGraph) {}
	SLATE_ARGUMENT(TSharedPtr<IPropertyHandle>, PacejkaConstantsHandle)
 SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
 {
 	PacejkaConstantsHandle =InArgs._PacejkaConstantsHandle ;
 	// Your existing Construct code
 }

	TSharedPtr<IPropertyHandle> PacejkaConstantsHandle;

 int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
{
	// Call the parent implementation of OnPaint
	int32 NewLayerId = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	// Get the size of the widget area we can draw in.
	FVector2D DrawSize = AllottedGeometry.GetLocalSize();

 
	// Draw dark gray background
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		NewLayerId,
		AllottedGeometry.ToPaintGeometry(),
		FCoreStyle::Get().GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		FLinearColor(0.1f, 0.1f, 0.1f) // Dark gray
	);

	// Draw grid lines
	const FLinearColor GridColor(0.2f, 0.2f, 0.2f);  // Light gray


 	// Vertical lines
 	for (int i = 1; i <= 10; ++i)
 	{
 		float X = i * DrawSize.X / 10.0f;
 		TArray<FVector2f> VertPoints;
 		VertPoints.Add(FVector2f(X, 0));
 		VertPoints.Add(FVector2f(X, DrawSize.Y));
 		FLinearColor CurrentColor = (i == 5) ? FLinearColor::White : GridColor;  // Make the center line white
 		FSlateDrawElement::MakeLines(OutDrawElements, NewLayerId, AllottedGeometry.ToPaintGeometry(), VertPoints, ESlateDrawEffect::None, CurrentColor, true);
 	}

 	// Horizontal lines
 	for (int i = 1; i <= 10; ++i)
 	{
 		float Y = i * DrawSize.Y / 10.0f;
 		TArray<FVector2f> HorzPoints;
 		HorzPoints.Add(FVector2f(0, Y));
 		HorzPoints.Add(FVector2f(DrawSize.X, Y));
 		FLinearColor CurrentColor = (i == 5) ? FLinearColor::White : GridColor;  // Make the center line white
 		FSlateDrawElement::MakeLines(OutDrawElements, NewLayerId, AllottedGeometry.ToPaintGeometry(), HorzPoints, ESlateDrawEffect::None, CurrentColor, true);
 	}
	// Replace these with your actual Pacejka constants
	float B = 1, C = 1, D = 1, E = 1;
	bool IsLat=false;
 	
 	if(PacejkaConstantsHandle.IsValid())
 	{
 		
 		PacejkaConstantsHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPacejkaConstants,B))->GetValue(B);
 		PacejkaConstantsHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPacejkaConstants,C))->GetValue(C);
 		PacejkaConstantsHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPacejkaConstants,D))->GetValue(D);
 		PacejkaConstantsHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPacejkaConstants,E))->GetValue(E);
 		IsLat=	PacejkaConstantsHandle->GetPropertyDisplayName().ToString().Equals(GET_MEMBER_NAME_CHECKED(UPacejkaTireModel,Lat).ToString());

 		
 		
 		
 	}
	TArray<FVector2f> Points;

 	FVector2f PeakForce=FVector2f::ZeroVector;
	const float Range=IsLat?1.5:1;
	// Generate points based on the Pacejka formula
 	for (float Slip = -Range; Slip <= Range; Slip += 0.02f)
 	{
 		float Force = D * FMath::Sin(C * FMath::Atan(B * Slip - E * (B * Slip - FMath::Atan(B * Slip))));
 		if(Force>PeakForce.Y)
 		{
 			PeakForce.Y=Force;
 			PeakForce.X=Slip;
 		}
 		float X = (Slip + 1) / 2.0f * DrawSize.X;
 		float Y = DrawSize.Y - ((Force + 2) / 4.0f * DrawSize.Y);  
 		Points.Add(FVector2f(X, Y));

 	
 	}
	// Draw the graph
	const FLinearColor LineColor(0.0f, 1.0f, 0.0f);  // Green
	FSlateDrawElement::MakeLines(
		OutDrawElements,
		NewLayerId,
		AllottedGeometry.ToPaintGeometry(),
		Points,
		ESlateDrawEffect::None,
		LineColor,
		true
	);

 	FVector2D Offset=FVector2D((PeakForce.X + 1) / 2.0f * DrawSize.X,DrawSize.Y - ((PeakForce.Y + 2) / 4.0f * DrawSize.Y))+FVector2D(-50,-50);
	const FString PeakText = FString::Printf(TEXT("Peak: %f \n (relative to wheel load) \n at Slip: %f"), PeakForce.Y, PeakForce.X);

 	
 	FSlateDrawElement::MakeText(
		   OutDrawElements,
		   NewLayerId,
		   AllottedGeometry.ToPaintGeometry(FVector2D(50,50),FSlateLayoutTransform(Offset)),
		   PeakText,
		   FCoreStyle::GetDefaultFontStyle("Mono",8),
		   ESlateDrawEffect::None,
		   FLinearColor::White
	   );

 	UE_LOG(LogTemp,Log,TEXT("Peak %s "),*PeakText)
	return NewLayerId;
}

private:
 TSharedPtr<SCanvas> GraphCanvas;
};

class MODULARMOVEMENTEDITOR_API FPacejkaDetailsCustomization : public IPropertyTypeCustomization
{
public:
 static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShared<FPacejkaDetailsCustomization>(); }
 
virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override{};
 virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
 FReply OnOpenEditorClicked() const;

	TSharedPtr<IPropertyHandle> PacejkaConstantsHandle;
};
