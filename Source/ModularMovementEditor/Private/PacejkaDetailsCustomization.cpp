//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 


#include "PacejkaDetailsCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PacejkaTireModel.h"


void FPacejkaDetailsCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle,
                                                     IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	

	if(MovementEditorModule->EditorWindow)
	{
		MovementEditorModule->EditorWindow.Get()->DestroyWindowImmediately();
	}
	
	PacejkaConstantsHandle=PropertyHandle;
	// Add original children back to the layout
	uint32 NumChildren;
	PropertyHandle->GetNumChildren(NumChildren);
	for (uint32 i = 0; i < NumChildren; ++i)
	{
		TSharedRef<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(i).ToSharedRef();
		ChildBuilder.AddProperty(ChildHandle);
	}


	ChildBuilder.AddCustomRow(FText::FromString("Open Preview"))
			.WholeRowWidget
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.1f)
				[
					SNew(SButton)
					.Text(FText::FromString("Open Preview"))
					.OnClicked(this, &FPacejkaDetailsCustomization::OnOpenEditorClicked)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.9f)
				[
					SNew(SSpacer)
				]
			];
}
FReply FPacejkaDetailsCustomization::OnOpenEditorClicked() 
{

	if(MovementEditorModule->EditorWindow)
	{
		MovementEditorModule->EditorWindow.Get()->DestroyWindowImmediately();
	}
	// Create the custom window
	TSharedRef<SWindow> EditorWindowRef = SNew(SWindow)
		.Title(FText::FromString("Pacejka Graph Preview"))
		.ClientSize(FVector2D(800, 600)).IsTopmostWindow(true)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock).Text(FText::FromString("Pacejka Force Curve"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1)
			[
				SNew(SPacejkaGraph).PacejkaConstantsHandle(PacejkaConstantsHandle)
			]
		];

	// Show the window
	FSlateApplication::Get().AddWindow(EditorWindowRef);

	MovementEditorModule->EditorWindow= TSharedPtr<SWindow>( EditorWindowRef);
	return FReply::Handled();
}
