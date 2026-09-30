# MiniRISC-V SPU Simulator

A complete **RISC-V RV32IM** Signal Processing Unit (SPU) simulator written in C++11.  
Assemble, execute, trace, step-debug, decompile, and inspect memory — all from one tool.

---

## Features

| Feature | Description |
|---------|-------------|
| **Full RV32I + M extension** | All base integer instructions + multiply/divide |
| **Pseudo-instructions** | `nop`, `li`, `mv`, `not`, `neg`, `j`, `jr`, `ret`, `call`, `beqz`, `bnez`, `seqz`, `snez` |
| **Assembler** | Two-pass assembler with labels, comments (`#`, `//`), and directives |
| **Step debugger** | Interactive single-step execution with full state display |
| **Instruction trace** | Automatic execution with per-instruction state dumps |
| **Decompiler (ASM → C++)** | Translates RISC-V assembly back into C++ pseudo-code |
| **C++ to ASM Translator** | Built-in translator: converts simple C++ directly to RISC-V assembly |
| **Memory hex dump** | Full hex dump with ASCII representation (like `xxd`) |
| **Register display** | All 32 registers shown with both `x0-x31` and ABI names (`zero`, `ra`, `sp`, `a0`...) |
| **Execution statistics** | Instruction count and cycle tracking |

---

## Quick Start

### Build

```bash
make
```

### How to Run Code using `make`

**1. Running C++ Files (`.cpp`)**
```bash
# Convert C++ to RISC-V assembly and run it immediately
make cpp2asm-run SRC=test.cpp

# Only convert C++ to assembly (saves to test.asm), without running
make cpp2asm SRC=test.cpp
```

**2. Running Assembly Files (`.asm`)**
```bash
# Run the assembly file and show the final state
make run ASM=test.asm

# Run it in interactive step-by-step debugger mode
make step ASM=test.asm

# Run it and show the memory hex dump at the end
make memory ASM=test.asm

# Show the assembly side-by-side with C++ pseudo-code (no execution)
make dump ASM=test.asm
```

**3. Direct Binary Usage**
If you prefer not to use `make` targets, you can use the compiled binary directly:
```bash
./build/mini-risc example.asm --trace       # Trace every instruction
./build/mini-risc example.asm --decompile   # Decompile back to full C++
./build/mini-risc mycode.cpp --cpp2asm-run  # Run C++ code
echo "addi a0, zero, 42\nhalt" | ./build/mini-risc -  # Read from stdin
```

**Tip:** Run `make help` to see all available commands and variables right in your terminal!

---

## Architecture

### CPU Model

- **32 registers** (`x0`-`x31`), where `x0` is hardwired to zero
- **4 KB memory** (configurable), byte-addressable, little-endian
- **Stack pointer** (`x2`/`sp`) initialized to top of memory
- **Program counter** tracks instruction index (addresses are `PC × 4`)

### Register ABI Names

| Register | ABI Name | Usage |
|----------|----------|-------|
| `x0` | `zero` | Hardwired zero |
| `x1` | `ra` | Return address |
| `x2` | `sp` | Stack pointer |
| `x3` | `gp` | Global pointer |
| `x4` | `tp` | Thread pointer |
| `x5-x7` | `t0-t2` | Temporaries |
| `x8` | `s0/fp` | Saved register / Frame pointer |
| `x9` | `s1` | Saved register |
| `x10-x17` | `a0-a7` | Function arguments / return values |
| `x18-x27` | `s2-s11` | Saved registers |
| `x28-x31` | `t3-t6` | Temporaries |

---

## Supported Instructions

### RV32I Base Integer Instructions

#### Arithmetic
| Instruction | Syntax | Operation |
|-------------|--------|-----------|
| `ADD` | `add rd, rs1, rs2` | `rd = rs1 + rs2` |
| `SUB` | `sub rd, rs1, rs2` | `rd = rs1 - rs2` |
| `ADDI` | `addi rd, rs1, imm` | `rd = rs1 + imm` |

#### Logical
| Instruction | Syntax | Operation |
|-------------|--------|-----------|
| `AND` | `and rd, rs1, rs2` | `rd = rs1 & rs2` |
| `OR` | `or rd, rs1, rs2` | `rd = rs1 \| rs2` |
| `XOR` | `xor rd, rs1, rs2` | `rd = rs1 ^ rs2` |
| `ANDI` | `andi rd, rs1, imm` | `rd = rs1 & imm` |
| `ORI` | `ori rd, rs1, imm` | `rd = rs1 \| imm` |
| `XORI` | `xori rd, rs1, imm` | `rd = rs1 ^ imm` |

