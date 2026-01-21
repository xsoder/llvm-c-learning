CC := clang.exe

CFLAGS := $(shell ../ccalc/llvm-21.1.8-Release/bin/llvm-config.exe --cflags)
LDLIBS := $(shell ../ccalc/llvm-21.1.8-Release/bin/llvm-config.exe --libs) -ladvapi32 -lntdll

all: main.exe

main.exe: main.c
	$(CC) $(CFLAGS) -o main.exe main.c $(LDLIBS)

clean:
	del main.exe
