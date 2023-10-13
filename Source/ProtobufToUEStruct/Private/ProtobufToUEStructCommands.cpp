// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProtobufToUEStructCommands.h"

#define LOCTEXT_NAMESPACE "FProtobufToUEStructModule"

void FProtobufToUEStructCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "ProtobufToUEStruct", "将Proto文件转换为蓝图结构体", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
