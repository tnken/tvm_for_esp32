#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define CLASS_MAX 10
#define STACK_MAX 256
#define INST_MAX 100
#define CONST_MAX 100
#define GLOBAL_MAX 256
#define LOCAL_MAX 10
#define FRAME_MAX 20
#define INSTANCE_VAL_MAX 10
#define IR_MAX 300
#define NUMBER_VAL(value) ((Value){ VAL_NUMBER, { .number = value } })
#define BOOL_VAL(value) ((Value){ VAL_BOOL, { .boolean = value } })
#define NIL_VAL() ((Value){.type = VAL_NIL})
#define FUNCTION_VAL(value) ((Value){VAL_FUNCTION, { .function = value}})
#define INSTANCE_VAL(value) ((Value){VAL_INSTANCE, { .instance = value}})
#define INSTANCE(value, index, size, variables) ((Instance){value, index, size, variables})
#define EXEC_RESULT(type, value) ((ExecResult){type, value})

struct Value;
struct Instance;
typedef enum {
  OP_CONSTANT,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_DONE,
  OP_EQ,
  OP_NEQ,
  OP_LESS,
  OP_GREATER,
  OP_LOAD_GLOBAL,
  OP_STORE_GLOBAL,
  OP_JNT,
  OP_JMP,
  OP_CALL,
  OP_RETURN_VAL,
  OP_LOAD_LOCAL,
	OP_STORE_LOCAL,
  OP_INSTANECE,
  OP_LOAD_METHOD,
  OP_CALL_METHOD,
  OP_LOAD_INSTANCE_VAL,
  OP_STORE_INSTANCE_VAL,
  OP_RETURN,
} opcode;

typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
  VAL_FUNCTION,
  VAL_INSTANCE,
} valueType;

typedef enum {
  CONST_INT,
  CONST_FUNC,
} constantType;

typedef struct {
  constantType type;
  uint16_t size;
  uint8_t *content;
  uint8_t method_index;
} Constant;

typedef struct {
  uint8_t index;
  uint16_t constant_size;
  Constant *constants;
  uint8_t instance_val_size;
} Class;

typedef struct Instance {
  Class *class;
  uint8_t index;
  uint8_t val_size;
  struct Value *variables;
} Instance;

typedef struct Value {
  valueType type;
  union {
    bool boolean;
    uint16_t number;
    Constant function;
    Instance instance;
  } as;
} Value;

typedef struct {
  uint16_t instruction_size;
  uint8_t *instructions;
  int ip;
  Value local[LOCAL_MAX];
  uint8_t arg_num;
  Value *bp;
  Constant *constants;
  bool f_method;
} Frame;

typedef struct {
  uint8_t class_size;
  Class classes[CLASS_MAX];
  Constant *constants;
  uint16_t constant_size;
  uint8_t *instructions;
  uint16_t instruction_size;
} Bytecode;

struct {
  Value stack[STACK_MAX];
  Value global[GLOBAL_MAX];
  Value *stack_top;
  Frame frames[FRAME_MAX];
  uint8_t frame_index;
} vm;

typedef enum {
  SUCCESS,
  ERROR_DIVISION_BY_ZERO,
  ERROR_UNKNOWN_OPCODE,
  ERROR_NO_METHOD,
  ERROR_OTHER,
} resultType;

typedef struct {
  resultType type;
  Value return_value;
} ExecResult;


ExecResult tarto_vm_run(char*);
