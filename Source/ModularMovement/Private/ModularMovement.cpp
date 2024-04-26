//Copyright Aurelion Iraj Mohtasham 2023. For distribution in epic store only 

#include "ModularMovement.h"


#include "ModularMovementSettings.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"
#include "PhysicsEngine/PhysicsSettings.h"
#define LOCTEXT_NAMESPACE "FModularMovementModule"
TSharedPtr< FSlateStyleSet > StyleSet = nullptr;
void FModularMovementModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

#if WITH_EDITOR
FString Path=IPluginManager::Get().FindPlugin(TEXT("ModularMovement"))->GetBaseDir() / TEXT("Resources");
	 StyleSet = MakeShareable(new FSlateStyleSet("ModularStyle"));
	StyleSet->SetContentRoot(	Path);
	StyleSet->SetCoreContentRoot(	IPluginManager::Get().FindPlugin(TEXT("ModularMovement"))->GetBaseDir() / TEXT("Resources"));
	

	StyleSet->Set("ClassIcon.ModularWheel", new FSlateImageBrush(Path/"ClassIcon.ModularWheel16.png", FVector2D(20.0f, 20.0f)));
	StyleSet->Set("ClassIcon.ModularMovementComponent", new FSlateImageBrush(Path/"ClassIcon.ModularMovementComponent.png", FVector2D(20.0f, 20.0f)));
	StyleSet->Set("ClassThumbnail.ModularVehicleData", new FSlateImageBrush(Path/"VehicleDA.png", FVector2D(64.0f, 64.0f)));
	StyleSet->Set("ClassThumbnail.ModularVehicleWheelData", new FSlateImageBrush(Path/"WheelDA.png", FVector2D(64.0f, 64.0f)));
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());



	const auto  Settings=UPhysicsSettings::Get();
	if(!Settings->bSubstepping&&!GetMutableDefault<UModularMovementSettings>()->SubstepShown)
	{
		FNotificationInfo Info(LOCTEXT("SubStepSettings", "Modular Movement Requires Substepping"));
		Info.ExpireDuration = 120.0f;
		Info.bFireAndForget = false;
		Info.bUseThrobber=false;
		Info.Image = FCoreStyle::Get().GetBrush(TEXT("MessageLog.Warning"));


		Info.ButtonDetails.Add(FNotificationButtonInfo(LOCTEXT("MultipleLSAsPopupDismiss", "Dismiss"), LOCTEXT("SubStepFixToolTipDismiss", "Dismiss this notification"), FSimpleDelegate::CreateRaw(this, &FModularMovementModule::OnSettingsDissmissed)));

		Notif= FSlateNotificationManager::Get().AddNotification(Info);
		if(Notif.IsValid())
		{
			Notif.Pin()->SetCompletionState(SNotificationItem::CS_Pending);
		}
	}
	
	

#endif
}

void FModularMovementModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
#if WITH_EDITOR
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
	ensure(StyleSet.IsUnique());
	StyleSet.Reset();
	#endif
}

void FModularMovementModule::OnSettingsFix()
{
	
	auto  Settings=UPhysicsSettings::Get();
	Settings->bSubstepping=true;
	Settings->bSubsteppingAsync=false;
	Settings->MaxSubsteps=16;
	Settings->MaxSubstepDeltaTime=0.004167;
	Settings->SaveConfig();
	if (Notif.IsValid())
	{
		TSharedPtr<SNotificationItem> NotifPopup = Notif.Pin();

		NotifPopup->SetCompletionState(SNotificationItem::CS_None);
		NotifPopup->SetExpireDuration(0.0f);
		NotifPopup->SetFadeOutDuration(0.0f);
		NotifPopup->ExpireAndFadeout();

		Notif.Reset();
	}
}

void FModularMovementModule::OnSettingsDissmissed()
{
	if (Notif.IsValid())
	{
		TSharedPtr<SNotificationItem> NotifPopup = Notif.Pin();

		NotifPopup->SetCompletionState(SNotificationItem::CS_None);
		NotifPopup->SetExpireDuration(0.0f);
		NotifPopup->SetFadeOutDuration(0.0f);
		NotifPopup->ExpireAndFadeout();

		Notif.Reset();
		auto Obj=UModularMovementSettings::StaticClass()->GetDefaultObject<UModularMovementSettings>();
		Obj->SubstepShown=true;
		Obj->SaveConfig();
		
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FModularMovementModule, ModularMovement)

DEFINE_LOG_CATEGORY(LogModularVehicle);