#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

int token; // current read token
char *src,  // pointer to source code
     *old_src; // for dump
int poolsize; // default size of some segments
int line; // line number of source code
int *text, // text/code segment
    *old_text; // for dump
char *data; // data segment
int *stack; // stack
int *pc, // program counter register, store the next instruction
    *sp, // stack pointer register, maintain the address of stack top
    *bp, // base pointer register, for function call
    ax, // general register, store the result of one instruction
    cycle; // counter of instructions

/* instructions our virtual machine support */
enum {
    LEA, IMM, JMP, CALL, JZ, JNZ, ENT, ADJ, LEV, LI, LC, SI, SC, PUSH,
    OR, XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD,
    OPEN, READ, CLOS, PRTF, MALC, MSET, MCMP, EXIT
};

void next() {
    token = *src++;
    return;
}

void expression(int level) {
    return;
}

void program() {
    next();
    while (token > 0) {
        printf("token is : %c\n", token);
        next();
    }
}

int eval() {
    int op, *tmp;
    cycle = 0;
    while (1) {
        cycle ++;
        op = *pc++; // get next operation code

        if (op == IMM)       { ax = *pc++; }
        else if (op == LC)   { ax = *(char *)ax; }
        else if (op == LI)   { ax = *(int *)ax; }
        else if (op == SC)   { ax = *(char *)*sp++ = ax; }
        else if (op == SI)   { *(int *)*sp++ = ax; }
        else if (op == PUSH) { *--sp = ax; }
        else if (op == JMP)  { pc = (int *)*pc; }
        else if (op == JZ)   { pc = ax ? pc + 1 : (int *)*pc; }
        else if (op == JNZ)  { pc = ax ? (int *)*pc : pc + 1; }
        else if (op == CALL) { *--sp = (int)(pc+1); pc = (int *)*pc; }
        else if (op == ENT)  { *--sp = (int)bp; bp = sp; sp = sp - *pc++; }
        else if (op == ADJ)  { sp = sp + *pc++; }
        else if (op == LEV)  { sp = bp; bp = (int *)*sp++; pc = (int *)*sp++; }
        else if (op == LEA)  { ax = (int)(bp + *pc++); }

        else if (op == OR)  ax = *sp++ | ax;
        else if (op == XOR) ax = *sp++ ^ ax;
        else if (op == AND) ax = *sp++ & ax;
        else if (op == EQ)  ax = *sp++ == ax;
        else if (op == NE)  ax = *sp++ != ax;
        else if (op == LT)  ax = *sp++ < ax;
        else if (op == LE)  ax = *sp++ <= ax;
        else if (op == GT)  ax = *sp++ >  ax;
        else if (op == GE)  ax = *sp++ >= ax;
        else if (op == SHL) ax = *sp++ << ax;
        else if (op == SHR) ax = *sp++ >> ax;
        else if (op == ADD) ax = *sp++ + ax;
        else if (op == SUB) ax = *sp++ - ax;
        else if (op == MUL) ax = *sp++ * ax;
        else if (op == DIV) ax = *sp++ / ax;
        else if (op == MOD) ax = *sp++ % ax;

        else if (op == EXIT) { printf("exit(%d)", *sp); return *sp; }
        else if (op == OPEN) { ax = open((char *)sp[1], sp[0]); }
        else if (op == CLOS) { ax = close(*sp); }
        else if (op == READ) { ax = read(sp[2], (char *)sp[1], *sp); }
        else if (op == PRTF) { tmp = sp + pc[1]; ax = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]); }
        else if (op == MALC) { ax = (int)malloc(*sp); }
        else if (op == MSET) { ax = (int)memset((char *)sp[2], sp[1], *sp); }
        else if (op == MCMP) { ax = memcmp((char *)sp[2], (char *)sp[1], *sp); }

        else { printf("unknown instruction:%d\n", op); return -1; }
    }
}

int main(int argc, char **argv) {
    int fd; // file descriptor
    int i;

    argc--; argv++;
    poolsize = 256 * 1024; // we apply for 256k bytes
    line = 1;

    // read source codes
    if ((fd = open(*argv, 0)) < 0) {
        printf("could not open(%s)\n", *argv);
        return -1;
    }
    if (!(src = old_src = (char*)malloc(poolsize))) {
        printf("could not malloc(%d) for source area\n", poolsize);
        return -1;
    }
    if ((i = read(fd, src, poolsize-1)) <= 0) {
        printf("read() returned %d\n", i);
        return -1;
    }
    src[i] = 0; // terminate with 0
    close(fd); // ignore possible error

    // allocate memory for text/data/stack
    if (!(text = old_text = (int*)malloc(poolsize))) {
        printf("could not malloc(%d) for code segment area\n", poolsize);
        return -1;
    }
    if (!(data = (char*)malloc(poolsize))) {
        printf("could not malloc(%d) for data segment area\n", poolsize);
        return -1;
    }
    if (!(stack = (int*)malloc(poolsize))) {
        printf("could not malloc(%d) for stack area\n", poolsize);
        return -1;
    }
    memset(text, 0, poolsize);
    memset(data, 0, poolsize);
    memset(stack, 0, poolsize);

    // init vm registers
    bp = sp = (int*)((int)stack + poolsize); // stack is initially empty, so they point to stack bottom(top)
    ax = 0;

    program();
    return eval();
}
