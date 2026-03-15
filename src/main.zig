const std = @import("std");
const Memory = @import("memory.zig").Memory;

const CPU = extern union {
    r16: extern struct {
        AF: u16 = 0,
        BC: u16 = 0,
        DE: u16 = 0,
        HL: u16 = 0,
        SP: u16 = 0,
        PC: u16 = 0,
    },
    r8: extern struct {
        F: u8,
        A: u8,
        C: u8,
        B: u8,
        E: u8,
        D: u8,
        L: u8,
        H: u8,
        P: u8,
        S: u8,
        PCLo: u8,
        PCHi: u8,
    },

    pub fn zflag(self: *const CPU) bool {
        return (self.r8.F & 0b10000000) == 0b1000000;
    }

    pub fn nflag(self: *const CPU) bool {
        return (self.r8.F & 0b01000000) == 0b0100000;
    }

    pub fn hflag(self: *const CPU) bool {
        return (self.r8.F & 0b00100000) == 0b0010000;
    }

    pub fn cflag(self: *const CPU) bool {
        return (self.r8.F & 0b00010000) == 0b0001000;
    }

    pub fn zflagOf(self: *CPU) void {
        self.r8.F &= 0b01110000;
    }

    pub fn zflagOn(self: *CPU) void {
        self.r8.F |= 0b10000000;
    }

    pub fn setZ(self: *CPU, value: bool) void {
        if (value) {
            self.zflagOn();
        } else {
            self.zflagOf();
        }
    }

    pub fn nflagOf(self: *CPU) void {
        self.r8.F &= 0b10110000;
    }

    pub fn nflagOn(self: *CPU) void {
        self.r8.F |= 0b01000000;
    }

    pub fn setN(self: *CPU, value: bool) void {
        if (value) {
            self.nflagOn();
        } else {
            self.nflagOf();
        }
    }

    pub fn hflagOf(self: *CPU) void {
        self.r8.F &= 0b11010000;
    }

    pub fn hflagOn(self: *CPU) void {
        self.r8.F |= 0b00100000;
    }

    pub fn setH(self: *CPU, value: bool) void {
        if (value) {
            self.hflagOn();
        } else {
            self.hflagOf();
        }
    }

    pub fn cflagOf(self: *CPU) void {
        self.r8.F &= 0b11100000;
    }

    pub fn cflagOn(self: *CPU) void {
        self.r8.F |= 0b00010000;
    }

    pub fn setC(self: *CPU, value: bool) void {
        if (value) {
            self.cflagOn();
        } else {
            self.cflagOf();
        }
    }
};

const Call = *const fn (cpu: *CPU, memory: *Memory) void;

fn nop(_: *CPU, _: *Memory) void {
    std.debug.print("Nop\n", .{});
}

fn undefinedCall(_: *CPU, _: *Memory) void {
    std.debug.print("Undefine\n", .{});
    std.posix.exit(0);
}

fn getR8(cpu: *CPU, comptime opcode: u8) *u8 {
    const r: u3 = @intCast((opcode >> 3) & 0b111);
    return switch (r) {
        0 => &cpu.r8.B,
        1 => &cpu.r8.C,
        2 => &cpu.r8.D,
        3 => &cpu.r8.E,
        4 => &cpu.r8.H,
        5 => &cpu.r8.L,
        6 => undefined,
        7 => &cpu.r8.A,
    };
}

fn getR16(cpu: *CPU, comptime opcode: u8) *u16 {
    const rr: u2 = @intCast((opcode >> 4) & 0b11);
    return switch (rr) {
        0 => &cpu.r16.BC,
        1 => &cpu.r16.DE,
        2 => &cpu.r16.HL,
        3 => &cpu.r16.SP,
    };
}

fn getR16Mem(cpu: *CPU, comptime opcode: u8) *u16 {
    const rr: u2 = @intCast((opcode >> 4) & 0b11);
    return switch (rr) {
        0 => &cpu.r16.BC,
        1 => &cpu.r16.DE,
        2 => &cpu.r16.HL,
        3 => &cpu.r16.HL,
    };
}

fn readImm16(cpu: *CPU, memory: *Memory) u16 {
    cpu.r16.PC += 1;
    const lo = memory.readU8(cpu.r16.PC);
    cpu.r16.PC += 1;
    const hi: u16 = @intCast(memory.readU8(cpu.r16.PC));
    const imm16 = lo | (hi << 8);
    return imm16;
}

