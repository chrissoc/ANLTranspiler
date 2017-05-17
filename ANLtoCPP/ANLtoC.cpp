/////////////////////////////////////////
//
// File Header Place Holder
//
/////////////////////////////////////////

#include "ANLtoC.h"
#include <string>
#include <unordered_map>
#include <array>
#include <accidental-noise-library/VM/kernel.h>
#include <sstream>
#include <tuple>
#include <vector>

using namespace anl;

namespace ANLtoC {
	struct ANLtoC_EmitData;
	struct Element
	{
		unsigned int Opcode;
		std::string Format;
	};
}

namespace ANLtoC {

	std::string InstructionToElement(ANLtoC_EmitData& Data, unsigned int index, std::vector<FunctionData> &FunctionList);

	std::string ToString(double d)
	{
		std::stringstream ss;
		ss.precision(std::numeric_limits<double>::max_digits10);
		ss << std::fixed << d;
		return ss.str();
	}

	bool IsOpCacheCandidate(InstructionListType& k, unsigned int index)
	{
		SInstruction& i = k[index];
		switch (i.opcode_)
		{
		case OP_ValueBasis:
		case OP_GradientBasis:
		case OP_SimplexBasis:
		case OP_CellularBasis:

		case OP_Bias:
		case OP_Gain:

		case OP_Pow:
		case OP_Cos:
		case OP_Sin:
		case OP_Tan:
		case OP_ACos:
		case OP_ASin:
		case OP_ATan:

		case OP_SmoothTiers:

		case OP_Sigmoid:

		case OP_HexTile:
		case OP_HexBump:
			return true;
		default:
			return false;
		}
	}

	bool IsOpFunctionCandidate(InstructionListType& k, unsigned int index)
	{
		SInstruction& instruction = k[index];
		switch (instruction.opcode_)
		{
			// the following is definatly not a candidate
		case OP_Constant:
		case OP_Seed:
		case OP_NamedInput:
		case OP_DX:
		case OP_DY:
		case OP_DZ:
		case OP_DU:
		case OP_DV:
		case OP_DW:
		case OP_X:
		case OP_Y:
		case OP_Z:
		case OP_U:
		case OP_V:
		case OP_W:

		case OP_GradientBasis:
		case OP_ValueBasis:
		case OP_SimplexBasis:
		case OP_CellularBasis:
			return false;
		default:
			break;
		}

		// otherwise if we meet all previous conditions then just check if it
		// is used as input into an existing qualifying function

		bool IsInput = false;
		for (std::size_t i = 0; i < k.size(); ++i)
		{
			SInstruction& instruction = k[i];
			switch (instruction.opcode_)
			{
			case OP_Select:
				if (instruction.sources_[0] == index ||
					instruction.sources_[1] == index ||
					instruction.sources_[2] == index ||
					instruction.sources_[3] == index ||
					instruction.sources_[4] == index) {
					IsInput = true;
				}
				break;
			}
			if (IsInput)
				break;
		}


		return IsInput;
	}

	struct ANLtoC_EmitData
	{
		InstructionListType& k;
		std::vector<std::string> DomainInputStack;
		// maps our kernal index to our cache index
		std::unordered_map<unsigned int, unsigned int> KernalToCacheMap;
		int CacheSize = 0;

		ANLtoC_EmitData(InstructionListType& k) : k(k) {}
	};


	// returns function name, stores function implementation in function list
	std::string SetupFunctionCall(ANLtoC_EmitData& Data, unsigned int index, std::vector<FunctionData> &FunctionList)
	{
		std::string FunctionName = "FunctionForIndex_" + std::to_string(index);

		// see if we already have a function setup
		for (FunctionData& d : FunctionList)
		{
			if (d.RelatedIndex == index)
				return FunctionName;
		}

		std::string function = 
			"double " + FunctionName + "(const Point EvalPoint, const ANL_CPP_NamedInput& NamedInput, bool CacheIsValid[], double Cache[])\n"
			"{\n"
			"\treturn ";

		function += InstructionToElement(Data, index, FunctionList);

		function +=
			";\n}\n";

		// Functions are pushed on the list in the order that is requred for proper dependency managment
		FunctionList.push_back({ function, index });

		return FunctionName;
	}
	
