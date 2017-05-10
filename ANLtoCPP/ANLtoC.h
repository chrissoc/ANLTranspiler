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
	std::string KernelToC(anl::CKernel& Kernel, anl::CInstructionIndex& Root);
}


