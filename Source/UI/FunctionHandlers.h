#pragma once
#include <cstdint>
#include <string>
#include <map>

class FSpectrumEmu;

struct FFunctionInfo
{
	std::string		FunctionName;
	uint16_t		StartAddress;
	uint16_t		EndAddress;

	std::map<uint16_t, int>	Callers;
	std::map<uint16_t, int>	ExitPoints;

	// Stats
	int				NoCalls = 0;
};


int FunctionTrapFunction(uint16_t pc, uint16_t nextpc, int ticks, uint64_t pins, FSpectrumEmu* pEmu);

void DrawFunctionInfo(FSpectrumEmu* pEmu);

//bool GetROMLabelName(uint16_t callAddr, std::string &labelName);