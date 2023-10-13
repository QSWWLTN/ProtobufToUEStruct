// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "ProtobufToUEStructStyle.h"

class FProtobufToUEStructCommands : public TCommands<FProtobufToUEStructCommands>
{
public:

	FProtobufToUEStructCommands()
		: TCommands<FProtobufToUEStructCommands>(TEXT("ProtobufToUEStruct"), NSLOCTEXT("Contexts", "ProtobufToUEStruct", "ProtobufToUEStruct Plugin"), NAME_None, FProtobufToUEStructStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
