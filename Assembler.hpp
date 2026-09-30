#pragma once

#include "MiniRISC.hpp"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

class Assembler {
public:
	bool assemble(const std::string& source, std::vector<Instruction>& output) {
		error_.clear();
		std::map<std::string, std::size_t> labels;
		std::vector<ParsedInstruction> parsed;
		std::istringstream input(source);
		std::string line;
		std::size_t lineNumber = 0;
		std::size_t address = 0;

		while (std::getline(input, line)) {
			++lineNumber;
			line = stripComments(line);
			line = trim(line);
			while (!line.empty()) {
				const std::size_t colon = line.find(':');
				const std::size_t space = line.find_first_of(" \t");
				if (colon == std::string::npos ||
					(space != std::string::npos && space < colon)) {
					break;
				}

				const std::string label = trim(line.substr(0, colon));
				if (!isIdentifier(label)) {
					return failAt(lineNumber, "invalid label");
				}
				if (labels.find(label) != labels.end()) {
					return failAt(lineNumber, "duplicate label: " + label);
				}
				labels[label] = address;
				line = trim(line.substr(colon + 1));
			}

			if (line.empty()) continue;
			if (line[0] == '.') {
				std::istringstream directiveLine(line);
				std::string directive;
				directiveLine >> directive;
				if (directive == ".text" || directive == ".data" ||
					directive == ".bss" || directive == ".rodata" ||
					directive == ".globl" || directive == ".global" ||
					directive == ".type" || directive == ".size" ||
					directive == ".file" || directive == ".ident" ||
					directive == ".option" || directive == ".align" ||
					directive == ".p2align" || directive == ".balign" ||
					directive == ".zero" || directive == ".word" ||
					directive == ".half" || directive == ".byte" ||
					directive == ".string" || directive == ".ascii" ||
					directive == ".asciz" || directive == ".equ" ||
					directive == ".set" || directive == ".space") {
					continue;
				}
				if (directive == ".section") {
					std::string section;
					directiveLine >> section;
					if (section == ".text") continue;
				}
				return failAt(lineNumber, "unsupported assembler directive: " + directive);
			}
			ParsedInstruction instruction = {lineNumber, address, line};
			parsed.push_back(instruction);
			address += 4;
		}

		std::vector<Instruction> result;
		for (std::size_t index = 0; index < parsed.size(); ++index) {
			Instruction instruction;
			if (!parseInstruction(parsed[index], labels, instruction)) {
				return false;
			}
			result.push_back(instruction);
		}

		output.swap(result);
		return true;
	}

	std::vector<Instruction> assemble(const std::string& source) {
		std::vector<Instruction> result;
		assemble(source, result);
		return result;
	}

	const std::string& getError() const { return error_; }

private:
	struct ParsedInstruction {
		std::size_t lineNumber;
		std::size_t address;
		std::string text;
	};

	std::string error_;

	static std::string trim(const std::string& text) {
		const std::size_t first = text.find_first_not_of(" \t\r\n");
		if (first == std::string::npos) return std::string();
		const std::size_t last = text.find_last_not_of(" \t\r\n");
		return text.substr(first, last - first + 1);
	}

	static std::string lower(std::string text) {
		std::transform(text.begin(), text.end(), text.begin(),
					   [](unsigned char character) {
						   return static_cast<char>(std::tolower(character));
					   });
		return text;
	}

	static std::string stripComments(const std::string& line) {
		std::size_t end = line.find('#');
		const std::size_t slashComment = line.find("//");
		if (slashComment != std::string::npos &&
			(end == std::string::npos || slashComment < end)) {
			end = slashComment;
		}
		return line.substr(0, end);
	}

	static bool isIdentifier(const std::string& text) {
		if (text.empty()) return false;
		const unsigned char first = static_cast<unsigned char>(text[0]);
		if (!std::isalpha(first) && text[0] != '_' && text[0] != '.' &&
			text[0] != '$') {
			return false;
		}
		for (std::size_t index = 1; index < text.size(); ++index) {
			const unsigned char character =
				static_cast<unsigned char>(text[index]);
			if (!std::isalnum(character) && text[index] != '_' &&
				text[index] != '.' && text[index] != '$') {
				return false;
			}
		}
		return true;
	}

	static std::vector<std::string> splitOperands(const std::string& text) {
		std::vector<std::string> operands;
		if (text.find(',') == std::string::npos) {
			std::istringstream input(text);
			std::string operand;
			while (input >> operand) operands.push_back(operand);
			return operands;
		}

		std::size_t start = 0;
		while (start <= text.size()) {
			const std::size_t comma = text.find(',', start);
			const std::size_t end = comma == std::string::npos ? text.size() : comma;
			operands.push_back(trim(text.substr(start, end - start)));
			if (comma == std::string::npos) break;
			start = comma + 1;
		}
		return operands;
	}

