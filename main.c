#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Target.h"
#include "llvm-c/TargetMachine.h"

#define ModuleID "module"

typedef enum {
    T_VAL,
    T_PLUS,
    T_MINUS,
    T_ASTERIK,
    T_SLASH,
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

typedef struct {
    LLVMContextRef ctx;
    LLVMModuleRef mod;
    LLVMBuilderRef builder;
    LLVMTypeRef ret;
    LLVMTypeRef fn;
    LLVMValueRef tmp;
    LLVMBasicBlockRef entry;
    LLVMExecutionEngineRef engine;
    char *err;
} llvm_callback_t;

void accept_input();
void parse_input(const char *line);

void alloc(tokarr_t *t);
void items_append_op(tokarr_t *t, char p);
void items_append_v(tokarr_t *t, int val);

LLVMValueRef parse_exp(llvm_callback_t *jit);
LLVMValueRef parse_term(llvm_callback_t *jit);
LLVMValueRef parse_val(llvm_callback_t *jit);

void llvm_jit_loop();

bool is_digit(char c);
bool is_whitespace(char c);
bool is_op(char c);

Token *peek();
void advance();

static tokarr_t token = {0};
size_t tok_index = 0;

LLVMValueRef
parse_val(llvm_callback_t *jit)
{
    Token *ctk = peek();
    LLVMValueRef lhs = NULL;

    if (ctk->type == T_VAL) {
        lhs = LLVMConstInt(jit->ret, ctk->value, 0);
        advance();
    }else {
        fprintf(stderr, "Expected value\n");
        return NULL;
    }
    return lhs;
}

LLVMValueRef
parse_term(llvm_callback_t *jit)
{
    LLVMValueRef lhs = parse_val(jit);
    if (!lhs) return NULL;

    Token *ctx = peek();
    LLVMValueRef rhs = NULL;

    while (ctx && (ctx->type == T_SLASH || ctx->type == T_ASTERIK)) {
        int op = ctx->type;
        advance();
        rhs = parse_val(jit);
        if (!rhs) return NULL;

        if (op != T_ASTERIK) lhs = LLVMBuildSDiv(jit->builder, lhs,rhs,"divtmp");
        else lhs = LLVMBuildMul(jit->builder, lhs, rhs, "multmp");
        ctx = peek();
    }
    return lhs;
}
LLVMValueRef
parse_exp(llvm_callback_t *jit)
{
    LLVMValueRef lhs = parse_term(jit);
    if (!lhs) return NULL;

    Token *ctx = peek();
    while (ctx) {
        if (ctx->type == T_PLUS || ctx->type == T_MINUS) {
            int op = ctx->type;
            advance();
            LLVMValueRef rhs = parse_term(jit);
            if (!rhs) return NULL;

            if (op == T_PLUS)
                lhs = LLVMBuildAdd(jit->builder, lhs, rhs, "addtmp");
            else
                lhs = LLVMBuildSub(jit->builder, lhs, rhs, "subtmp");
        } else {
            break;
        }
        ctx = peek();
    }

    return lhs;
}


Token*
peek()
{
    if (tok_index < token.count)
        return &token.items[tok_index];
    return NULL;
}

void
advance()
{
    tok_index++;
}

void
alloc(tokarr_t *t)
{
    if (t->capacity == 0) {
        t->capacity = 256;
        t->items = malloc(sizeof(*t->items) * t->capacity);
    }
    if (t->count >= t->capacity) {
        t->capacity *= 2;
        t->items = realloc(t->items, sizeof(*t->items) * t->capacity);
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
        case '*': t->items[t->count].type = T_ASTERIK; break;
        case '/': t->items[t->count].type = T_SLASH; break;
        default: { 
            printf("Unknown token '%c'", p); 
            t->items[t->count].type = T_END; break;
        } break;
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

bool
is_digit(char c)
{
    return c >= '0' && c <= '9';
}

bool
is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool
is_op(char c)
{
   return c == '+' || c == '-' || c == '*' || c == '/';
}

void
accept_input() 
{
    char buf[256];
print:
    printf(">>> ");
    fflush(stdout);

    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        buf[strcspn(buf, "\n")] = 0;
        if (strcmp(buf, "exit") == 0) abort();
        token.count = 0;
        tok_index = 0;
        parse_input((const char *)buf);
        llvm_jit_loop();

        goto print;
    }
}

void
llvm_jit_loop()
{
    static llvm_callback_t jit = {0};
    static bool initialized = false;

    if (token.count == 0) return;

    if (!initialized) {
        LLVMLinkInMCJIT();
        LLVMInitializeNativeTarget();
        LLVMInitializeNativeAsmPrinter();
        LLVMInitializeNativeAsmParser();
        initialized = true;
    }

    jit.err = NULL;

    jit.ctx = LLVMContextCreate();
    jit.mod = LLVMModuleCreateWithNameInContext(ModuleID, jit.ctx);
    jit.builder = LLVMCreateBuilderInContext(jit.ctx);

    jit.ret = LLVMInt32Type();
    jit.fn = LLVMFunctionType(jit.ret, NULL, 0, false);

    jit.tmp = LLVMAddFunction(jit.mod, "tmp", jit.fn);

    jit.entry = LLVMAppendBasicBlockInContext(jit.ctx, jit.tmp, "entry");
    LLVMPositionBuilderAtEnd(jit.builder, jit.entry);

    LLVMValueRef final_val = parse_exp(&jit);

    if (final_val) {
        LLVMBuildRet(jit.builder, final_val);

        if (LLVMCreateExecutionEngineForModule(&jit.engine, jit.mod, &jit.err)
            == 0) {
            int (*tmp_func)() = (int (*)())LLVMGetFunctionAddress(jit.engine,
                                                                  "tmp");
            printf("Result: %d\n", tmp_func());
        fflush(stdout);

        LLVMDisposeExecutionEngine(jit.engine);
            } else {
                fprintf(stderr, "Failed to create execution engine: %s\n",
                        jit.err);
                LLVMDisposeMessage(jit.err);
                LLVMDisposeModule(jit.mod);
            }
    }

    LLVMContextDispose(jit.ctx);
}

int
main()
{
    accept_input();
    return 0;
}
