/////////////////////////////////////////
//
// File Header Place Holder
//
/////////////////////////////////////////

#include <string>

namespace anl {
	class CKernel;
	class CInstructionIndex;
}

namespace ANLtoC {
	void KernelToC(anl::CKernel& Kernel, anl::CInstructionIndex& Root, std::string& ExpressionToExecute, std::string& NamedInputStructGuts);
}


