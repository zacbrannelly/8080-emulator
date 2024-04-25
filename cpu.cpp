#include "cpu.h"
#include <string>

// ========================================
// Miscellaneous Group
// ========================================

uint32_t nop(CPUState& cpu) {
  cpu.pc++;
  return 1;
}

// ========================================
// Data Transfer Group
// ========================================

uint32_t move_immediate(uint8_t dst_reg, CPUState& cpu) {
  *cpu.registers[dst_reg] = cpu.get_immediate_value8();
  cpu.pc += 2;
  return 2;
}

uint32_t move_register(uint8_t dst_reg, uint8_t src_reg, CPUState& cpu) {
  *cpu.registers[dst_reg] = *cpu.registers[src_reg];
  cpu.pc++;
  return 1;
}

uint32_t move_from_hl_indirect(uint8_t dst_reg, CPUState& cpu) {
  uint16_t addr = cpu.get_register_pair_value(HL_REGISTER);
  *cpu.registers[dst_reg] = cpu.ram[addr];
  cpu.pc += 1;
  return 2;
}

uint32_t move_to_hl_indirect(uint8_t src_reg, CPUState& cpu) {
  uint16_t addr = cpu.get_register_pair_value(HL_REGISTER);
  cpu.ram[addr] = *cpu.registers[src_reg];
  cpu.pc += 1;
  return 2;
}

uint32_t move_to_memory_immediate(CPUState& cpu) {
  uint16_t addr = cpu.get_register_pair_value(HL_REGISTER);
  cpu.ram[addr] = cpu.get_immediate_value8();
  cpu.pc += 2;
  return 3;
}

uint32_t load_register_pair_immediate(uint8_t dst_reg_pair, CPUState& cpu) {
  uint8_t low_byte = cpu.ram[cpu.pc + 1];
  uint8_t high_byte = cpu.ram[cpu.pc + 2];

  *cpu.register_pair_low[dst_reg_pair] = low_byte;
  *cpu.register_pair_high[dst_reg_pair] = high_byte;

  cpu.pc += 3;

  return 3;
}

uint32_t load_accumulator_direct(CPUState& cpu) {
  uint16_t addr = cpu.get_immediate_value16();
  cpu.a = cpu.ram[addr];
  cpu.pc += 3;

  return 4;
}

uint32_t store_accumulator_direct(CPUState& cpu) {
  uint16_t addr = cpu.get_immediate_value16();
  cpu.ram[addr] = cpu.a;
  cpu.pc += 3;

  return 4;
}

uint32_t load_hl_direct(CPUState& cpu) {
  uint16_t addr = cpu.get_immediate_value16();
  cpu.l = cpu.ram[addr];
  cpu.h = cpu.ram[addr + 1];
  cpu.pc += 3;

  return 5;
}

uint32_t store_hl_direct(CPUState& cpu) {
  uint16_t addr = cpu.get_immediate_value16();
  cpu.ram[addr] = cpu.l;
  cpu.ram[addr + 1] = cpu.h;
  cpu.pc += 3;

  return 5;
}

uint32_t load_accumulator_indirect(uint8_t src_reg_pair, CPUState& cpu) {
  uint16_t addr = cpu.get_register_pair_value(src_reg_pair);
  cpu.a = cpu.ram[addr];
  cpu.pc++;

  return 2;
}

uint32_t store_accumulator_indirect(uint8_t dst_reg_pair, CPUState& cpu) {
  uint16_t addr = cpu.get_register_pair_value(dst_reg_pair);
  cpu.ram[addr] = cpu.a;
  cpu.pc++;

  return 2;
}

uint32_t exchange_hl_and_de(CPUState& cpu) {
  uint8_t temp = cpu.h;
  cpu.h = cpu.d;
  cpu.d = temp;

  temp = cpu.l;
  cpu.l = cpu.e;
  cpu.e = temp;

  cpu.pc++;

  return 1;
}

// ========================================
// Arithmetic Group
// ========================================

void resolve_flags_after_add(uint16_t result, CPUState& cpu) {
  cpu.a = result & 0xFF;
  cpu.zero = cpu.a == 0;
  cpu.sign = cpu.a & 0x80;
  cpu.parity = __builtin_parity(cpu.a);
  cpu.carry = result > 0xFF;
  cpu.aux_carry = (result & 0b11111) > 0b1111;
}

enum CarryOperation {
  NO_CARRY,
  WITH_CARRY,
  WITH_BORROW
};

void add_value_to_accum(uint8_t value, CPUState& cpu, CarryOperation carry_op = NO_CARRY) {
  uint8_t carry = 0;
  switch (carry_op) {
    case WITH_CARRY:
      carry = cpu.carry ? 1 : 0;
      break;
    case WITH_BORROW:
      carry = cpu.carry ? -1 : 0;
      break;
    default:
      break;
  }
  uint16_t result = (uint16_t)cpu.a + (uint16_t)value + (uint16_t)carry;
  resolve_flags_after_add(result, cpu);
}

