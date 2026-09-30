#pragma once

#include "Decompiler.hpp"
#include "MiniRISC.hpp"
#include "Opcode.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>

class Display {
public:
	enum class Mode {
		AllAtOnce,
		EachOperation
	};

	void showAll(const MiniRISC& cpu, std::ostream& output = std::cout,
				 bool showZeroMemory = false) const {
		output << "\n";
		output << "╔══════════════════════════════════════════════════════════════════╗\n";
		output << "║                    MiniRISC-V SPU Simulator                     ║\n";
		output << "╠══════════════════════════════════════════════════════════════════╣\n";
		output << "║ PC: " << std::setw(5) << cpu.getProgramCounter()
			   << " / " << std::setw(5) << cpu.getProgramSize()
			   << " instructions";
		output << std::string(33 - std::to_string(cpu.getProgramCounter()).size()
							   - std::to_string(cpu.getProgramSize()).size(), ' ');
		output << "║\n";
		output << "║ State: " << std::setw(10) << std::left << stateName(cpu.getState())
			   << std::right;
		output << " │ Executed: " << std::setw(8) << cpu.getExecutedCount()
			   << " instructions  ║\n";
		if (!cpu.getError().empty()) {
			output << "║ Error: " << cpu.getError();
			std::size_t padding = 57 - cpu.getError().size();
			if (padding > 0 && padding < 200) output << std::string(padding, ' ');
			output << "║\n";
		}
		output << "╠══════════════════════════════════════════════════════════════════╣\n";

		// Registers with ABI names
		output << "║ Registers (32 × 32-bit)                                        ║\n";
		output << "╟──────────────────────────────────────────────────────────────────╢\n";
		const std::ios::fmtflags oldFlags = output.flags();
		const char oldFill = output.fill();
		for (std::size_t index = 0; index < 32; ++index) {
			if (index % 4 == 0) output << "║ ";
			std::ostringstream regStr;
			regStr << 'x' << std::dec << index;
			std::string regLabel = regStr.str();
			std::string abiLabel = registerABIName(static_cast<unsigned>(index));
			output << std::setw(3) << std::left << regLabel << "("
				   << std::setw(4) << abiLabel << ")="
				   << "0x" << std::hex << std::setw(8) << std::setfill('0')
				   << std::right << cpu.getRegister(index)
				   << std::setfill(' ') << std::dec;
			if (index % 4 == 3)
				output << " ║\n";
			else
				output << " ";
		}
		output.flags(oldFlags);
		output.fill(oldFill);

		// Memory hex dump
		output << "╠══════════════════════════════════════════════════════════════════╣\n";
		output << "║ Memory (hex dump, non-zero regions)                            ║\n";
		output << "╟──────────────────────────────────────────────────────────────────╢\n";
		showMemoryHexDump(cpu, output, showZeroMemory);

		output << "╚══════════════════════════════════════════════════════════════════╝\n";
	}

	void showMemoryHexDump(const MiniRISC& cpu, std::ostream& output = std::cout,
						   bool showZero = false) const {
		const std::size_t memSize = cpu.getMemorySize();
		const std::ios::fmtflags oldFlags = output.flags();
		const char oldFill = output.fill();
		bool anyNonZero = false;

		for (std::size_t addr = 0; addr < memSize; addr += 16) {
			// Check if this 16-byte line has any non-zero bytes
			bool lineHasData = false;
			for (std::size_t j = 0; j < 16 && (addr + j) < memSize; ++j) {
				if (cpu.getMemoryByte(addr + j) != 0) {
					lineHasData = true;
					break;
				}
			}
			if (!lineHasData && !showZero) continue;
			anyNonZero = true;

			// Address
			output << "║ 0x" << std::hex << std::setw(4) << std::setfill('0')
				   << addr << ": ";

			// Hex bytes
			for (std::size_t j = 0; j < 16; ++j) {
				if ((addr + j) < memSize) {
					output << std::setw(2) << std::setfill('0')
						   << static_cast<unsigned>(cpu.getMemoryByte(addr + j));
				} else {
					output << "  ";
				}
				if (j == 7) output << "  ";
				else if (j < 15) output << " ";
			}

			// ASCII representation
			output << "  |";
			for (std::size_t j = 0; j < 16 && (addr + j) < memSize; ++j) {
				const unsigned char c = cpu.getMemoryByte(addr + j);
				output << (std::isprint(c) ? static_cast<char>(c) : '.');
			}
			output << "|";

			// Padding to align the box
			std::size_t lineBytes = std::min(static_cast<std::size_t>(16), memSize - addr);
			if (lineBytes < 16) {
				output << std::string(16 - lineBytes, ' ');
			}
			output << " ║\n";
		}

		if (!anyNonZero) {
			output << "║   (all memory is zero)                                          ║\n";
		}

		output.flags(oldFlags);
		output.fill(oldFill);
	}

