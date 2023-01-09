#include "vm.h"
#include "common.h"
#include "compiler.h"
#include "debug.h"

#include <stdarg.h>
#include <stdio.h>

VirtualMachine vm;

void push(Value value) {
  *vm.stack_top = value;
  vm.stack_top++;
}

Value pop() {
  vm.stack_top--;
  return *vm.stack_top;
}

static Value peek(int distance) {
  return vm.stack_top[-1 - distance];
}

static void reset_stack() {
  vm.stack_top = vm.stack;
  return;
}

static void runtime_error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  size_t instruction = vm.ip - vm.chunk->code - 1;
  int line = vm.chunk->lines[instruction];
  fprintf(stderr, "[line %d] in script\n", line);
  reset_stack();
}

static InterpretResult run() {
  // we use macro instead of functions to avoid jumping
  // through and manipulating the stack, making the program
  // fast
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

  // See
  // https://craftinginterpreters.com/a-virtual-machine.html#binary-operators
  // for the macro trick explanation
#define BINARY_OP(valueType, op)                                                \
  do {                                                                          \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                           \
      runtime_error("Operands must be numbers.");                               \
      return INTERPRET_RUNTIME_ERROR;                                           \
    }                                                                           \
    double b = AS_NUMBER(pop());                                                \
    double a = AS_NUMBER(pop());                                                \
    push(valueType(a op b));                                                    \
  } while (false)

  // Main Loop
  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
    printf("          ");
    for (Value *slot = vm.stack; slot < vm.stack_top; slot++) {
      printf("[ ");
      print_value(*slot);
      printf(" ]");
    }
    printf("\n");
#endif

    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT: {
        Value constant = READ_CONSTANT();
        push(constant);
        break;
      }
      case OP_TRUE: {
        push(BOOL_VAL(true));
        break;
      }
      case OP_FALSE: {
        push(BOOL_VAL(false));
        break;
      }
      case OP_NIL: {
        push(NIL_VAL);
        break;
      }
      case OP_ADD: {
        BINARY_OP(NUMBER_VAL, +);
        break;
      }
      case OP_SUBTRACT: {
        BINARY_OP(NUMBER_VAL, -);
        break;
      }
      case OP_MULTIPLY: {
        BINARY_OP(NUMBER_VAL, *);
        break;
      }
      case OP_DIVIDE: {
        BINARY_OP(NUMBER_VAL, /);
        break;
      }
      case OP_NEGATE: {
        if (!IS_NUMBER(peek(0))) {
          runtime_error("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }

        push(NUMBER_VAL(-AS_NUMBER(pop())));
        break;
      }
      case OP_RETURN: {
        return INTERPRET_OK;
      }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

void init_vm() {
  reset_stack();
  return;
}

void free_vm() {}

InterpretResult interpret(const char *source) {
  Chunk chunk;
  init_chunk(&chunk);

  if (!compile(source, &chunk)) {
    free_chunk(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;

  InterpretResult result = run();
  free_chunk(&chunk);

  return result;
}