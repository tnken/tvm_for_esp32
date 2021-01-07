#include "vm.h"

void vm_init()
{
  vm = (typeof(vm)){NULL};
  vm.stack_top = vm.stack;
}

void vm_push(Value value)
{
  *vm.stack_top = value;
  vm.stack_top++;
}

Value vm_pop()
{
  vm.stack_top--;
  return *vm.stack_top;
}

uint8_t trans(unsigned char c)
{
	if('0'<=c && c<='9') return (c-0x30);
	if('A'<=c && c<='F') return (c+0x0A-0x41);
	if('a'<=c && c<='f') return (c+0x0A-0x61);
	return 0;
}

uint16_t decode_constant(uint8_t upper, uint8_t lower)
{
  // big endian
  return (255*upper + lower);
}

ExecResult exec_interpret(uint8_t *bytecode)
{
  vm_init();

  vm.bp = bytecode;
  vm.ip = vm.bp;
  bool f = true;
  while(f) {
    uint8_t instruction = *vm.ip++;
    switch(instruction) {
      case OP_CONSTANT: {
        uint8_t upper = *vm.ip++;
        uint8_t lower = *vm.ip++;
        uint16_t constant = decode_constant(upper, lower);
        vm_push(NUMBER_VAL(constant));
        break;
      }
      case OP_ADD: {
        Value r = vm_pop();
        Value l = vm_pop();
        vm_push(NUMBER_VAL(l.as.number+r.as.number));
        break;
      }
      case OP_SUB: {
        Value r = vm_pop();
        Value l = vm_pop();
        vm_push(NUMBER_VAL(l.as.number-r.as.number));
        break;
      }
      case OP_MUL: {
        Value r = vm_pop();
        Value l = vm_pop();
        vm_push(NUMBER_VAL(l.as.number*r.as.number));
        break;
      }
      case OP_DIV: {
        Value r = vm_pop();
        Value l = vm_pop();
        if (r.as.number == 0) {
          return EXEC_RESULT(ERROR_DIVISION_BY_ZERO, NIL_VAL());
        }
        vm_push(NUMBER_VAL(l.as.number/r.as.number));
        break;
      }
      case OP_EQ: {
        Value r = vm_pop();
        Value l = vm_pop();
        if (l.as.number-r.as.number == 0) {
          vm_push(BOOL_VAL(true));
          break;
        }
        vm_push(BOOL_VAL(false));
        break;
      }
      case OP_NEQ: {
        Value r = vm_pop();
        Value l = vm_pop();
        if (l.as.number-r.as.number == 0) {
          vm_push(BOOL_VAL(false));
          break;
        }
        vm_push(BOOL_VAL(true));
        break;
      }
      case OP_LESS: {
        Value r = vm_pop();
        Value l = vm_pop();
        if (r.as.number-l.as.number > 0) {
          vm_push(BOOL_VAL(true));
          break;
        }
        vm_push(BOOL_VAL(false));
        break;
      }
      case OP_GREATER: {
        Value r = vm_pop();
        Value l = vm_pop();
        if (l.as.number-r.as.number > 0) {
          vm_push(BOOL_VAL(true));
          break;
        }
        vm_push(BOOL_VAL(false));
        break;
      }
      case OP_DONE: {
        f = false;
        break;
      }
      case OP_LOAD_GLOBAL: {
        uint8_t index = *vm.ip++;
        vm_push(vm.global[index]);
        break;
      }
      case OP_STORE_GLOBAL: {
        uint8_t index = *vm.ip++;
        vm.global[index] = vm_pop();
        vm_push(vm.global[index]);
        break;
      }
      case OP_JNT: {
        Value condition = vm_pop();
        uint8_t upper = *vm.ip++;
        uint8_t lower = *vm.ip++;
        uint16_t jmp_offset = decode_constant(upper, lower);
        if (condition.as.boolean) {
          break;
        }
        vm.ip = vm.bp + jmp_offset;
        break;
      }
      case OP_JMP: {
        uint8_t upper = *vm.ip++;
        uint8_t lower = *vm.ip++;
        uint16_t jmp_offset = decode_constant(upper, lower);
        vm.ip = vm.bp + jmp_offset;
        break;
      }
      default:
        return EXEC_RESULT(ERROR_UNKNOWN_OPCODE, NIL_VAL());
    }
  }

  Value val = vm_pop();
  return EXEC_RESULT(SUCCESS, val);
}

uint8_t* parse_bytecode(char* str)
{
  uint8_t* bytecode;
  // TODO: replace 100
  bytecode = (uint8_t *)malloc(100);
  if(!bytecode) {
  }
  int p = 0, cnt = 0;
  while (cnt < strlen(str)){
    uint8_t up =  16*trans(str[cnt++]);
    uint8_t low = trans(str[cnt++]);
    bytecode[p++] = up+low;
  }
  return bytecode;
}

ExecResult tarto_vm_run(char* input)
{
  uint8_t* bytecode = parse_bytecode(input);
  return exec_interpret(bytecode);
}
