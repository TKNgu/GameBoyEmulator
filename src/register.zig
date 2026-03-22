const std = @import("std");

pub const Register = extern union {
    // Register struct
    r16: extern struct {
        AF: u16,
        BC: u16,
        DE: u16,
        HL: u16,
        SP: u16,
        PC: u16,
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

    pub fn debug(self: *const Register) void {
        std.debug.print(
            "AF: 0x{X:04}\nBC: 0x{X:04}\nDE: 0x{X:04}\nHL: 0x{X:04}\nSP: 0x{X:04}\nPC: 0x{X:04}\n",
            .{ self.r16.AF, self.r16.BC, self.r16.DE, self.r16.HL, self.r16.SP, self.r16.PC },
        );
    }
};
