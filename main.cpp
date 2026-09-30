#include "Assembler.hpp"
#include "CppToAsm.hpp"
#include "Decompiler.hpp"
#include "Display.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

void printUsage(const char* executable) {
	std::cout << "Usage: " << executable
			  << " <input-file|-> [options]\n"
			  << "\nExecution modes (input = .asm file):\n"
			  << "  (none)        Run and show final state\n"
			  << "  --trace       Show each instruction and state without pausing\n"
			  << "  --step        Interactive step-by-step execution\n"
			  << "\nAnalysis modes (input = .asm file):\n"
			  << "  --dump        List assembled instructions with C++ translation\n"
			  << "  --decompile   Show full C++ pseudo-code decompilation\n"
			  << "  --memory      Show memory hex dump after execution\n"
			  << "  --memory-all  Show full memory dump (including zeros)\n"
			  << "\nC++ conversion (input = .cpp file):\n"
			  << "  --cpp2asm              Convert C++ to RISC-V assembly\n"
			  << "  --cpp2asm -o out.asm   Convert and save to file\n"
			  << "  --cpp2asm-run          Convert, assemble, and run\n"
			  << "  --cpp2asm-run -o f.asm Convert, save, assemble, and run\n"
			  << "\nGeneral:\n"
			  << "  -o <file>     Save generated assembly to file\n"
			  << "  -             Read source from stdin\n"
			  << "  --help, -h    Show this help\n";
}

bool readSource(const std::string& path, std::string& source) {
	std::ostringstream contents;
	if (path == "-") {
		contents << std::cin.rdbuf();
	} else {
		std::ifstream input(path.c_str());
		if (!input) {
			std::cerr << "Could not open input file: " << path << '\n';
			return false;
		}
		contents << input.rdbuf();
	}
	source = contents.str();
	return true;
}

// Derive .asm output filename from a .cpp input filename
std::string deriveAsmName(const std::string& cppPath) {
	std::string base = cppPath;
	std::size_t dot = base.rfind('.');
	if (dot != std::string::npos) {
		base = base.substr(0, dot);
	}
	return base + ".asm";
}

} // namespace

int main(int argc, char* argv[]) {
	if (argc < 2 || std::string(argv[1]) == "--help" ||
		std::string(argv[1]) == "-h") {
		printUsage(argv[0]);
		return argc < 2 ? 1 : 0;
	}

	// Parse arguments: <input> [mode] [-o output]
	const std::string inputPath = argv[1];
	std::string mode = "run";
	std::string outputPath;

	for (int i = 2; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "-o" && i + 1 < argc) {
			outputPath = argv[++i];
		} else if (arg[0] == '-') {
			mode = arg;
		} else {
			std::cerr << "Unknown argument: " << arg << '\n';
			printUsage(argv[0]);
			return 1;
		}
	}

	const bool validMode = (mode == "run" || mode == "--trace" ||
							mode == "--step" || mode == "--dump" ||
							mode == "--decompile" || mode == "--memory" ||
							mode == "--memory-all" || mode == "--cpp2asm" ||
							mode == "--cpp2asm-run");
	if (!validMode) {
		std::cerr << "Unknown option: " << mode << '\n';
		printUsage(argv[0]);
		return 1;
	}

	std::string source;
	if (!readSource(inputPath, source)) return 1;

	// C++ to RISC-V translation modes
	if (mode == "--cpp2asm" || mode == "--cpp2asm-run") {
		CppTranslator translator;
		std::string asmOutput;
		if (!translator.translate(source, asmOutput)) {
			std::cerr << translator.getError() << '\n';
			return 1;
		}

		// Show side-by-side C++ and generated assembly
		CppTranslator::showSideBySide(source, asmOutput);

		// Print the raw assembly
		std::cout << "\n=== Generated RISC-V Assembly ===\n" << asmOutput << "\n";

		// Save to file if -o specified, or auto-derive name
		std::string savePath = outputPath;
		if (savePath.empty()) {
			savePath = deriveAsmName(inputPath);
		}
		{
			std::ofstream outFile(savePath.c_str());
			if (!outFile) {
				std::cerr << "Could not write output file: " << savePath << '\n';
				return 1;
			}
			outFile << asmOutput;
			std::cout << "=== Saved to: " << savePath << " ===\n";
		}

		if (mode == "--cpp2asm-run") {
			// Assemble and run the generated assembly
			Assembler assembler;
			std::vector<Instruction> program;
			if (!assembler.assemble(asmOutput, program)) {
				std::cerr << "Assembly error in generated code: "
						  << assembler.getError() << '\n';
				return 1;
			}
			MiniRISC cpu;
			cpu.loadProgram(program);
			Display display;

			std::cout << "\n=== Assembled Listing ===\n";
			display.showProgram(cpu);

			std::cout << "\n=== Executing ===\n";
			display.run(cpu);
			return cpu.getState() == MiniRISC::State::Faulted ? 1 : 0;
		}
		return 0;
	}

	Assembler assembler;
	std::vector<Instruction> program;
	if (!assembler.assemble(source, program)) {
		std::cerr << "Assembly error: " << assembler.getError() << '\n';
		return 1;
	}

	MiniRISC cpu;
	cpu.loadProgram(program);
	Display display;

	if (mode == "--dump") {
		display.showProgram(cpu);
		return 0;
	}

	if (mode == "--decompile") {
		display.showDecompiled(cpu);
		return 0;
	}

	if (mode == "--trace") {
		display.run(cpu, Display::Mode::EachOperation);
		return cpu.getState() == MiniRISC::State::Faulted ? 1 : 0;
	}

	if (mode == "--step") {
		std::size_t executed = 0;
		while (executed < 100000 &&
			   cpu.getProgramCounter() < cpu.getProgramSize() &&
			   cpu.getState() != MiniRISC::State::Halted &&
			   cpu.getState() != MiniRISC::State::Faulted) {
			std::cout << "\nPress Enter to execute next instruction (q to quit): ";
			std::string response;
			if (!std::getline(std::cin, response) ||
				response == "q" || response == "Q") {
				break;
			}
			if (display.step(cpu)) ++executed;
		}
		if (cpu.getProgramCounter() >= cpu.getProgramSize() &&
			cpu.getState() == MiniRISC::State::Running) {
			cpu.step();
			display.showAll(cpu);
		}
		std::cout << "\nExecution complete. " << executed << " instructions executed.\n";
		return cpu.getState() == MiniRISC::State::Faulted ? 1 : 0;
	}

	if (mode == "--memory" || mode == "--memory-all") {
		display.run(cpu);
		std::cout << "\nDetailed memory view:\n";
		display.showMemory(cpu, 0, 0, std::cout, mode == "--memory-all");
		return cpu.getState() == MiniRISC::State::Faulted ? 1 : 0;
	}

	// Default: run and show final state
	display.run(cpu);
	return cpu.getState() == MiniRISC::State::Faulted ? 1 : 0;
}