	void showMemory(const MiniRISC& cpu, std::size_t start = 0,
					std::size_t length = 0, std::ostream& output = std::cout,
					bool showZeroBytes = false) const {
		const std::size_t memorySize = cpu.getMemorySize();
		if (start >= memorySize) {
			output << "  (empty range)\n";
			return;
		}
		const std::size_t count = length == 0
									  ? memorySize - start
									  : std::min(length, memorySize - start);
		bool displayedByte = false;
		const std::ios::fmtflags oldFlags = output.flags();
		const char oldFill = output.fill();
		for (std::size_t address = start; address < start + count; ++address) {
			const std::uint8_t value = cpu.getMemoryByte(address);
			if (value == 0 && !showZeroBytes) continue;
			output << "  0x" << std::hex << std::setw(8) << std::setfill('0')
				   << address << ": 0x" << std::setw(2)
				   << static_cast<unsigned int>(value) << '\n';
			displayedByte = true;
		}
		output.flags(oldFlags);
		output.fill(oldFill);
		if (!displayedByte && !showZeroBytes) {
			output << "  (all bytes in this range are zero)\n";
		}
	}

	void showProgram(const MiniRISC& cpu, std::ostream& output = std::cout) const {
		output << "\n┌─────────────────────────────────────────────────────────────────────────┐\n";
		output << "│ Program Listing (Assembly)                                              │\n";
		output << "├──────────┬──────────────────────────────┬───────────────────────────────┤\n";
		output << "│ Address  │ RISC-V Assembly               │ C++ Pseudo-code               │\n";
		output << "├──────────┼──────────────────────────────┼───────────────────────────────┤\n";
		for (std::size_t index = 0; index < cpu.getProgramSize(); ++index) {
			const Instruction* instruction = cpu.getInstructionAt(index);
			if (!instruction) break;

			// Address column
			std::ostringstream addrStr;
			addrStr << "0x" << std::hex << std::setw(4) << std::setfill('0') << (index * 4);

			// Assembly column
			std::ostringstream asmStr;
			showInstruction(*instruction, asmStr);

			// C++ pseudo-code column
			std::string cppStr = Decompiler::decompile(*instruction);

			output << "│ " << std::setw(8) << std::left << addrStr.str()
				   << " │ " << std::setw(28) << std::left << asmStr.str()
				   << " │ " << std::setw(29) << std::left << cppStr
				   << " │" << std::right << "\n";
		}
		output << "└──────────┴──────────────────────────────┴───────────────────────────────┘\n";
	}

	// Show decompiled C++ code for the full program
	void showDecompiled(const MiniRISC& cpu, std::ostream& output = std::cout) const {
		Decompiler::decompileProgram(cpu, output);
	}

