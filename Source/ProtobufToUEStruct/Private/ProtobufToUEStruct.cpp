// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProtobufToUEStruct.h"
#include "ProtobufToUEStructStyle.h"
#include "ProtobufToUEStructCommands.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"

#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Kismet2/StructureEditorUtils.h"
#include "UserDefinedStructure/UserDefinedStructEditorData.h"
#include "ObjectTools.h"
#include "Misc/FileHelper.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/EnumEditorUtils.h"

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/compiler/importer.h>

#include "Tools\JsonObjectConverterEx.h"
#include "../Public/ProtobufPluginPublicStruct.h"

using namespace google::protobuf;

#define LOCTEXT_NAMESPACE "FProtobufToUEStructModule"

void FProtobufToUEStructModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FProtobufToUEStructStyle::Initialize();
	FProtobufToUEStructStyle::ReloadTextures();

	FProtobufToUEStructCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FProtobufToUEStructCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FProtobufToUEStructModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FProtobufToUEStructModule::RegisterMenus));
}

void FProtobufToUEStructModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FProtobufToUEStructStyle::Shutdown();

	FProtobufToUEStructCommands::Unregister();
}

class FileErrorCollector : public compiler::MultiFileErrorCollector
{
	virtual void AddError(const std::string& filename, int line, int column, const std::string& message) override
	{
		std::cout << "file: " << filename << ", line: " << line << ", col: " << column << ", message: " << message << std::endl;
	}
};

bool RemoveOldStruct(const FString& FilePath) {
	UObject* OldObject = Cast<UObject>(StaticLoadObject(UObject::StaticClass(), nullptr, *FilePath));
	if (OldObject->IsValidLowLevel()) {

		TArray<UObject*> DeleteArray;
		DeleteArray.Add(OldObject);

		if (ObjectTools::DeleteObjects(DeleteArray, false, ObjectTools::EAllowCancelDuringDelete::CancelNotAllowed) == DeleteArray.Num()) {
			return true;
		}
		return false;
	}
	return true;
}

