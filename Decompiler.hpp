#pragma once

#include "MiniRISC.hpp"
#include "Opcode.hpp"

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

// Converts RISC-V assembly instructions into C++-like pseudo-code
class Decompiler {
public:
	// Decompile a single instruction into C++ pseudo-code
	static std::string decompile(const Instruction& instr) {
		std::ostringstream out;
		const char* rd = regName(instr.rd);
		const char* rs1 = regName(instr.rs1);
		const char* rs2 = regName(instr.rs2);
		const int imm = instr.immediate;

		switch (instr.opcode) {
		case HALT:  out << "exit(0);"; break;
		case ADD:   out << rd << " = " << rs1 << " + " << rs2 << ";"; break;
		case SUB:   out << rd << " = " << rs1 << " - " << rs2 << ";"; break;
		case ADDI:  out << rd << " = " << rs1 << " + " << imm << ";"; break;
		case AND:   out << rd << " = " << rs1 << " & " << rs2 << ";"; break;
		case OR:    out << rd << " = " << rs1 << " | " << rs2 << ";"; break;
		case XOR:   out << rd << " = " << rs1 << " ^ " << rs2 << ";"; break;
		case ANDI:  out << rd << " = " << rs1 << " & " << imm << ";"; break;
		case ORI:   out << rd << " = " << rs1 << " | " << imm << ";"; break;
		case XORI:  out << rd << " = " << rs1 << " ^ " << imm << ";"; break;
		case SLL:   out << rd << " = " << rs1 << " << " << rs2 << ";"; break;
		case SRL:   out << rd << " = (unsigned)" << rs1 << " >> " << rs2 << ";"; break;
		case SRA:   out << rd << " = (signed)" << rs1 << " >> " << rs2 << ";"; break;
		case SLLI:  out << rd << " = " << rs1 << " << " << imm << ";"; break;
		case SRLI:  out << rd << " = (unsigned)" << rs1 << " >> " << imm << ";"; break;
		case SRAI:  out << rd << " = (signed)" << rs1 << " >> " << imm << ";"; break;
		case SLT:   out << rd << " = ((signed)" << rs1 << " < (signed)" << rs2 << ") ? 1 : 0;"; break;
		case SLTU:  out << rd << " = ((unsigned)" << rs1 << " < (unsigned)" << rs2 << ") ? 1 : 0;"; break;
		case SLTI:  out << rd << " = ((signed)" << rs1 << " < " << imm << ") ? 1 : 0;"; break;
		case SLTIU: out << rd << " = ((unsigned)" << rs1 << " < " << imm << ") ? 1 : 0;"; break;
		case MUL:   out << rd << " = " << rs1 << " * " << rs2 << ";"; break;
		case MULH:  out << rd << " = ((int64_t)(signed)" << rs1 << " * (signed)" << rs2 << ") >> 32;"; break;
		case MULHSU:out << rd << " = ((int64_t)(signed)" << rs1 << " * (unsigned)" << rs2 << ") >> 32;"; break;
		case MULHU: out << rd << " = ((uint64_t)" << rs1 << " * " << rs2 << ") >> 32;"; break;
		case DIV:   out << rd << " = (signed)" << rs1 << " / (signed)" << rs2 << ";"; break;
		case DIVU:  out << rd << " = (unsigned)" << rs1 << " / (unsigned)" << rs2 << ";"; break;
		case REM:   out << rd << " = (signed)" << rs1 << " % (signed)" << rs2 << ";"; break;
		case REMU:  out << rd << " = (unsigned)" << rs1 << " % (unsigned)" << rs2 << ";"; break;

		// Memory operations
		case LW:
		case LDR:
			out << rd << " = *(int32_t*)(memory + " << rs1 << " + " << imm << ");"; break;
		case LH:
			out << rd << " = *(int16_t*)(memory + " << rs1 << " + " << imm << ");"; break;
		case LB:
			out << rd << " = *(int8_t*)(memory + " << rs1 << " + " << imm << ");"; break;
		case LHU:
			out << rd << " = *(uint16_t*)(memory + " << rs1 << " + " << imm << ");"; break;
		case LBU:
			out << rd << " = *(uint8_t*)(memory + " << rs1 << " + " << imm << ");"; break;
		case SW:
		case STR:
			out << "*(int32_t*)(memory + " << rs1 << " + " << imm << ") = " << rs2 << ";"; break;
		case SH:
			out << "*(int16_t*)(memory + " << rs1 << " + " << imm << ") = " << rs2 << ";"; break;
		case SB:
			out << "*(int8_t*)(memory + " << rs1 << " + " << imm << ") = " << rs2 << ";"; break;

		// Branches
		case BEQ:
			out << "if (" << rs1 << " == " << rs2 << ") goto PC+" << imm << ";"; break;
		case BNE:
			out << "if (" << rs1 << " != " << rs2 << ") goto PC+" << imm << ";"; break;
		case BLT:
			out << "if ((signed)" << rs1 << " < (signed)" << rs2 << ") goto PC+" << imm << ";"; break;
		case BGE:
			out << "if ((signed)" << rs1 << " >= (signed)" << rs2 << ") goto PC+" << imm << ";"; break;
		case BLTU:
			out << "if ((unsigned)" << rs1 << " < (unsigned)" << rs2 << ") goto PC+" << imm << ";"; break;
		case BGEU:
			out << "if ((unsigned)" << rs1 << " >= (unsigned)" << rs2 << ") goto PC+" << imm << ";"; break;

		// Jumps
		case JAL:
			if (instr.rd == 0)
				out << "goto PC+" << imm << ";";
			else if (instr.rd == 1)
				out << "call function_at(PC+" << imm << ");";
			else
				out << rd << " = PC+4; goto PC+" << imm << ";";
			break;
		case JALR:
			if (instr.rd == 0 && instr.rs1 == 1 && imm == 0)
				out << "return;";
			else if (instr.rd == 0)
				out << "goto *(" << rs1 << " + " << imm << ");";
			else
				out << rd << " = PC+4; goto *(" << rs1 << " + " << imm << ");";
			break;

		// Upper immediate
		case LUI:
			out << rd << " = " << imm << " << 12;  // = 0x" << std::hex << (static_cast<unsigned>(imm) << 12) << std::dec << ";"; break;
		case AUIPC:
			out << rd << " = PC + (" << imm << " << 12);"; break;

		// System
		case FENCE:
		case FENCE_I:
			out << "__sync_synchronize();"; break;
		case ECALL:
			out << "syscall();"; break;
		case EBREAK:
			out << "__debugbreak();"; break;

		default:
			out << "/* unknown instruction */";
		}
		return out.str();
	}

	// Decompile a full program to C++ pseudo-code
	static void decompileProgram(const MiniRISC& cpu,
								std::ostream& output = std::cout) {
		output << "// Decompiled RISC-V program -> C++ pseudo-code\n";
		output << "// Registers: x0-x31 (x0=zero, x1=ra, x2=sp, x10-x17=a0-a7)\n";
		output << "\n";
		output << "void program() {\n";
		output << "    uint32_t x[32] = {0};  // 32 registers\n";
		output << "    uint8_t memory[4096];  // memory\n";
		output << "\n";

		for (std::size_t i = 0; i < cpu.getProgramSize(); ++i) {
			const Instruction* instr = cpu.getInstructionAt(i);
			if (!instr) break;

			output << "    /* 0x" << std::hex << std::setw(4) << std::setfill('0')
				   << (i * 4) << std::dec << " */  ";
			output << decompile(*instr) << "\n";
		}

		output << "}\n";
	}

private:
	static const char* regName(unsigned int reg) {
		static const char* names[32] = {
			"zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
			"s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
			"a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
			"s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
		};
		if (reg < 32) return names[reg];
		return "???";
	}
};
