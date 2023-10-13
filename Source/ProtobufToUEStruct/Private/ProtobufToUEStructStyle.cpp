// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProtobufToUEStructStyle.h"
#include "ProtobufToUEStruct.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FProtobufToUEStructStyle::StyleInstance = nullptr;

void FProtobufToUEStructStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FProtobufToUEStructStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FProtobufToUEStructStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("ProtobufToUEStructStyle"));
	return StyleSetName;
}


const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FProtobufToUEStructStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("ProtobufToUEStructStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("ProtobufToUEStruct")->GetBaseDir() / TEXT("Resources"));

	Style->Set("ProtobufToUEStruct.PluginAction", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon20x20));
	return Style;
}

void FProtobufToUEStructStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FProtobufToUEStructStyle::Get()
{
	return *StyleInstance;
}
