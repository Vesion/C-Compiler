#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>


int fd,       // file descriptor
    poolsz,   // default size we allocate memory
    *idmain;  // trace `main` function

char *p;      // read source code
char *data;   // global and static data segment
int *e;       // emitted code segment

int *id,      // currently parsed identifier
    *sym,     // symbol table
    tk,       // current token
    ival,     // current token value
    ty,       // current expression type
    loc;      // local variable offset

int *pc, *sp, *bp, a, // vm registers
    cycle; // instructions counter

// print and debug
int line, *le, src, debug;
char *lp, *ins;

// tokens and classes (operators last and in precedence order)
enum {
    Num = 128, Fun, Sys, Glo, Loc, Id,
    Char, Else, Enum, If, Int, Return, Sizeof, While,
    Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};

// instructions
enum { LEA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,
    OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
    OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT };

// types
enum { CHAR, INT, PTR };

// identifier fields
// we do not support struct, so use memory block (table), Idsz is the size of table
// Tk:    identifier(Id), keywords(char, else, enum...)
// Hash:  *calculate with Tk*
// Name:  identifier's name
// Class: number, function, system call, global, local
// Type:  char, int, pointer
// Val:   identifier's value
enum { Tk, Hash, Name, Class, Type, Val, HClass, HType, HVal, Idsz };


void next()
{
    char *pp;

    while ((tk = *p)) {
        ++p;
        if (tk == '\n') {
            if (src) {
                printf("%d: %.*s", line, (int)(p - lp), lp); lp = p; // print source
                while (le < e) { // print IR
                    printf("%8.4s", &ins[*++le * 5]);
                    if (*le <= ADJ) printf(" %d\n", *++le); else printf("\n"); // instructions befoore ADJ have one operand, but here prints not-yet-assigned operand
                }
            }
            ++line;
        }
        else if (tk == '#') { // ignore all includes and macros
            while (*p != 0 && *p != '\n') ++p;
        }
        else if ((tk >= 'a' && tk <= 'z') || (tk >= 'A' && tk <= 'Z') || tk == '_') { // identifier
            pp = p - 1; // pp point to the first character
            while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_')
                tk = tk * 147 + *p++;
            tk = (tk << 6) + (p - pp); // calculate hash
            id = sym;
            while (id[Tk]) { // scan the whole symbol table to check if it has appeared
                if (tk == id[Hash] && !memcmp((char *)id[Name], pp, p - pp)) { tk = id[Tk]; return; } // if found, use it
                id = id + Idsz;
            }
            // this is a new one
            id[Name] = (int)pp; // its name is address of the first character
            id[Hash] = tk;
            tk = id[Tk] = Id; // it is a identifier, keywords will be reassigned
            return;
        }
        else if (tk >= '0' && tk <= '9') { // number literal
            if ((ival = tk - '0')) { while (*p >= '0' && *p <= '9') ival = ival * 10 + *p++ - '0'; } // decimal
            else if (*p == 'x' || *p == 'X') { // begin with 0x, hex
                while ((tk = *++p) && ((tk >= '0' && tk <= '9') || (tk >= 'a' && tk <= 'f') || (tk >= 'A' && tk <= 'F')))
                    ival = ival * 16 + (tk & 15) + (tk >= 'A' ? 9 : 0); // ascii trick
            }
            else { while (*p >= '0' && *p <= '7') ival = ival * 8 + *p++ - '0'; } // begin with 0, oct
            tk = Num;
            return;
        }
        else if (tk == '/') {
            if (*p == '/') { // begin with //, comment
                ++p;
                while (*p != 0 && *p != '\n') ++p;
            }
            else { // division sign
                tk = Div;
                return;
            }
        }
        else if (tk == '\'' || tk == '"') { // begin with ' or "
            pp = data;
            while (*p != 0 && *p != tk) { // read until close quote
                if ((ival = *p++) == '\\') {
                    if ((ival = *p++) == 'n') ival = '\n'; // escape \n
                }
                if (tk == '"') *data++ = ival; // string literal, copy into data character by character
            }
            ++p;
            if (tk == '"') ival = (int)pp; // string literal, ival is address of the first character
            else tk = Num; // charater literal, treat as number
            return;
        }
        else if (tk == '=') { if (*p == '=') { ++p; tk = Eq; } else tk = Assign; return; } // equal, assign
        else if (tk == '+') { if (*p == '+') { ++p; tk = Inc; } else tk = Add; return; } // add, increase
        else if (tk == '-') { if (*p == '-') { ++p; tk = Dec; } else tk = Sub; return; } // subtract, decrease
        else if (tk == '!') { if (*p == '=') { ++p; tk = Ne; } return; } // not equal
        else if (tk == '<') { if (*p == '=') { ++p; tk = Le; } else if (*p == '<') { ++p; tk = Shl; } else tk = Lt; return; } // less, less or equal, left shift
        else if (tk == '>') { if (*p == '=') { ++p; tk = Ge; } else if (*p == '>') { ++p; tk = Shr; } else tk = Gt; return; } // greater, greater or equal, right shift
        else if (tk == '|') { if (*p == '|') { ++p; tk = Lor; } else tk = Or; return; } // logical or, or
        else if (tk == '&') { if (*p == '&') { ++p; tk = Lan; } else tk = And; return; } // logical and, and
        else if (tk == '^') { tk = Xor; return; } // xor
        else if (tk == '%') { tk = Mod; return; } // mod
        else if (tk == '*') { tk = Mul; return; } // multiply
        else if (tk == '[') { tk = Brak; return; } // subscript
        else if (tk == '?') { tk = Cond; return; } // ternary condition expression (? :)
        else if (tk == '~' || tk == ';' || tk == '{' || tk == '}' || tk == '(' || tk == ')' || tk == ']' || tk == ',' || tk == ':') return; // treat these as themselves
    }
}


