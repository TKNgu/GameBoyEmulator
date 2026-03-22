const std = @import("std");

// 16KB ROM banks
const GB_ROM_BANK_SIZE = 16 * 1024;
// 8KB RAM banks
const GB_RAM_BANK_SIZE = 8 * 1024;

// GB ROMs are at least 32KB (2 banks)
const GB_CART_MIN_SIZE = GB_ROM_BANK_SIZE * 2;

// I think the biggest licensed GB cartridge is 8MB but let's add a margin in
// case there are homebrews with even bigger carts.
const GB_CART_MAX_SIZE = 32 * 1024 * 1024;

const GB_CART_OFF_TITLE = 0x134;
const GB_CART_OFF_GBC = 0x143;
const GB_CART_OFF_TYPE = 0x147;
const GB_CART_OFF_ROM_BANKS = 0x148;
const GB_CART_OFF_RAM_BANKS = 0x149;

pub const Memory = struct {
    memory: [0xffff]u8,

    pub fn load(memory: *Memory, fileRomPath: [:0]const u8) !void {
        const fileData = try std.fs.cwd().openFile(fileRomPath, .{});
        defer fileData.close();

        const stat = try fileData.stat();
        if (stat.size < GB_CART_MIN_SIZE or stat.size > GB_CART_MAX_SIZE) {
            std.debug.print("File rom size error\n", .{});
            return;
        }

        const size = try fileData.read(memory.memory[0..stat.size]);
        if (size != stat.size) {
            std.debug.print("Error read rom\n", .{});
        }
    }

    pub fn readU8(memory: *const Memory, address: u16) u8 {
        return memory.memory[address];
    }

    pub fn writeU8(memory: *Memory, address: u16, value: u8) void {
        memory.memory[address] = value;
    }

    pub fn readI8(memory: *Memory, address: u16) i8 {
        const value = memory.readU8(address);
        return @bitCast(value);
    }
};
