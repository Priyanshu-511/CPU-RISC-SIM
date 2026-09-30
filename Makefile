CXX ?= c++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -Wpedantic
BUILD_DIR ?= build
TARGET := $(BUILD_DIR)/mini-risc
ASM ?= example.asm
SRC ?= test.cpp
OUT ?=

HEADERS := Assembler.hpp CppToAsm.hpp Decompiler.hpp Display.hpp MiniRISC.hpp Opcode.hpp

.PHONY: all run trace step dump decompile memory cpp2asm cpp2asm-run clean help

all: $(TARGET)

$(TARGET): main.cpp $(HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) main.cpp -o $@ $(LDFLAGS) $(LDLIBS)

run: $(TARGET)
	./$(TARGET) "$(ASM)"

trace: $(TARGET)
	./$(TARGET) "$(ASM)" --trace

step: $(TARGET)
	./$(TARGET) "$(ASM)" --step

dump: $(TARGET)
	./$(TARGET) "$(ASM)" --dump

decompile: $(TARGET)
	./$(TARGET) "$(ASM)" --decompile

memory: $(TARGET)
	./$(TARGET) "$(ASM)" --memory

# Convert C++ source to RISC-V assembly (built-in, no cross-compiler needed)
# Auto-saves to <name>.asm, or specify OUT=output.asm
# Usage: make cpp2asm SRC=test.cpp
#        make cpp2asm SRC=test.cpp OUT=output.asm
cpp2asm: $(TARGET)
ifdef OUT
	./$(TARGET) "$(SRC)" --cpp2asm -o "$(OUT)"
else
	./$(TARGET) "$(SRC)" --cpp2asm
endif

# Convert C++ to RISC-V assembly, save, AND run in the simulator
# Usage: make cpp2asm-run SRC=test.cpp
#        make cpp2asm-run SRC=test.cpp OUT=output.asm
cpp2asm-run: $(TARGET)
ifdef OUT
	./$(TARGET) "$(SRC)" --cpp2asm-run -o "$(OUT)"
else
	./$(TARGET) "$(SRC)" --cpp2asm-run
endif

clean:
	rm -f $(TARGET)

help:
	@echo "MiniRISC-V SPU Simulator"
	@echo "========================"
	@echo ""
	@echo "Assembly targets:"
	@echo "  all          Build the simulator"
	@echo "  run          Run assembly file (ASM=file.asm)"
	@echo "  trace        Run with instruction tracing"
	@echo "  step         Interactive step-by-step execution"
	@echo "  dump         Show assembly listing with C++ translation"
	@echo "  decompile    Generate C++ pseudo-code from assembly"
	@echo "  memory       Show memory hex dump after execution"
	@echo ""
	@echo "C++ conversion targets:"
	@echo "  cpp2asm      Convert C++ to RISC-V assembly and save"
	@echo "  cpp2asm-run  Convert, save, assemble, and run"
	@echo ""
	@echo "Utility:"
	@echo "  clean        Remove build artifacts"
	@echo "  help         Show this help"
	@echo ""
	@echo "Variables:"
	@echo "  ASM=<file>   Assembly input file      (default: example.asm)"
	@echo "  SRC=<file>   C++ source to convert    (default: test.cpp)"
	@echo "  OUT=<file>   Output .asm file          (default: auto from SRC)"
	@echo "  CXX=<cmd>    C++ compiler             (default: c++)"
	@echo ""
	@echo "Examples:"
	@echo "  make cpp2asm                       # test.cpp -> test.asm"
	@echo "  make cpp2asm SRC=mycode.cpp        # mycode.cpp -> mycode.asm"
	@echo "  make cpp2asm SRC=a.cpp OUT=b.asm   # a.cpp -> b.asm"
	@echo "  make cpp2asm-run SRC=test.cpp      # convert + run"
	@echo "  make run ASM=test.asm              # run saved assembly"