uint32_t add_register(uint8_t add_reg, CPUState& cpu) {
  add_value_to_accum(*cpu.registers[add_reg], cpu);
  cpu.pc++;

  return 1;
}

uint32_t add_memory(CPUState& cpu) {
  uint16_t addr = cpu.get_register_pair_value(HL_REGISTER);
  add_value_to_accum(cpu.ram[addr], cpu);
  cpu.pc++;

  return 2;
}

uint32_t add_immediate(CPUState& cpu) {
  add_value_to_accum(cpu.get_immediate_value8(), cpu);
  cpu.pc += 2;

  return 2;
}

uint32_t add_register_with_carry(uint8_t add_reg, CPUState& cpu) {
  uint8_t carry = cpu.carry ? 1 : 0;
  add_value_to_accum(*cpu.registers[add_reg] + carry, cpu);
  cpu.pc++;

  return 1;
}

uint32_t add_memory_with_carry(CPUState& cpu) {
  uint16_t addr = cpu.get_register_pair_value(HL_REGISTER);
  add_value_to_accum(cpu.ram[addr], cpu, WITH_CARRY);
  cpu.pc++;

  return 2;
}

uint32_t add_immediate_with_carry(CPUState& cpu) {
  add_value_to_accum(cpu.get_immediate_value8(), cpu, WITH_CARRY);
  cpu.pc += 2;

  return 2;
}

uint32_t subtract_register(uint8_t sub_reg, CPUState& cpu) {
  add_value_to_accum(-*cpu.registers[sub_reg], cpu);
  cpu.pc++;

  return 1;
}

uint32_t subtract_memory(CPUState& cpu) {
  uint16_t addr = cpu.get_register_pair_value(HL_REGISTER);
  add_value_to_accum(-cpu.ram[addr], cpu);
  cpu.pc++;

  return 2;
}

uint32_t subtract_immediate(CPUState& cpu) {
  add_value_to_accum(-cpu.get_immediate_value8(), cpu);
  cpu.pc += 2;

  return 2;
}

uint32_t subtract_register_with_borrow(uint8_t sub_reg, CPUState& cpu) {
  add_value_to_accum(-*cpu.registers[sub_reg], cpu, WITH_BORROW);
  cpu.pc++;

  return 1;
}

uint32_t subtract_memory_with_borrow(CPUState& cpu) {
  uint16_t addr = cpu.get_register_pair_value(HL_REGISTER);
  add_value_to_accum(-cpu.ram[addr], cpu, WITH_BORROW);
  cpu.pc++;

  return 2;
}

uint32_t subtract_immediate_with_borrow(CPUState& cpu) {
  add_value_to_accum(-cpu.get_immediate_value8(), cpu, WITH_BORROW);
  cpu.pc += 2;

  return 2;
}

uint32_t increment_register(uint8_t reg, CPUState& cpu, uint8_t increment = 1) {
  // IMPORTANT: Does not affect the carry flag.
  (*cpu.registers[reg]) += increment;
  cpu.zero = *cpu.registers[reg] == 0;
  cpu.sign = *cpu.registers[reg] & 0x80;
  cpu.parity = __builtin_parity(*cpu.registers[reg]);
  cpu.aux_carry = (*cpu.registers[reg] & 0b11111) > 0b1111;
  cpu.pc++;

  return 1;
}

uint32_t increment_memory(CPUState& cpu, uint8_t increment = 1) {
  // IMPORTANT: Does not affect the carry flag.
  uint16_t addr = cpu.get_register_pair_value(HL_REGISTER);
  cpu.ram[addr] += increment;
  cpu.zero = cpu.ram[addr] == 0;
  cpu.sign = cpu.ram[addr] & 0x80;
  cpu.parity = __builtin_parity(cpu.ram[addr]);
  cpu.aux_carry = (cpu.ram[addr] & 0b11111) > 0b1111;
  cpu.pc++;

  return 3;
}

uint32_t increment_memory_op(CPUState& cpu) {
  return increment_memory(cpu);
}

uint32_t decrement_register(uint8_t reg, CPUState& cpu, uint8_t decrement = 1) {
  return increment_register(reg, cpu, -decrement);
}

uint32_t decrement_memory(CPUState& cpu, uint8_t decrement = 1) {
  return increment_memory(cpu, -decrement);
}

uint32_t decrement_memory_op(CPUState& cpu) {
  return decrement_memory(cpu);
}

