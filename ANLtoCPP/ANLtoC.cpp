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

using namespace anl;

namespace ANLtoC {
	struct Element
	{
		unsigned int Opcode;
		std::string Format;
	};
}

namespace ANLtoC {

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

	struct ANLtoC_EmitData
	{
		InstructionListType& k;
		std::vector<std::string> DomainInputStack;
		// maps our kernal index to our cache index
		std::unordered_map<unsigned int, unsigned int> KernalToCacheMap;
		int CacheSize = 0;

		ANLtoC_EmitData(InstructionListType& k) : k(k) {}
	};
	
	// replaces ^ with the Point structure representing the current coordinates
	// replaces ~ with an evaluated statment
	template<int size>
	std::string RecursiveFormat(ANLtoC_EmitData& Data, std::string Format, std::array<unsigned int, size> args)
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

				StringToInsert += InstructionToElement(Data, args[ArgIndex]);

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

	std::string InstructionToElement(ANLtoC_EmitData& Data, unsigned int index)
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
			return RecursiveFormat(Data, std::string("ValueBasis(^,(int)~,(unsigned int)~"), args);
		}

		case OP_GradientBasis:
		{
			std::array<unsigned int, 2> args;
			// { Interpolation, seed }
			args = { i.sources_[0], i.sources_[1], };
			return RecursiveFormat(Data, std::string("GradientBasis(^,(int)~,(unsigned int)~)"), args);
		}

		case OP_SimplexBasis:
		{
			std::array<unsigned int, 1> args;
			// { seed }
			args = { i.sources_[0], };
			return RecursiveFormat(Data, std::string("SimplexBasis(^,(unsigned int)~)"), args);
		}

		case OP_CellularBasis:
		{
			std::array<unsigned int, 10> args;
			args = { i.sources_[0], i.sources_[1], i.sources_[2], i.sources_[3], i.sources_[4], i.sources_[5], i.sources_[6], i.sources_[7], i.sources_[8], i.sources_[9] };
			return RecursiveFormat(Data, std::string("CellularBasis(^,(unsigned int)~,~,~,~,~,~,~,~,~,(unsigned int)~)"), args);
		}

		case OP_Add:
		{
			std::array<unsigned int, 2> args;
			args = { i.sources_[0], i.sources_[1] };
			return RecursiveFormat(Data, std::string("(~ + ~)"), args);
		}
		case OP_Subtract:
		{
			std::array<unsigned int, 2> args;
			args = { i.sources_[0], i.sources_[1] };
			return RecursiveFormat(Data, std::string("(~ - ~)"), args);
		}
		case OP_Multiply:
		{
			std::array<unsigned int, 2> args;
			args = { i.sources_[0], i.sources_[1] };
			return RecursiveFormat(Data, std::string("(~ * ~)"), args);
		}
		case OP_Divide:
		{
			std::array<unsigned int, 2> args;
			args = { i.sources_[0], i.sources_[1] };
			return RecursiveFormat(Data, std::string("(~ / ~)"), args);
		}

		case OP_Bias:
		{
			std::array<unsigned int, 2> args;
			args = { i.sources_[0], i.sources_[1] };
			return RecursiveFormat(Data, std::string("bias(std::max(0.0,std::min(1.0,~)), std::max(0.0,std::min(1.0,~)))"), args);
		}
		case OP_Gain:
		{
			std::array<unsigned int, 2> args;
			args = { i.sources_[0], i.sources_[1] };
			return RecursiveFormat(Data, std::string("gain(std::max(0.0,std::min(1.0,~)), std::max(0.0,std::min(1.0,~)))"), args);
		}
		case OP_Max:
		{
			std::array<unsigned int, 2> args;
			args = { i.sources_[0], i.sources_[1] };
			return RecursiveFormat(Data, std::string("std::max(~,~)"), args);
		}
		case OP_Min:
		{
			std::array<unsigned int, 2> args;
			args = { i.sources_[0], i.sources_[1] };
			return RecursiveFormat(Data, std::string("std::min(~,~)"), args);
		}
		case OP_Abs:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[0], };
			return RecursiveFormat(Data, std::string("std::abs(~)"), args);
		}
		case OP_Pow:
		{
			std::array<unsigned int, 2> args;
			args = { i.sources_[0], i.sources_[1], };
			return RecursiveFormat(Data, std::string("std::pow(~,~)"), args);
		}
		case OP_Cos:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[0], };
			return RecursiveFormat(Data, std::string("std::cos(~)"), args);
		}
		case OP_Sin:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[0], };
			return RecursiveFormat(Data, std::string("std::sin(~)"), args);
		}
		case OP_Tan:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[0], };
			return RecursiveFormat(Data, std::string("std::tan(~)"), args);
		}
		case OP_ACos:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[0], };
			return RecursiveFormat(Data, std::string("std::acos(~)"), args);
		}
		case OP_ASin:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[0], };
			return RecursiveFormat(Data, std::string("std::asin(~)"), args);
		}
		case OP_ATan:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[0], };
			return RecursiveFormat(Data, std::string("std::atan(~)"), args);
		}

		case OP_Tiers:
		{
			std::array<unsigned int, 2> args;
			// { value, number of steps }
			args = { i.sources_[0], i.sources_[1], };
			return RecursiveFormat(Data, std::string("std::floor(~ * (double)((int)~))"), args);
		}

		case OP_SmoothTiers:
		{
			std::array<unsigned int, 2> args;
			// { value, number of steps }
			args = { i.sources_[0], i.sources_[1], };
			return RecursiveFormat(Data, std::string("SmoothTiers(~,~)"), args);
		}

		case OP_ScaleDomain:
		{
			std::array<unsigned int, 1> args;
			// scale i.sources_[0] by i.sources_[1]
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.Scale(~))"), args);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();
			return s;
		}

		case OP_ScaleX:
		{
			std::array<unsigned int, 1> args;
			// scale i.sources_[0] by i.sources_[1]
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.ScaleX(~))"), args);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();
			return s;
		}
		case OP_ScaleY:
		{
			std::array<unsigned int, 1> args;
			// scale i.sources_[0] by i.sources_[1]
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.ScaleY(~))"), args);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();
			return s;
		}
		case OP_ScaleZ:
		{
			std::array<unsigned int, 1> args;
			// scale i.sources_[0] by i.sources_[1]
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.ScaleZ(~))"), args);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();
			return s;
		}
		case OP_ScaleW:
		{
			std::array<unsigned int, 1> args;
			// scale i.sources_[0] by i.sources_[1]
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.ScaleW(~))"), args);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();
			return s;
		}
		case OP_ScaleU:
		{
			std::array<unsigned int, 1> args;
			// scale i.sources_[0] by i.sources_[1]
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.ScaleU(~))"), args);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();
			return s;
		}
		case OP_ScaleV:
		{
			std::array<unsigned int, 1> args;
			// scale i.sources_[0] by i.sources_[1]
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.ScaleV(~))"), args);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();
			return s;
		}

		case OP_TranslateX:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.TranslateX(~))"), args);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();
			return s;
		}
		case OP_TranslateY:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.TranslateY(~))"), args);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();
			return s;
		}
		case OP_TranslateZ:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.TranslateZ(~))"), args);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();
			return s;
		}
		case OP_TranslateW:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.TranslateW(~))"), args);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();
			return s;
		}
		case OP_TranslateU:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.TranslateU(~))"), args);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();
			return s;
		}
		case OP_TranslateV:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.TranslateV(~))"), args);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();
			return s;
		}

		case OP_TranslateDomain:
		{
			std::array<unsigned int, 1> args;
			args = { i.sources_[1] };
			std::string s = RecursiveFormat(Data, std::string("(^.Translate(~))"), args);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();
			return s;
		}

		case OP_RotateDomain:
		{
			std::array<unsigned int, 4> args;
			args = { i.sources_[1], i.sources_[2], i.sources_[3], i.sources_[4], };
			std::string s = RecursiveFormat(Data, std::string("RotateDomain(^,~,~,~,~)"), args);
			Data.DomainInputStack.push_back(s);
			s = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();
			return s;
		}

		case OP_Blend:
		{
			std::array<unsigned int, 4> args;
			// { low, high, low, control }
			args = { i.sources_[0], i.sources_[1], i.sources_[0], i.sources_[2] };
			return RecursiveFormat(Data, std::string("(~ + (~ - ~) * ~)"), args);
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
			return RecursiveFormat(Data, std::string(s), args);
		}

		case OP_X: return RecursiveFormat(Data, std::string("(^.x)"), EmptyArgs);
		case OP_Y: return RecursiveFormat(Data, std::string("(^.y)"), EmptyArgs);
		case OP_Z: return RecursiveFormat(Data, std::string("(^.z)"), EmptyArgs);
		case OP_W: return RecursiveFormat(Data, std::string("(^.w)"), EmptyArgs);
		case OP_U: return RecursiveFormat(Data, std::string("(^.u)"), EmptyArgs);
		case OP_V: return RecursiveFormat(Data, std::string("(^.v)"), EmptyArgs);

		case OP_DX:
		{
			std::array<unsigned int, 1> args;
			// { value, spacing }
			args = { i.sources_[0] };// value
			std::string OriginalValue = RecursiveFormat(Data, std::string("~"), args);
			args = { i.sources_[1] };// spacing
			std::string TranslateToNewPoint = RecursiveFormat(Data, std::string("(^ + Point(~,0.0,0.0,0.0,0.0,0.0))"), args);
			Data.DomainInputStack.push_back(TranslateToNewPoint);
			args = { i.sources_[0] };// value
			std::string TranslatedValue = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();

			std::string FinalResult;
			args = { i.sources_[1] };// spacing
			return RecursiveFormat(Data, std::string("((" + OriginalValue + " - " + TranslatedValue + ") / ~)"), args);
		}
		case OP_DY:
		{
			std::array<unsigned int, 1> args;
			// { value, spacing }
			args = { i.sources_[0] };// value
			std::string OriginalValue = RecursiveFormat(Data, std::string("~"), args);
			args = { i.sources_[1] };// spacing
			std::string TranslateToNewPoint = RecursiveFormat(Data, std::string("(^ + Point(0.0,~,0.0,0.0,0.0,0.0))"), args);
			Data.DomainInputStack.push_back(TranslateToNewPoint);
			args = { i.sources_[0] };// value
			std::string TranslatedValue = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();

			std::string FinalResult;
			args = { i.sources_[1] };// spacing
			return RecursiveFormat(Data, std::string("((" + OriginalValue + " - " + TranslatedValue + ") / ~)"), args);
		}
		case OP_DZ:
		{
			std::array<unsigned int, 1> args;
			// { value, spacing }
			args = { i.sources_[0] };// value
			std::string OriginalValue = RecursiveFormat(Data, std::string("~"), args);
			args = { i.sources_[1] };// spacing
			std::string TranslateToNewPoint = RecursiveFormat(Data, std::string("(^ + Point(0.0,0.0,~,0.0,0.0,0.0))"), args);
			Data.DomainInputStack.push_back(TranslateToNewPoint);
			args = { i.sources_[0] };// value
			std::string TranslatedValue = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();

			std::string FinalResult;
			args = { i.sources_[1] };// spacing
			return RecursiveFormat(Data, std::string("((" + OriginalValue + " - " + TranslatedValue + ") / ~)"), args);
		}
		case OP_DW:
		{
			std::array<unsigned int, 1> args;
			// { value, spacing }
			args = { i.sources_[0] };// value
			std::string OriginalValue = RecursiveFormat(Data, std::string("~"), args);
			args = { i.sources_[1] };// spacing
			std::string TranslateToNewPoint = RecursiveFormat(Data, std::string("(^ + Point(0.0,0.0,0.0,~,0.0,0.0))"), args);
			Data.DomainInputStack.push_back(TranslateToNewPoint);
			args = { i.sources_[0] };// value
			std::string TranslatedValue = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();

			std::string FinalResult;
			args = { i.sources_[1] };// spacing
			return RecursiveFormat(Data, std::string("((" + OriginalValue + " - " + TranslatedValue + ") / ~)"), args);
		}
		case OP_DU:
		{
			std::array<unsigned int, 1> args;
			// { value, spacing }
			args = { i.sources_[0] };// value
			std::string OriginalValue = RecursiveFormat(Data, std::string("~"), args);
			args = { i.sources_[1] };// spacing
			std::string TranslateToNewPoint = RecursiveFormat(Data, std::string("(^ + Point(0.0,0.0,0.0,0.0,~,0.0))"), args);
			Data.DomainInputStack.push_back(TranslateToNewPoint);
			args = { i.sources_[0] };// value
			std::string TranslatedValue = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();

			std::string FinalResult;
			args = { i.sources_[1] };// spacing
			return RecursiveFormat(Data, std::string("((" + OriginalValue + " - " + TranslatedValue + ") / ~)"), args);
		}
		case OP_DV:
		{
			std::array<unsigned int, 1> args;
			// { value, spacing }
			args = { i.sources_[0] };// value
			std::string OriginalValue = RecursiveFormat(Data, std::string("~"), args);
			args = { i.sources_[1] };// spacing
			std::string TranslateToNewPoint = RecursiveFormat(Data, std::string("(^ + Point(0.0,0.0,0.0,0.0,0.0,~))"), args);
			Data.DomainInputStack.push_back(TranslateToNewPoint);
			args = { i.sources_[0] };// value
			std::string TranslatedValue = InstructionToElement(Data, i.sources_[0]);
			Data.DomainInputStack.pop_back();

			std::string FinalResult;
			args = { i.sources_[1] };// spacing
			return RecursiveFormat(Data, std::string("((" + OriginalValue + " - " + TranslatedValue + ") / ~)"), args);
		}

		case OP_Sigmoid:
		{
			std::array<unsigned int, 3> args;
			// { r, s, c }
			args = { i.sources_[2], i.sources_[0], i.sources_[1] };
			return RecursiveFormat(Data, std::string("(1.0 / (1.0 + std::exp(-~ * (~ - ~)))"), args);
		}
		case OP_Radial:
		{
			std::array<unsigned int, 0> args = {};
			return RecursiveFormat(Data, std::string("(^.Length())"), args);
		}
		case OP_Clamp:
		{
			std::array<unsigned int, 3> args;
			// { low, high, value }
			args = { i.sources_[1], i.sources_[2], i.sources_[0] };
			return RecursiveFormat(Data, std::string("std::max(~, std::min(~, ~))"), args);
		}
		case OP_HexTile:
		{
			std::array<unsigned int, 1> args;
			// { seed }
			args = { i.sources_[0] };
			return RecursiveFormat(Data, std::string("HexTile(^,(unsigned int)~)"), args);
		}
		case OP_HexBump:
		{
			std::array<unsigned int, 0> args = {};
			return RecursiveFormat(Data, std::string("HexBump(^)"), args);
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
			return RecursiveFormat(Data, std::string("~"), args);
		}
		case OP_CombineRGBA:
			return "OP_CombineRGBA is unsupported.";

		default:
			return "Error!";
		}
	}
}

void ANLtoC::KernelToC(anl::CKernel& Kernel, anl::CInstructionIndex& Root, std::string& ExpressionToExecute, std::string& NamedInputStructGuts)
{
	ANLtoC_EmitData Data(*Kernel.getKernel());
	Data.DomainInputStack.push_back("EvalPoint");

	unsigned int index = Root.GetIndex();
	
	std::string Body = InstructionToElement(Data, index);

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



