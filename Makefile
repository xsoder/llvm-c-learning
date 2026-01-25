CC := clang.exe

CFLAGS := $(shell ../ccalc/llvm-21.1.8-Release/bin/llvm-config.exe --cflags) -O3
LDLIBS := $(shell ../ccalc/llvm-21.1.8-Release/bin/llvm-config.exe --libs) -ladvapi32 -lntdll

all: main.exe



main.exe: main.c lexer.c
	$(CC) $(CFLAGS) -o main.exe main.c lexer.c $(LDLIBS)

clean:
	del main.exe