uint32_t increment_register_pair(uint8_t reg_pair, CPUState& cpu, uint16_t increment = 1) {
  // IMPORTANT: No flags are affected.
  uint16_t value = cpu.get_register_pair_value(reg_pair);
  value += increment;
  *cpu.register_pair_low[reg_pair] = value & 0xFF;
  *cpu.register_pair_high[reg_pair] = value >> 8;
  cpu.pc++;

  return 1;
}

uint32_t decrement_register_pair(uint8_t reg_pair, CPUState& cpu, uint16_t decrement = 1) {
  return increment_register_pair(reg_pair, cpu, -decrement);
}

uint32_t decimal_adjust_accumulator(CPUState& cpu) {
  // If the least significant nibble of the accumulator is greater than 9 or the auxiliary carry flag is set,
  // add 6 to the accumulator.
  if ((cpu.a & 0x0F) > 9 || cpu.aux_carry) {
    uint16_t result = cpu.a + 6;
    resolve_flags_after_add(result, cpu);
  }

  // If the most significant nibble of the accumulator is greater than 9 or the carry flag is set,
  // add 6 to the most significant nibble.
  uint16_t high_nibble = cpu.a >> 4;
  if (high_nibble > 9 || cpu.carry) {
    uint16_t result = high_nibble + 6;
    result = (result << 4) | (cpu.a & 0x0F);
    resolve_flags_after_add(result, cpu);
  }
  cpu.pc++;

  return 1;
}

uint32_t add_register_pair_to_hl(uint8_t reg_pair, CPUState& cpu) {
  // IMPORTANT: Only the carry flag is affected.
  uint16_t hl = cpu.get_register_pair_value(HL_REGISTER);
  uint16_t reg_pair_value = cpu.get_register_pair_value(reg_pair);
  uint32_t result = hl + reg_pair_value;
  cpu.h = (result >> 8) & 0xFF;
  cpu.l = result & 0xFF;
  cpu.carry = result > 0xFFFF;
  cpu.pc++;

  return 3;
}

// ========================================
// Logical Group
// ========================================

uint32_t and_register(uint8_t reg, CPUState& cpu) {
  cpu.a &= *cpu.registers[reg];
  cpu.zero = cpu.a == 0;
  cpu.sign = cpu.a & 0x80;
  cpu.parity = __builtin_parity(cpu.a);
  cpu.aux_carry = false;
  cpu.carry = false;
  cpu.pc++;

  return 1;
}

uint32_t and_memory(CPUState& cpu) {
  uint16_t addr = cpu.get_register_pair_value(HL_REGISTER);
  cpu.a &= cpu.ram[addr];
  cpu.zero = cpu.a == 0;
  cpu.sign = cpu.a & 0x80;
  cpu.parity = __builtin_parity(cpu.a);
  cpu.aux_carry = false;
  cpu.carry = false;
  cpu.pc++;

  return 2;
}

uint32_t and_immediate(CPUState& cpu) {
  cpu.a &= cpu.get_immediate_value8();
  cpu.zero = cpu.a == 0;
  cpu.sign = cpu.a & 0x80;
  cpu.parity = __builtin_parity(cpu.a);
  cpu.aux_carry = false;
  cpu.carry = false;
  cpu.pc += 2;

  return 2;
}

uint32_t xor_register(uint8_t reg, CPUState& cpu) {
  cpu.a ^= *cpu.registers[reg];
  cpu.zero = cpu.a == 0;
  cpu.sign = cpu.a & 0x80;
  cpu.parity = __builtin_parity(cpu.a);
  cpu.aux_carry = false;
  cpu.carry = false;
  cpu.pc++;

  return 1;
}

uint32_t xor_memory(CPUState& cpu) {
  uint16_t addr = cpu.get_register_pair_value(HL_REGISTER);
  cpu.a ^= cpu.ram[addr];
  cpu.zero = cpu.a == 0;
  cpu.sign = cpu.a & 0x80;
  cpu.parity = __builtin_parity(cpu.a);
  cpu.aux_carry = false;
  cpu.carry = false;
  cpu.pc++;

  return 2;
}

uint32_t xor_immediate(CPUState& cpu) {
  cpu.a ^= cpu.get_immediate_value8();
  cpu.zero = cpu.a == 0;
  cpu.sign = cpu.a & 0x80;
  cpu.parity = __builtin_parity(cpu.a);
  cpu.aux_carry = false;
  cpu.carry = false;
  cpu.pc += 2;

  return 2;
}

uint32_t or_register(uint8_t reg, CPUState& cpu) {
  cpu.a |= *cpu.registers[reg];
  cpu.zero = cpu.a == 0;
  cpu.sign = cpu.a & 0x80;
  cpu.parity = __builtin_parity(cpu.a);
  cpu.aux_carry = false;
  cpu.carry = false;
  cpu.pc++;

  return 1;
}

