const flagsHelper = @import("flags_helper.zig");

pub fn rlca(poiter: *u8, flags: *u8) void {
    const value = poiter.*;
    const carry = (value >> 7) & 1;
    poiter.* = (value << 1) | carry;
    flagsHelper.setZ(flags, false);
    flagsHelper.setN(flags, false);
    flagsHelper.setH(flags, false);
    flagsHelper.setC(flags, carry == 1);
}

pub fn rrca(poiter: *u8, flags: *u8) void {
    const value = poiter.*;
    const carry = value & 0b1;
    poiter.* = (value >> 1) | (carry << 7);
    flagsHelper.setZ(flags, false);
    flagsHelper.setN(flags, false);
    flagsHelper.setH(flags, false);
    flagsHelper.setC(flags, carry == 1);
}

pub fn rla(poiter: *u8, flags: *u8) void {
    const value = poiter.*;
    const oldC = flagsHelper.getC(flags.*);
    const newC: u1 = @truncate(value >> 7);
    poiter.* = (value << 1) | @intFromBool(oldC);
    flagsHelper.setZ(flags, false);
    flagsHelper.setN(flags, false);
    flagsHelper.setH(flags, false);
    flagsHelper.setC(flags, newC == 1);
}

pub fn rra(poiter: *u8, flags: *u8) void {
    const value = poiter.*;
    const oldC = @as(u8, @intFromBool(flagsHelper.getC(flags.*)));
    const newC = value & 0b1;
    poiter.* = (value >> 1) | (oldC << 7);
    flagsHelper.setZ(flags, false);
    flagsHelper.setN(flags, false);
    flagsHelper.setH(flags, false);
    flagsHelper.setC(flags, newC == 1);
}

pub fn daa(poiter: *u8, flags: *u8) void {
    var value = poiter.*;
    var carry = flagsHelper.getC(flags.*);
    if (!flagsHelper.getN(flags.*)) {
        if (flagsHelper.getH(flags.*) or (value & 0x0F) > 9) {
            value += 0x06;
        }
        if (carry or value > 0x9F) {
            value += 0x60;
            carry = true;
        }
    } else {
        if (flagsHelper.getH(flags.*)) {
            value -%= 0x06;
        }
        if (carry) {
            value -%= 0x60;
        }
    }
    poiter.* = value;
    flagsHelper.setZ(flags, value == 0);
    flagsHelper.setH(flags, false);
    flagsHelper.setC(flags, carry);
}

pub fn cpl(poiter: *u8, flags: *u8) void {
    poiter.* = ~poiter.*;
    flagsHelper.setN(flags, true);
    flagsHelper.setH(flags, true);
}

pub fn scf(flags: *u8) void {
    flagsHelper.setN(flags, false);
    flagsHelper.setH(flags, false);
    flagsHelper.setC(flags, true);
}

pub fn ccf(flags: *u8) void {
    const carry = flagsHelper.getC(flags.*);
    flagsHelper.setN(flags, false);
    flagsHelper.setH(flags, false);
    flagsHelper.setC(flags, !carry);
}