void expr(int lev) // operator precedence
{
    int t, *d;

    if (!tk) { printf("%d: unexpected eof in expression\n", line); exit(-1); }
    else if (tk == Num) { *++e = IMM; *++e = ival; next(); ty = INT; } // integer literal
    else if (tk == '"') { // string literal
        *++e = IMM; *++e = ival; next();
        while (tk == '"') next(); // multi-lines C style string
        data = (char *)((int)data + sizeof(int) & -sizeof(int)); // memory alignment with 4 bytes, like #program pack(4)
        ty = PTR;
    }
    else if (tk == Sizeof) { // sizeof(char|int|pointer)
        next(); if (tk == '(') next(); else { printf("%d: open paren expected in sizeof\n", line); exit(-1); }
        ty = INT; if (tk == Int) next(); else if (tk == Char) { next(); ty = CHAR; }
        while (tk == Mul) { next(); ty = ty + PTR; }
        if (tk == ')') next(); else { printf("%d: close paren expected in sizeof\n", line); exit(-1); }
        *++e = IMM; *++e = (ty == CHAR) ? sizeof(char) : sizeof(int);
        ty = INT;
    }
    else if (tk == Id) { // identifier
        d = id; next(); // id will advance in next(), so backup with d
        if (tk == '(') { // function dispatch
            next();
            t = 0; // number of arguments
            while (tk != ')') { expr(Assign); *++e = PSH; ++t; if (tk == ',') next(); } // evaluate each arguments, push them
            next();
            if (d[Class] == Sys) *++e = d[Val]; // system call, like printf, memset
            else if (d[Class] == Fun) { *++e = JSR; *++e = d[Val]; } // user defined function, jump into it, id[Val] is its first instruction
            else { printf("%d: bad function call\n", line); exit(-1); }
            if (t) { *++e = ADJ; *++e = t; } // leave function, need to adjust stack
            ty = d[Type];
        }
        else if (d[Class] == Num) { *++e = IMM; *++e = d[Val]; ty = INT; } // enum, only enum's Class is Num
        else { // variable
            if (d[Class] == Loc) { *++e = LEA; *++e = loc - d[Val]; } // load local address, its address is id[Val]-loc
            else if (d[Class] == Glo) { *++e = IMM; *++e = d[Val]; } // load global address, its address is id[Val]
            else { printf("%d: undefined variable\n", line); exit(-1); }
            *++e = ((ty = d[Type]) == CHAR) ? LC : LI; // load value from address, rvalue
        }
    }
    else if (tk == '(') {
        next();
        if (tk == Int || tk == Char) { // explicit type cast
            t = (tk == Int) ? INT : CHAR; next();
            while (tk == Mul) { next(); t = t + PTR; }
            if (tk == ')') next(); else { printf("%d: bad cast\n", line); exit(-1); }
            expr(Inc); // expr in () is high precedence, same below
            ty = t;
        }
        else { // (expr)
            expr(Assign);
            if (tk == ')') next(); else { printf("%d: close paren expected\n", line); exit(-1); }
        }
    }
    else if (tk == Mul) { // dereference
        next(); expr(Inc);
        if (ty > INT) ty = ty - PTR; else { printf("%d: bad dereference\n", line); exit(-1); }
        *++e = (ty == CHAR) ? LC : LI;
    }
    else if (tk == And) { // get address
        next(); expr(Inc);
        if (*e == LC || *e == LI) --e;  // according to dereference, remove LC/LI
        else { printf("%d: bad address-of\n", line); exit(-1); }
        ty = ty + PTR;
    }
    else if (tk == '!') { next(); expr(Inc); *++e = PSH; *++e = IMM; *++e = 0; *++e = EQ; ty = INT; } // !expr --> expr == 0
    else if (tk == '~') { next(); expr(Inc); *++e = PSH; *++e = IMM; *++e = -1; *++e = XOR; ty = INT; } // ~expr --> expr ^ -1
    else if (tk == Add) { next(); expr(Inc); ty = INT; } // +expr
    else if (tk == Sub) { // -expr --> 0-expr
        next(); *++e = IMM;
        if (tk == Num) { *++e = -ival; next(); } else { *++e = -1; *++e = PSH; expr(Inc); *++e = MUL; }
        ty = INT;
    }
    else if (tk == Inc || tk == Dec) { // ++expr, --expr
        t = tk; next(); expr(Inc);
        if (*e == LC) { *e = PSH; *++e = LC; } else if (*e == LI) { *e = PSH; *++e = LI; } // push address
        else { printf("%d: bad lvalue in pre-increment\n", line); exit(-1); }
        *++e = PSH; // push value
        *++e = IMM; *++e = (ty > PTR) ? sizeof(int) : sizeof(char); // pointer type, increase/decrease address
        *++e = (t == Inc) ? ADD : SUB;
        *++e = (ty == CHAR) ? SC : SI; // store value
    }
    else { printf("%d: bad expression\n", line); exit(-1); }

    while (tk >= lev) { // "precedence climbing" or "Top Down Operator Precedence" method
        t = ty; // backup
        if (tk == Assign) { // expr = expr
            next();
            // a big trick here:
            // source: 
            //      x = 1;
            // IR:
            //      LEA LI              // when read x, treat as rvalue
            //      LEA PSH             // when read =, treat as lvalue, erase LI, replace with PSH
            //      LEA PSH IMM(1) SI   // store 1 into x
            if (*e == LC || *e == LI) *e = PSH;
            else { printf("%d: bad lvalue in assignment\n", line); exit(-1); }
            expr(Assign); *++e = ((ty = t) == CHAR) ? SC : SI; // evaluate rvalue and assign lvalue
        }
        else if (tk == Cond) { // a?:b:c, similar to if-else
            next();
            *++e = BZ; d = ++e;
            expr(Assign);
            if (tk == ':') next(); else { printf("%d: conditional missing colon\n", line); exit(-1); }
            *d = (int)(e + 3); *++e = JMP; d = ++e;
            expr(Cond);
            *d = (int)(e + 1);
        }
        else if (tk == Lor) { next(); *++e = BNZ; d = ++e; expr(Lan); *d = (int)(e + 1); ty = INT; } // ||
        else if (tk == Lan) { next(); *++e = BZ;  d = ++e; expr(Or);  *d = (int)(e + 1); ty = INT; } // &&
        else if (tk == Or)  { next(); *++e = PSH; expr(Xor); *++e = OR;  ty = INT; } // |
        else if (tk == Xor) { next(); *++e = PSH; expr(And); *++e = XOR; ty = INT; } // ^
        else if (tk == And) { next(); *++e = PSH; expr(Eq);  *++e = AND; ty = INT; } // &
        else if (tk == Eq)  { next(); *++e = PSH; expr(Lt);  *++e = EQ;  ty = INT; } // ==
        else if (tk == Ne)  { next(); *++e = PSH; expr(Lt);  *++e = NE;  ty = INT; } // !=
        else if (tk == Lt)  { next(); *++e = PSH; expr(Shl); *++e = LT;  ty = INT; } // <
        else if (tk == Gt)  { next(); *++e = PSH; expr(Shl); *++e = GT;  ty = INT; } // >
        else if (tk == Le)  { next(); *++e = PSH; expr(Shl); *++e = LE;  ty = INT; } // <=
        else if (tk == Ge)  { next(); *++e = PSH; expr(Shl); *++e = GE;  ty = INT; } // >=
        else if (tk == Shl) { next(); *++e = PSH; expr(Add); *++e = SHL; ty = INT; } // <<
        else if (tk == Shr) { next(); *++e = PSH; expr(Add); *++e = SHR; ty = INT; } // >>
        else if (tk == Add) { // a+b
            next(); *++e = PSH; expr(Mul);
            if ((ty = t) > PTR) { *++e = PSH; *++e = IMM; *++e = sizeof(int); *++e = MUL;  }
            *++e = ADD;
        }
        else if (tk == Sub) { // a-b
            next(); *++e = PSH; expr(Mul);
            if (t > PTR && t == ty) { *++e = SUB; *++e = PSH; *++e = IMM; *++e = sizeof(int); *++e = DIV; ty = INT; }
            else if ((ty = t) > PTR) { *++e = PSH; *++e = IMM; *++e = sizeof(int); *++e = MUL; *++e = SUB; }
            else *++e = SUB;
        }
        else if (tk == Mul) { next(); *++e = PSH; expr(Inc); *++e = MUL; ty = INT; } // a * b
        else if (tk == Div) { next(); *++e = PSH; expr(Inc); *++e = DIV; ty = INT; } // a / b
        else if (tk == Mod) { next(); *++e = PSH; expr(Inc); *++e = MOD; ty = INT; } // a % b
        else if (tk == Inc || tk == Dec) { // expr++/expr--
            if (*e == LC) { *e = PSH; *++e = LC; }
            else if (*e == LI) { *e = PSH; *++e = LI; }
            else { printf("%d: bad lvalue in post-increment\n", line); exit(-1); }
            *++e = PSH; *++e = IMM; *++e = (ty > PTR) ? sizeof(int) : sizeof(char);
            *++e = (tk == Inc) ? ADD : SUB;
            *++e = (ty == CHAR) ? SC : SI;
            *++e = PSH; *++e = IMM; *++e = (ty > PTR) ? sizeof(int) : sizeof(char);
            *++e = (tk == Inc) ? SUB : ADD;
            next();
        }
        else if (tk == Brak) { // a[]
            next(); *++e = PSH; expr(Assign);
            if (tk == ']') next(); else { printf("%d: close bracket expected\n", line); exit(-1); }
            if (t > PTR) { *++e = PSH; *++e = IMM; *++e = sizeof(int); *++e = MUL;  }
            else if (t < PTR) { printf("%d: pointer type expected\n", line); exit(-1); }
            *++e = ADD;
            *++e = ((ty = t - PTR) == CHAR) ? LC : LI;
        }
        else { printf("%d: compiler error tk=%d\n", line, tk); exit(-1); }
    }
}


