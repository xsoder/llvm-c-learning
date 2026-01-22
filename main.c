#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Target.h"
#include "llvm-c/TargetMachine.h"

#define ModuleID "module"

typedef enum {
    T_VAL = 0,
    T_PLUS,
    T_MINUS,
    T_END
} token_t;

typedef struct {
    int  value;
    int  type;
    char op;
} Token;

typedef struct {
    size_t count;
    size_t capacity;
    Token *items;
} tokarr_t;

void accept_input();
void parse_input(const char *line);


void alloc(tokarr_t *t);
void items_append_op(tokarr_t *t, char p);
void items_append_v(tokarr_t *t, int val);

void llvm_jit();

bool is_digit(char c);
bool is_whitespace(char c);
bool is_op(char c);

static tokarr_t token = {0};

void
alloc(tokarr_t *t)
{
    if (t->capacity == 0) {
        t->capacity = 256;
        t->items = malloc(sizeof(Token) * t->capacity);
    }
    if (t->count >= t->capacity) {
        t->capacity *= 2;
        t->items = realloc(t->items, sizeof(Token) * t->capacity);
    }
}
void
items_append_v(tokarr_t *t, int val)
{
    alloc(t);
    t->items[t->count].value  = val;
    t->items[t->count].type = T_VAL;
    t->count++;
}

void
items_append_op(tokarr_t *t, char p)
{
    alloc(t);
    t->items[t->count].op = p;
    switch(p) {
        case '+': t->items[t->count].type = T_PLUS; break;
        case '-': t->items[t->count].type = T_MINUS; break;
    }
    t->count++;
}

void
parse_input(const char *line)
{
    for (int i = 0; i < strlen(line); i++) {
        if (is_whitespace(line[i])) continue;
        else if (is_op(line[i])) {
            char p = line[i];
            items_append_op(&token, p);
            continue;
        }
        else if (is_digit(line[i])) {
            int val = 0;
            while((i < strlen(line) && is_digit(line[i])))
            {
                val = val * 10 + (line[i] - '0');
                i++;
            }
            i--;
            items_append_v(&token, val);
            continue;
        } 
    }
    fflush(stdout);
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool is_op(char c) {
    return c == '+' || c == '-';
}

void
accept_input()
{
    char buf[256];

print:
    printf(">>> ");
    fflush(stdout);
    token.count = 0;

    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        buf[strcspn(buf, "\n")] = 0;
        if (strcmp(buf, "exit") == 0) abort();
        parse_input((const char *)buf);
        llvm_jit();
        goto print;
    }
}

void
llvm_jit()
{
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();

    LLVMContextRef ctx = LLVMContextCreate();
    LLVMModuleRef mod = LLVMModuleCreateWithNameInContext(ModuleID, ctx);
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(ctx);

    LLVMTypeRef ret = LLVMInt32TypeInContext(ctx);
    LLVMTypeRef fn = LLVMFunctionType(ret, NULL, 0, false);

    LLVMValueRef add = LLVMAddFunction(mod, "add", fn);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(ctx, add, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    LLVMValueRef sum = LLVMConstInt( ret, token.items[0].value, 0 );
    for (size_t i = 1; i + 1 < token.count; i += 2) {
        Token op  = token.items[i];
        Token val = token.items[i + 1];
        
        LLVMValueRef rhs = LLVMConstInt(ret, val.value, 0);
        
        if (op.type == T_PLUS) {
            sum = LLVMBuildAdd(builder, sum, rhs, "addtmp");
        } else if (op.type == T_MINUS) {
            sum = LLVMBuildSub(builder, sum, rhs, "subtmp");
        }
    }

    LLVMBuildRet(builder, sum);

    LLVMDisposeBuilder(builder);

    LLVMExecutionEngineRef engine;
    char *err = NULL;
    if (LLVMCreateExecutionEngineForModule(&engine, mod, &err) != 0) {
        fprintf(stderr, "EE error: %s\n", err);
        LLVMDisposeMessage(err);
        return;
    }

    int (*add_func)() = (int (*)())LLVMGetFunctionAddress(engine, "add");
    printf("Result: %d\n", add_func());

    LLVMDisposeExecutionEngine(engine);
    LLVMContextDispose(ctx);
}

int
main()
{
    accept_input();
    return 0;
}