	// replaces ^ with the Point structure representing the current coordinates
	// replaces ~ with an evaluated statment
	template<int size>
	std::string RecursiveFormat(ANLtoC_EmitData& Data, std::string Format, std::array<unsigned int, size> args, std::vector<FunctionData> &FunctionList)
	{
		int ArgIndex = 0;
		for (int i = 0; i < Format.size(); ++i)
		{
			if (Format[i] == '~')
			{
				if (ArgIndex >= args.size())
					return "ArgIndex ERROR!";
				Format.erase(i, 1);
				std::string StringToInsert;

				unsigned int CacheIndex = 0;
				const bool IsCachable = IsOpCacheCandidate(Data.k, args[ArgIndex]);
				const bool IsFunctionCandidate = IsOpFunctionCandidate(Data.k, args[ArgIndex]);
				if(IsCachable)
				{
					auto CacheIndexItr = Data.KernalToCacheMap.find(args[ArgIndex]);
					if (CacheIndexItr == Data.KernalToCacheMap.end()) {
						Data.KernalToCacheMap[args[ArgIndex]] = Data.CacheSize;
						CacheIndex = Data.CacheSize;
						Data.CacheSize++;
					}
					else {
						CacheIndex = CacheIndexItr->second;
					}
					StringToInsert += "(CacheIsValid[" + std::to_string(CacheIndex) + "] ? (Cache[" + std::to_string(CacheIndex) + "]) : (CacheIsValid[" + std::to_string(CacheIndex) + "]=true,Cache[" + std::to_string(CacheIndex) + "]=(";
					//StringToInsert += "(CacheIsValid[" + std::to_string(args[ArgIndex]) + "] ? (Cache[" + std::to_string(args[ArgIndex]) + "]) : (CacheIsValid[" + std::to_string(args[ArgIndex]) + "]=true,Cache[" + std::to_string(args[ArgIndex]) + "]=(";
				}
				if (IsFunctionCandidate)
				{
					std::string FunctionName = SetupFunctionCall(Data, args[ArgIndex], FunctionList);
					StringToInsert += FunctionName + "(EvalPoint, NamedInput, CacheIsValid, Cache)";
				}
				else
				{
					StringToInsert += InstructionToElement(Data, args[ArgIndex], FunctionList);
				}

				if (IsCachable)
				{
					StringToInsert += ")))";
				}

				Format.insert(i, StringToInsert);
				i += (int)StringToInsert.size() - 1;
				ArgIndex++;
			}
			else if (Format[i] == '^')
			{
				Format.erase(i, 1);
				auto StringToInsert = Data.DomainInputStack.back();
				if (Data.DomainInputStack.size() == 1) {
					// we have "EvalPoint", make a modifiable copy of this variable
					StringToInsert = "Point(" + StringToInsert + ")";
				}
				Format.insert(i, StringToInsert);
				i += (int)StringToInsert.size() - 1;
			}
		}
		return Format;
	}

