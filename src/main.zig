const std = @import("std");
const Register = @import("register.zig").Register;
const Memory = @import("memory.zig").Memory;
const flagsHelper = @import("flags_helper.zig");
const opcodeHelper = @import("opcode_helper.zig");

const ALU = struct {
    pub const Result = struct {
        result: u8,
        Z: bool,
        N: bool,
        H: bool,
        C: bool,
    };

    // ========================
    // ADD (a + b)
    // ========================
    pub inline fn add(a: u8, b: u8) Result {
        const sum16: u16 = @as(u16, a) + @as(u16, b);
        const result: u8 = @truncate(sum16);

        return .{
            .result = result,
            .Z = result == 0,
            .N = false,
            .H = ((a & 0x0F) + (b & 0x0F)) > 0x0F,
            .C = sum16 > 0xFF,
        };
    }

    // ========================
    // ADC (a + b + carry)
    // ========================
    pub inline fn adc(a: u8, b: u8, c: bool) Result {
        const carry: u1 = @intFromBool(c);

        const sum16: u16 =
            @as(u16, a) +
            @as(u16, b) +
            carry;

        const result: u8 = @truncate(sum16);

        return .{
            .result = result,
            .Z = result == 0,
            .N = false,
            .H = ((a & 0x0F) + (b & 0x0F) + carry) > 0x0F,
            .C = sum16 > 0xFF,
        };
    }

    // ========================
    // SUB (a - b)
    // ========================
    pub inline fn sub(a: u8, b: u8) Result {
        const result: u8 = a -% b;

        return .{
            .result = result,
            .Z = result == 0,
            .N = true,
            .H = (a & 0x0F) < (b & 0x0F),
            .C = a < b,
        };
    }

    // ========================
    // SBC (a - b - carry)
    // ========================
    pub inline fn sbc(a: u8, b: u8, c: bool) Result {
        const carry: u1 = @intFromBool(c);

        const result: u8 = a -% b -% carry;

        return .{
            .result = result,
            .Z = result == 0,
            .N = true,
            .H = (a & 0x0F) < ((b & 0x0F) + carry),
            .C = @as(u16, a) < (@as(u16, b) + carry),
        };
    }

    // ========================
    // INC (a + 1)
    // ⚠️ không update C
    // ========================
    pub inline fn inc(a: u8) Result {
        const result: u8 = a +% 1;

        return .{
            .result = result,
            .Z = result == 0,
            .N = false,
            .H = (a & 0x0F) == 0x0F,
            .C = false, // caller phải giữ nguyên C
        };
    }

    // ========================
    // DEC (a - 1)
    // ⚠️ không update C
    // ========================
    pub inline fn dec(a: u8) Result {
        const result: u8 = a -% 1;

        return .{
            .result = result,
            .Z = result == 0,
            .N = true,
            .H = (a & 0x0F) == 0x00,
            .C = false, // caller giữ nguyên
        };
    }
};

const Opcode = *const fn (register: *Register, memory: *Memory) void;

fn fetchCycle(register: *Register, memory: *Memory) u8 {
    const opcode = memory.readU8(register.r16.PC);
    register.r16.PC += 1;
    return opcode;
}

