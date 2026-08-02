#pragma once

#include "CoreMinimal.h"
#include "P2CSessionSearchResult.generated.h"

USTRUCT(BlueprintType)
struct P2C_API FP2CSessionSearchResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 ResultIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString HostName;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 PingInMs = 0;
};