uint32_t or_memory(CPUState& cpu) {
  uint16_t addr = cpu.get_register_pair_value(HL_REGISTER);
  cpu.a |= cpu.ram[addr];
  cpu.zero = cpu.a == 0;
  cpu.sign = cpu.a & 0x80;
  cpu.parity = __builtin_parity(cpu.a);
  cpu.aux_carry = false;
  cpu.carry = false;
  cpu.pc++;

  return 2;
}

uint32_t or_immediate(CPUState& cpu) {
  cpu.a |= cpu.get_immediate_value8();
  cpu.zero = cpu.a == 0;
  cpu.sign = cpu.a & 0x80;
  cpu.parity = __builtin_parity(cpu.a);
  cpu.aux_carry = false;
  cpu.carry = false;
  cpu.pc += 2;

  return 2;
}

uint32_t compare_register(uint8_t reg, CPUState& cpu) {
  uint16_t value = (uint16_t)*cpu.registers[reg];
  uint16_t result = cpu.a - value;
  cpu.zero = result == 0;
  cpu.sign = result & 0x80;
  cpu.parity = __builtin_parity(result);
  cpu.carry = cpu.a < value;
  cpu.aux_carry = (result & 0b11111) > 0b1111;
  cpu.pc++;

  return 1;
}

uint32_t compare_memory(CPUState& cpu) {
  uint16_t addr = cpu.get_register_pair_value(HL_REGISTER);
  uint16_t value = (uint16_t)cpu.ram[addr];
  uint16_t result = cpu.a - value;
  cpu.zero = result == 0;
  cpu.sign = result & 0x80;
  cpu.parity = __builtin_parity(result);
  cpu.carry = cpu.a < value;
  cpu.aux_carry = (result & 0b11111) > 0b1111;
  cpu.pc++;

  return 2;
}

uint32_t compare_immediate(CPUState& cpu) {
  uint16_t value = (uint16_t)cpu.get_immediate_value8();
  uint16_t result = cpu.a - value;
  cpu.zero = result == 0;
  cpu.sign = result & 0x80;
  cpu.parity = __builtin_parity(result);
  cpu.carry = cpu.a < value;
  cpu.aux_carry = (result & 0b11111) > 0b1111;
  cpu.pc += 2;

  return 2;
}

uint32_t rotate_left(CPUState& cpu) {
  uint8_t msb = (cpu.a & 0x80) >> 7;
  cpu.a = (cpu.a << 1) | msb;
  cpu.carry = msb == 1;
  cpu.pc++;

  return 1;
}

uint32_t rotate_right(CPUState& cpu) {
  uint8_t lsb = cpu.a & 0x01;
  cpu.a = cpu.a >> 1 | (lsb << 7);
  cpu.carry = lsb == 1;
  cpu.pc++;

  return 1;
}

uint32_t rotate_left_through_carry(CPUState& cpu) {
  uint8_t msb = (cpu.a & 0x80) >> 7;
  cpu.a = (cpu.a << 1) | (cpu.carry ? 1 : 0);
  cpu.carry = msb == 1;
  cpu.pc++;

  return 1;
}

uint32_t rotate_right_through_carry(CPUState& cpu) {
  uint8_t lsb = cpu.a & 0x01;
  cpu.a = (cpu.a >> 1) | (cpu.carry ? 1 << 7 : 0);
  cpu.carry = lsb == 1;
  cpu.pc++;

  return 1;
}

uint32_t complement_accumulator(CPUState& cpu) {
  cpu.a = ~cpu.a;
  cpu.pc++;

  return 1;
}

uint32_t complement_carry_flag(CPUState& cpu) {
  cpu.carry = !cpu.carry;
  cpu.pc++;

  return 1;
}

uint32_t set_carry_flag(CPUState& cpu) {
  cpu.carry = true;
  cpu.pc++;

  return 1;
}

// ========================================
// Branch Group
// ========================================

uint32_t jump(CPUState& cpu) {
  cpu.pc = cpu.get_immediate_value16();
  return 3;
}

bool evaluate_condition(uint8_t condition_flag, CPUState& cpu) {
  switch (condition_flag) {
    case NOT_ZERO_FLAG:
      return !cpu.zero;
    case ZERO_FLAG:
      return cpu.zero;
    case NO_CARRY_FLAG:
      return !cpu.carry;
    case CARRY_FLAG:
      return cpu.carry;
    case PARITY_ODD_FLAG:
      return !cpu.parity;
    case PARITY_EVEN_FLAG:
      return cpu.parity;
    case SIGN_POSITIVE_FLAG:
      return !cpu.sign;
    case SIGN_NEGATIVE_FLAG:
      return cpu.sign;
    default:
      return false;
  }
}

