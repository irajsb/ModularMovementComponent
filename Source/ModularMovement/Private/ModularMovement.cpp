// Copyright Epic Games, Inc. All Rights Reserved.

#include "ModularMovement.h"

#include "ClassIconFinder.h"
#include "IPluginManager.h"

#define LOCTEXT_NAMESPACE "FModularMovementModule"
TSharedPtr< FSlateStyleSet > StyleSet = nullptr;
void FModularMovementModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

FString Path=IPluginManager::Get().FindPlugin(TEXT("ModularMovement"))->GetBaseDir() / TEXT("Resources");
	 StyleSet = MakeShareable(new FSlateStyleSet("ModularStyle"));
	StyleSet->SetContentRoot(	Path);
	StyleSet->SetCoreContentRoot(	IPluginManager::Get().FindPlugin(TEXT("ModularMovement"))->GetBaseDir() / TEXT("Resources"));
	UE_LOG(LogTemp,Log,TEXT( " Plugin dir %s"),*	(IPluginManager::Get().FindPlugin(TEXT("ModularMovement"))->GetBaseDir() / TEXT("Resources")))



	StyleSet->Set("ClassIcon.ModularWheel", new FSlateImageBrush(Path/"ClassIcon.ModularWheel16.png", FVector2D(20.0f, 20.0f)));
	StyleSet->Set("ClassIcon.ModularMovementComponent", new FSlateImageBrush(Path/"ClassIcon.ModularMovementComponent.png", FVector2D(20.0f, 20.0f)));
	StyleSet->Set("ClassThumbnail.ModularVehicleData", new FSlateImageBrush(Path/"VehicleDA.png", FVector2D(64.0f, 64.0f)));
	StyleSet->Set("ClassThumbnail.ModularVehicleWheelData", new FSlateImageBrush(Path/"WheelDA.png", FVector2D(64.0f, 64.0f)));
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
	
}

void FModularMovementModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
	ensure(StyleSet.IsUnique());
	StyleSet.Reset();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FModularMovementModule, ModularMovement)

DEFINE_LOG_CATEGORY(LogModularVehicle);