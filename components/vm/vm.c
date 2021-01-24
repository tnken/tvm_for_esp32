#include "vm.h"

Frame *current_frame()
{
  return &vm.frames[vm.frame_index-1];
}

Frame pop_frame()
{
  vm.stack_top = current_frame()->bp;
  vm.frame_index--;
  return vm.frames[vm.frame_index];
}

void push_frame(Frame frame)
{
  frame.bp = vm.stack_top-frame.arg_num;
  vm.frames[vm.frame_index] = frame;
  vm.frame_index++;
}

Frame new_frame(uint16_t ins_size, uint8_t* ins, uint8_t arg_num, Constant *constants, bool f_method)
{
  Frame frame;
  frame.instruction_size = ins_size;
  frame.instructions = ins;
  frame.ip = -1;
  frame.arg_num =  arg_num;
  frame.constants = constants;
  frame.f_method = f_method;
  return frame;
}

void vm_init(Bytecode b)
{
  vm.stack_top = vm.stack;
  Frame main_func = new_frame(b.instruction_size, b.instructions, 0, b.constants, false);
  main_func.bp = vm.stack_top;
  vm.frames[0] = main_func;
  vm.frame_index = 1;
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

uint8_t calc_byte(unsigned char upper, unsigned char lower) {
  uint8_t up =  16*trans(upper);
  uint8_t low = trans(lower);
  return up + low;
}

uint16_t decode_constant(uint8_t upper, uint8_t lower)
{
  // big endian
  return (255*upper + lower);
}

Constant find_method(Bytecode bytecode, uint8_t class_id, uint8_t method_id) {
  Constant c;
  for (int i=0; i<bytecode.classes[class_id].constant_size; i++) {
    c = bytecode.classes[class_id].constants[i];
    if (c.type == CONST_FUNC && c.method_index == method_id) {
      break;
    }
  }
  return c;
}

ExecResult exec_interpret(Bytecode b)
{
  vm_init(b);

  while(current_frame()->ip < current_frame()->instruction_size - 1) {
    current_frame()->ip++;
    uint8_t* ins = current_frame()->instructions;
    uint16_t ip = current_frame()->ip;
    uint8_t op = ins[ip];

    switch(op) {
      case OP_CONSTANT: {
        uint8_t upper = ins[ip+1];
        uint8_t lower = ins[ip+2];
        current_frame()->ip += 2;
        uint16_t constant_index = decode_constant(upper, lower);
        Constant c =  current_frame()->constants[constant_index-1];
        switch(c.type) {
          case CONST_INT: {
            upper = c.content[0];
            lower = c.content[1];
            vm_push(NUMBER_VAL(decode_constant(upper, lower)));
            break;
          }
          case CONST_FUNC: {
            vm_push(FUNCTION_VAL(c));
            break;
          }
        }
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
        break;
      }
      case OP_LOAD_GLOBAL: {
        uint8_t index = ins[++current_frame()->ip];
        vm_push(vm.global[index]);
        break;
      }
      case OP_STORE_GLOBAL: {
        current_frame()->ip++;
        uint8_t index = ins[current_frame()->ip];
        vm.global[index] = vm_pop();
        break;
      }
      case OP_JNT: {
        Value condition = vm_pop();
        uint8_t upper = ins[++current_frame()->ip];
        uint8_t lower = ins[++current_frame()->ip];
        uint16_t jmp_offset = decode_constant(upper, lower);
        if (condition.as.boolean) {
          break;
        }
        current_frame()->ip = jmp_offset-1;
        break;
      }
      case OP_JMP: {
        uint8_t upper = ins[++current_frame()->ip];
        uint8_t lower = ins[++current_frame()->ip];
        uint16_t jmp_offset = decode_constant(upper, lower);
        current_frame()->ip = jmp_offset-1;
        break;
      }
      case OP_CALL: {
        int8_t arg_num = ins[++current_frame()->ip];
        Value constant = *(vm.stack_top-arg_num-1);

        if (constant.as.function.type != CONST_FUNC) return EXEC_RESULT(ERROR_OTHER, NIL_VAL());
        push_frame(new_frame(constant.as.function.size, constant.as.function.content, arg_num, b.constants, false));
        break;
      }
      case OP_RETURN_VAL: {
        Value val = vm_pop();
        Frame f = pop_frame();
        Value function = vm_pop(); // pop function
        if (function.as.function.method_index == 0 && f.f_method) {
          // constructor
          break;
        }
        if (f.f_method) {
          vm_pop(); // pop receiver
        }
        vm_push(val);
        break;
      }
      case OP_LOAD_LOCAL: {
        uint8_t index = ins[++current_frame()->ip];
        if (index < current_frame()->arg_num) {
          Value arg_val = current_frame()->bp[index];
          vm_push(arg_val);
          break;
        }
        vm_push(current_frame()->local[index]);
        break;
      }
      case OP_STORE_LOCAL: {
        uint8_t index = ins[++current_frame()->ip];
        if (index < current_frame()->arg_num) {
          current_frame()->bp[index] = vm_pop();
          break;
        }
        current_frame()->local[index] = vm_pop();
        break;
      }
      case OP_INSTANECE: {
        uint8_t class_index = ins[++current_frame()->ip];
        Class c = b.classes[class_index];
        // TODO 確保できるinstance valサイズの拡張
        Value *variables = calloc(sizeof(Value), INSTANCE_VAL_MAX);
        vm_push(INSTANCE_VAL(INSTANCE(&c, class_index, c.instance_val_size, variables)));
        break;
      }
      case OP_CALL_METHOD: {
        uint8_t arg_num = ins[++current_frame()->ip];
        Value val = *(vm.stack_top-arg_num-1);
        Value receiver = *(vm.stack_top-arg_num-2);
        push_frame(new_frame(val.as.function.size, val.as.function.content, arg_num, b.classes[receiver.as.instance.index].constants, true));
        break;
      }
      case OP_LOAD_METHOD: {
        Value receiver = vm_pop();
        if (receiver.type != VAL_INSTANCE) {
          return EXEC_RESULT(ERROR_NO_METHOD, NIL_VAL());
        }
        uint8_t index = ins[++current_frame()->ip];
        Constant constant = find_method(b, receiver.as.instance.index, index);
        vm_push(receiver);
        vm_push(FUNCTION_VAL(constant));
        break;
      }
      case OP_LOAD_INSTANCE_VAL: {
        uint8_t index = ins[++current_frame()->ip];
        Value receiver = *(current_frame()->bp-2);
        vm_push(receiver.as.instance.variables[index]);
        break;
      }
      case OP_STORE_INSTANCE_VAL: {
        uint8_t index = ins[++current_frame()->ip];
        Value receiver = *(current_frame()->bp-2);
        receiver.as.instance.variables[index] = vm_pop();
        break;
      }
      case OP_RETURN: {
        Frame f = pop_frame();
        Value function = vm_pop(); // pop function
        if (function.as.function.method_index == 0 && f.f_method) {
          // constructor
          break;
        }
        if (f.f_method) {
          vm_pop(); // pop receiver
        }
        vm_push(NIL_VAL());
        break;
      }
      default:
        return EXEC_RESULT(ERROR_UNKNOWN_OPCODE, NIL_VAL());
    }
  }
  Value val = vm_pop();
  return EXEC_RESULT(SUCCESS, val);
}

Bytecode parse_bytecode(char* str)
{
  Bytecode bytecode;
  uint8_t* insts = calloc(sizeof(uint8_t), INST_MAX);
  Constant* constants = calloc(sizeof(Constant), CONST_MAX);
  int cnt = 0;
  int pos = 0;

  // u4 skip magic number
  for (int i=0; i<4; i++) {
    cnt += 2;
    pos++;
  }

  // u1 parse class_pool_count
  uint8_t up =  str[cnt++];
  uint8_t low = str[cnt++];
  uint8_t class_pool_size = calc_byte(up, low);

  // parse class pool
  for (int i=0; i<class_pool_size; i++) {
    up =  str[cnt++];
    low = str[cnt++];
    uint8_t instance_val_size = calc_byte(up, low);

    up =  str[cnt++];
    low = str[cnt++];
    uint8_t upper = calc_byte(up, low);
    pos++;
    up =  str[cnt++];
    low = str[cnt++];
    pos++;
    uint8_t lower= calc_byte(up, low);
    uint8_t class_constant_pool_size = decode_constant(upper, lower);
    Constant* class_constants = calloc(sizeof(Constant), CONST_MAX);

    // parse class constant pool
    for (int j=0; j<class_constant_pool_size; j++) {
      up =  str[cnt++];
      low = str[cnt++];
      pos++;
      uint8_t class_const_type = calc_byte(up, low);
      uint8_t class_func_id = 0;
      if (class_const_type == CONST_FUNC) {
        up =  str[cnt++];
        low = str[cnt++];
        pos++;
        class_func_id = calc_byte(up, low);
      }
      up =  str[cnt++];
      low = str[cnt++];
      pos++;
      upper = calc_byte(up, low);
      up =  str[cnt++];
      low = str[cnt++];
      pos++;
      lower = calc_byte(up, low);
      uint16_t class_const_size = decode_constant(upper, lower);
      switch (class_const_type) {
        case CONST_INT: {
          uint8_t *content = calloc(sizeof(uint8_t), 2);
          for (int k=0; k<class_const_size; k++) {
            up =  str[cnt++];
            low = str[cnt++];
            pos++;
            content[k] = calc_byte(up, low);
          }
          class_constants[j].content = content;
          break;
        }
        case CONST_FUNC: {
          uint8_t *content = calloc(sizeof(uint8_t), INST_MAX);
          for (int k=0; k<class_const_size; k++) {
            up =  str[cnt++];
            low = str[cnt++];
            pos++;
            content[k] = calc_byte(up, low);
          }
          class_constants[j].content = content;
          break;
        }
      }
      class_constants[j].type = class_const_type;
      class_constants[j].size = class_const_size;
      class_constants[j].method_index = class_func_id;
    }
    bytecode.classes[i].constant_size = class_constant_pool_size;
    bytecode.classes[i].constants = class_constants;
    bytecode.classes[i].index = i;
    bytecode.classes[i].instance_val_size = instance_val_size;
  }

  // parse constant_pool_count
  up =  str[cnt++];
  low = str[cnt++];
  uint8_t upper = calc_byte(up, low);
  pos++;
  up =  str[cnt++];
  low = str[cnt++];
  pos++;
  uint8_t lower= calc_byte(up, low);
  uint8_t constant_pool_size = decode_constant(upper, lower);

  // parse content
  for (int i=0; i<constant_pool_size; i++) {
    up =  str[cnt++];
    low = str[cnt++];
    pos++;
    uint8_t const_type = calc_byte(up, low);
    if (const_type == CONST_FUNC) {
      // skip function id
      up =  str[cnt++];
      low = str[cnt++];
      pos++;
    }
    up =  str[cnt++];
    low = str[cnt++];
    pos++;
    upper = calc_byte(up, low);
    up =  str[cnt++];
    low = str[cnt++];
    pos++;
    lower = calc_byte(up, low);
    uint16_t const_size = decode_constant(upper, lower);
    switch (const_type) {
      case CONST_INT: {
        uint8_t *content = calloc(sizeof(uint8_t), 2);
        for (int j=0; j<const_size; j++) {
          up =  str[cnt++];
          low = str[cnt++];
          pos++;
          content[j] = calc_byte(up, low);
        }
        constants[i].content = content;
        break;
      }
      case CONST_FUNC: {
        uint8_t *content = calloc(sizeof(uint8_t), INST_MAX);
        for (int j=0; j<const_size; j++) {
          up =  str[cnt++];
          low = str[cnt++];
          pos++;
          content[j] = calc_byte(up, low);
        }
        constants[i].content = content;
        break;
      }
    }
    constants[i].type = const_type;
    constants[i].size = const_size;
  }

  // parse instructions
  up =  str[cnt++];
  low = str[cnt++];
  pos++;
  upper = calc_byte(up, low);
  up =  str[cnt++];
  low = str[cnt++];
  pos++;
  lower = calc_byte(up, low);
  uint16_t inst_size = decode_constant(upper, lower);
  // parse content
  for (int i=0; i<inst_size; i++) {
    uint8_t up =  str[cnt++];
    uint8_t low = str[cnt++];
    pos++;
    insts[i] = calc_byte(up, low);
  }

  bytecode.instruction_size = inst_size;
  bytecode.instructions = insts;
  bytecode.constants = constants;
  bytecode.constant_size = constant_pool_size;
  bytecode.class_size = class_pool_size;
  return bytecode;
}

ExecResult tarto_vm_run(char* input)
{
  Bytecode bytecode = parse_bytecode(input);

  // for debug
  // printf("** instruction**\n");
  // for (int i=0; i<bytecode.instruction_size; i++) {
  //   printf("%d: %d\n", i, bytecode.instructions[i]);
  // }

  ExecResult er = exec_interpret(bytecode);
  free(bytecode.constants);
  free(bytecode.instructions);
  return er;
}
