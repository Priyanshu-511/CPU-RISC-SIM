#pragma once

#include "Opcode.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct Instruction {
	Opcode opcode;
	std::uint8_t rd;
	std::uint8_t rs1;
	std::uint8_t rs2;
	std::int32_t immediate;

	Instruction(Opcode opcode = HALT, std::uint8_t rd = 0,
				std::uint8_t rs1 = 0, std::uint8_t rs2 = 0,
				std::int32_t immediate = 0)
		: opcode(opcode), rd(rd), rs1(rs1), rs2(rs2), immediate(immediate) {}
};

class MiniRISC {
public:
	enum class State {
		Ready,
		Running,
		Halted,
		Faulted
	};

	explicit MiniRISC(std::size_t memorySize = 4096)
		: memory_(memorySize, 0), pc_(0), state_(State::Ready),
		  executedCount_(0), cycleCount_(0) {
		registers_.fill(0);
		// Initialize stack pointer to top of memory
		registers_[2] = static_cast<std::uint32_t>(memorySize);
	}

	void loadProgram(const std::vector<Instruction>& program) {
		program_ = program;
		reset();
	}

	void reset() {
		registers_.fill(0);
		std::fill(memory_.begin(), memory_.end(), 0);
		// Initialize stack pointer to top of memory
		registers_[2] = static_cast<std::uint32_t>(memory_.size());
		pc_ = 0;
		state_ = State::Ready;
		error_.clear();
		executedCount_ = 0;
		cycleCount_ = 0;
	}

