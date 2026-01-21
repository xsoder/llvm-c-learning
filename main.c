#include <stdio.h>
#include <stdbool.h>
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Target.h"
#include "llvm-c/TargetMachine.h"

#define ModuleID "test_module"

int
main(void)
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

    LLVMValueRef val = LLVMAddFunction(mod, "add", fn);

    LLVMBasicBlockRef entry =
    LLVMAppendBasicBlockInContext(ctx, val, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    LLVMValueRef c1 = LLVMConstInt(ret, 34, 0);
    LLVMValueRef c2 = LLVMConstInt(ret, 35, 0);
    LLVMValueRef sum = LLVMBuildAdd(builder, c1, c2, "sum");
    LLVMBuildRet(builder, sum);

    LLVMDisposeBuilder(builder);

    LLVMExecutionEngineRef engine;
    char *err = NULL;
    if (LLVMCreateExecutionEngineForModule(&engine, mod, &err) != 0) {
        fprintf(stderr, "EE error: %s\n", err);
        LLVMDisposeMessage(err);
        return 1;
    }

    int (*add_func)() = (int (*)())LLVMGetFunctionAddress(engine, "add");

    printf("Result: %d\n", add_func());

    LLVMDisposeExecutionEngine(engine);
    LLVMContextDispose(ctx);
    return 0;
}
