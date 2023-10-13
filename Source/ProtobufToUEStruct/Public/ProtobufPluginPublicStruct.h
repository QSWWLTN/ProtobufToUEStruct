#pragma once

#include "CoreMinimal.h"

#include "ProtobufPluginPublicStruct.generated.h"

USTRUCT(BlueprintType, Category = "ProtobufToUEStruct")
struct FProtoConfig {
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "ProtobufToUEStruct", BlueprintReadWrite)
		TArray<FString> TargetArray;
};