	bool step(MiniRISC& cpu, std::ostream& output = std::cout) const {
		const std::size_t pc = cpu.getProgramCounter();
		const Instruction* instruction = cpu.getInstructionAt(pc);
		if (instruction == NULL) {
			cpu.step();
			output << "No instruction at PC " << pc << ".\n";
			showAll(cpu, output);
			return false;
		}

		output << "\n── Step " << cpu.getExecutedCount() + 1
			   << " ────────────────────────────────────────\n";
		output << "  PC 0x" << std::hex << std::setw(4) << std::setfill('0') << (pc * 4)
			   << std::dec << std::setfill(' ') << ": ";
		showInstruction(*instruction, output);
		output << "\n";
		output << "  C++: " << Decompiler::decompile(*instruction) << "\n";

		const bool executed = cpu.step();
		showAll(cpu, output);
		return executed;
	}

	std::size_t run(MiniRISC& cpu, Mode mode = Mode::AllAtOnce,
					std::ostream& output = std::cout,
					std::size_t maxSteps = 100000) const {
		std::size_t executed = 0;
		if (mode == Mode::EachOperation) {
			while (executed < maxSteps &&
				   cpu.getProgramCounter() < cpu.getProgramSize() &&
				   cpu.getState() != MiniRISC::State::Halted &&
				   cpu.getState() != MiniRISC::State::Faulted) {
				if (!step(cpu, output)) return executed;
				++executed;
			}
			if (cpu.getState() == MiniRISC::State::Running &&
				cpu.getProgramCounter() >= cpu.getProgramSize()) {
				cpu.step();
				showAll(cpu, output);
			}
			if (executed == maxSteps &&
				cpu.getState() != MiniRISC::State::Halted &&
				cpu.getState() != MiniRISC::State::Faulted) {
				output << "Execution stopped at the step limit.\n";
			}
			return executed;
		}

		while (executed < maxSteps && cpu.step()) ++executed;
		if (cpu.getState() == MiniRISC::State::Running &&
			cpu.getProgramCounter() >= cpu.getProgramSize()) {
			cpu.step();
		}
		if (executed == maxSteps &&
			cpu.getState() != MiniRISC::State::Halted &&
			cpu.getState() != MiniRISC::State::Faulted) {
			output << "Execution stopped at the step limit.\n";
		}
		showAll(cpu, output);
		return executed;
	}

private:
	static const char* stateName(MiniRISC::State state) {
		switch (state) {
		case MiniRISC::State::Ready: return "READY";
		case MiniRISC::State::Running: return "RUNNING";
		case MiniRISC::State::Halted: return "HALTED";
		case MiniRISC::State::Faulted: return "FAULTED";
		}
		return "UNKNOWN";
	}

	static const char* opcodeName(Opcode opcode) {
		switch (opcode) {
		case HALT: return "HALT";
		case ADDI: return "ADDI";
		case ADD: return "ADD";
		case SUB: return "SUB";
		case LDR: return "LDR";
		case STR: return "STR";
		case LB: return "LB";
		case LH: return "LH";
		case LW: return "LW";
		case LBU: return "LBU";
		case LHU: return "LHU";
		case SB: return "SB";
		case SH: return "SH";
		case SW: return "SW";
		case SLL: return "SLL";
		case SLT: return "SLT";
		case SLTU: return "SLTU";
		case XOR: return "XOR";
		case SRL: return "SRL";
		case SRA: return "SRA";
		case OR: return "OR";
		case AND: return "AND";
		case SLTI: return "SLTI";
		case SLTIU: return "SLTIU";
		case XORI: return "XORI";
		case ORI: return "ORI";
		case ANDI: return "ANDI";
		case SLLI: return "SLLI";
		case SRLI: return "SRLI";
		case SRAI: return "SRAI";
		case BEQ: return "BEQ";
		case BNE: return "BNE";
		case BLT: return "BLT";
		case BGE: return "BGE";
		case BLTU: return "BLTU";
		case BGEU: return "BGEU";
		case JAL: return "JAL";
		case JALR: return "JALR";
		case LUI: return "LUI";
		case AUIPC: return "AUIPC";
		case FENCE: return "FENCE";
		case FENCE_I: return "FENCE.I";
		case ECALL: return "ECALL";
		case EBREAK: return "EBREAK";
		case MUL: return "MUL";
		case MULH: return "MULH";
		case MULHSU: return "MULHSU";
		case MULHU: return "MULHU";
		case DIV: return "DIV";
		case DIVU: return "DIVU";
		case REM: return "REM";
		case REMU: return "REMU";
		case NOP: return "NOP";
		case LI: return "LI";
		case MV: return "MV";
		case NOT: return "NOT";
		case NEG: return "NEG";
		case J: return "J";
		case JR: return "JR";
		case RET: return "RET";
		case CALL: return "CALL";
		case BEQZ: return "BEQZ";
		case BNEZ: return "BNEZ";
		case SEQZ: return "SEQZ";
		case SNEZ: return "SNEZ";
		}
		return "UNKNOWN";
	}