	static bool parseRegister(std::string name, std::uint8_t& reg) {
		name = lower(trim(name));
		if (name.size() > 1 && name[0] == 'x') {
			std::int32_t number;
			if (parseInteger(name.substr(1), number) && number >= 0 && number < 32) {
				reg = static_cast<std::uint8_t>(number);
				return true;
			}
			return false;
		}

		static const char* aliases[32] = {
			"zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
			"s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
			"a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
			"s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
		};
		for (std::uint8_t index = 0; index < 32; ++index) {
			if (name == aliases[index] || (index == 8 && name == "fp")) {
				reg = index;
				return true;
			}
		}
		return false;
	}

	static bool parseInteger(const std::string& text, std::int32_t& value) {
		if (text.empty()) return false;
		errno = 0;
		char* end = NULL;
		const long long parsed = std::strtoll(text.c_str(), &end, 0);
		if (errno == ERANGE || end == text.c_str() || *end != '\0' ||
			parsed < -2147483647LL - 1 || parsed > 2147483647LL) {
			return false;
		}
		value = static_cast<std::int32_t>(parsed);
		return true;
	}

	static bool parseMemoryOperand(const std::string& text, std::uint8_t& base,
								   std::int32_t& offset) {
		const std::size_t open = text.find('(');
		const std::size_t close = text.find(')');
		if (open == std::string::npos || close != text.size() - 1 || close < open) {
			return false;
		}
		const std::string offsetText = trim(text.substr(0, open));
		const std::string baseText = text.substr(open + 1, close - open - 1);
		if (!parseRegister(baseText, base)) return false;
		if (offsetText.empty()) {
			offset = 0;
			return true;
		}
		return parseInteger(offsetText, offset);
	}

	static bool oneOf(Opcode opcode, std::initializer_list<Opcode> choices) {
		return std::find(choices.begin(), choices.end(), opcode) != choices.end();
	}

	static const std::map<std::string, Opcode>& opcodes() {
		static const std::map<std::string, Opcode> values = {
			{"halt", HALT}, {"addi", ADDI}, {"add", ADD}, {"sub", SUB},
			{"ldr", LDR}, {"str", STR}, {"lb", LB}, {"lh", LH},
			{"lw", LW}, {"lbu", LBU}, {"lhu", LHU}, {"sb", SB},
			{"sh", SH}, {"sw", SW}, {"sll", SLL}, {"slt", SLT},
			{"sltu", SLTU}, {"xor", XOR}, {"srl", SRL}, {"sra", SRA},
			{"or", OR}, {"and", AND}, {"slti", SLTI}, {"sltiu", SLTIU},
			{"xori", XORI}, {"ori", ORI}, {"andi", ANDI}, {"slli", SLLI},
			{"srli", SRLI}, {"srai", SRAI}, {"beq", BEQ}, {"bne", BNE},
			{"blt", BLT}, {"bge", BGE}, {"bltu", BLTU}, {"bgeu", BGEU},
			{"jal", JAL}, {"jalr", JALR}, {"lui", LUI}, {"auipc", AUIPC},
			{"fence", FENCE}, {"fence_i", FENCE_I}, {"fence.i", FENCE_I},
			{"ecall", ECALL}, {"ebreak", EBREAK}, {"mul", MUL},
			{"mulh", MULH}, {"mulhsu", MULHSU}, {"mulhu", MULHU},
			{"div", DIV}, {"divu", DIVU}, {"rem", REM}, {"remu", REMU},
			// Pseudo-instructions
			{"nop", NOP}, {"li", LI}, {"mv", MV}, {"not", NOT},
			{"neg", NEG}, {"j", J}, {"jr", JR}, {"ret", RET},
			{"call", CALL}, {"beqz", BEQZ}, {"bnez", BNEZ},
			{"seqz", SEQZ}, {"snez", SNEZ}
		};
		return values;
	}

