#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "opcode.h"

#define STACK_MAX 256
#define GLOBAL_MAX 256
#define NUMBER_VAL(value) ((Value){ VAL_NUMBER, { .number = value } })
#define BOOL_VAL(value) ((Value){ VAL_BOOL, { .boolean = value } })
#define NIL_VAL() ((Value){.type = VAL_NIL})
#define EXEC_RESULT(type, value) ((ExecResult){type, value})

typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
} valueType;

typedef struct {
  valueType type;
  union {
    bool boolean;
    uint16_t number;
  } as;
} Value;

struct {
  uint8_t *ip;
  uint8_t *bp;
  Value stack[STACK_MAX];
  Value global[GLOBAL_MAX];
  Value *stack_top;
} vm;

typedef enum {
  SUCCESS,
  ERROR_DIVISION_BY_ZERO,
  ERROR_UNKNOWN_OPCODE,
} resultType;

typedef struct {
  resultType type;
  Value return_value;
} ExecResult;

ExecResult tarto_vm_run(char*);
