const std = @import("std");
const Register = @import("register.zig").Register;
const Memory = @import("memory.zig").Memory;
const flagsHelper = @import("flags_helper.zig");
const opcodeHelper = @import("opcode_helper.zig");
const ALU = @import("alu.zig").ALU;

const Opcode = *const fn (register: *Register, memory: *Memory) void;

fn fetchCycle(register: *Register, memory: *Memory) u8 {
    const opcode = memory.readU8(register.r16.PC);
    register.r16.PC += 1;
    return opcode;
}

fn getR16Mem(register: *Register, r16Mem: u2) u16 {
    return switch (r16Mem) {
        0b0 => register.r16.BC,
        0b1 => register.r16.DE,
        0b10 => HLP: {
            const tmp = register.r16.HL;
            register.r16.HL += 1;
            break :HLP tmp;
        },
        0b11 => HLS: {
            const tmp = register.r16.HL;
            register.r16.HL -= 1;
            break :HLS tmp;
        },
    };
}

fn readU16(memory: *Memory, register: *Register) u16 {
    const lo: u16 = @intCast(memory.readU8(register.r16.PC));
    register.r16.PC += 1;
    const hi: u16 = @intCast(memory.readU8(register.r16.PC));
    register.r16.PC += 1;
    return (hi << 8) | lo;
}

fn getR16(register: *Register, comptime r16Code: u2) *u16 {
    return switch (r16Code) {
        0b0 => &register.r16.BC,
        0b1 => &register.r16.DE,
        0b10 => &register.r16.HL,
        0b11 => &register.r16.SP,
    };
}

fn getR8(register: *Register, comptime r8Code: u3) ?*u8 {
    return switch (r8Code) {
        0b0 => return &register.r8.B,
        0b1 => return &register.r8.C,
        0b10 => return &register.r8.D,
        0b11 => return &register.r8.E,
        0b100 => return &register.r8.H,
        0b101 => return &register.r8.L,
        0b110 => return null,
        0b111 => return &register.r8.A,
    };
}

fn nop(_: *Register, _: *Memory) void {}

fn jumpImm8(register: *Register, memory: *Memory) void {
    const offset = memory.readI8(register.r16.PC);
    register.r16.PC = @intCast(@as(i32, register.r16.PC) + offset);
}

fn jrNZImm8(register: *Register, memory: *Memory) void {
    if (!flagsHelper.getN(register.r8.F)) {
        jumpImm8(register, memory);
    }
}

fn jrZImm8(register: *Register, memory: *Memory) void {
    if (flagsHelper.getN(register.r8.F)) {
        jumpImm8(register, memory);
    }
}

fn jrNCImm8(register: *Register, memory: *Memory) void {
    if (!flagsHelper.getC(register.r8.F)) {
        jumpImm8(register, memory);
    }
}

fn jrCImm8(register: *Register, memory: *Memory) void {
    if (flagsHelper.getC(register.r8.F)) {
        jumpImm8(register, memory);
    }
}

fn stop(_: *Register, _: *Memory) void {}

