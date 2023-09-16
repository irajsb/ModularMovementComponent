//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#include "ModularMovementEditor.h"

#include "PacejkaDetailsCustomization.h"
#include "PacejkaTireModel.h"
#include "PropertyEditorModule.h"
#include "TankTrackComponent.h"



static const FName ModularMovementEditorTabName("ModularMovementEditor");

#define LOCTEXT_NAMESPACE "FModularMovementEditorModule"

void FModularMovementEditorModule::StartupModule()
{

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(FPacejkaConstants::StaticStruct()->GetFName(),FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPacejkaDetailsCustomization::MakeInstance));
	
}

void FModularMovementEditorModule::ShutdownModule()
{
	
}




#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FModularMovementEditorModule, ModularMovementEditor)