	// Returns true when one instruction was executed.
	bool step() {
		if (state_ == State::Halted || state_ == State::Faulted) {
			return false;
		}
		if (pc_ >= program_.size()) {
			state_ = State::Halted;
			return false;
		}

		state_ = State::Running;
		const Instruction& instruction = program_[pc_];
		if (instruction.rd >= registers_.size() ||
			instruction.rs1 >= registers_.size() ||
			instruction.rs2 >= registers_.size()) {
			return fail("register index out of range");
		}

		const std::size_t currentPc = pc_;
		std::size_t nextPc = pc_ + 1;
		const std::uint32_t first = registers_[instruction.rs1];
		const std::uint32_t second = registers_[instruction.rs2];
		const std::uint32_t immediate =
			static_cast<std::uint32_t>(instruction.immediate);

		switch (instruction.opcode) {
		case HALT:
			state_ = State::Halted;
			break;
		case ADDI:
			writeRegister(instruction.rd, first + immediate);
			break;
		case ADD:
			writeRegister(instruction.rd, first + second);
			break;
		case SUB:
			writeRegister(instruction.rd, first - second);
			break;
		case LDR:
		case LW:
			if (!loadValue(first, instruction.immediate, 4, false,
						   instruction.rd)) return false;
			break;
		case STR:
		case SW:
			if (!storeValue(first, instruction.immediate, 4, second)) return false;
			break;
		case LB:
			if (!loadValue(first, instruction.immediate, 1, true,
						   instruction.rd)) return false;
			break;
		case LH:
			if (!loadValue(first, instruction.immediate, 2, true,
						   instruction.rd)) return false;
			break;
		case LBU:
			if (!loadValue(first, instruction.immediate, 1, false,
						   instruction.rd)) return false;
			break;
		case LHU:
			if (!loadValue(first, instruction.immediate, 2, false,
						   instruction.rd)) return false;
			break;
		case SB:
			if (!storeValue(first, instruction.immediate, 1, second)) return false;
			break;
		case SH:
			if (!storeValue(first, instruction.immediate, 2, second)) return false;
			break;
		case SLL:
			writeRegister(instruction.rd, first << (second & 31));
			break;
		case SLT:
			writeRegister(instruction.rd,
						  signedValue(first) < signedValue(second));
			break;
		case SLTU:
			writeRegister(instruction.rd, first < second);
			break;
		case XOR:
			writeRegister(instruction.rd, first ^ second);
			break;
		case SRL:
			writeRegister(instruction.rd, first >> (second & 31));
			break;
		case SRA:
			writeRegister(instruction.rd, arithmeticShiftRight(first, second & 31));
			break;
		case OR:
			writeRegister(instruction.rd, first | second);
			break;
		case AND:
			writeRegister(instruction.rd, first & second);
			break;
		case SLTI:
			writeRegister(instruction.rd,
						  signedValue(first) < instruction.immediate);
			break;
		case SLTIU:
			writeRegister(instruction.rd, first < immediate);
			break;
		case XORI:
			writeRegister(instruction.rd, first ^ immediate);
			break;
		case ORI:
			writeRegister(instruction.rd, first | immediate);
			break;
		case ANDI:
			writeRegister(instruction.rd, first & immediate);
			break;
		case SLLI:
			writeRegister(instruction.rd, first << (immediate & 31));
			break;
		case SRLI:
			writeRegister(instruction.rd, first >> (immediate & 31));
			break;
		case SRAI:
			writeRegister(instruction.rd,
						  arithmeticShiftRight(first, immediate & 31));
			break;
		case BEQ:
			if (first == second && !branchTo(currentPc, instruction.immediate,
											   nextPc)) return false;
			break;
		case BNE:
			if (first != second && !branchTo(currentPc, instruction.immediate,
											   nextPc)) return false;
			break;
		case BLT:
			if (signedValue(first) < signedValue(second) &&
				!branchTo(currentPc, instruction.immediate, nextPc)) return false;
			break;
		case BGE:
			if (signedValue(first) >= signedValue(second) &&
				!branchTo(currentPc, instruction.immediate, nextPc)) return false;
			break;
		case BLTU:
			if (first < second && !branchTo(currentPc, instruction.immediate,
											  nextPc)) return false;
			break;
		case BGEU:
			if (first >= second && !branchTo(currentPc, instruction.immediate,
											  nextPc)) return false;
			break;
		case JAL: {
			writeRegister(instruction.rd,
						  static_cast<std::uint32_t>((currentPc + 1) * 4));
			if (!branchTo(currentPc, instruction.immediate, nextPc)) return false;
			break;
		}
		case JALR: {
			writeRegister(instruction.rd,
						  static_cast<std::uint32_t>((currentPc + 1) * 4));
			std::int64_t target = static_cast<std::int64_t>(first) +
								  instruction.immediate;
			target &= ~std::int64_t(1);
			if (!addressToPc(target, nextPc)) return false;
			break;
		}
		case LUI:
			writeRegister(instruction.rd, immediate << 12);
			break;
		case AUIPC:
			writeRegister(instruction.rd,
						  static_cast<std::uint32_t>(currentPc * 4) +
							  (immediate << 12));
			break;
		case FENCE:
		case FENCE_I:
			break;
		case ECALL:
			return fail("ECALL has no environment handler");
		case EBREAK:
			return fail("breakpoint instruction");
		case MUL:
			writeRegister(instruction.rd, first * second);
			break;
		case MULH:
			writeRegister(instruction.rd, static_cast<std::uint32_t>(
				(static_cast<std::int64_t>(signedValue(first)) *
				 signedValue(second)) >> 32));
			break;
		case MULHSU:
			writeRegister(instruction.rd, static_cast<std::uint32_t>(
				(static_cast<std::int64_t>(signedValue(first)) * second) >> 32));
			break;
		case MULHU:
			writeRegister(instruction.rd, static_cast<std::uint32_t>(
				(static_cast<std::uint64_t>(first) * second) >> 32));
			break;
		case DIV:
			writeRegister(instruction.rd, signedDivide(first, second));
			break;
		case DIVU:
			writeRegister(instruction.rd, second == 0 ? 0xffffffffu : first / second);
			break;
		case REM:
			writeRegister(instruction.rd, signedRemainder(first, second));
			break;
		case REMU:
			writeRegister(instruction.rd, second == 0 ? first : first % second);
			break;
		default:
			return fail("unknown opcode");
		}

		++executedCount_;
		++cycleCount_;
		registers_[0] = 0;
		pc_ = nextPc;
		return true;
	}

	std::size_t run(std::size_t maxSteps = 100000) {
		std::size_t executed = 0;
		while (executed < maxSteps && step()) {
			++executed;
		}
		return executed;
	}

	std::uint32_t getRegister(std::size_t index) const {
		return registers_.at(index);
	}

	void setRegister(std::size_t index, std::uint32_t value) {
		if (index != 0) registers_.at(index) = value;
	}

	std::uint8_t getMemoryByte(std::size_t address) const {
		return memory_.at(address);
	}

	std::size_t getMemorySize() const { return memory_.size(); }

	std::size_t getProgramSize() const { return program_.size(); }

	const Instruction* getInstructionAt(std::size_t index) const {
		return index < program_.size() ? &program_[index] : NULL;
	}

	void setMemoryByte(std::size_t address, std::uint8_t value) {
		memory_.at(address) = value;
	}

	std::size_t getProgramCounter() const { return pc_; }
	State getState() const { return state_; }
	const std::string& getError() const { return error_; }

