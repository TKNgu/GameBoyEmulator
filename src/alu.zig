pub const ALU = struct {
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
