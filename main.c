#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Target.h"
#include "llvm-c/TargetMachine.h"

#define ModuleID "module"

#include "lexer.h"

static tokarr_t token = {0};
static size_t tok_index = 0;

typedef struct {
    LLVMContextRef ctx;
    LLVMBuilderRef builder;
    LLVMTypeRef ret;
    LLVMTypeRef fn;
    LLVMValueRef tmp;
    LLVMExecutionEngineRef engine;
    LLVMBasicBlockRef entry;
    char *err;
} llvm_callback_t;

void accept_input(llvm_callback_t *jit);

LLVMValueRef parse_exp(llvm_callback_t *jit);
LLVMValueRef parse_term(llvm_callback_t *jit);
LLVMValueRef parse_val(llvm_callback_t *jit);

void llvm_init(llvm_callback_t *jit);
void llvm_jit_loop(llvm_callback_t *jit);

LLVMValueRef
parse_val(llvm_callback_t *jit)
{
    token_s *ctk = peek(&token, &tok_index);
    LLVMValueRef lhs = NULL;

    if (ctk->type == T_VAL) {
        lhs = LLVMConstInt(jit->ret, ctk->value, 0);
        advance(&tok_index);
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

    token_s *ctx = peek(&token, &tok_index);
    LLVMValueRef rhs = NULL;

    while (ctx && (ctx->type == T_SLASH || ctx->type == T_ASTERIK)) {
        int op = ctx->type;
        advance(&tok_index);
        rhs = parse_val(jit);
        if (!rhs) return NULL;

        if (op != T_ASTERIK) lhs = LLVMBuildSDiv(jit->builder,
lhs,rhs,"divtmp");
        else lhs = LLVMBuildMul(jit->builder, lhs, rhs, "multmp");
        ctx = peek(&token, &tok_index);
    }
    return lhs;
}

LLVMValueRef
parse_exp(llvm_callback_t *jit)
{
    LLVMValueRef lhs = parse_term(jit);
    if (!lhs) return NULL;

    token_s *ctx = peek(&token, &tok_index);
    while (ctx) {
        if (ctx->type == T_PLUS || ctx->type == T_MINUS) {
            int op = ctx->type;
            advance(&tok_index);
            LLVMValueRef rhs = parse_term(jit);
            if (!rhs) return NULL;

            if (op == T_PLUS)
                lhs = LLVMBuildAdd(jit->builder, lhs, rhs, "addtmp");
            else
                lhs = LLVMBuildSub(jit->builder, lhs, rhs, "subtmp");
        } else {
            break;
        }
        ctx = peek(&token, &tok_index);
    }

    return lhs;
}

void
accept_input(llvm_callback_t *jit)
{
    char buf[256];
print:
    printf(">>> ");
    fflush(stdout);

    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        buf[strcspn(buf, "\n")] = 0;
        if (strcmp(buf, "exit") == 0) exit(EXIT_SUCCESS);
        token.count = 0;
        tok_index = 0;
        parse_input((const char *)buf, &token);
        llvm_jit_loop(jit);

        goto print;
    }
}

void
llvm_init(llvm_callback_t *jit)
{
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();

    jit->ctx = LLVMContextCreate();
    jit->builder = LLVMCreateBuilderInContext(jit->ctx);
}


void
llvm_jit_loop(llvm_callback_t *jit)
{
    static int expr_id = 0;
    static bool initialized = false;

    if (token.count == 0) return;

    if (!initialized) {
        llvm_init(jit);
        initialized = true;
    }
    LLVMModuleRef mod = LLVMModuleCreateWithNameInContext("repl", jit->ctx);

    jit->err = NULL;

    jit->ret = LLVMInt32Type();
    jit->fn = LLVMFunctionType(jit->ret, NULL, 0, false);

    char name[32];
    snprintf(name, sizeof(name), "expr%d", expr_id++);
    jit->tmp = LLVMAddFunction(mod, name, jit->fn);

    jit->entry = LLVMAppendBasicBlockInContext(jit->ctx, jit->tmp, "entry");
    LLVMPositionBuilderAtEnd(jit->builder, jit->entry);

    LLVMValueRef final_val = parse_exp(jit);

    if (final_val) {
        if (LLVMCreateExecutionEngineForModule(&jit->engine, mod,
                &jit->err)) {
                fprintf(stderr, "%s\n", jit->err);
            exit(1);
        }
        LLVMValueRef ff = LLVMBuildRet(jit->builder, final_val);

        uint64_t addr = LLVMGetFunctionAddress(jit->engine, name);
        if (!addr) {
            fprintf(stderr, "Failed to get function address for %s\n", name);
            LLVMDeleteFunction(jit->tmp);
            LLVMDisposeExecutionEngine(jit->engine);
            return;
        }

        int (*tmp_func)() = (int (*)())addr;
        printf("Result: %d\n", tmp_func());
        printf("%s\n", LLVMGetDataLayout(mod));
        LLVMDumpModule(mod);
        LLVMDisposeModule(mod);
    }
}

int
main()
{
    static llvm_callback_t jit = {0};
    accept_input(&jit);

    LLVMDisposeExecutionEngine(jit.engine);
    LLVMContextDispose(jit.ctx);
    return 0;
}
