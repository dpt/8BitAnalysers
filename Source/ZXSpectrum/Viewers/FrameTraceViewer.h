#pragma once


#include "CodeAnalyser/CodeAnalyser.h"

#include <cstdint>
#include <vector>
#include <string>

class FSpectrumEmu;
class FZXGraphicsView;

struct FFrameOverviewItem
{
	std::string		Label;
	uint16_t		FunctionAddress;
	uint16_t		LabelAddress;
};

struct FMemoryDiff
{
	uint16_t	Address;
	uint8_t		OldVal;
	uint8_t		NewVal;
};

struct FSpeccyFrameTrace
{
	void*					Texture;
	uint8_t					MemoryDump[1 << 16];	// 64K
	void*					CPUState = nullptr;
	std::vector<uint16_t>	InstructionTrace;
	std::vector<FMemoryAccess>	ScreenPixWrites;
	std::vector<FMemoryAccess>	ScreenAttrWrites;

	std::vector<FFrameOverviewItem>	FrameOverview;
	std::vector<FMemoryDiff>	MemoryDiffs;
};

class FFrameTraceViewer
{
public:
	void	Init(FSpectrumEmu* pEmu);
	void	Shutdown();
	void	CaptureFrame();
	void	Draw();
private:
	void	RestoreFrame(const FSpeccyFrameTrace& frame);
	void	DrawInstructionTrace(const FSpeccyFrameTrace& frame);
	void	GenerateTraceOverview(FSpeccyFrameTrace& frame);
	void	GenerateMemoryDiff(const FSpeccyFrameTrace& frameA, const FSpeccyFrameTrace& frameB, std::vector<FMemoryDiff>& outDiff);
	void	DrawTraceOverview(const FSpeccyFrameTrace& frame);
	void	DrawFrameScreenWritePixels(const FSpeccyFrameTrace& frame, int lastIndex = -1);
	void	DrawScreenWrites(const FSpeccyFrameTrace& frame);
	void	DrawMemoryDiffs(const FSpeccyFrameTrace& frame);

	FSpectrumEmu* pSpectrumEmu = nullptr;

	int					ShowFrame = 0;
	int					CurrentTraceFrame = 0;
	bool				RestoreOnScrub = false;
	static const int	kNoFramesInTrace = 300;
	FSpeccyFrameTrace	FrameTrace[kNoFramesInTrace];

	int		SelectedTraceLine = -1;
	int		PixelWriteline = -1;
	FZXGraphicsView*	ShowWritesView = nullptr;

};