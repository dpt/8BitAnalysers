#include "SkoolkitExporter.h"
#include "CodeAnalyser/CodeAnalyser.h"
#include "Debug/Debug.h"
#include "Shared/Util/Misc.h"

#include "SkoolFile.h"
#include "SkoolFileInfo.h"

#include <cassert>
#include <chrono>

bool IsSpectrumChar(char value)
{
	return value >= 32 && value < 127 && value != 94 && value != 96;
}

class FSkoolKitExporter
{
public:
	FSkoolKitExporter(FCodeAnalysisState& state, const FSkoolFileInfo* pSkoolInfo = nullptr)
		: State(state)
		, pSkoolInfo(pSkoolInfo)
	{
	}
	void BuildSkoolFile(uint16_t startAddr, uint16_t endAddr)
	{
		FSkoolEntry* pCurEntry = nullptr;

		for (int addr = startAddr; addr <= endAddr;)
		{
			pCodeInfo = State.GetCodeInfoForAddress(addr);
			if (!pCodeInfo || pCodeInfo->bDisabled == true)
				pDataInfo = State.GetReadDataInfoForAddress(addr);

			ProcessLabel(addr);

			const SkoolDirective addrSubBlockDirective = GetSubBlockDirective(addr);
			const SkoolDirective addrBlockDirective = GetBlockDirective(addr, addrSubBlockDirective);

			if (ShouldAddNewEntry(addrSubBlockDirective, addr))
			{
				pCurEntry = SkoolFile.AddEntry(addrBlockDirective, addr);
				CurBlockDirective = addrBlockDirective;
			}

			AddInstruction(pCurEntry, addr);

			CurSubBlockDirective = addrSubBlockDirective;
	
			addr += GetAddrByteSize();
		}
	}

	uint16_t GetAddrByteSize() const
	{
		if (pCodeInfo != nullptr)
			return pCodeInfo->ByteSize;
		if (pDataInfo)
			return pDataInfo->ByteSize;
		
		return 1;
	}

	SkoolDirective GetBlockDirective(uint16_t addr, SkoolDirective subBlockDirective) const
	{
		if (pSkoolInfo)
		{
			if (const FSkoolFileLocation* pBlock = pSkoolInfo->GetLocation(addr))
			{
				if (pBlock->BlockDirective != SkoolDirective::None)
					return pBlock->BlockDirective;

				// if we've got a sub-block directive then the block directive should stay the same
				// todo find out if this is needed?
				if (pBlock->SubBlockDirective != SkoolDirective::None)
					return CurSubBlockDirective;
			}
		}

		if (pDataInfo != nullptr)
		{
			// todo unused flag?

			if (pDataInfo->bGameState)
				return SkoolDirective::GameStatusBuffer; 
		}

		return subBlockDirective;
	}

	SkoolDirective GetSubBlockDirective(uint16_t addr) const
	{
		if (pSkoolInfo)
		{
			if (const FSkoolFileLocation* pBlock = pSkoolInfo->GetLocation(addr))
			{
				if (pBlock->SubBlockDirective != SkoolDirective::None)
					return pBlock->SubBlockDirective;
			}
		}

		if (pCodeInfo != nullptr)
		{
			return SkoolDirective::Code;
		}
		
		if (pLabelInfo != nullptr)
		{
			if (pLabelInfo->LabelType == LabelType::Function)
				return SkoolDirective::Code;
		}

		if (pDataInfo != nullptr)
		{
			if (pDataInfo->DataType == DataType::Word || pDataInfo->DataType == DataType::WordArray)
				return SkoolDirective::WordData;
			if (pDataInfo->DataType == DataType::Text)
				return SkoolDirective::Text;
			return SkoolDirective::Data;
		}
		
		assert(0);
		return SkoolDirective::None;
	}

	bool ShouldAddNewEntry(SkoolDirective itemDirective, uint16_t addr) const
	{
		if (pSkoolInfo)
		{
			if (const FSkoolFileLocation* pBlock = pSkoolInfo->GetLocation(addr))
			{
				if (pBlock->bBranchDestination)
					return false;
				else if (pBlock->BlockDirective != SkoolDirective::None)
					return true;
				else if (pBlock->SubBlockDirective != SkoolDirective::None)
					return false;
			}
		}

		if (pLabelInfo != nullptr)
		{
			if (pLabelInfo->LabelType == LabelType::Function)
			{
				// Always add an entry for each function
				return true;
			}
		}

		if (itemDirective == SkoolDirective::None)
			return false;

		if (itemDirective != CurSubBlockDirective)
			return true;

		return false;
	}