fn getR16Mem(register: *Register, index: u8) u16 {
    const r16Mem: u2 = @truncate(index >> 4);
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

fn getR16(register: *Register, comptime index: u8) *u16 {
    const r16Code: u2 = @truncate(index >> 4);
    return switch (r16Code) {
        0b0 => &register.r16.BC,
        0b1 => &register.r16.DE,
        0b10 => &register.r16.HL,
        0b11 => &register.r16.SP,
    };
}

fn getR8(register: *Register, comptime index: u8) ?*u8 {
    const r8Code = @as(u3, (index >> 3) & 0b111);
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

fn buildInstructionBlock0(index: u8) Opcode {
    if (index == 0x00) {
        return nop;
    } else if ((index & 0b1111) == 0b1) {
        return struct {
            fn ldR16Imm16(register: *Register, memory: *Memory) void {
                const value = readU16(memory, register);
                const r16 = getR16(register, index);
                r16.* = value;
            }
        }.ldR16Imm16;
    } else if ((index & 0b1111) == 0b10) {
        return struct {
            fn ldMemR16MemA(register: *Register, memory: *Memory) void {
                const address = getR16Mem(register, index);
                memory.writeU8(address, register.r8.A);
            }
        }.ldMemR16MemA;
    } else if ((index & 0b1111) == 0b1010) {
        return struct {
            fn ldAMemR16Mem(register: *Register, memory: *Memory) void {
                const address = getR16Mem(register, index);
                register.r8.A = memory.readU8(address);
            }
        }.ldAMemR16Mem;
    } else if (index == 0b1000) {
        return struct {
            fn ldMemImm16SP(register: *Register, memory: *Memory) void {
                const address = readU16(memory, register);
                memory.writeU8(address, register.r8.P);
                memory.writeU8(address + 1, register.r8.S);
            }
        }.ldMemImm16SP;
    } else if ((index & 0b111) == 0b11) {
        return struct {
            fn incdecR16(register: *Register, _: *Memory) void {
                const tmp = getR16(register, index);
                if ((index & 0b1000) == 0b1000) {
                    tmp.* -= 1;
                } else {
                    tmp.* += 1;
                }
            }
        }.incdecR16;
    } else if ((index & 0b1111) == 0b1001) {
        return struct {
            fn addHLR16(register: *Register, _: *Memory) void {
                const r16 = getR16(register, index).*;
                const lo: u8 = @truncate(r16);
                const hi: u8 = @truncate(r16 >> 8);
                var result = ALU.add(register.r8.L, lo);
                register.r8.L = result.result;
                result = ALU.adc(register.r8.H, hi, result.C);
                register.r8.H = result.result;
                flagsHelper.setH(&register.r8.F, result.H);
                flagsHelper.setC(&register.r8.F, result.C);
            }
        }.addHLR16;
    } else if ((index & 0b110) == 0b100) {
        return struct {
            fn incdecR8(register: *Register, memory: *Memory) void {
                const optionR8 = getR8(register, index);
                const aluOpcode = index & 0b1;
                if (optionR8) |r8| {
                    const result = if (aluOpcode == 0b1)
                        ALU.inc(r8.*)
                    else
                        ALU.dec(r8.*);
                    r8.* = result.result;
                    flagsHelper.setZ(&register.r8.F, result.Z);
                    flagsHelper.nflagOf(&register.r8.F);
                    flagsHelper.setH(&register.r8.F, result.H);
                } else {
                    const data = memory.readU8(register.r16.HL);
                    const result = if (aluOpcode == 0b1)
                        ALU.inc(data)
                    else
                        ALU.dec(data);
                    memory.writeU8(register.r16.HL, result.result);
                    flagsHelper.setZ(&register.r8.F, result.Z);
                    flagsHelper.nflagOf(&register.r8.F);
                    flagsHelper.setH(&register.r8.F, result.H);
                }
            }
        }.incdecR8;
    } else if ((index & 0b111) == 0b110) {
        return struct {
            fn ldR8Imm8(register: *Register, memory: *Memory) void {
                const value = memory.readU8(register.r16.PC);
                register.r16.PC += 1;
                const optionR8 = getR8(register, index);
                if (optionR8) |r8| {
                    r8.* = value;
                } else {
                    memory.writeU8(register.r16.HL, value);
                }
            }
        }.ldR8Imm8;
    } else if ((index & 0b111) == 0b111) {
        return struct {
            fn bitOpcode(register: *Register, _: *Memory) void {
                switch (@as(u3, @truncate(index >> 3))) {
                    0b0 => opcodeHelper.rlca(&register.r8.A, &register.r8.F),
                    0b1 => opcodeHelper.rrca(&register.r8.A, &register.r8.F),
                    0b10 => opcodeHelper.rla(&register.r8.A, &register.r8.F),
                    0b11 => opcodeHelper.rra(&register.r8.A, &register.r8.F),
                    0b100 => opcodeHelper.daa(&register.r8.A, &register.r8.F),
                    0b101 => opcodeHelper.cpl(&register.r8.A, &register.r8.F),
                    0b110 => opcodeHelper.scf(&register.r8.F),
                    0b111 => opcodeHelper.ccf(&register.r8.F),
                }
            }
        }.bitOpcode;
    } else if (index == 0b11000) {
        return jumpImm8;
    } else {
        return struct {
            fn undefinedOpcode(_: *Register, _: *Memory) void {
                std.debug.panic("Undefined opcode 0x{X:02}\n", .{index});
            }
        }.undefinedOpcode;
    }
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