void AddProtoStruct(FString PackPath, const FileDescriptor* FileDesc, const FProtoConfig& Config) {
	check(FileDesc && "添加结构时参数错误");

	FString ProtoPackName;
	for (auto FileName : Config.TargetArray) {
		const Descriptor* MessageDes = FileDesc->pool()->FindMessageTypeByName(TCHAR_TO_ANSI(*FileName));

		if (MessageDes == nullptr) {
			continue;
		}

		TArray<FString> T;
		FileName.ParseIntoArray(T, TEXT("."));
		if (T.Num() < 2) {
			continue;
		}
		ProtoPackName = T[0];

		//创建结构体
		{
			UPackage* Pack = CreatePackage(nullptr, *(TEXT("/Game/Code_Struct/Proto/") / T[0] + TEXT("/") / T[1]));
			check(Pack && "创建新的结构时发生错误");

			UUserDefinedStruct* Struct = FStructureEditorUtils::CreateUserDefinedStruct(Pack, FName(*T[1]), RF_Public | RF_Standalone | RF_Transactional);
			check(Struct && "创建结构体失败");

			TArray<FString> NameList;

			int32 StructIndex = MessageDes->field_count();
			for (int32 i = 0; i <= 0xff; i++) {
				const FieldDescriptor* Field = MessageDes->FindFieldByNumber(i);
				if (Field == nullptr) {
					continue;
				}
				StructIndex--;

				FName TypeName;
				NameList.Add(Field->name().c_str());

				switch (Field->cpp_type()) {
				case FieldDescriptor::CppType::CPPTYPE_BOOL:
					TypeName = UEdGraphSchema_K2::PC_Boolean;
					break;
				case FieldDescriptor::CppType::CPPTYPE_DOUBLE:
					TypeName = UEdGraphSchema_K2::PC_Double;
					break;
				case FieldDescriptor::CppType::CPPTYPE_ENUM:
					TypeName = UEdGraphSchema_K2::PC_Byte;
					break;
				case FieldDescriptor::CppType::CPPTYPE_FLOAT:
					TypeName = UEdGraphSchema_K2::PC_Float;
					break;
				case FieldDescriptor::CppType::CPPTYPE_INT32:
					TypeName = UEdGraphSchema_K2::PC_Int;
					break;
				case FieldDescriptor::CppType::CPPTYPE_INT64:
					TypeName = UEdGraphSchema_K2::PC_Int64;
					break;
				case FieldDescriptor::CppType::CPPTYPE_STRING:
					TypeName = UEdGraphSchema_K2::PC_String;
					break;
				case FieldDescriptor::CppType::CPPTYPE_MESSAGE:
					{
						TArray<FString> CreateNameList;
						FString(Field->message_type()->full_name().c_str()).ParseIntoArray(CreateNameList, TEXT("."));
						if (CreateNameList.Num() < 2) {
							continue;
						}
						FString TargetStructPath = TEXT("/Game/Code_Struct/Proto/") + CreateNameList[0] / CreateNameList[1] + TEXT(".") + CreateNameList[1];
						UStruct* CreateStruct = LoadObject<UStruct>(nullptr, *TargetStructPath);
						if (CreateStruct->IsValidLowLevel()) {
							FStructureEditorUtils::AddVariable(Struct, FEdGraphPinType(UEdGraphSchema_K2::PC_Struct, NAME_None, CreateStruct, EPinContainerType::None, false, FEdGraphTerminalType()));
							continue;
						}

						FString PackRootPath = FPaths::ProjectContentDir() + TEXT("/Code_Struct/Proto/");
						FProtoConfig TempConfig;
						TempConfig.TargetArray.Add(UTF8_TO_TCHAR(Field->message_type()->full_name().c_str()));
						AddProtoStruct(PackRootPath, Field->message_type()->file(), TempConfig);

						CreateStruct = LoadObject<UStruct>(nullptr, *TargetStructPath);
						if (CreateStruct->IsValidLowLevel()) {
							FStructureEditorUtils::AddVariable(Struct, FEdGraphPinType(UEdGraphSchema_K2::PC_Struct, NAME_None, CreateStruct, EPinContainerType::None, false, FEdGraphTerminalType()));
							continue;
						}
					}
					break;
				}

				FStructureEditorUtils::AddVariable(Struct, FEdGraphPinType(TypeName, NAME_None, nullptr, EPinContainerType::None, false, FEdGraphTerminalType()));

				if (StructIndex <= 0) {
					break;
				}
			}
			TArray<FStructVariableDescription>& StructDesc = FStructureEditorUtils::GetVarDesc(Struct);
			FStructureEditorUtils::RemoveVariable(Struct, StructDesc[0].VarGuid);		//去掉默认的结构

			for (int i = 0; i < NameList.Num(); i++) {
				FStructureEditorUtils::RenameVariable(Struct, StructDesc[i].VarGuid, NameList[i]);
			}
			NameList.Empty();

			UPackage::SavePackage(Pack, Struct, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *(PackPath / T[0] + TEXT("/") / T[1] + TEXT(".uasset")), GError, nullptr, true, true, SAVE_NoError);
		}

		//创建结构体内的枚举
		{
			int32 EnumIndex = MessageDes->enum_type_count();
			UE_LOG(LogTemp, Error, TEXT("%d"), EnumIndex);
			for (int32 i = 0; i < EnumIndex; i++) {
				const EnumDescriptor* EnumType = MessageDes->enum_type(i);
				if (EnumType == nullptr) {
					continue;
				}
				FString EnumTypeName = EnumType->name().c_str();
				UPackage* Pack = CreatePackage(nullptr, *(TEXT("/Game/Code_Struct/Proto/") / T[0] + TEXT("/") / T[1] + TEXT("/") / EnumTypeName));
				check(Pack && "创建新的结构时发生错误");

				UUserDefinedEnum* EnumPack = Cast<UUserDefinedEnum>(FEnumEditorUtils::CreateUserDefinedEnum(Pack, FName(*EnumTypeName), RF_Public | RF_Standalone | RF_Transactional));

				int32 TargetSize = EnumType->value_count();
				UE_LOG(LogTemp, Error, TEXT("%d"), TargetSize);
				for (int j = 0; j < TargetSize; j++) {
					FEnumEditorUtils::AddNewEnumeratorForUserDefinedEnum(EnumPack);
					FEnumEditorUtils::SetEnumeratorDisplayName(EnumPack, j, FText::FromString(EnumType->value(j)->name().c_str()));
				}

				UPackage::SavePackage(Pack, EnumPack, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *(PackPath / T[0] + TEXT("/") / T[1] + TEXT("/") / EnumTypeName + TEXT(".uasset")), GError, nullptr, true, true, SAVE_NoError);
			}
		}
	}

	//创建结构体外的枚举
	{
		int32 EnumIndex = FileDesc->enum_type_count();
		for (int32 i = 0; i < EnumIndex; i++) {
			const EnumDescriptor* EnumType = FileDesc->enum_type(i);
			if (EnumType == nullptr) {
				continue;
			}
			FString EnumTypeName = EnumType->name().c_str();
			UPackage* Pack = nullptr;
			if (ProtoPackName.IsEmpty()) {
				Pack = CreatePackage(nullptr, *(TEXT("/Game/Code_Struct/Proto/") / EnumTypeName));
			}
			else {
				Pack = CreatePackage(nullptr, *(TEXT("/Game/Code_Struct/Proto/") / ProtoPackName + TEXT("/") / EnumTypeName));
			}
			check(Pack && "创建新的结构时发生错误");

			UUserDefinedEnum* EnumPack = Cast<UUserDefinedEnum>(FEnumEditorUtils::CreateUserDefinedEnum(Pack, FName(*EnumTypeName), RF_Public | RF_Standalone | RF_Transactional));

			TArray<TPair<FName, int64>> Enums;

			int32 TargetSize = EnumType->value_count();
			for (int j = 0; j < TargetSize; j++) {
				FString EnumNameString = EnumPack->GenerateNewEnumeratorName();
				const FString FullNameStr = EnumPack->GenerateFullEnumName(*EnumNameString);
				Enums.Add({ *FullNameStr, EnumType->value(j)->number()});
			}

			EnumPack->SetEnums(Enums, EnumPack->GetCppForm());
			for (int j = 0; j < TargetSize; j++) {
				FEnumEditorUtils::SetEnumeratorDisplayName(EnumPack, j, FText::FromString(EnumType->value(j)->name().c_str()));
			}

			if (ProtoPackName.IsEmpty()) {
				UPackage::SavePackage(Pack, EnumPack, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *(PackPath / EnumTypeName + TEXT(".uasset")), GError, nullptr, true, true, SAVE_NoError);
			}
			else {
				UPackage::SavePackage(Pack, EnumPack, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *(PackPath / ProtoPackName + TEXT("/") / EnumTypeName + TEXT(".uasset")), GError, nullptr, true, true, SAVE_NoError);
			}
		}
	}
}

