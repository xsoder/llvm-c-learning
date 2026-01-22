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
    Token *tk;
} tokarr_t;

void accept_input();
void parse_input(const char *line);

void tk_append(Token *t, int val);

bool is_digit(char c);
bool is_whitespace(char c);
bool is_op(char c);

static tokarr_t token = {0};
static int token_count = 0;

void
t_alloc(tokarr_t *t)
{
    if (t->capacity == 0) {
        t->capacity = 256;
        t->tk = (Token *)malloc(sizeof(t->tk) * t->capacity);
    }
    if (t->count >= t->capacity) {
        t->capacity *= 2;
        t->tk = (Token *)realloc(t->tk, sizeof(t->tk) * t->capacity);
    }
}

void
tk_append_v(tokarr_t *t, int val)
{
    t_alloc(t);
    t->tk[t->count].value  = val;
    t->tk[t->count].type = T_VAL;
    t->count++;
}


void
tk_append_op(tokarr_t *t, char p)
{
    t_alloc(t);
    t->tk[t->count].op = p;
    switch(p) {
    case '+': t->tk[t->count].type = T_PLUS; break;
    case '-': t->tk[t->count].type = T_MINUS; break;
    }
    t->count++;
}

void
parse_input(const char *line)
{
    for (int i = 0; i < strlen(line); i++) {
        if (is_whitespace(line[i])) continue;
        else if (is_digit(line[i])) {
            int val = 0;
            while((i < strlen(line) && is_digit(line[i])))
            {
                val = val * 10 + (line[i] - '0');
                i++;
            }
            i--;
            tk_append_v(&token, val);
            continue;
        } 
        else if (is_op(line[i])) {
            char p = line[i];
            tk_append_op(&token, p);
            continue;
        }
        printf("%c", line[i]);
    }
    printf("\n");
    fflush(stdout);
    for(size_t i = 0; i < token.count; ++i) {
        if (token.tk[i].type == T_VAL) printf("TOKEN VALUE: %d\n", token.tk[i].value);
        else printf("TOKEN OP: %c\n", token.tk[i].op);
    }
    printf("\n");
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

    for(size_t i = 0; i < token.count; ++i) {
        if (token.tk[i].type == T_VAL) printf("TOKEN VALUE: %d\n", token.tk[i].value); // da_append(c, token.tk[i].value)
        else continue;
    }

    LLVMValueRef c1  = LLVMConstInt(ret, 42, 0);
    LLVMValueRef c2  = LLVMConstInt(ret, 429, 0);
    LLVMValueRef sum = LLVMBuildAdd(builder, c1, c2, "sum");
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
