#pragma once

struct FCodeAnalysisState;
struct FCodeInfo;

#include <map>

struct FInstructionInfo
{
	const char* Description = nullptr;
};

typedef std::map<uint8_t, FInstructionInfo> InstructionInfoMap;

void ShowCodeToolTip(FCodeAnalysisState &state, const FCodeInfo *pCodeInfo);