void stmt()
{
    // 6 kinds of statements:
    // 1. if (expr) <stmt> [else <stmt>]
    // 2. while (expr) <stmt>
    // 3. return expr;
    // 4. { <stmt> }
    // 5. <empty stmt>;
    // 6. expr;

    int *a, *b;
    if (tk == If) {
        //   if (expr)             expr(Assgin)
        //                       BZ  next or X
        //      { <stmt> }       stmt()
        //   else                  JMP Y
        // X:   
        //      { <stmt> }       stmt()
        // Y:
        //
        next();
        if (tk == '(') next(); else { printf("%d: open paren expected\n", line); exit(-1); }
        expr(Assign);
        if (tk == ')') next(); else { printf("%d: close paren expected\n", line); exit(-1); }
        *++e = BZ;  // if true (a != 0), branch pc+1, else (a == 0), branch *pc
        b = ++e;
        stmt(); // true branch
        if (tk == Else) {
            *b = (int)(e + 3); // e+3 is the first address in else block
            *++e = JMP; // after true branch finished, jump out of if-else unconditionally
            b = ++e;
            next(); stmt(); // false branch
        }
        *b = (int)(e + 1);
        // e: BZ  X  <stmt>  JMP  Y  <X:stmt>  <Y:...>
    }
    else if (tk == While) {
        // X:                     
        //   while (expr)       expr(Assign) 
        //                      BZ Y
        //   <statement>        stmt()
        //                      JMP X
        // Y:                    
        //
        next();
        a = e + 1;
        if (tk == '(') next(); else { printf("%d: open paren expected\n", line); exit(-1); }
        expr(Assign);
        if (tk == ')') next(); else { printf("%d: close paren expected\n", line); exit(-1); }
        *++e = BZ; b = ++e;
        stmt();
        *++e = JMP; *++e = (int)a;
        *b = (int)(e + 1);
    }
    else if (tk == Return) {
        next();
        if (tk != ';') expr(Assign);
        *++e = LEV;
        if (tk == ';') next(); else { printf("%d: semicolon expected\n", line); exit(-1); }
    }
    else if (tk == '{') { // { <stmt> }
        next();
        while (tk != '}') stmt();
        next();
    }
    else if (tk == ';') { // ;
        next();
    }
    else { // expression;
        expr(Assign);
        if (tk == ';') next(); else { printf("%d: semicolon expected\n", line); exit(-1); }
    }
}