uint32_t conditional_jump(uint8_t condition_flag, CPUState& cpu) {
  if (evaluate_condition(condition_flag, cpu)) {
    jump(cpu);
  } else {
    cpu.pc += 3;
  }

  // TODO: Figure out if this is correct.
  return 3;
}

uint32_t call(CPUState& cpu) {
  uint16_t addr = cpu.get_immediate_value16();
  uint16_t next_instruction = cpu.pc + 3;
  cpu.push_stack(next_instruction);
  cpu.pc = addr;

  return 5;
}

uint32_t condition_call(uint8_t condition_flag, CPUState& cpu) {
  if (evaluate_condition(condition_flag, cpu)) {
    return call(cpu);
  } else {
    cpu.pc += 3;
  }

  return 3;
}

uint32_t return_from_subroutine(CPUState& cpu) {
  uint16_t addr = cpu.pop_stack();
  cpu.pc = addr;

  return 3;
}

uint32_t conditional_return(uint8_t condition_flag, CPUState& cpu) {
  if (evaluate_condition(condition_flag, cpu)) {
    return return_from_subroutine(cpu);
  } else {
    cpu.pc++;
  }

  return 1;
}

uint32_t restart(uint8_t restart_code, CPUState& cpu) {
  uint16_t next_instruction = cpu.pc + 1;
  cpu.push_stack(next_instruction);
  cpu.pc = restart_code << 3; // Multiply by 8

  return 3;
}

uint32_t jump_to_hl(CPUState& cpu) {
  cpu.pc = cpu.get_register_pair_value(HL_REGISTER);
  return 1;
}

// ========================================
// Stack, I/O, and Machine Control Group
// ========================================

uint32_t push(uint8_t reg_pair, CPUState& cpu) {
  if (reg_pair == SP_REGISTER) {
    throw std::runtime_error("Error: Cannot push SP register.");
  }
  uint16_t value = cpu.get_register_pair_value(reg_pair);
  cpu.push_stack(value);
  cpu.pc++;

  return 3;
}

uint32_t pop(uint8_t reg_pair, CPUState& cpu) {
  if (reg_pair == SP_REGISTER) {
    throw std::runtime_error("Error: Cannot pop SP register.");
  }
  uint16_t value = cpu.pop_stack();
  *cpu.register_pair_low[reg_pair] = value & 0xFF;
  *cpu.register_pair_high[reg_pair] = value >> 8;
  cpu.pc++;

  return 3;
}

uint32_t push_processor_state(CPUState& cpu) {
  uint8_t low_byte = cpu.a;
  uint8_t high_byte = 0;
  high_byte |= cpu.sign << 7;
  high_byte |= cpu.zero << 6;
  high_byte |= cpu.aux_carry << 4;
  high_byte |= cpu.parity << 2;
  high_byte |= 1 << 1; // Unused bit
  high_byte |= cpu.carry;

  uint16_t value = (high_byte << 8) | low_byte;
  cpu.push_stack(value);
  cpu.pc++;

  return 3;
}

uint32_t pop_processor_state(CPUState& cpu) {
  uint16_t value = cpu.pop_stack();
  uint8_t low_byte = value & 0xFF;
  uint8_t high_byte = value >> 8;

  cpu.a = low_byte;
  cpu.sign = high_byte & 0x80;
  cpu.zero = high_byte & 0x40;
  cpu.aux_carry = high_byte & 0x10;
  cpu.parity = high_byte & 0x04;
  cpu.carry = high_byte & 0x01;
  cpu.pc++;

  return 3;
}

uint32_t exchange_stack_top_with_hl(CPUState& cpu) {
  uint16_t stack_top = cpu.pop_stack();
  uint16_t hl = cpu.get_register_pair_value(HL_REGISTER);
  cpu.push_stack(hl);
  *cpu.register_pair_low[HL_REGISTER] = stack_top & 0xFF;
  *cpu.register_pair_high[HL_REGISTER] = stack_top >> 8;
  cpu.pc++;

  return 5;
}

uint32_t move_hl_to_stack_pointer(CPUState& cpu) {
  cpu.sp = cpu.get_register_pair_value(HL_REGISTER);
  cpu.pc++;

  return 1;
}

// Special shift register instructions via the IN and OUT instructions.
void transfer_to_shift_register(CPUState& cpu) {
  // Stores the accumulator in the shift register.
  // Puts it in the most significant byte and moves the previous value to the least significant byte.
  cpu.shift_register = cpu.shift_register >> 8 | cpu.a << 8;
}

void transfer_to_shift_offset(CPUState& cpu) {
  // Stores the least significant 3 bits of the accumulator in the shift offset.
  // Since the shift register can only shift by 0-7 bits, we only need 3 bits.
  cpu.shift_offset = cpu.a & 0x7;
}