	void ProcessLabel(uint16_t addr)
	{
		bIsBranchDestination = false;

		pLabelInfo = State.GetLabelForAddress(addr);
		if (pLabelInfo)
		{
			// todo warn if label already exists?
			if (pLabelInfo->Name.substr(0, 6) == "label_")
			{
				bIsBranchDestination = true;
				// todo make more robust. check address in string matches address in fitem
			}
			else if (pLabelInfo->Name.substr(0, 9) == "function_")
			{
				// ignore
			}
			else if (pLabelInfo->Name.substr(0, 5) == "data_")
			{
				// ignore
			}
			else
			{
				SkoolFile.AddLabel(pLabelInfo->Address, pLabelInfo->Name);
			}
		}
	}

	// Try to add a code/data instruction to the entry from the FItem.
	// Returns true if we added one sucessfully.
	bool AddInstruction(FSkoolEntry* pEntry, uint16_t addr)
	{
		if (pEntry)
		{
			FItem* pItem = nullptr;
			std::string operationText;
			
			if (pCodeInfo != nullptr)
			{
				UpdateCodeInfoForAddress(State, addr); // what does this do again?
				operationText = pCodeInfo->Text;
				pItem = pCodeInfo;
			}
			else if (pDataInfo != nullptr)
			{
				operationText = MakeDataAsmText(pDataInfo);
				pItem = pDataInfo;
			}
			else
			{
				// this shouldn't be possible
				assert(0);
				return false;
			}

			std::string commentLines;
			if (FCommentBlock* pCommentBlock = State.GetCommentBlockForAddress(addr))
			{
				commentLines = pCommentBlock->Comment;
			}

			char prefix = ' ';
			if (pEntry->Instructions.empty())
			{
				// the first instruction in the entry should have the directive set to the block type
				prefix = GetCharFromDirective(pEntry->Type);
			}
			else
			{
				if (pSkoolInfo)
				{
					if (const FSkoolFileLocation* pLocation = pSkoolInfo->GetLocation(addr))
					{
						if (pLocation->bBranchDestination)
							bIsBranchDestination = true;
					}
				}
			
				if (bIsBranchDestination)
				{
					prefix = '*';
				}
			}

			assert(pItem);
			pEntry->AddInstruction(addr, pItem->Comment, operationText, prefix, commentLines);
			return true;
		}
		return false;
	}
	
	bool Export(const char* pFilename, uint16_t startAddr, uint16_t endAddr, FSkoolFile::Base base = FSkoolFile::Base::Hexadecimal)
	{
		Base = base;
		
		if (pSkoolInfo)
		{
			startAddr = pSkoolInfo->StartAddr;
			endAddr = pSkoolInfo->EndAddr;
		}

		BuildSkoolFile(startAddr, endAddr);

		//SkoolFile.Dump();
		
		return SkoolFile.Export(pFilename, base);
	}