	static bool isRegisterOperation(Opcode opcode) {
		switch (opcode) {
		case ADD: case SUB: case SLL: case SLT: case SLTU: case XOR:
		case SRL: case SRA: case OR: case AND: case MUL: case MULH:
		case MULHSU: case MULHU: case DIV: case DIVU: case REM: case REMU:
			return true;
		default:
			return false;
		}
	}

	static bool isImmediateOperation(Opcode opcode) {
		switch (opcode) {
		case ADDI: case SLTI: case SLTIU: case XORI: case ORI: case ANDI:
		case SLLI: case SRLI: case SRAI: case LUI: case AUIPC:
			return true;
		default:
			return false;
		}
	}

	static bool isLoadOperation(Opcode opcode) {
		switch (opcode) {
		case LDR: case LB: case LH: case LW: case LBU: case LHU:
			return true;
		default:
			return false;
		}
	}

	static bool isStoreOperation(Opcode opcode) {
		switch (opcode) {
		case STR: case SB: case SH: case SW:
			return true;
		default:
			return false;
		}
	}

	static bool isBranchOperation(Opcode opcode) {
		switch (opcode) {
		case BEQ: case BNE: case BLT: case BGE: case BLTU: case BGEU:
			return true;
		default:
			return false;
		}
	}

	static void showInstruction(const Instruction& instruction,
								std::ostream& output) {
		output << opcodeName(instruction.opcode);
		if (isRegisterOperation(instruction.opcode)) {
			output << " x" << static_cast<unsigned int>(instruction.rd)
				   << ", x" << static_cast<unsigned int>(instruction.rs1)
				   << ", x" << static_cast<unsigned int>(instruction.rs2);
		} else if (isImmediateOperation(instruction.opcode)) {
			output << " x" << static_cast<unsigned int>(instruction.rd)
				   << ", x" << static_cast<unsigned int>(instruction.rs1)
				   << ", " << instruction.immediate;
		} else if (isLoadOperation(instruction.opcode)) {
			output << " x" << static_cast<unsigned int>(instruction.rd)
				   << ", " << instruction.immediate << "(x"
				   << static_cast<unsigned int>(instruction.rs1) << ')';
		} else if (isStoreOperation(instruction.opcode)) {
			output << " x" << static_cast<unsigned int>(instruction.rs2)
				   << ", " << instruction.immediate << "(x"
				   << static_cast<unsigned int>(instruction.rs1) << ')';
		} else if (isBranchOperation(instruction.opcode)) {
			output << " x" << static_cast<unsigned int>(instruction.rs1)
				   << ", x" << static_cast<unsigned int>(instruction.rs2)
				   << ", " << instruction.immediate;
		} else if (instruction.opcode == JAL) {
			output << " x" << static_cast<unsigned int>(instruction.rd)
				   << ", " << instruction.immediate;
		} else if (instruction.opcode == JALR) {
			output << " x" << static_cast<unsigned int>(instruction.rd)
				   << ", " << instruction.immediate << "(x"
				   << static_cast<unsigned int>(instruction.rs1) << ')';
		}
	}
};