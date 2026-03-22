pub fn getZ(flags: u8) bool {
    return (flags & 0b10000000) == 0b1000000;
}

pub fn getN(flags: u8) bool {
    return (flags & 0b01000000) == 0b0100000;
}

pub fn getH(flags: u8) bool {
    return (flags & 0b00100000) == 0b0010000;
}

pub fn getC(flags: u8) bool {
    return (flags & 0b00010000) == 0b0001000;
}

pub fn zflagOf(flags: *u8) void {
    flags.* &= 0b01110000;
}

pub fn zflagOn(flags: *u8) void {
    flags.* |= 0b10000000;
}

pub fn setZ(flags: *u8, value: bool) void {
    if (value) {
        zflagOf(flags);
    } else {
        zflagOf(flags);
    }
}

pub fn nflagOf(flags: *u8) void {
    flags.* &= 0b10110000;
}

pub fn nflagOn(flags: *u8) void {
    flags.* |= 0b01000000;
}

pub fn setN(flags: *u8, value: bool) void {
    if (value) {
        nflagOn(flags);
    } else {
        nflagOf(flags);
    }
}

pub fn hflagOf(flags: *u8) void {
    flags.* &= 0b11010000;
}

pub fn hflagOn(flags: *u8) void {
    flags.* |= 0b00100000;
}

pub fn setH(flags: *u8, value: bool) void {
    if (value) {
        hflagOn(flags);
    } else {
        hflagOf(flags);
    }
}

pub fn cflagOf(flags: *u8) void {
    flags.* &= 0b11100000;
}

pub fn cflagOn(flags: *u8) void {
    flags.* |= 0b00010000;
}

pub fn setC(flags: *u8, value: bool) void {
    if (value) {
        cflagOn(flags);
    } else {
        cflagOf(flags);
    }
}