#### Shift
| Instruction | Syntax | Operation |
|-------------|--------|-----------|
| `SLL` | `sll rd, rs1, rs2` | `rd = rs1 << rs2` |
| `SRL` | `srl rd, rs1, rs2` | `rd = rs1 >> rs2` (logical) |
| `SRA` | `sra rd, rs1, rs2` | `rd = rs1 >> rs2` (arithmetic) |
| `SLLI` | `slli rd, rs1, imm` | `rd = rs1 << imm` |
| `SRLI` | `srli rd, rs1, imm` | `rd = rs1 >> imm` (logical) |
| `SRAI` | `srai rd, rs1, imm` | `rd = rs1 >> imm` (arithmetic) |

#### Compare
| Instruction | Syntax | Operation |
|-------------|--------|-----------|
| `SLT` | `slt rd, rs1, rs2` | `rd = (rs1 < rs2) ? 1 : 0` (signed) |
| `SLTU` | `sltu rd, rs1, rs2` | `rd = (rs1 < rs2) ? 1 : 0` (unsigned) |
| `SLTI` | `slti rd, rs1, imm` | `rd = (rs1 < imm) ? 1 : 0` (signed) |
| `SLTIU` | `sltiu rd, rs1, imm` | `rd = (rs1 < imm) ? 1 : 0` (unsigned) |

#### Load / Store
| Instruction | Syntax | Operation |
|-------------|--------|-----------|
| `LW` | `lw rd, offset(rs1)` | Load 32-bit word |
| `LH` | `lh rd, offset(rs1)` | Load 16-bit half (sign-extended) |
| `LB` | `lb rd, offset(rs1)` | Load 8-bit byte (sign-extended) |
| `LHU` | `lhu rd, offset(rs1)` | Load 16-bit half (zero-extended) |
| `LBU` | `lbu rd, offset(rs1)` | Load 8-bit byte (zero-extended) |
| `SW` | `sw rs2, offset(rs1)` | Store 32-bit word |
| `SH` | `sh rs2, offset(rs1)` | Store 16-bit half |
| `SB` | `sb rs2, offset(rs1)` | Store 8-bit byte |

#### Branch
| Instruction | Syntax | Operation |
|-------------|--------|-----------|
| `BEQ` | `beq rs1, rs2, label` | Branch if `rs1 == rs2` |
| `BNE` | `bne rs1, rs2, label` | Branch if `rs1 != rs2` |
| `BLT` | `blt rs1, rs2, label` | Branch if `rs1 < rs2` (signed) |
| `BGE` | `bge rs1, rs2, label` | Branch if `rs1 >= rs2` (signed) |
| `BLTU` | `bltu rs1, rs2, label` | Branch if `rs1 < rs2` (unsigned) |
| `BGEU` | `bgeu rs1, rs2, label` | Branch if `rs1 >= rs2` (unsigned) |

#### Jump
| Instruction | Syntax | Operation |
|-------------|--------|-----------|
| `JAL` | `jal rd, label` | Jump and link (rd = PC+4, jump to label) |
| `JALR` | `jalr rd, offset(rs1)` | Jump and link register |

#### Upper Immediate
| Instruction | Syntax | Operation |
|-------------|--------|-----------|
| `LUI` | `lui rd, imm` | `rd = imm << 12` |
| `AUIPC` | `auipc rd, imm` | `rd = PC + (imm << 12)` |

#### System
| Instruction | Syntax | Operation |
|-------------|--------|-----------|
| `HALT` | `halt` | Stop execution |
| `ECALL` | `ecall` | Environment call |
| `EBREAK` | `ebreak` | Breakpoint |
| `FENCE` | `fence` | Memory fence (no-op in simulator) |

### RV32M Multiply/Divide Extension

| Instruction | Syntax | Operation |
|-------------|--------|-----------|
| `MUL` | `mul rd, rs1, rs2` | `rd = (rs1 × rs2)[31:0]` |
| `MULH` | `mulh rd, rs1, rs2` | `rd = (rs1 × rs2)[63:32]` (signed×signed) |
| `MULHSU` | `mulhsu rd, rs1, rs2` | `rd = (rs1 × rs2)[63:32]` (signed×unsigned) |
| `MULHU` | `mulhu rd, rs1, rs2` | `rd = (rs1 × rs2)[63:32]` (unsigned×unsigned) |
| `DIV` | `div rd, rs1, rs2` | `rd = rs1 ÷ rs2` (signed) |
| `DIVU` | `divu rd, rs1, rs2` | `rd = rs1 ÷ rs2` (unsigned) |
| `REM` | `rem rd, rs1, rs2` | `rd = rs1 % rs2` (signed) |
| `REMU` | `remu rd, rs1, rs2` | `rd = rs1 % rs2` (unsigned) |

### Pseudo-instructions