fn buildInstructionBlock0(index: u8) Opcode {
    // ---- 0x00 ----
    if (index == 0x00) {
        return nop;
    }

    // ---- LD r16, imm16 ---- 00 rr 0001
    if ((index & 0b11001111) == 0b00000001) {
        return struct {
            fn op(register: *Register, memory: *Memory) void {
                const value = readU16(memory, register);
                const r16 = getR16(register, @truncate(index >> 4));
                r16.* = value;
            }
        }.op;
    }

    // ---- LD (r16), A ---- 00 rr 0010
    if ((index & 0b11001111) == 0b00000010) {
        return struct {
            fn op(register: *Register, memory: *Memory) void {
                const addr = getR16Mem(register, @truncate(index >> 4));
                memory.writeU8(addr, register.r8.A);
            }
        }.op;
    }

    // ---- LD A, (r16) ---- 00 rr 1010
    if ((index & 0b11001111) == 0b00001010) {
        return struct {
            fn op(register: *Register, memory: *Memory) void {
                const addr = getR16Mem(register, @truncate(index >> 4));
                register.r8.A = memory.readU8(addr);
            }
        }.op;
    }

    // ---- LD (a16), SP ---- 0x08
    if (index == 0x08) {
        return struct {
            fn op(register: *Register, memory: *Memory) void {
                const addr = readU16(memory, register);
                memory.writeU8(addr, register.r8.P);
                memory.writeU8(addr + 1, register.r8.S);
            }
        }.op;
    }

    // ---- INC r16 ---- 00 rr 0011
    if ((index & 0b11001111) == 0b00000011) {
        return struct {
            fn op(register: *Register, _: *Memory) void {
                const r16 = getR16(register, @truncate(index >> 4));
                r16.* += 1;
            }
        }.op;
    }

    // ---- DEC r16 ---- 00 rr 1011
    if ((index & 0b11001111) == 0b00001011) {
        return struct {
            fn op(register: *Register, _: *Memory) void {
                const r16 = getR16(register, @truncate(index >> 4));
                r16.* -= 1;
            }
        }.op;
    }

    // ---- ADD HL, r16 ---- 00 rr 1001
    if ((index & 0b11001111) == 0b00001001) {
        return struct {
            fn op(register: *Register, _: *Memory) void {
                const value = getR16(register, @truncate(index >> 4)).*;
                const lo: u8 = @truncate(value);
                const hi: u8 = @truncate(value >> 8);

                var r = ALU.add(register.r8.L, lo);
                register.r8.L = r.result;
                r = ALU.adc(register.r8.H, hi, r.C);
                register.r8.H = r.result;

                flagsHelper.setN(&register.r8.F, false);
                flagsHelper.setH(&register.r8.F, r.H);
                flagsHelper.setC(&register.r8.F, r.C);
            }
        }.op;
    }

    // ---- INC r8 ---- 00 rrr 100
    if ((index & 0b11000111) == 0b00000100) {
        return struct {
            fn op(register: *Register, memory: *Memory) void {
                const opt = getR8(register, @truncate(index >> 3));
                if (opt) |r8| {
                    const r = ALU.inc(r8.*);
                    r8.* = r.result;

                    flagsHelper.setZ(&register.r8.F, r.Z);
                    flagsHelper.setN(&register.r8.F, false);
                    flagsHelper.setH(&register.r8.F, r.H);
                } else {
                    const v = memory.readU8(register.r16.HL);
                    const r = ALU.inc(v);
                    memory.writeU8(register.r16.HL, r.result);

                    flagsHelper.setZ(&register.r8.F, r.Z);
                    flagsHelper.setN(&register.r8.F, false);
                    flagsHelper.setH(&register.r8.F, r.H);
                }
            }
        }.op;
    }

    // ---- DEC r8 ---- 00 rrr 101
    if ((index & 0b11000111) == 0b00000101) {
        return struct {
            fn op(register: *Register, memory: *Memory) void {
                const opt = getR8(register, @truncate(index >> 3));
                if (opt) |r8| {
                    const r = ALU.dec(r8.*);
                    r8.* = r.result;

                    flagsHelper.setZ(&register.r8.F, r.Z);
                    flagsHelper.setN(&register.r8.F, false);
                    flagsHelper.setH(&register.r8.F, r.H);
                } else {
                    const v = memory.readU8(register.r16.HL);
                    const r = ALU.dec(v);
                    memory.writeU8(register.r16.HL, r.result);

                    flagsHelper.setZ(&register.r8.F, r.Z);
                    flagsHelper.setN(&register.r8.F, false);
                    flagsHelper.setH(&register.r8.F, r.H);
                }
            }
        }.op;
    }

    // ---- LD r8, imm8 ---- 00 rrr 110
    if ((index & 0b11000111) == 0b00000110) {
        return struct {
            fn op(register: *Register, memory: *Memory) void {
                const value = memory.readU8(register.r16.PC);
                register.r16.PC += 1;

                const opt = getR8(register, @truncate(index >> 3));
                if (opt) |r8| {
                    r8.* = value;
                } else {
                    memory.writeU8(register.r16.HL, value);
                }
            }
        }.op;
    }

    // ---- misc ops ---- 00 xxx 111
    if ((index & 0b11000111) == 0b00000111) {
        return struct {
            fn op(register: *Register, _: *Memory) void {
                switch (@as(u3, @truncate(index >> 3))) {
                    0 => opcodeHelper.rlca(&register.r8.A, &register.r8.F),
                    1 => opcodeHelper.rrca(&register.r8.A, &register.r8.F),
                    2 => opcodeHelper.rla(&register.r8.A, &register.r8.F),
                    3 => opcodeHelper.rra(&register.r8.A, &register.r8.F),
                    4 => opcodeHelper.daa(&register.r8.A, &register.r8.F),
                    5 => opcodeHelper.cpl(&register.r8.A, &register.r8.F),
                    6 => opcodeHelper.scf(&register.r8.F),
                    7 => opcodeHelper.ccf(&register.r8.F),
                }
            }
        }.op;
    }

    // ---- JR d8 ---- 0x18
    if (index == 0x18) {
        return jumpImm8;
    }

    // ---- JR cc ---- 00 ccc 000 (0x20,28,30,38)
    if ((index & 0b11100111) == 0b00100000) {
        const cond: u2 = @truncate(index >> 3);

        return switch (cond) {
            0 => jrNZImm8,
            1 => jrZImm8,
            2 => jrNCImm8,
            3 => jrCImm8,
        };
    }

    // ---- STOP ---- 0x10
    if (index == 0x10) {
        return stop;
    }

    return struct {
        fn undefinedOpcode(_: *Register, _: *Memory) void {
            std.debug.print("Undefined opcode 0x{X:02}\n", .{index});
            std.posix.exit(1);
        }
    }.undefinedOpcode;
}

