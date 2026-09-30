#pragma once

#include <string>

// Simulator instruction IDs. These are not the encoded RISC-V opcode fields.
enum Opcode {
    HALT = 0x00,
    ADDI = 0x01, // ADDI Rd, Rs1, Imm (Rd = Rs1 + Imm)
    ADD  = 0x02, // ADD  Rd, Rs1, Rs2 (Rd = Rs1 + Rs2)
    SUB  = 0x03, // SUB  Rd, Rs1, Rs2 (Rd = Rs1 - Rs2)
    LDR  = 0x04,
    STR  = 0x05,

    // RV32I loads and stores
    LB,
    LH,
    LW,
    LBU,
    LHU,
    SB,
    SH,
    SW,

    // RV32I integer register-register operations
    SLL,
    SLT,
    SLTU,
    XOR,
    SRL,
    SRA,
    OR,
    AND,

    // RV32I integer immediate operations
    SLTI,
    SLTIU,
    XORI,
    ORI,
    ANDI,
    SLLI,
    SRLI,
    SRAI,

    // RV32I control flow
    BEQ,
    BNE,
    BLT,
    BGE,
    BLTU,
    BGEU,
    JAL,
    JALR,

    // RV32I upper-immediate and memory-ordering instructions
    LUI,
    AUIPC,
    FENCE,
    FENCE_I,

    // RV32I system instructions
    ECALL,
    EBREAK,

    // RV32M multiply/divide extension
    MUL,
    MULH,
    MULHSU,
    MULHU,
    DIV,
    DIVU,
    REM,
    REMU,

    // Pseudo-instructions (handled internally by assembler)
    NOP,   // -> ADDI x0, x0, 0
    LI,    // -> LUI + ADDI or just ADDI
    MV,    // -> ADDI rd, rs, 0
    NOT,   // -> XORI rd, rs, -1
    NEG,   // -> SUB rd, x0, rs
    J,     // -> JAL x0, offset
    JR,    // -> JALR x0, rs, 0
    RET,   // -> JALR x0, ra, 0
    CALL,  // -> JAL ra, offset
    BEQZ,  // -> BEQ rs, x0, offset
    BNEZ,  // -> BNE rs, x0, offset
    SEQZ,  // -> SLTIU rd, rs, 1
    SNEZ,  // -> SLTU rd, x0, rs
};

// Register ABI names for display
inline const char* registerABIName(unsigned int reg) {
    static const char* names[32] = {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
        "s0/fp", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
        "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
        "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
    };
    if (reg < 32) return names[reg];
    return "???";
}