fn updateHL(cpu: *CPU, comptime opcode: u8) void {
    const rr = (opcode >> 4) & 0b11;

    if (rr == 2) cpu.r16.HL += 1;
    if (rr == 3) cpu.r16.HL -= 1;
}

fn makeFnBlock0(opcode: u8) Call {
    return switch (opcode & 0b00001111) {
        0b0000 => nop,
        0b0001 => struct {
            fn ldR16Imm16(cpu: *CPU, memory: *Memory) void {
                const r16 = getR16(cpu, opcode);
                r16.* = readImm16(cpu, memory);
            }
        }.ldR16Imm16,
        0b0010 => struct {
            fn ldR16memA(cpu: *CPU, memory: *Memory) void {
                const r16Mem = getR16Mem(cpu, opcode);
                memory.writeU8(r16Mem.*, cpu.r8.A);
                updateHL(cpu, opcode);
            }
        }.ldR16memA,
        0b1010 => struct {
            fn ldAR16mem(cpu: *CPU, memory: *Memory) void {
                const r16Mem = getR16Mem(cpu, opcode);
                cpu.r8.A = memory.readU8(r16Mem.*);
                updateHL(cpu, opcode);
            }
        }.ldAR16mem,
        0b1000 => struct {
            fn ldImm16SP(cpu: *CPU, memory: *Memory) void {
                const imm16 = readImm16(cpu, memory);
                memory.writeU8(imm16, cpu.r8.P);
                memory.writeU8(imm16 + 1, cpu.r8.S);
            }
        }.ldImm16SP,
        0b0011 => struct {
            fn incR16(cpu: *CPU, _: *Memory) void {
                const r16 = getR16(cpu, opcode);
                r16.* += 1;
            }
        }.incR16,
        0b1011 => struct {
            fn decR16(cpu: *CPU, _: *Memory) void {
                const r16 = getR16(cpu, opcode);
                r16.* -= 1;
            }
        }.decR16,
        0b1001 => struct {
            fn addHLR16(cpu: *CPU, _: *Memory) void {
                const r16 = getR16(cpu, opcode).*;
                const sum: u32 = @as(u32, @intCast(cpu.r16.HL)) +
                    @as(u32, @intCast(r16));
                cpu.r16.HL = @as(u16, @intCast(sum));
                cpu.r8.F &= 0b10010000;
                if (((cpu.r16.HL & 0xfff) + (r16 & 0xfff)) > 0xfff) {
                    cpu.r8.F |= 0x20;
                }
                if (sum > 0xffff) {
                    cpu.r8.F |= 0x10;
                }
            }
        }.addHLR16,
        else => {
            return switch (opcode & 0b00000111) {
                0b100 => struct {
                    fn incR8(cpu: *CPU, _: *Memory) void {
                        const r8 = getR8(cpu, opcode);
                        const old = r8.*;
                        r8.* += 1;
                        cpu.setZ(r8.* == 0);
                        cpu.nflagOf();
                        cpu.setH(((old & 0xF) + 1) > 0xF);
                    }
                }.incR8,
                0b101 => struct {
                    fn decR8(cpu: *CPU, _: *Memory) void {
                        const r8 = getR8(cpu, opcode);
                        const old = r8.*;
                        r8.* -= 1;
                        cpu.setZ(r8.* == 0);
                        cpu.nflagOf();
                        cpu.setH((old & 0xF) == 0);
                    }
                }.decR8,
                0b110 => struct {
                    fn ldR8Imm8(cpu: *CPU, memory: *Memory) void {
                        const r8 = getR8(cpu, opcode);
                        cpu.r16.PC += 1;
                        r8.* = memory.readU8(cpu.r16.PC);
                    }
                }.ldR8Imm8,
                else => undefinedCall,
            };
        },
    };
}

fn makeFn(opcode: u8) Call {
    switch (opcode & 0b11000000) {
        0b00000000 => return makeFnBlock0(opcode),
        else => return undefinedCall,
    }
}

fn buildOpcodeTable() [256]Call {
    var call: [256]Call = undefined;
    for (0..256) |id| {
        call[id] = makeFn(id);
    }
    return call;
}

pub fn main() !void {
    const call = comptime buildOpcodeTable();

    var memory: Memory = undefined;
    try memory.load("data/Tetris.gb");

    var cpu = CPU{ .r16 = .{
        .PC = 0x100,
    } };

    while (true) {
        const opcode = memory.readU8(cpu.r16.PC);
        std.debug.print("Opcode 0x{X:0>2}\n", .{opcode});
        call[opcode](&cpu, &memory);
        cpu.r16.PC += 1;
    }
}