	bool parseInstruction(const ParsedInstruction& parsed,
						  const std::map<std::string, std::size_t>& labels,
						  Instruction& instruction) {
		const std::size_t separator = parsed.text.find_first_of(" \t");
		std::string mnemonic = lower(parsed.text.substr(0, separator));
		const std::string operandText = separator == std::string::npos
											? std::string()
											: trim(parsed.text.substr(separator + 1));
		const std::vector<std::string> operands = splitOperands(operandText);
		const std::map<std::string, Opcode>::const_iterator found =
			opcodes().find(mnemonic);
		if (found == opcodes().end()) {
			return failAt(parsed.lineNumber, "unknown instruction: " + mnemonic);
		}

		instruction = Instruction(found->second);
		const Opcode opcode = found->second;
		std::int32_t immediate = 0;

		if (oneOf(opcode, {HALT, FENCE, FENCE_I, ECALL, EBREAK})) {
			if (opcode == NOP || opcode == RET) {
				// These have no operands but are pseudo-instructions
			} else {
				if (!operands.empty()) return operandError(parsed, "instruction takes no operands");
			}
			return true;
		}

		// Handle pseudo-instructions by converting to base instructions
		if (opcode == NOP) {
			instruction = Instruction(ADDI, 0, 0, 0, 0);
			return true;
		}
		if (opcode == RET) {
			instruction = Instruction(JALR, 0, 1, 0, 0); // jalr x0, ra, 0
			return true;
		}
		if (opcode == MV) {
			if (operands.size() != 2 || !parseRegister(operands[0], instruction.rd) ||
				!parseRegister(operands[1], instruction.rs1)) {
				return operandError(parsed, "expected rd, rs");
			}
			instruction.opcode = ADDI;
			instruction.immediate = 0;
			return true;
		}
		if (opcode == NOT) {
			if (operands.size() != 2 || !parseRegister(operands[0], instruction.rd) ||
				!parseRegister(operands[1], instruction.rs1)) {
				return operandError(parsed, "expected rd, rs");
			}
			instruction.opcode = XORI;
			instruction.immediate = -1;
			return true;
		}
		if (opcode == NEG) {
			if (operands.size() != 2 || !parseRegister(operands[0], instruction.rd) ||
				!parseRegister(operands[1], instruction.rs2)) {
				return operandError(parsed, "expected rd, rs");
			}
			instruction.opcode = SUB;
			instruction.rs1 = 0; // x0
			return true;
		}
		if (opcode == LI) {
			if (operands.size() != 2 || !parseRegister(operands[0], instruction.rd) ||
				!parseInteger(operands[1], immediate)) {
				return operandError(parsed, "expected rd, immediate");
			}
			instruction.opcode = ADDI;
			instruction.rs1 = 0; // x0
			instruction.immediate = immediate;
			return true;
		}
		if (opcode == J) {
			if (operands.size() != 1 ||
				!parseTarget(operands[0], parsed.address, labels, immediate)) {
				return operandError(parsed, "expected label or byte offset");
			}
			instruction.opcode = JAL;
			instruction.rd = 0; // x0 (discard return address)
			instruction.immediate = immediate;
			return true;
		}
		if (opcode == JR) {
			if (operands.size() != 1 || !parseRegister(operands[0], instruction.rs1)) {
				return operandError(parsed, "expected rs");
			}
			instruction.opcode = JALR;
			instruction.rd = 0; // x0
			instruction.immediate = 0;
			return true;
		}
		if (opcode == CALL) {
			if (operands.size() != 1 ||
				!parseTarget(operands[0], parsed.address, labels, immediate)) {
				return operandError(parsed, "expected label or byte offset");
			}
			instruction.opcode = JAL;
			instruction.rd = 1; // ra
			instruction.immediate = immediate;
			return true;
		}
		if (opcode == BEQZ) {
			if (operands.size() != 2 || !parseRegister(operands[0], instruction.rs1) ||
				!parseTarget(operands[1], parsed.address, labels, immediate)) {
				return operandError(parsed, "expected rs, label or byte offset");
			}
			instruction.opcode = BEQ;
			instruction.rs2 = 0; // x0
			instruction.immediate = immediate;
			return true;
		}
		if (opcode == BNEZ) {
			if (operands.size() != 2 || !parseRegister(operands[0], instruction.rs1) ||
				!parseTarget(operands[1], parsed.address, labels, immediate)) {
				return operandError(parsed, "expected rs, label or byte offset");
			}
			instruction.opcode = BNE;
			instruction.rs2 = 0; // x0
			instruction.immediate = immediate;
			return true;
		}
		if (opcode == SEQZ) {
			if (operands.size() != 2 || !parseRegister(operands[0], instruction.rd) ||
				!parseRegister(operands[1], instruction.rs1)) {
				return operandError(parsed, "expected rd, rs");
			}
			instruction.opcode = SLTIU;
			instruction.immediate = 1;
			return true;
		}
		if (opcode == SNEZ) {
			if (operands.size() != 2 || !parseRegister(operands[0], instruction.rd) ||
				!parseRegister(operands[1], instruction.rs2)) {
				return operandError(parsed, "expected rd, rs");
			}
			instruction.opcode = SLTU;
			instruction.rs1 = 0; // x0
			return true;
		}

		if (oneOf(opcode, {ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND,
						   MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU})) {
			if (operands.size() != 3 || !parseRegister(operands[0], instruction.rd) ||
				!parseRegister(operands[1], instruction.rs1) ||
				!parseRegister(operands[2], instruction.rs2)) {
				return operandError(parsed, "expected rd, rs1, rs2");
			}
			return true;
		}

		if (oneOf(opcode, {ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI,
						   SRAI})) {
			if (operands.size() != 3 || !parseRegister(operands[0], instruction.rd) ||
				!parseRegister(operands[1], instruction.rs1) ||
				!parseInteger(operands[2], immediate)) {
				return operandError(parsed, "expected rd, rs1, immediate");
			}
			instruction.immediate = immediate;
			return true;
		}

		if (oneOf(opcode, {LB, LH, LW, LBU, LHU, LDR})) {
			if (operands.size() != 2 || !parseRegister(operands[0], instruction.rd) ||
				!parseMemoryOperand(operands[1], instruction.rs1, immediate)) {
				return operandError(parsed, "expected rd, offset(rs1)");
			}
			instruction.immediate = immediate;
			return true;
		}

		if (oneOf(opcode, {SB, SH, SW, STR})) {
			if (operands.size() != 2 || !parseRegister(operands[0], instruction.rs2) ||
				!parseMemoryOperand(operands[1], instruction.rs1, immediate)) {
				return operandError(parsed, "expected rs2, offset(rs1)");
			}
			instruction.immediate = immediate;
			return true;
		}

		if (oneOf(opcode, {BEQ, BNE, BLT, BGE, BLTU, BGEU})) {
			if (operands.size() != 3 || !parseRegister(operands[0], instruction.rs1) ||
				!parseRegister(operands[1], instruction.rs2) ||
				!parseTarget(operands[2], parsed.address, labels, immediate)) {
				return operandError(parsed, "expected rs1, rs2, label or byte offset");
			}
			instruction.immediate = immediate;
			return true;
		}

		if (opcode == JAL) {
			if (operands.size() == 1) {
				instruction.rd = 1;
				if (!parseTarget(operands[0], parsed.address, labels, immediate)) {
					return operandError(parsed, "expected label or byte offset");
				}
			} else if (operands.size() == 2 &&
					   parseRegister(operands[0], instruction.rd) &&
					   parseTarget(operands[1], parsed.address, labels, immediate)) {
			} else {
				return operandError(parsed, "expected rd, label or byte offset");
			}
			instruction.immediate = immediate;
			return true;
		}

		if (opcode == JALR) {
			if (operands.size() == 2 &&
				parseRegister(operands[0], instruction.rd) &&
				parseMemoryOperand(operands[1], instruction.rs1, immediate)) {
				instruction.immediate = immediate;
				return true;
			}
			if (operands.size() == 3 &&
				parseRegister(operands[0], instruction.rd) &&
				parseRegister(operands[1], instruction.rs1) &&
				parseInteger(operands[2], immediate)) {
				instruction.immediate = immediate;
				return true;
			}
			return operandError(parsed, "expected rd, offset(rs1) or rd, rs1, offset");
		}

		if (oneOf(opcode, {LUI, AUIPC})) {
			if (operands.size() != 2 || !parseRegister(operands[0], instruction.rd) ||
				!parseInteger(operands[1], immediate)) {
				return operandError(parsed, "expected rd, immediate");
			}
			instruction.immediate = immediate;
			return true;
		}

		return failAt(parsed.lineNumber, "unsupported instruction form");
	}

	static bool parseTarget(const std::string& token, std::size_t currentAddress,
							const std::map<std::string, std::size_t>& labels,
							std::int32_t& offset) {
		std::map<std::string, std::size_t>::const_iterator label = labels.find(token);
		if (label != labels.end()) {
			const std::int64_t difference =
				static_cast<std::int64_t>(label->second) - currentAddress;
			if (difference < -2147483647LL - 1 || difference > 2147483647LL) {
				return false;
			}
			offset = static_cast<std::int32_t>(difference);
			return true;
		}
		return parseInteger(token, offset);
	}

	bool operandError(const ParsedInstruction& parsed, const std::string& message) {
		return failAt(parsed.lineNumber, message);
	}

	bool failAt(std::size_t lineNumber, const std::string& message) {
		std::ostringstream output;
		output << "line " << lineNumber << ": " << message;
		error_ = output.str();
		return false;
	}
};