void transfer_from_shift_register(CPUState& cpu) {
  // Completes the shift operation requested by the OUT instructions.
  cpu.a = (cpu.shift_register >> (8 - cpu.shift_offset)) & 0xFF;
}

uint32_t input_from_port(CPUState& cpu) {
  uint8_t port = cpu.get_immediate_value8();
  if (port < 3) {
    cpu.a = cpu.input_ports[port];
  }
  
  switch (port) {
    case 3:
      transfer_from_shift_register(cpu);
      break;
  }

  cpu.pc += 2;
  return 3;
}

uint32_t output_to_port(CPUState& cpu) {
  uint8_t port = cpu.get_immediate_value8();

  // Output is cpu.a
  switch (port) {
    case 2:
      transfer_to_shift_offset(cpu);
      break;
    case 4:
      transfer_to_shift_register(cpu);
      break;
  }

  cpu.pc += 2;
  return 3;
}

uint32_t enable_interrupts(CPUState& cpu) {
  cpu.enable_interrupt = true;
  cpu.pc++;

  return 1;
}

uint32_t disable_interrupts(CPUState& cpu) {
  cpu.enable_interrupt = false;
  cpu.pc++;

  return 1;
}

uint32_t halt(CPUState& cpu) {
  cpu.halt = true;
  cpu.pc++;

  return 1;
}