int parse() {
    int bt, ty, i;
    line = 1;
    next();
    while (tk) {
        bt = INT; // base type, for pointer type
        if (tk == Int) next();
        else if (tk == Char) { next(); bt = CHAR; }
        
        else if (tk == Enum) { // enum definition
            next();
            if (tk != '{') next(); // ignore enum name
            if (tk == '{') {
                next();
                i = 0; // enum starts from default 0
                while (tk != '}') {
                    if (tk != Id) { printf("%d: bad enum identifier %d\n", line, tk); exit(-1); }
                    next();
                    if (tk == Assign) { // if assign manually
                        next();
                        if (tk != Num) { printf("%d: bad enum initializer\n", line); exit(-1); }
                        i = ival;
                        next();
                    }
                    id[Class] = Num; id[Type] = INT; id[Val] = i++; // enum is integer, enumerate its value
                    if (tk == ',') next();
                }
                next();
            }
        }
        while (tk != ';' && tk != '}') {
            ty = bt; // real type
            while (tk == Mul) { next(); ty = ty + PTR; } // pointer type
            if (tk != Id) { printf("%d: bad global declaration\n", line); exit(-1); } // a identifier must follow a type name, like: int a;
            if (id[Class]) { printf("%d: duplicate global definition\n", line); exit(-1); } // check symbol table in case duplicates
            next();
            id[Type] = ty;

            if (tk == '(') { // function definition
                id[Class] = Fun;
                id[Val] = (int)(e + 1); // function's val is body's first instruction
                next(); i = 0;
                while (tk != ')') { // parameters
                    ty = INT;
                    if (tk == Int) next();
                    else if (tk == Char) { next(); ty = CHAR; }
                    while (tk == Mul) { next(); ty = ty + PTR; }
                    if (tk != Id) { printf("%d: bad parameter declaration\n", line); exit(-1); }
                    if (id[Class] == Loc) { printf("%d: duplicate parameter definition\n", line); exit(-1); }
                    // parameters are also locals
                    id[HClass] = id[Class]; id[Class] = Loc;
                    id[HType]  = id[Type];  id[Type] = ty;
                    id[HVal]   = id[Val];   id[Val] = i++; // local offset
                    next();
                    if (tk == ',') next();
                }
                next();
                if (tk != '{') { printf("%d: bad function definition\n", line); exit(-1); }
                loc = ++i; // for count inner variables, more details in vm.md
                next();
                while (tk == Int || tk == Char) { // inner variables
                    bt = (tk == Int) ? INT : CHAR;
                    next();
                    while (tk != ';') {
                        ty = bt;
                        while (tk == Mul) { next(); ty = ty + PTR; }
                        if (tk != Id) { printf("%d: bad local declaration\n", line); exit(-1); }
                        if (id[Class] == Loc) { printf("%d: duplicate local definition\n", line); exit(-1); }
                        // enter local context, backup
                        id[HClass] = id[Class]; id[Class] = Loc;
                        id[HType]  = id[Type];  id[Type] = ty;
                        id[HVal]   = id[Val];   id[Val] = ++i; // local offset
                        next();
                        if (tk == ',') next();
                    }
                    next();
                }
                *++e = ENT; // first instruction is entering body
                *++e = i - loc; // amount of all locals, for sp extension
                while (tk != '}') stmt(); // function body
                *++e = LEV; // in case of no explicit `return` in body
                id = sym; // unwind symbol table locals
                while (id[Tk]) {
                    if (id[Class] == Loc) { id[Class] = id[HClass]; id[Type] = id[HType]; id[Val] = id[HVal]; }
                    id = id + Idsz;
                }
            }
            else { // not enum and function
                id[Class] = Glo;
                id[Val] = (int)data;
                data = data + sizeof(int);
            }
            if (tk == ',') next();
        }
        next();
    }
    return 0;
}