const FileDescriptor* GetFileDescriptorFromProtoFile(FString PackName, const FProtoConfig& Config, const std::string& FilePath, FString& Name) {
	auto Pos = FilePath.find_last_of('/');
	std::string Path;
	std::string FileName;

	if (Pos == std::string::npos) {
		FileName = FilePath;
	}
	else {
		Path = FilePath.substr(0, Pos);
		FileName = FilePath.substr(Pos + 1);
	}

	Pos = FileName.find_last_of('.');
	if (Pos == std::string::npos) {
		Name = UTF8_TO_TCHAR(FileName.c_str());
	}
	else {
		Name = UTF8_TO_TCHAR(FileName.substr(0, Pos).c_str());
	}

	compiler::DiskSourceTree sourceTree;
	FileErrorCollector Error;

	sourceTree.MapPath("", TCHAR_TO_ANSI(*FPaths::ConvertRelativePathToFull(Path.c_str())));
	compiler::Importer importer(&sourceTree, &Error);

	AddProtoStruct(PackName, importer.Import(FileName), Config);

	return importer.Import(FileName);
}

void FProtobufToUEStructModule::PluginButtonClicked()
{
	//按键事件
	void* ParentWindowHandle = FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle();
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	check(DesktopPlatform && "无法加载选择窗口模块");
	
	//打开文件选择对话框
	uint32 SelectionFlag = 0;
	TArray<FString> OutFileNames;
	FString DefaultPath = FPaths::ProjectContentDir();
	DesktopPlatform->OpenFileDialog(ParentWindowHandle, TEXT("选择Proto文件"), DefaultPath, FString(""), TEXT("(Proto Files)|*.proto;)"), SelectionFlag, OutFileNames);

	SelectionFlag = 0;
	TArray<FString> ConfigFileName;
	DesktopPlatform->OpenFileDialog(ParentWindowHandle, TEXT("选择配置文件"), DefaultPath, FString(""), TEXT("(Json Files)|*.json;)"), SelectionFlag, ConfigFileName);

	if (ConfigFileName.Num() == 0) {
		return;
	}

	FString ConfigString;
	FFileHelper::LoadFileToString(ConfigString, *ConfigFileName[0]);
	FProtoConfig Config;

	if (!Q_FJsonObjectConverterEX::JsonObjectStringToUStruct(ConfigString, &Config, 0, 0)) {
		return;
	}

	FString PackRootPath = FPaths::ProjectContentDir() + TEXT("/Code_Struct/Proto/");

	for (auto Data : OutFileNames) {
		FString FileName;
		const FileDescriptor* T = GetFileDescriptorFromProtoFile(PackRootPath / FileName, Config, TCHAR_TO_ANSI(*Data), FileName);
		if (T == nullptr) {
			continue;
		}
	}

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("转换完成")));
}

void FProtobufToUEStructModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FProtobufToUEStructCommands::Get().PluginAction, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FProtobufToUEStructCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FProtobufToUEStructModule, ProtobufToUEStruct)