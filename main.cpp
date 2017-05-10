/////////////////////////////////////////
//
// File Header Place Holder
//
/////////////////////////////////////////


#include <ANLtoCPP/ANLtoC.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <memory>
#include <ctime>
#include <Output.h>

#define ANL_IMPLEMENTATION
// ANL is currently in a transition to a single file format, thus
// we need to IMPLEMENT_STB even though we don't use it.
#define IMPLEMENT_STB
#include <accidental-noise-library/anl.h>
#include <accidental-noise-library/lang/NoiseParser.h>

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cerr << "Missing arguments." << std::endl;
		std::cerr << "USAGE: ANLTranspiler.exe anlLangSourceFile.anl > ResultFile.cpp" << std::endl;
		std::cerr << "  The anlLangSourceFile.anl will be parsed and converted to an internal" << std::endl;
		std::cerr << "  anl::CKernel which will then be converted to cplusplus and sent to stdout" << std::endl;
		return 0;
	}

	std::string FileName = argv[1];

	FILE* f = nullptr;
	if (0 != fopen_s(&f, FileName.c_str(), "r")) {
		std::cerr << "Unable to open file: " << FileName << std::endl;
		return -9;
	}
	if (fseek(f, 0, SEEK_END) != 0) {
		std::cerr << "Seek Error for file: " << FileName << std::endl;
		return -10;
	}

	long FileLength = ftell(f);

	if (fseek(f, 0, SEEK_SET) != 0) {
		std::cerr << "Seek Error for file: " << FileName << std::endl;
		return -10;
	}

	std::string FullText;
	{
		auto Buffer = std::make_unique<uint8_t[]>(FileLength+1);
		size_t AmountRead = fread(Buffer.get(), 1, FileLength, f);
		if (AmountRead != FileLength && feof(f) == 0) {
			std::cerr << "Read Error for file: " << FileName << std::endl;
			return -10;
		}
		fclose(f);
		Buffer[AmountRead] = 0;
		FullText = (char*)Buffer.get();
	}

	std::unique_ptr<anl::lang::NoiseParser> NoiseParser;
	NoiseParser = std::make_unique<anl::lang::NoiseParser>(FullText);

	bool success = NoiseParser->Parse();
	if (!success)
	{
		auto ErrorMessages = NoiseParser->FormErrorMsgs();
		std::cerr << "Error parsing TerrainV noise file: " << FileName
			<< "\nErrors:\n" << ErrorMessages
			<< "\n\n Contents of \"" << FileName
			<< "\"\n" << FullText << std::endl;
		return -30;
	}
	else
	{
		std::string Code = ANLtoC::KernelToC(NoiseParser->GetKernel(), NoiseParser->GetParseResult());
		Code = OutputFullCppFile(Code);
		std::string header = "// Generated file - Do not edit. Generated by ANLTranspiler at ";
		time_t CurrentTime = time(0);
		char TimeBuffer[100];
		ctime_s(TimeBuffer, 100, &CurrentTime);
		header.append(TimeBuffer);
		header.append("\n");
		Code.insert(0, header);
		std::cout << Code << std::endl;

		anl::CNoiseExecutor vm(NoiseParser->GetKernel());
		double VMResult = vm.evaluateScalar(0.5, 0.5, NoiseParser->GetParseResult());
	}

	return 0;
}