void init_cpu_state(CPUState& cpu) {
  // No Operation - 00-000-000
  cpu.opcodes[0x00] = nop;

  // ========================================
  // Data Transfer Group
  // ========================================
  for (uint8_t r1 : cpu.register_masks) {
    // Move Immediate to Register (r1) - 00-ddd-110
    cpu.opcodes[0x06 | r1 << 3] = [r1](CPUState& cpu) {
      return move_immediate(r1, cpu);
    };
    
    for (uint8_t r2 : cpu.register_masks) {
      // Move Register Instructions (r1, r2) - 01-ddd-sss
      cpu.opcodes[0x40 | r1 << 3 | r2] = [r1, r2](CPUState& cpu) {
        return move_register(r1, r2, cpu);
      };
    }

    // Move from memory location stored in HL to register (r1) - 01-ddd-110
    cpu.opcodes[0x46 | r1 << 3] = [r1](CPUState& cpu) {
      return move_from_hl_indirect(r1, cpu);
    };

    // Move register (r1) to memory location stored in HL - 01-110-sss
    cpu.opcodes[0x70 | r1] = [r1](CPUState& cpu) {
      return move_to_hl_indirect(r1, cpu);
    };
  }

  // Move immediate data (next byte) to memory location stored in HL - 00-110-110
  cpu.opcodes[0x36] = move_to_memory_immediate;

  for (uint8_t rp : cpu.register_pair_masks) {
    // Load register pair immediate - 00-rp-0001
    cpu.opcodes[0x01 | rp << 4] = [rp](CPUState& cpu) {
      return load_register_pair_immediate(rp, cpu);
    };
  }

  // Load accumulator direct - 00-111-010
  cpu.opcodes[0x3A] = load_accumulator_direct;

  // Store accumulator direct - 00-110-110
  cpu.opcodes[0x32] = store_accumulator_direct;

  // Load HL direct - 00-101-010
  cpu.opcodes[0x2A] = load_hl_direct;

  // Store HL direct - 00-100-010
  cpu.opcodes[0x22] = store_hl_direct;

  // Load accumulator indirect - 00-rp-1010 (only BC and DE registers are supported)
  cpu.opcodes[0x0A | BC_REGISTER << 4] = [](CPUState& cpu) {
    return load_accumulator_indirect(BC_REGISTER, cpu);
  };
  cpu.opcodes[0x0A | DE_REGISTER << 4] = [](CPUState& cpu) {
    return load_accumulator_indirect(DE_REGISTER, cpu);
  };

  // Store accumulator indirect - 00-rp-0010 (only BC and DE registers are supported)
  cpu.opcodes[0x02 | BC_REGISTER << 4] = [](CPUState& cpu) {
    return store_accumulator_indirect(BC_REGISTER, cpu);
  };
  cpu.opcodes[0x02 | DE_REGISTER << 4] = [](CPUState& cpu) {
    return store_accumulator_indirect(DE_REGISTER, cpu);
  };

  // Exchange HL and DE - 11-101-011
  cpu.opcodes[0xEB] = exchange_hl_and_de;

  // ========================================
  // Arithmetic Group
  // ========================================

  // Add Register - 10-000-sss
  for (uint8_t r : cpu.register_masks) {
    cpu.opcodes[0x80 | r] = [r](CPUState& cpu) {
      return add_register(r, cpu);
    };
  }

  // Add Memory - 10-000-110
  cpu.opcodes[0x86] = add_memory;

  // Add Immediate - 11-000-110
  cpu.opcodes[0xC6] = add_immediate;

  // Add Register with carry - 10-001-sss
  for (uint8_t r : cpu.register_masks) {
    cpu.opcodes[0x88 | r] = [r](CPUState& cpu) {
      return add_register_with_carry(r, cpu);
    };
  }

  // Add Memory with carry - 10-001-110
  cpu.opcodes[0x8E] = add_memory_with_carry;

  // Add Immediate with carry - 11-001-110
  cpu.opcodes[0xCE] = add_immediate_with_carry;

  // Subtract Register - 10-010-sss
  for (uint8_t r : cpu.register_masks) {
    cpu.opcodes[0x90 | r] = [r](CPUState& cpu) {
      return subtract_register(r, cpu);
    };
  }

  // Subtract Memory - 10-010-110
  cpu.opcodes[0x96] = subtract_memory;

  // Subtract Immediate - 11-010-110
  cpu.opcodes[0xD6] = subtract_immediate;

  // Subtract Register with borrow - 10-011-sss
  for (uint8_t r : cpu.register_masks) {
    cpu.opcodes[0x98 | r] = [r](CPUState& cpu) {
      return subtract_register_with_borrow(r, cpu);
    };
  }

  // Subtract Memory with borrow - 10-011-110
  cpu.opcodes[0x9E] = subtract_memory_with_borrow;

  // Subtract Immediate with borrow - 11-011-110
  cpu.opcodes[0xDE] = subtract_immediate_with_borrow;

  // Increment Register - 00-ddd-100
  for (uint8_t r : cpu.register_masks) {
    cpu.opcodes[0x04 | r << 3] = [r](CPUState& cpu) {
      return increment_register(r, cpu);
    };
  }

  // Increment Memory - 00-110-100
  cpu.opcodes[0x34] = increment_memory_op;

  // Decrement Register - 00-ddd-101
  for (uint8_t r : cpu.register_masks) {
    cpu.opcodes[0x05 | r << 3] = [r](CPUState& cpu) {
      return decrement_register(r, cpu);
    };
  }

  // Decrement Memory - 00-110-101
  cpu.opcodes[0x35] = decrement_memory_op;

  // Increment Register Pair - 00-rp-0011
  for (uint8_t rp : cpu.register_pair_masks) {
    cpu.opcodes[0x03 | rp << 4] = [rp](CPUState& cpu) {
      return increment_register_pair(rp, cpu);
    };
  }

  // Decrement Register Pair - 00-rp-1011
  for (uint8_t rp : cpu.register_pair_masks) {
    cpu.opcodes[0x0B | rp << 4] = [rp](CPUState& cpu) {
      return decrement_register_pair(rp, cpu);
    };
  }

  // Add Register Pair to HL - 00-rp-1001
  for (uint8_t rp : cpu.register_pair_masks) {
    cpu.opcodes[0x09 | rp << 4] = [rp](CPUState& cpu) {
      return add_register_pair_to_hl(rp, cpu);
    };
  }

  // Decimal Adjust Accumulator - 00-100-111
  cpu.opcodes[0x27] = decimal_adjust_accumulator;

  // ========================================
  // Logical Group
  // ========================================

  // AND Register - 10-100-sss
  for (uint8_t r : cpu.register_masks) {
    cpu.opcodes[0xA0 | r] = [r](CPUState& cpu) {
      return and_register(r, cpu);
    };
  }

  // AND Memory - 10-100-110
  cpu.opcodes[0xA6] = and_memory;

  // AND Immediate - 11-100-110
  cpu.opcodes[0xE6] = and_immediate;

  // Exclusive OR Register - 10-101-sss
  for (uint8_t r : cpu.register_masks) {
    cpu.opcodes[0xA8 | r] = [r](CPUState& cpu) {
      return xor_register(r, cpu);
    };
  }

  // Exclusive OR Memory - 10-101-110
  cpu.opcodes[0xAE] = xor_memory;

  // Exclusive OR Immediate - 11-101-110
  cpu.opcodes[0xEE] = xor_immediate;

  // OR Register - 10-110-sss
  for (uint8_t r : cpu.register_masks) {
    cpu.opcodes[0xB0 | r] = [r](CPUState& cpu) {
      return or_register(r, cpu);
    };
  }

  // OR Memory - 10-110-110
  cpu.opcodes[0xB6] = or_memory;

  // OR Immediate - 11-110-110
  cpu.opcodes[0xF6] = or_immediate;

  // Compare Register - 10-111-sss
  for (uint8_t r : cpu.register_masks) {
    cpu.opcodes[0xB8 | r] = [r](CPUState& cpu) {
      return compare_register(r, cpu);
    };
  }

  // Compare Memory - 10-111-110
  cpu.opcodes[0xBE] = compare_memory;

  // Compare Immediate - 11-111-110
  cpu.opcodes[0xFE] = compare_immediate;

  // Rotate Left - 00-000-111
  cpu.opcodes[0x07] = rotate_left;

  // Rotate Right - 00-001-111
  cpu.opcodes[0x0F] = rotate_right;

  // Rotate Left through Carry - 00-010-111
  cpu.opcodes[0x17] = rotate_left_through_carry;

  // Rotate Right through Carry - 00-011-111
  cpu.opcodes[0x1F] = rotate_right_through_carry;

  // Complement Accumulator - 00-101-111
  cpu.opcodes[0x2F] = complement_accumulator;

  // Complement Carry - 00-111-111
  cpu.opcodes[0x3F] = complement_carry_flag;

  // Set Carry - 00-110-111
  cpu.opcodes[0x37] = set_carry_flag;

  // ========================================
  // Branch Group
  // ========================================

  // Jump - 11-000-011
  cpu.opcodes[0xC3] = jump;

  // Conditional Jump - 11-ccc-010
  for (uint8_t condition_flag : cpu.condition_flags) {
    cpu.opcodes[0xC2 | condition_flag << 3] = [condition_flag](CPUState& cpu) {
      return conditional_jump(condition_flag, cpu);
    };
  }

  // Call - 11-001-101
  cpu.opcodes[0xCD] = call;

  // Conditional Call - 11-ccc-100
  for (uint8_t condition_flag : cpu.condition_flags) {
    cpu.opcodes[0xC4 | condition_flag << 3] = [condition_flag](CPUState& cpu) {
      return condition_call(condition_flag, cpu);
    };
  }

  // Return - 11-001-001
  cpu.opcodes[0xC9] = return_from_subroutine;

  // Conditional Return - 11-ccc-000
  for (uint8_t condition_flag : cpu.condition_flags) {
    cpu.opcodes[0xC0 | condition_flag << 3] = [condition_flag](CPUState& cpu) {
      return conditional_return(condition_flag, cpu);
    };
  }

  // Restart - 11-nnn-111 (nnn = 0-7)
  for (uint8_t restart_code = 0; restart_code < 8; restart_code++) {
    cpu.opcodes[0xC7 | restart_code << 3] = [restart_code](CPUState& cpu) {
      return restart(restart_code, cpu);
    };
  }

  // Jump to HL - 11-101-001
  cpu.opcodes[0xE9] = jump_to_hl;

  // ========================================
  // Stack, I/O, and Machine Control Group
  // ========================================

  // Push Register Pair - 11-rp-0101
  for (uint8_t rp : cpu.register_pair_masks) {
    cpu.opcodes[0xC5 | rp << 4] = [rp](CPUState& cpu) {
      return push(rp, cpu);
    };
  }

  // Pop Register Pair - 11-rp-0001
  for (uint8_t rp : cpu.register_pair_masks) {
    cpu.opcodes[0xC1 | rp << 4] = [rp](CPUState& cpu) {
      return pop(rp, cpu);
    };
  }

  // Push Processor State - 11-110-101
  cpu.opcodes[0xF5] = push_processor_state;

  // Pop Processor State - 11-110-001
  cpu.opcodes[0xF1] = pop_processor_state;

  // Exchange Stack Top with HL - 11-100-011
  cpu.opcodes[0xE3] = exchange_stack_top_with_hl;

  // Move HL to Stack Pointer - 11-111-001
  cpu.opcodes[0xF9] = move_hl_to_stack_pointer;

  // Input - 11-011-011
  cpu.opcodes[0xDB] = input_from_port;

  // Output - 11-010-011
  cpu.opcodes[0xD3] = output_to_port;

  // Enable Interrupts - 11-111-011
  cpu.opcodes[0xFB] = enable_interrupts;

  // Disable Interrupts - 11-110-011
  cpu.opcodes[0xF3] = disable_interrupts;

  // Halt - 01-110-110
  cpu.opcodes[0x76] = halt;
}

uint32_t cycle_cpu(CPUState& cpu) {
  uint8_t opcode = cpu.ram[cpu.pc];
  auto instruction_it = cpu.opcodes.find(opcode);
  if (instruction_it == cpu.opcodes.end()) {
    throw std::runtime_error("Error: Unimplemented opcode " + std::to_string(opcode));
  }

  // Execute the instruction.
  auto cycles = instruction_it->second(cpu);
  return cycles;
}

void interrupt_cpu(CPUState& cpu, uint8_t interrupt_num) {
  if (cpu.enable_interrupt) {
    cpu.push_stack(cpu.pc);
    cpu.pc = 8 * interrupt_num;
    cpu.enable_interrupt = false;
  }
}
