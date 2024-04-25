#include <map>
#include <vector>

#define A_REGISTER 0b111
#define B_REGISTER 0b000
#define C_REGISTER 0b001
#define D_REGISTER 0b010
#define E_REGISTER 0b011
#define H_REGISTER 0b100
#define L_REGISTER 0b101

#define BC_REGISTER 0b00
#define DE_REGISTER 0b01
#define HL_REGISTER 0b10
#define SP_REGISTER 0b11

#define NOT_ZERO_FLAG 0b000
#define ZERO_FLAG 0b001
#define NO_CARRY_FLAG 0b010
#define CARRY_FLAG 0b011
#define PARITY_ODD_FLAG 0b100
#define PARITY_EVEN_FLAG 0b101
#define SIGN_POSITIVE_FLAG 0b110
#define SIGN_NEGATIVE_FLAG 0b111

struct CPUState {
  // Registers
  uint8_t a = 0, b = 0, c = 0, d = 0, e = 0, h = 0, l = 0;

  // Special Registers
  uint16_t pc = 0, sp = 0xf000, shift_register = 0;
  uint8_t shift_offset = 0;

  // Flags
  bool zero = false, sign = false, parity = false, carry = false, aux_carry = false;
  bool enable_interrupt = false, halt = false;

  // Memory (64KB)
  uint8_t* ram = new uint8_t[0x10000];

  // Instruction Set
  std::map<uint8_t, std::function<uint32_t(CPUState&)>> opcodes;

  // Input Ports
  uint8_t input_ports[3] = {0, 0, 0};

  // Register Map
  std::map<uint8_t, uint8_t*> registers = {
    {A_REGISTER, &a},
    {B_REGISTER, &b},
    {C_REGISTER, &c},
    {D_REGISTER, &d},
    {E_REGISTER, &e},
    {H_REGISTER, &h},
    {L_REGISTER, &l}
  };

  // Register Masks
  std::vector<uint8_t> register_masks = {
    A_REGISTER,
    B_REGISTER,
    C_REGISTER,
    D_REGISTER,
    E_REGISTER,
    H_REGISTER,
    L_REGISTER
  };

  // Register Pair Masks
  std::vector<uint8_t> register_pair_masks = {
    BC_REGISTER,
    DE_REGISTER,
    HL_REGISTER,
    SP_REGISTER
  };

  // Register Pair Map
  std::map<uint8_t, uint8_t*> register_pair_high = {
    {BC_REGISTER, &b},
    {DE_REGISTER, &d},
    {HL_REGISTER, &h},
    {SP_REGISTER, (uint8_t*)&sp + 1}
  };
  std::map<uint8_t, uint8_t*> register_pair_low = {
    {BC_REGISTER, &c},
    {DE_REGISTER, &e},
    {HL_REGISTER, &l},
    {SP_REGISTER, (uint8_t*)&sp}
  };

  std::vector<uint8_t> condition_flags = {
    NOT_ZERO_FLAG,
    ZERO_FLAG,
    NO_CARRY_FLAG,
    CARRY_FLAG,
    PARITY_ODD_FLAG,
    PARITY_EVEN_FLAG,
    SIGN_POSITIVE_FLAG,
    SIGN_NEGATIVE_FLAG
  };

  uint16_t get_immediate_value16() const {
    return (ram[pc + 2] << 8) | ram[pc + 1];
  }

  uint8_t get_immediate_value8() const {
    return ram[pc + 1];
  }

  uint16_t get_register_pair_value(uint8_t const& reg_pair) const {
    return (*register_pair_high.at(reg_pair) << 8) | *register_pair_low.at(reg_pair);
  }

  void push_stack(uint16_t value) {
    sp -= 2;
    ram[sp] = value & 0xFF;
    ram[sp + 1] = value >> 8;
  }

  uint16_t pop_stack() {
    uint16_t value = ram[sp] | (ram[sp + 1] << 8);
    sp += 2;
    return value;
  }
};

void init_cpu_state(CPUState& cpu);
uint32_t cycle_cpu(CPUState& cpu);
void interrupt_cpu(CPUState& cpu, uint8_t interrupt);