int eval() {
    int i, *t;
    cycle = 0;
    while (1) {
        i = *pc++; ++cycle;
        if (debug) {
            printf("%d> %.4s", cycle, &ins[i * 5]);
            if (i <= ADJ) printf(" %d\n", *pc); else printf("\n"); // instructions before ADJ have one operand
        }
        if      (i == LEA) a = (int)(bp + *pc++);                             // load local address
        else if (i == IMM) a = *pc++;                                         // load global address or immediate
        else if (i == JMP) pc = (int *)*pc;                                   // jump
        else if (i == JSR) { *--sp = (int)(pc + 1); pc = (int *)*pc; }        // jump to subroutine
        else if (i == BZ)  pc = a ? pc + 1 : (int *)*pc;                      // branch if zero
        else if (i == BNZ) pc = a ? (int *)*pc : pc + 1;                      // branch if not zero
        else if (i == ENT) { *--sp = (int)bp; bp = sp; sp = sp - *pc++; }     // enter subroutine
        else if (i == ADJ) sp = sp + *pc++;                                   // stack adjust
        else if (i == LEV) { sp = bp; bp = (int *)*sp++; pc = (int *)*sp++; } // leave subroutine
        else if (i == LI)  a = *(int *)a;                                     // load int
        else if (i == LC)  a = *(char *)a;                                    // load char
        else if (i == SI)  *(int *)*sp++ = a;                                 // store int
        else if (i == SC)  a = *(char *)*sp++ = a;                            // store char
        else if (i == PSH) *--sp = a;                                         // push

        else if (i == OR)  a = *sp++ |  a;
        else if (i == XOR) a = *sp++ ^  a;
        else if (i == AND) a = *sp++ &  a;
        else if (i == EQ)  a = *sp++ == a;
        else if (i == NE)  a = *sp++ != a;
        else if (i == LT)  a = *sp++ <  a;
        else if (i == GT)  a = *sp++ >  a;
        else if (i == LE)  a = *sp++ <= a;
        else if (i == GE)  a = *sp++ >= a;
        else if (i == SHL) a = *sp++ << a;
        else if (i == SHR) a = *sp++ >> a;
        else if (i == ADD) a = *sp++ +  a;
        else if (i == SUB) a = *sp++ -  a;
        else if (i == MUL) a = *sp++ *  a;
        else if (i == DIV) a = *sp++ /  a;
        else if (i == MOD) a = *sp++ %  a;

        else if (i == OPEN) a = open((char *)sp[1], *sp);
        else if (i == READ) a = read(sp[2], (char *)sp[1], *sp);
        else if (i == CLOS) a = close(*sp);
        else if (i == PRTF) { t = sp + pc[1]; a = printf((char *)t[-1], t[-2], t[-3], t[-4], t[-5], t[-6]); }
        else if (i == MALC) a = (int)malloc(*sp);
        else if (i == MSET) a = (int)memset((char *)sp[2], sp[1], *sp);
        else if (i == MCMP) a = memcmp((char *)sp[2], (char *)sp[1], *sp);
        else if (i == EXIT) { printf("exit(%d) cycle = %d\n", *sp, cycle); return *sp; }
        else { printf("unknown instruction = %d! cycle = %d\n", i, cycle); exit(-1); }
    }
}