	std::string InstructionToElement(ANLtoC_EmitData& Data, unsigned int index, std::vector<FunctionData> &FunctionList)
	{
		std::array<unsigned int, 0> EmptyArgs = {};
		SInstruction& i = Data.k[index];
		switch (i.opcode_)
		{
		case OP_NOP:
		case OP_Seed:
		case OP_Constant:
			return ToString(i.outfloat_);

		case OP_NamedInput:
		{
			return "NamedInput." + i.namedInput;
		}

		case OP_ValueBasis:
		{
			std::array<unsigned int, 2> args;
			// { Interpolation, seed }
			args = { i.sources_[0], i.sources_[1], };
			return RecursiveFormat(Data, std::string("ValueBasis(^,(int)~,(unsigned int)~"), args, FunctionList);
		}

		case OP_GradientBasis:
		{
			std::array<unsigned int, 2> args;
			// { Interpolation, seed }
			args = { i.sources_[0], i.sources_[1], };
			return RecursiveFormat(Data, std::string("GradientBasis(^,(int)~,(unsigned int)~)"), args, FunctionList);
		}

		case OP_SimplexBasis:
		{
			std::array<unsigned int, 1> args;
			// { seed }
			args = { i.sources_[0], };
			return RecursiveFormat(Data, std::string("SimplexBasis(^,(unsigned int)~)"), args, FunctionList);
		}

		case OP_CellularBasis:
		{
			std::array<unsigned int, 10> args;
			args = { i.sources_[0], i.sources_[1], i.sources_[2], i.sources_[3], i.sources_[4], i.sources_[5], i.sources_[6], i.sources_[7], i.sources_[8], i.sources_[9] };
			return RecursiveFormat(Data, std::string("CellularBasis(^,(unsigned int)~,~,~,~,~,~,~,~,~,(unsigned int)~)"), args, FunctionList);
		}

		case OP_Add:
		{
			std::array<unsigned int, 2> args;
			args = { i.sources_[0], i.sources_[1] };
			return RecursiveFormat(Data, std::string("(~ + ~)"), args, FunctionList);
		}
		case OP_Subtract:
		{
			std::array<unsigned int, 2> args;
			args = { i.sources_[0], i.sources_[1] };
			return RecursiveFormat(Data, std::string("(~ - ~)"), args, FunctionList);
		}
		case OP_Multiply:
		{
			std::array<unsigned int, 2> args;
			args = { i.sources_[0], i.sources_[1] };
			return RecursiveFormat(Data, std::string("(~ * ~)"), args, FunctionList);
		}
		case OP_Divide:
		{
			std::array<unsigned int, 2> args;
			args = { i.sources_[0], i.sources_[1] };
			return RecursiveFormat(Data, std::string("(~ / ~)"), args, FunctionList);
		}

		case OP_Bias:
		{
			std::array<unsigned int, 2> args;
			args = { i.sources_[0], i.sources_[1] };
			return RecursiveFormat(Data, std::string("bias(std::max(0.0,std::min(1.0,~)), std::max(0.0,std::min(1.0,~)))"), args, FunctionList);
		}
		case OP_Gain:
		{
			std::array<unsigned int, 2> args;
			args = { i.sources_[0], i.sources_[1] };
			return RecursiveFormat(Data, std::string("gain(std::max(0.0,std::min(1.0,~)), std::max(0.0,std::min(1.0,~)))"), args, FunctionList);
		}
		case OP_Max:
		{
			std::array<unsigned int, 2> args;
			args = { i.sources_[0], i.sources_[1] };
			return RecursiveFormat(Data, std::string("std::max(~,~)"), args, FunctionList);
		}
		case OP_Min:
		{
			std::array<unsigned int, 2> args;
			args = { i.sources_[0], i.sources_[1] };
			return RecursiveFormat(Data, std::string("std::min(~,~)"), args, FunctionList);
		}
		case OP_Abs:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[0], };
			return RecursiveFormat(Data, std::string("std::abs(~)"), args, FunctionList);
		}
		case OP_Pow:
		{
			std::array<unsigned int, 2> args;
			args = { i.sources_[0], i.sources_[1], };
			return RecursiveFormat(Data, std::string("std::pow(~,~)"), args, FunctionList);
		}
		case OP_Cos:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[0], };
			return RecursiveFormat(Data, std::string("std::cos(~)"), args, FunctionList);
		}
		case OP_Sin:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[0], };
			return RecursiveFormat(Data, std::string("std::sin(~)"), args, FunctionList);
		}
		case OP_Tan:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[0], };
			return RecursiveFormat(Data, std::string("std::tan(~)"), args, FunctionList);
		}
		case OP_ACos:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[0], };
			return RecursiveFormat(Data, std::string("std::acos(~)"), args, FunctionList);
		}
		case OP_ASin:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[0], };
			return RecursiveFormat(Data, std::string("std::asin(~)"), args, FunctionList);
		}
		case OP_ATan:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[0], };
			return RecursiveFormat(Data, std::string("std::atan(~)"), args, FunctionList);
		}

		case OP_Tiers:
		{
			std::array<unsigned int, 2> args;
			// { value, number of steps }
			args = { i.sources_[0], i.sources_[1], };
			return RecursiveFormat(Data, std::string("std::floor(~ * (double)((int)~))"), args, FunctionList);
		}

		case OP_SmoothTiers:
		{
			std::array<unsigned int, 2> args;
			// { value, number of steps }
			args = { i.sources_[0], i.sources_[1], };
			return RecursiveFormat(Data, std::string("SmoothTiers(~,~)"), args, FunctionList);
		}

		case OP_ScaleDomain:
		{
			std::array<unsigned int, 1> args;
			// scale i.sources_[0] by i.sources_[1]
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.Scale(~))"), args, FunctionList);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();
			return s;
		}

		case OP_ScaleX:
		{
			std::array<unsigned int, 1> args;
			// scale i.sources_[0] by i.sources_[1]
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.ScaleX(~))"), args, FunctionList);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();
			return s;
		}
		case OP_ScaleY:
		{
			std::array<unsigned int, 1> args;
			// scale i.sources_[0] by i.sources_[1]
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.ScaleY(~))"), args, FunctionList);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();
			return s;
		}
		case OP_ScaleZ:
		{
			std::array<unsigned int, 1> args;
			// scale i.sources_[0] by i.sources_[1]
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.ScaleZ(~))"), args, FunctionList);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();
			return s;
		}
		case OP_ScaleW:
		{
			std::array<unsigned int, 1> args;
			// scale i.sources_[0] by i.sources_[1]
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.ScaleW(~))"), args, FunctionList);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();
			return s;
		}
		case OP_ScaleU:
		{
			std::array<unsigned int, 1> args;
			// scale i.sources_[0] by i.sources_[1]
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.ScaleU(~))"), args, FunctionList);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();
			return s;
		}
		case OP_ScaleV:
		{
			std::array<unsigned int, 1> args;
			// scale i.sources_[0] by i.sources_[1]
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.ScaleV(~))"), args, FunctionList);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();
			return s;
		}

		case OP_TranslateX:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.TranslateX(~))"), args, FunctionList);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();
			return s;
		}
		case OP_TranslateY:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.TranslateY(~))"), args, FunctionList);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();
			return s;
		}
		case OP_TranslateZ:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.TranslateZ(~))"), args, FunctionList);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();
			return s;
		}
		case OP_TranslateW:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.TranslateW(~))"), args, FunctionList);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();
			return s;
		}
		case OP_TranslateU:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.TranslateU(~))"), args, FunctionList);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();
			return s;
		}
		case OP_TranslateV:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.TranslateV(~))"), args, FunctionList);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();
			return s;
		}

		case OP_TranslateDomain:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.Translate(~))"), args, FunctionList);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();
			return s;
		}

		case OP_RotateDomain:
		{
			std::array<unsigned int, 4> args;
			args = { i.sources_[1], i.sources_[2], i.sources_[3], i.sources_[4], };
			std::string s = RecursiveFormat(Data, std::string("RotateDomain(^,~,~,~,~)"), args, FunctionList);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();
			return s;
		}

		case OP_Blend:
		{
			std::array<unsigned int, 4> args;
			// { low, high, low, control }
			args = { i.sources_[0], i.sources_[1], i.sources_[0], i.sources_[2] };
			return RecursiveFormat(Data, std::string("(~ + (~ - ~) * ~)"), args, FunctionList);
		}
		case OP_Select:
		{
			std::array<unsigned int, 18> args;
			//i.sources_[0], i.sources_[1], i.sources_[2], i.sources_[3], i.sources_[4],
			// low,				 high,			control,		threshold,		falloff,

			// {	falloff,			control,	threshold,		falloff,		low,		control,		threshold,		falloff,		high,			low,			high,		control,		threshold,		falloff,	control,		threshold,		low,			high }
			args = { i.sources_[4], i.sources_[2], i.sources_[3], i.sources_[4], i.sources_[0], i.sources_[2], i.sources_[3], i.sources_[4], i.sources_[1], i.sources_[0], i.sources_[1], i.sources_[2], i.sources_[3], i.sources_[4], i.sources_[2], i.sources_[3], i.sources_[0], i.sources_[1], };
			std::string s = R"(
			((/*falloff*/~ > 0.0) ?
			(
				((/*control*/~<(/*threshold*/~ - /*falloff*/~)) ?
				(
					/*low*/~
				)
					: ((/*control*/~>(/*threshold*/~ + /*falloff*/~)) ?
					(
						/*high*/~
					)
						:
					(
						/*low high blend*/
						Select_Blend(~,~,~,~,~)
					))
				)
			)
				:
			(
				((/*control*/~ < /*threshold*/~) ?
				(
					/*low*/~
				)
					:
				(
					/*high*/~
				))
			))
			)";
			return RecursiveFormat(Data, std::string(s), args, FunctionList);
		}

		case OP_X: return RecursiveFormat(Data, std::string("(^.x)"), EmptyArgs, FunctionList);
		case OP_Y: return RecursiveFormat(Data, std::string("(^.y)"), EmptyArgs, FunctionList);
		case OP_Z: return RecursiveFormat(Data, std::string("(^.z)"), EmptyArgs, FunctionList);
		case OP_W: return RecursiveFormat(Data, std::string("(^.w)"), EmptyArgs, FunctionList);
		case OP_U: return RecursiveFormat(Data, std::string("(^.u)"), EmptyArgs, FunctionList);
		case OP_V: return RecursiveFormat(Data, std::string("(^.v)"), EmptyArgs, FunctionList);

		case OP_DX:
		{
			std::array<unsigned int, 1> args;
			// { value, spacing }
			args = { i.sources_[0] };// value
			std::string OriginalValue = RecursiveFormat(Data, std::string("~"), args, FunctionList);
			args = { i.sources_[1] };// spacing
			std::string TranslateToNewPoint = RecursiveFormat(Data, std::string("(^ + Point(~,0.0,0.0,0.0,0.0,0.0))"), args, FunctionList);
			Data.DomainInputStack.push_back(TranslateToNewPoint);
			args = { i.sources_[0] };// value
			std::string TranslatedValue = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();

			std::string FinalResult;
			args = { i.sources_[1] };// spacing
			return RecursiveFormat(Data, std::string("((" + OriginalValue + " - " + TranslatedValue + ") / ~)"), args, FunctionList);
		}
		case OP_DY:
		{
			std::array<unsigned int, 1> args;
			// { value, spacing }
			args = { i.sources_[0] };// value
			std::string OriginalValue = RecursiveFormat(Data, std::string("~"), args, FunctionList);
			args = { i.sources_[1] };// spacing
			std::string TranslateToNewPoint = RecursiveFormat(Data, std::string("(^ + Point(0.0,~,0.0,0.0,0.0,0.0))"), args, FunctionList);
			Data.DomainInputStack.push_back(TranslateToNewPoint);
			args = { i.sources_[0] };// value
			std::string TranslatedValue = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();

			std::string FinalResult;
			args = { i.sources_[1] };// spacing
			return RecursiveFormat(Data, std::string("((" + OriginalValue + " - " + TranslatedValue + ") / ~)"), args, FunctionList);
		}
		case OP_DZ:
		{
			std::array<unsigned int, 1> args;
			// { value, spacing }
			args = { i.sources_[0] };// value
			std::string OriginalValue = RecursiveFormat(Data, std::string("~"), args, FunctionList);
			args = { i.sources_[1] };// spacing
			std::string TranslateToNewPoint = RecursiveFormat(Data, std::string("(^ + Point(0.0,0.0,~,0.0,0.0,0.0))"), args, FunctionList);
			Data.DomainInputStack.push_back(TranslateToNewPoint);
			args = { i.sources_[0] };// value
			std::string TranslatedValue = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();

			std::string FinalResult;
			args = { i.sources_[1] };// spacing
			return RecursiveFormat(Data, std::string("((" + OriginalValue + " - " + TranslatedValue + ") / ~)"), args, FunctionList);
		}
		case OP_DW:
		{
			std::array<unsigned int, 1> args;
			// { value, spacing }
			args = { i.sources_[0] };// value
			std::string OriginalValue = RecursiveFormat(Data, std::string("~"), args, FunctionList);
			args = { i.sources_[1] };// spacing
			std::string TranslateToNewPoint = RecursiveFormat(Data, std::string("(^ + Point(0.0,0.0,0.0,~,0.0,0.0))"), args, FunctionList);
			Data.DomainInputStack.push_back(TranslateToNewPoint);
			args = { i.sources_[0] };// value
			std::string TranslatedValue = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();

			std::string FinalResult;
			args = { i.sources_[1] };// spacing
			return RecursiveFormat(Data, std::string("((" + OriginalValue + " - " + TranslatedValue + ") / ~)"), args, FunctionList);
		}
		case OP_DU:
		{
			std::array<unsigned int, 1> args;
			// { value, spacing }
			args = { i.sources_[0] };// value
			std::string OriginalValue = RecursiveFormat(Data, std::string("~"), args, FunctionList);
			args = { i.sources_[1] };// spacing
			std::string TranslateToNewPoint = RecursiveFormat(Data, std::string("(^ + Point(0.0,0.0,0.0,0.0,~,0.0))"), args, FunctionList);
			Data.DomainInputStack.push_back(TranslateToNewPoint);
			args = { i.sources_[0] };// value
			std::string TranslatedValue = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();

			std::string FinalResult;
			args = { i.sources_[1] };// spacing
			return RecursiveFormat(Data, std::string("((" + OriginalValue + " - " + TranslatedValue + ") / ~)"), args, FunctionList);
		}
		case OP_DV:
		{
			std::array<unsigned int, 1> args;
			// { value, spacing }
			args = { i.sources_[0] };// value
			std::string OriginalValue = RecursiveFormat(Data, std::string("~"), args, FunctionList);
			args = { i.sources_[1] };// spacing
			std::string TranslateToNewPoint = RecursiveFormat(Data, std::string("(^ + Point(0.0,0.0,0.0,0.0,0.0,~))"), args, FunctionList);
			Data.DomainInputStack.push_back(TranslateToNewPoint);
			args = { i.sources_[0] };// value
			std::string TranslatedValue = InstructionToElement(Data, i.sources_[0], FunctionList);
			Data.DomainInputStack.pop_back();

			std::string FinalResult;
			args = { i.sources_[1] };// spacing
			return RecursiveFormat(Data, std::string("((" + OriginalValue + " - " + TranslatedValue + ") / ~)"), args, FunctionList);
		}

		case OP_Sigmoid:
		{
			std::array<unsigned int, 3> args;
			// { r, s, c }
			args = { i.sources_[2], i.sources_[0], i.sources_[1] };
			return RecursiveFormat(Data, std::string("(1.0 / (1.0 + std::exp(-~ * (~ - ~)))"), args, FunctionList);
		}
		case OP_Radial:
		{
			std::array<unsigned int, 0> args = {};
			return RecursiveFormat(Data, std::string("(^.Length())"), args, FunctionList);
		}
		case OP_Clamp:
		{
			std::array<unsigned int, 3> args;
			// { low, high, value }
			args = { i.sources_[1], i.sources_[2], i.sources_[0] };
			return RecursiveFormat(Data, std::string("std::max(~, std::min(~, ~))"), args, FunctionList);
		}
		case OP_HexTile:
		{
			std::array<unsigned int, 1> args;
			// { seed }
			args = { i.sources_[0] };
			return RecursiveFormat(Data, std::string("HexTile(^,(unsigned int)~)"), args, FunctionList);
		}
		case OP_HexBump:
		{
			std::array<unsigned int, 0> args = {};
			return RecursiveFormat(Data, std::string("HexBump(^)"), args, FunctionList);
		}
		
		case OP_Color:
			return "OP_Color is unsupported.";
		case OP_ExtractRed:
			return "OP_ExtractRed is unsupported.";
		case OP_ExtractGreen:
			return "OP_ExtractGreen is unsupported.";
		case OP_ExtractBlue:
			return "OP_Color is unsupported.";
		case OP_ExtractAlpha:
			return "OP_Color is unsupported.";
		case OP_Grayscale:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[0] };
			return RecursiveFormat(Data, std::string("~"), args, FunctionList);
		}
		case OP_CombineRGBA:
			return "OP_CombineRGBA is unsupported.";

		default:
			return "Error!";
		}
	}
}

