/////////////////////////////////////////
//
// File Header Place Holder
//
/////////////////////////////////////////

#include <string>
#include <vector>

namespace anl {
	class CKernel;
	class CInstructionIndex;
}

namespace ANLtoC {
	struct FunctionData
	{
		std::string FunctionImplementation;
		unsigned int RelatedIndex;
	};

	void KernelToC(anl::CKernel& Kernel, const anl::CInstructionIndex& Root, std::string& ExpressionToExecute, std::string& NamedInputStructGuts, std::vector<FunctionData>& FunctionList);
}