int main(int argc, char **argv)
{
    int i, *t; // temps

    --argc; ++argv;
    if (argc > 0 && **argv == '-' && (*argv)[1] == 's') { src = 1; --argc; ++argv; }
    if (argc > 0 && **argv == '-' && (*argv)[1] == 'd') { debug = 1; --argc; ++argv; }
    if (argc < 1) { printf("usage: ./xc [-s] [-d] file ...\n"); return -1; }
    // instructions names, format with the same width(5), for -s and -d print
    ins = "LEA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,"
        "OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,"
        "OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT,";

    if ((fd = open(*argv, 0)) < 0) { printf("could not open(%s)\n", *argv); return -1; }

    poolsz = 256 * 1024; // arbitrary size
    if (!(sym = malloc(poolsz))) { printf("could not malloc(%d) symbol area\n", poolsz); return -1; }
    if (!(le = e = malloc(poolsz))) { printf("could not malloc(%d) text area\n", poolsz); return -1; }
    if (!(data = malloc(poolsz))) { printf("could not malloc(%d) data area\n", poolsz); return -1; }
    if (!(sp = malloc(poolsz))) { printf("could not malloc(%d) stack area\n", poolsz); return -1; }
    memset(sym, 0, poolsz); memset(e, 0, poolsz); memset(data, 0, poolsz); memset(sp, 0, poolsz);

    p = "char else enum if int return sizeof while "
        "open read close printf malloc memset memcmp exit void main";
    i = Char; while (i <= While) { next(); id[Tk] = i++; } // add keywords to symbol table
    i = OPEN; while (i <= EXIT) { next(); id[Class] = Sys; id[Type] = INT; id[Val] = i++; } // add library to symbol table
    next(); id[Tk] = Char; // treat void as char type, due to lack of void type
    next(); idmain = id; // keep track of main

    if (!(lp = p = malloc(poolsz))) { printf("could not malloc(%d) source area\n", poolsz); return -1; }
    if ((i = read(fd, p, poolsz-1)) <= 0) { printf("read() returned %d\n", i); return -1; }
    p[i] = 0; // let source ends up with \0
    close(fd);

    parse(); // syntax directed translation

    if (!(pc = (int *)idmain[Val])) { printf("main() not defined\n"); return -1; }
    if (src) return 0;

    // setup stack
    sp = (int *)((int)sp + poolsz);
    *--sp = EXIT; // call exit if main returns
    *--sp = PSH; t = sp;
    *--sp = argc;
    *--sp = (int)argv;
    *--sp = (int)t;

    // run
    return eval();
}
