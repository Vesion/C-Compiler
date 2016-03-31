#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

int token; // current read token
int token_val; // value of current token, mainly for number
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

int *current_id; // current parsed ID
int *symbols; // symbol table
int *idmain; // the 'main' function

// instructions of VM
enum {
    LEA, IMM, JMP, CALL, JZ, JNZ, ENT, ADJ, LEV, LI, LC, SI, SC, PUSH,
    OR, XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD,
    OPEN, READ, CLOS, PRTF, MALC, MSET, MCMP, EXIT
};

// tokens and classes
enum {
    Num = 128, Fun, Sys, Glo, Loc, Id,
    Char, Else, Enum, If, Int, Return, Sizeof, While,
    Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};

// identifier
enum {
    Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize
};

// types of variable/function
enum {
    CHAR, INT, PTR
};

int basetype; // the type of a declaration
int expr_type; // the type of an expression


void next() {
    char *last_pos;
    int hash;

    while (token == *src) {
        ++src;

        if (token == '\n') { ++line; }
        else if (token == '#') {
            while (*src != 0 && *src != '\n') ++src;
            return;
        }
        else if ((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || (token == '_')) {
            // parse identifier
            last_pos = src - 1;
            hash = token;
            while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_')) {
                hash = hash * 147 + *src;
                ++src;
            }
            // check if has been in symbols table, linear search
            current_id = symbols;
            while (current_id[Token]) {
                if (current_id[Hash] == hash && !memcmp((char*)current_id[Name], last_pos, src-last_pos)) {
                    token = current_id[Token];
                    return;
                }
                current_id = current_id + IdSize;
            }
            // store new identifier
            current_id[Name] = (int)last_pos;
            current_id[Hash] = hash;
            token = current_id[Token] = Id;
            return;
        }
        else if (token >= '0' && token <= '9') {
            // parse numbers, decimal/hex/octal
            token_val = token - '0';
            if (token > 0) { // decimal
                while (*src >= '0' && *src <= '9') {
                    token_val = token_val * 10 + *src++ - '0';
                }
            } else { // start with 0
                if (*src == 'x' || *src == 'X') { // hex
                    token = *src++;
                    while ((token >= '0' && token <= '9') || (token >= 'a' && token <= 'f') || (token <= 'A' && token >= 'F')) {
                        token_val = token_val * 16 + (token & 15) + (token >= 'A' ? 9  : 0);
                        token = *src++;
                    }
                } else { // oct
                    while (*src >= '0' && *src <= '7') {
                        token_val = token_val * 8 + *src++ - '0';
                    }
                }
            }
            token = Num;
            return;
        }
        else if (token == '"' || token == '\'') {
            // parse string literal, tinyc only support '\n' escape character
            // store string literal into data segement
            last_pos = data;
            while (*src != 0 && *src != token) {
                token_val = *src++;
                if (token_val == '\\') {
                    token_val = *src++;
                    if (token_val == 'n') token_val = '\n';
                }
                if (token == '"') {
                    *data++ = token_val;
                }
            }
            ++src;
            if (token == '"') {
                token_val = (int)last_pos;
            } else { // if it is a single character, set its token as Num
                token = Num;
            }
            return;
        }
        else if (token == '/') {
            if (*src == '/') { // skip comments, tinyc only support // comments
                while (*src != 0 && *src != '\n') ++src;
            } else {
                token = Div;
                return;
            }
        }
        else if (token == '=') {
            // parse '=' and '=='
            if (*src == '=') {
                ++src;
                token = Eq;
            } else {
                token = Assign;
            }
            return;
        }
        else if (token == '+') {
            // parse '+' and '++'
            if (*src == '+') {
                ++src;
                token = Inc;
            } else {
                token = Add;
            }
            return;
        }
        else if (token == '-') {
            // parse '-' and '--'
            if (*src == '-') {
                ++src;
                token = Dec;
            } else {
                token = Sub;
            }
            return;
        }
        else if (token == '!') {
            // parse '!='
            if (*src == '=') {
                ++src;
                token = Ne;
            }
            return;
        }
        else if (token == '<') {
            // parse '<=', '<<' and '<'
            if (*src == '=') {
                ++src;
                token = Le;
            } else if (*src == '<') {
                ++src;
                token = Shl;
            } else {
                token = Lt;
            }
            return;
        }
        else if (token == '>') {
            // parse '>=', '>>' and '>'
            if (*src == '=') {
                ++src;
                token = Ge;
            } else if (*src == '>') {
                ++src;
                token = Shr;
            } else {
                token = Gt;
            }
            return;
        }
        else if (token == '|') {
            // parse '|' and '||'
            if (*src == '|') {
                ++src;
                token = Lor;
            } else {
                token = Or;
            }
            return;
        }
        else if (token == '&') {
            // parse '&' and '&&'
            if (*src == '&') {
                ++src;
                token = Lan;
            } else {
                token = And;
            }
            return;
        }
        else if (token == '^') {
            token = Xor;
            return;
        }
        else if (token == '%') {
            token = Mod;
            return;
        }
        else if (token == '*') {
            token = Mul;
            return;
        }
        else if (token == '[') {
            token = Brak;
            return;
        }
        else if (token == '?') {
            token = Cond;
            return;
        }
        else if (token == '~' || token == ';' || token == '{' || token == '}' || token == '(' || token == ')' || token == ']' || token == ',' || token == ':') {
            // return character as token directly
            return;
        }
    }
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
    int *tmp;

    argc--; argv++;
    poolsize = 256 * 1024; // arbitrary size
    line = 1;

    // allocate memory for text/data/stack
    if (!(text = old_text = malloc(poolsize))) {
        printf("could not malloc(%d) for code segment area\n", poolsize);
        return -1;
    }
    if (!(data = malloc(poolsize))) {
        printf("could not malloc(%d) for data segment area\n", poolsize);
        return -1;
    }
    if (!(stack = malloc(poolsize))) {
        printf("could not malloc(%d) for stack area\n", poolsize);
        return -1;
    }
    memset(text, 0, poolsize);
    memset(data, 0, poolsize);
    memset(stack, 0, poolsize);

    // init vm registers
    bp = sp = (int*)((int)stack + poolsize); // stack is initially empty, so they point to stack bottom(top)
    ax = 0;

    // add keywords to symbol table
    src = "char else enum if int return sizeof while open read close printf malloc memset memcmp exit void main"; 
    i = Char;
    while (i <= While) {
        next();
        current_id[Token]++;
    }
    i = OPEN;
    while (i <= EXIT) {
        next();
        current_id[Class] = Sys;
        current_id[Type] = INT;
        current_id[Value] = i++;
    }
    next(); current_id[Token] = Char; // handle void type
    next(); idmain = current_id; // keep track of main

    // read source codes
    if ((fd = open(*argv, 0)) < 0) {
        printf("could not open(%s)\n", *argv);
        return -1;
    }
    if (!(src = old_src = malloc(poolsize))) {
        printf("could not malloc(%d) for source area\n", poolsize);
        return -1;
    }
    if ((i = read(fd, src, poolsize-1)) <= 0) {
        printf("read() returned %d\n", i);
        return -1;
    }
    src[i] = 0; // terminate with 0
    close(fd); // ignore possible error

    program();

    if (!(pc = (int*)idmain[Value])) {
        printf("main() not defined\n");
        return -1;
    }

    // setup stack
    sp = (int*)((int)stack + poolsize);
    *--sp = EXIT;
    *--sp = PUSH; tmp = sp;
    *--sp = argc;
    *--sp = (int)argv;
    *--sp = (int)tmp;

    return eval();
}