| Pseudo | Expansion | Description |
|--------|-----------|-------------|
| `nop` | `addi x0, x0, 0` | No operation |
| `li rd, imm` | `addi rd, x0, imm` | Load immediate |
| `mv rd, rs` | `addi rd, rs, 0` | Register move |
| `not rd, rs` | `xori rd, rs, -1` | Bitwise NOT |
| `neg rd, rs` | `sub rd, x0, rs` | Negate |
| `j label` | `jal x0, label` | Unconditional jump |
| `jr rs` | `jalr x0, rs, 0` | Jump register |
| `ret` | `jalr x0, ra, 0` | Return from function |
| `call label` | `jal ra, label` | Call function |
| `beqz rs, label` | `beq rs, x0, label` | Branch if zero |
| `bnez rs, label` | `bne rs, x0, label` | Branch if not zero |
| `seqz rd, rs` | `sltiu rd, rs, 1` | Set if equal zero |
| `snez rd, rs` | `sltu rd, x0, rs` | Set if not equal zero |

---

## Example Programs

### Hello World (add two numbers)

```asm
.text
.globl main
main:
    addi t0, zero, 5    # t0 = 5
    addi t1, zero, 7    # t1 = 7
    add  a0, t0, t1     # a0 = t0 + t1 = 12
    sw   a0, 0(zero)    # store result to memory[0]
    lw   a1, 0(zero)    # load it back to verify
    halt
```

### Factorial (5! = 120)

```asm
.text
.globl main
main:
    addi a0, zero, 5    # n = 5
    addi a1, zero, 1    # result = 1
loop:
    beq  a0, zero, done # if n == 0, done
    mul  a1, a1, a0     # result *= n
    addi a0, a0, -1     # n--
    jal  zero, loop      # goto loop
done:
    add  a0, a1, zero   # move result to a0
    sw   a0, 0(zero)    # store to memory
    halt
```

### Fibonacci

```asm
.text
.globl main
main:
    addi a0, zero, 10   # compute fib(10)
    addi t0, zero, 0    # fib(0) = 0
    addi t1, zero, 1    # fib(1) = 1
    addi a0, a0, -1     # count = n - 1
fib_loop:
    beq  a0, zero, fib_done
    add  t2, t0, t1     # next = prev + curr
    add  t0, t1, zero   # prev = curr
    add  t1, t2, zero   # curr = next
    addi a0, a0, -1     # count--
    jal  zero, fib_loop
fib_done:
    add  a0, t1, zero   # result in a0
    sw   a0, 0(zero)    # store to memory
    halt
```

### Full C++ Translation and Execution Example

You can write C++ code and let the simulator translate, assemble, and run it all at once using `make cpp2asm-run`.

**1. Write your `test.cpp`:**
```cpp
#include <iostream>
using namespace std;

int main() {
    int a = 5;
    int b = 6;
    int c = a + b;
    cout << c << "\n";
}
```

**2. Run it through the simulator:**
```bash
make cpp2asm-run SRC=test.cpp
```

**3. The resulting pipeline output:**
```
┌─────────────────────────────────┬────────────────────────────────────────────┐
│ C++ Source                      │ RISC-V Assembly                            │
├─────────────────────────────────┼────────────────────────────────────────────┤
│ int main() {                    │ main:                                      │
│     int a = 5;                  │     # int a = 5                            │
│                                 │     addi t0, zero, 5                       │
│     int b = 6;                  │     # int b = 6                            │
│                                 │     addi t1, zero, 6                       │
│     int c = a + b;              │     # int c = a + b                        │
│                                 │     add t2, t0, t1                         │
│     cout << c << "\n";          │     # cout << c << "\n"                    │
│                                 │     sw t2, 100(zero)                       │
│ }                               │     halt                                   │
└─────────────────────────────────┴────────────────────────────────────────────┘

=== Generated RISC-V Assembly ===
# Auto-generated RISC-V assembly from C++ source
.text
.globl main
main:
    addi t0, zero, 5
    addi t1, zero, 6
    add t2, t0, t1
    sw t2, 100(zero)
    halt

=== Saved to: test.asm ===

=== Assembled Listing ===
┌─────────────────────────────────────────────────────────────────────────┐
│ Program Listing (Assembly)                                              │
├──────────┬──────────────────────────────┬───────────────────────────────┤
│ Address  │ RISC-V Assembly              │ C++ Pseudo-code               │
├──────────┼──────────────────────────────┼───────────────────────────────┤
│ 0x0000   │ ADDI x5, x0, 5               │ t0 = zero + 5;                │
│ 0x0004   │ ADDI x6, x0, 6               │ t1 = zero + 6;                │
│ 0x0008   │ ADD x7, x5, x6               │ t2 = t0 + t1;                 │
│ 0x000c   │ SW x7, 100(x0)               │ *(int32_t*)(memory + 100) = t2│
│ 0x0010   │ HALT                         │ exit(0);                      │
└──────────┴──────────────────────────────┴───────────────────────────────┘

=== Executing ===
║ Registers (32 × 32-bit)                                        ║
║ x7 (t2  )=0x0000000b                                           ║
║ Memory (hex dump, non-zero regions)                            ║
║ 0x0060: 00 00 00 00 0b 00 00 00  00 00 00 00 00 00 00 00       ║
```