fn buildInstructionBlock1(index: u8) Opcode {
    return struct {
        fn undefinedOpcode(_: *Register, _: *Memory) void {
            std.debug.print("Undefined opcode 0x{X:02}\n", .{index});
            std.posix.exit(1);
        }
    }.undefinedOpcode;
}

fn buildInstructionBlock2(index: u8) Opcode {
    return struct {
        fn undefinedOpcode(_: *Register, _: *Memory) void {
            std.debug.print("Undefined opcode 0x{X:02}\n", .{index});
            std.posix.exit(1);
        }
    }.undefinedOpcode;
}

fn buildInstructionBlock3(index: u8) Opcode {
    return struct {
        fn undefinedOpcode(_: *Register, _: *Memory) void {
            std.debug.print("Undefined opcode 0x{X:02}\n", .{index});
            std.posix.exit(1);
        }
    }.undefinedOpcode;
}

fn buildInstructionSet() [256]Opcode {
    var instructionSet: [256]Opcode = undefined;
    for (0..256) |index| {
        switch (@as(u2, @truncate(index >> 6))) {
            0b0 => instructionSet[index] = buildInstructionBlock0(index),
            0b1 => instructionSet[index] = buildInstructionBlock1(index),
            0b10 => instructionSet[index] = buildInstructionBlock2(index),
            0b11 => instructionSet[index] = buildInstructionBlock3(index),
        }
    }
    return instructionSet;
}

pub fn main() !void {
    const instructionSet: [256]Opcode = comptime buildInstructionSet();

    var memory: Memory = undefined;
    try memory.load("data/Tetris.gb");

    var register: Register = undefined;

    // Init
    register.r16.PC = 0x100;
    var opcode = fetchCycle(&register, &memory);

    while (true) {
        instructionSet[opcode](&register, &memory);
        opcode = fetchCycle(&register, &memory);
    }
}