void ANLtoC::KernelToC(anl::CKernel& Kernel, anl::CInstructionIndex& Root, std::string& ExpressionToExecute, std::string& NamedInputStructGuts, std::vector<FunctionData> &FunctionList)
{
	ANLtoC_EmitData Data(*Kernel.getKernel());
	Data.DomainInputStack.push_back("EvalPoint");

	unsigned int index = Root.GetIndex();
	
	std::string Body = InstructionToElement(Data, index, FunctionList);

	// search through the Kernel and generate a list of all NamedInput
	NamedInputStructGuts.clear();
	std::vector<std::tuple<std::string, double>> NameList = Kernel.ListNamedInput();
	for (auto& NameValuePair : NameList)
	{
		std::string& Name = std::get<0>(NameValuePair);
		double DefaultValue = std::get<1>(NameValuePair);

		NamedInputStructGuts += "\tdouble " + Name + " = " + ToString(DefaultValue) + ";\n";
	}
	

	ExpressionToExecute.clear();
	ExpressionToExecute += "\tbool CacheIsValid[" + std::to_string(Data.CacheSize) + "];\n";
	ExpressionToExecute += "\tdouble Cache[" + std::to_string(Data.CacheSize) + "];\n";
	ExpressionToExecute += "\tfor(int i = 0; i < " + std::to_string(Data.CacheSize) + "; ++i)\n";
	ExpressionToExecute += "\t\tCacheIsValid[i] = false;\n";
	ExpressionToExecute += "\n";
	ExpressionToExecute += "\tdouble FinalResult = ";
	ExpressionToExecute += Body;
	ExpressionToExecute += ";";
}