---

## Output Examples

### `--dump` — Assembly with C++ Translation

```
┌─────────────────────────────────────────────────────────────────────────┐
│ Program Listing (Assembly)                                              │
├──────────┬──────────────────────────────┬───────────────────────────────┤
│ Address  │ RISC-V Assembly              │ C++ Pseudo-code               │
├──────────┼──────────────────────────────┼───────────────────────────────┤
│ 0x0000   │ ADDI x5, x0, 5              │ t0 = zero + 5;               │
│ 0x0004   │ ADDI x6, x0, 7              │ t1 = zero + 7;               │
│ 0x0008   │ ADD x10, x5, x6             │ a0 = t0 + t1;                │
│ 0x000c   │ SW x10, 0(x0)               │ *(int32_t*)(memory + 0) = a0;│
│ 0x0010   │ LW x11, 0(x0)              │ a1 = *(int32_t*)(memory + 0);│
│ 0x0014   │ HALT                        │ exit(0);                      │
└──────────┴──────────────────────────────┴───────────────────────────────┘
```

### `--step` — Interactive Debugger

```
── Step 1 ─────────────────────────────────────────
  PC 0x0000: ADDI x5, x0, 5
  C++: t0 = zero + 5;

╔══════════════════════════════════════════════════════════════════════╗
║                    MiniRISC-V SPU Simulator                        ║
╠══════════════════════════════════════════════════════════════════════╣
║ PC:     1 /     6 instructions                                     ║
║ State: RUNNING    │ Executed:        1 instructions                ║
╠══════════════════════════════════════════════════════════════════════╣
║ Registers (32 × 32-bit)                                            ║
║ x0 (zero)=0x00000000 x1 (ra  )=0x00000000 ...                     ║
╠══════════════════════════════════════════════════════════════════════╣
║ Memory (hex dump, non-zero regions)                                ║
║   (all memory is zero)                                             ║
╚══════════════════════════════════════════════════════════════════════╝
```

### Memory Hex Dump

```
║ 0x0000: 0c 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................ ║
```

---

## Project Structure

```
Risc_simulator/
├── main.cpp          # Entry point — CLI and mode dispatch
├── Assembler.hpp     # Two-pass assembler with pseudo-instruction expansion
├── MiniRISC.hpp      # CPU core — registers, memory, instruction execution
├── Display.hpp       # Rich output formatting — registers, memory hex dump
├── Decompiler.hpp    # ASM → C++ pseudo-code decompiler
├── CppToAsm.hpp      # Built-in C++ subset to RISC-V assembly translator
├── Opcode.hpp        # Instruction opcode enum and register ABI names
├── File.hpp          # File I/O utility
├── Makefile          # Build system
├── example.asm       # Example: add two numbers
├── test.asm          # Auto-generated from test.cpp
├── test.cpp          # Sample C++ code
└── build/
    └── mini-risc     # Compiled binary
```

---

## How It Works

### Compilation & Execution Pipeline

```
    .cpp file ───▶ CppTranslator ───▶ Generated .asm file
                        │
                  (if using cpp2asm)
                        │
                        ▼
                    ┌──────────────┐
    .asm file ────▶ │  Assembler   │ ─── 2-pass assembly ──▶ Instruction vector
                    └──────────────┘
                           │
                    ┌──────┴───────┐
                    │   MiniRISC   │ ◀── Fetch / Decode / Execute loop
                    │    (CPU)     │
                    └──────┬───────┘
                           │
                    ┌──────┴───────┐
                    │   Display    │ ─── Registers, Memory, Decompiled C++
                    └──────────────┘
```

### Decompilation Pipeline

```
   Instruction vector ──▶ Decompiler ──▶ C++ pseudo-code
        (binary)             │
                      Maps each opcode
                      to C++ equivalent
```

---

## Building & Requirements

- **Compiler**: Any C++11 compliant compiler (`g++`, `clang++`)
- **Platform**: Linux, macOS, Windows (with MinGW)
- **Dependencies**: None (standard library only)

```bash
# Build with default compiler
make

# Build with specific compiler
make CXX=g++

# Build with debug symbols
make CXXFLAGS="-std=c++11 -g -O0 -Wall"

# Clean and rebuild
make clean && make
```

---

## License

This project is open-source. Feel free to use, modify, and distribute.