	std::string MakeDataAsmText(const FDataInfo* pDataInfo)
	{
		std::string asmText;
		char tmp[16] = { 0 };
		const bool bHex = Base == FSkoolFile::Base::Hexadecimal;
		if (pDataInfo->DataType == DataType::Byte)
		{
			snprintf(tmp, sizeof(tmp), bHex ? "DEFB $%02X" : "DEFB %u", State.CPUInterface->ReadByte(pDataInfo->Address));
			asmText = tmp;
		}
		else if (pDataInfo->DataType == DataType::ByteArray 
			|| pDataInfo->DataType == DataType::Graphics 
			|| pDataInfo->DataType == DataType::Blob
			|| pDataInfo->DataType == DataType::Bitmap
			|| pDataInfo->DataType == DataType::CharacterMap
			|| pDataInfo->DataType == DataType::ColAttr)
		{
			asmText = "DEFB ";
			const uint16_t numItems = pDataInfo->ByteSize;
			for (int i=0; i<numItems; i++)
			{
				snprintf(tmp, sizeof(tmp), bHex ? "$%02X," : "%u,", State.CPUInterface->ReadByte(pDataInfo->Address + i));
				asmText += tmp;
			}
			// remove last comma
			asmText.pop_back();
		}
		else if (pDataInfo->DataType == DataType::Word)
		{
			snprintf(tmp, sizeof(tmp), bHex ? "DEFW $%04X" : "DEFW %u", State.CPUInterface->ReadWord(pDataInfo->Address));
			asmText = tmp;
		}
		else if (pDataInfo->DataType == DataType::WordArray)
		{
			const uint16_t numItems = pDataInfo->ByteSize / 2;
			asmText = "DEFW ";
			for (int i = 0; i < numItems; i++)
			{
				snprintf(tmp, sizeof(tmp), bHex ? "$%04X," : "%d,", State.CPUInterface->ReadWord(pDataInfo->Address + i));
				asmText += tmp;
			}
			// remove last comma
			asmText.pop_back();
		}
		else if (pDataInfo->DataType == DataType::Text)
		{
			asmText = "DEFM ";

			bool bInString = false;
			bool bContainsText = false;
			for (int i = 0; i < pDataInfo->ByteSize; i++)
			{
				const uint8_t ch = State.CPUInterface->ReadByte(pDataInfo->Address + i);
				if (IsSpectrumChar(ch & 0x7f))
				{
					if (ch & 0x80)
					{
						if (bInString)
							asmText += "\","; // close quote

						snprintf(tmp, sizeof(tmp), bHex ? "\"%c\"+$80" : "\"%c\"+128", ch & 0x7f);
						asmText += tmp;
						bInString = false;
					}
					else
					{
						if (!bInString)
						{
							asmText += '"'; // open quote
							bInString = true;
						}
						asmText += ch;
					}
					bContainsText = true;
				}
				else
				{
					if (bInString)
					{
						asmText += '"'; // close quote
						asmText += ','; 
						bInString = false;
					}

					snprintf(tmp, sizeof(tmp), bHex ? "$%02X" : "%d", ch);
						
					asmText += tmp;

					if (i < pDataInfo->ByteSize-1) 
						asmText += ','; 
				}
			}
			if (bInString)
				asmText += '"';

			// if we've had no text in this statement then convert to DEFB.
			// text blocks can contain DEFB statements for text markers etc.
			// todo remove this? not sure it's still needed now we have subblock directives
			if (!bContainsText)
				asmText[3] = 'B';
		}
		else
		{
			asmText = "TODO";
#ifndef RELEASE
			assert(0);
#endif // !RELEASE
		}
		return asmText;
	}
private:

	// temporary variables used when iterating through all memory locations to build the skoolfile
	bool bIsBranchDestination = false;						// the current instruction is a "mid-routine entry point" ie * prefix
	FDataInfo* pDataInfo = nullptr;								// this will be set if the current address is data
	FCodeInfo* pCodeInfo = nullptr;								// this will be set if the current address is code
	FLabelInfo* pLabelInfo = nullptr;							// this will be set if the current address has a label
	SkoolDirective CurBlockDirective = SkoolDirective::None;
	SkoolDirective CurSubBlockDirective = SkoolDirective::None;

	FSkoolFile::Base Base = FSkoolFile::Base::Hexadecimal;

	FSkoolFile SkoolFile;
	FCodeAnalysisState& State;
	const FSkoolFileInfo* pSkoolInfo = nullptr;
};

// See here for a description of Skoolkit skool files
// https://skoolkit.ca/docs/skoolkit/skool-files.html
bool ExportSkoolFile(FCodeAnalysisState& state, const char* pTextFileName, FSkoolFile::Base base /* = FSkoolFile::Base::Hexadecimal*/, const FSkoolFileInfo* pSkoolInfo /* = nullptr */, uint16_t startAddr /* = 0x4000*/, uint16_t endAddr /* = 0xffff*/)
{
	auto t1 = std::chrono::high_resolution_clock::now();

	FSkoolKitExporter exporter = FSkoolKitExporter(state, pSkoolInfo);

	const ENumberDisplayMode previousDisplayMode = GetNumberDisplayMode();

	if (base == FSkoolFile::Base::Hexadecimal)
		SetNumberDisplayMode(ENumberDisplayMode::HexDollar);
	else
		SetNumberDisplayMode(ENumberDisplayMode::Decimal);

	bool bExportedOk = exporter.Export(pTextFileName, startAddr, endAddr, base);

	if (bExportedOk)
		LOGINFO("Successfully exported '%s'", pTextFileName);
	else
		LOGINFO("Failed to export '%s'", pTextFileName);

	SetNumberDisplayMode(previousDisplayMode);
	state.SetCodeAnalysisDirty();

	std::chrono::duration<double, std::milli> ms_double = std::chrono::high_resolution_clock::now() - t1;
	LOGDEBUG("Exporting %s took %.2f ms", pTextFileName, ms_double);
	return true;
}