	std::size_t getExecutedCount() const { return executedCount_; }
	std::size_t getCycleCount() const { return cycleCount_; }

	// Read a 32-bit word from memory (little-endian)
	std::uint32_t getMemoryWord(std::size_t address) const {
		if (address + 4 > memory_.size()) return 0;
		std::uint32_t value = 0;
		for (std::size_t i = 0; i < 4; ++i) {
			value |= static_cast<std::uint32_t>(memory_[address + i]) << (i * 8);
		}
		return value;
	}

	// Write a 32-bit word to memory (little-endian)
	void setMemoryWord(std::size_t address, std::uint32_t value) {
		if (address + 4 > memory_.size()) return;
		for (std::size_t i = 0; i < 4; ++i) {
			memory_[address + i] = static_cast<std::uint8_t>(value >> (i * 8));
		}
	}

private:
	std::array<std::uint32_t, 32> registers_;
	std::vector<std::uint8_t> memory_;
	std::vector<Instruction> program_;
	std::size_t pc_;
	State state_;
	std::string error_;
	std::size_t executedCount_;
	std::size_t cycleCount_;

	bool fail(const std::string& message) {
		state_ = State::Faulted;
		error_ = message;
		return false;
	}

	void writeRegister(std::size_t index, std::uint32_t value) {
		if (index != 0) registers_[index] = value;
	}

	static std::int32_t signedValue(std::uint32_t value) {
		return static_cast<std::int32_t>(value);
	}

	static std::uint32_t arithmeticShiftRight(std::uint32_t value,
											   std::uint32_t amount) {
		if (amount == 0) return value;
		std::uint32_t result = value >> amount;
		if ((value & 0x80000000u) != 0) {
			result |= 0xffffffffu << (32 - amount);
		}
		return result;
	}

	static std::uint32_t signedDivide(std::uint32_t dividend,
									 std::uint32_t divisor) {
		const std::int32_t left = signedValue(dividend);
		const std::int32_t right = signedValue(divisor);
		if (right == 0) return 0xffffffffu;
		if (left == (-2147483647 - 1) && right == -1) return dividend;
		return static_cast<std::uint32_t>(left / right);
	}

	static std::uint32_t signedRemainder(std::uint32_t dividend,
										 std::uint32_t divisor) {
		const std::int32_t left = signedValue(dividend);
		const std::int32_t right = signedValue(divisor);
		if (right == 0) return dividend;
		if (left == (-2147483647 - 1) && right == -1) return 0;
		return static_cast<std::uint32_t>(left % right);
	}

	bool memoryAddress(std::uint32_t base, std::int32_t offset,
					   std::size_t width, std::size_t& address) {
		const std::int64_t effective = static_cast<std::int64_t>(base) + offset;
		if (effective < 0 ||
			static_cast<std::uint64_t>(effective) + width > memory_.size()) {
			return fail("memory access out of range");
		}
		address = static_cast<std::size_t>(effective);
		return true;
	}

	bool loadValue(std::uint32_t base, std::int32_t offset, std::size_t width,
				   bool signExtend, std::size_t destination) {
		std::size_t address;
		if (!memoryAddress(base, offset, width, address)) return false;
		std::uint32_t value = 0;
		for (std::size_t index = 0; index < width; ++index) {
			value |= static_cast<std::uint32_t>(memory_[address + index])
					 << (index * 8);
		}
		if (signExtend && width < 4 &&
			(value & (1u << (width * 8 - 1))) != 0) {
			value |= 0xffffffffu << (width * 8);
		}
		writeRegister(destination, value);
		return true;
	}

	bool storeValue(std::uint32_t base, std::int32_t offset, std::size_t width,
					std::uint32_t value) {
		std::size_t address;
		if (!memoryAddress(base, offset, width, address)) return false;
		for (std::size_t index = 0; index < width; ++index) {
			memory_[address + index] = static_cast<std::uint8_t>(value >> (index * 8));
		}
		return true;
	}

	bool addressToPc(std::int64_t address, std::size_t& targetPc) {
		if (address < 0 || address % 4 != 0) {
			return fail("instruction address is invalid or misaligned");
		}
		targetPc = static_cast<std::size_t>(address / 4);
		return true;
	}

	bool branchTo(std::size_t currentPc, std::int32_t offset,
				  std::size_t& targetPc) {
		const std::int64_t currentAddress =
			static_cast<std::int64_t>(currentPc) * 4;
		return addressToPc(currentAddress + offset, targetPc);
	}
};