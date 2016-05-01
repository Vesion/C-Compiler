#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>


char *p, *data;
int fd, poolsz, *idmain, *e, *id, *sym, tk, ival, ty, loc, *pc, *sp, *bp, a, cycle; 
int line;

enum {
    Num = 128, Fun, Sys, Glo, Loc, Id,
    Char, Else, Enum, If, Int, Return, Sizeof, While,
    Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};
enum { LEA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,
    OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
    OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT };
enum { CHAR, INT, PTR };
enum { Tk, Hash, Name, Class, Type, Val, HClass, HType, HVal, Idsz };


void next()
{
    char *pp;
    while ((tk = *p)) {
        ++p;
        if (tk == '\n') { ++line; }
        else if (tk == '#') { while (*p != 0 && *p != '\n') ++p; }
        else if ((tk >= 'a' && tk <= 'z') || (tk >= 'A' && tk <= 'Z') || tk == '_') { 
            pp = p - 1; 
            while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_')
                tk = tk * 147 + *p++;
            tk = (tk << 6) + (p - pp); 
            id = sym;
            while (id[Tk]) { 
                if (tk == id[Hash] && !memcmp((char *)id[Name], pp, p - pp)) { tk = id[Tk]; return; } 
                id = id + Idsz;
            }
            id[Name] = (int)pp; 
            id[Hash] = tk;
            tk = id[Tk] = Id; 
            return;
        }
        else if (tk >= '0' && tk <= '9') { 
            if ((ival = tk - '0')) { while (*p >= '0' && *p <= '9') ival = ival * 10 + *p++ - '0'; } 
            else if (*p == 'x' || *p == 'X') { 
                while ((tk = *++p) && ((tk >= '0' && tk <= '9') || (tk >= 'a' && tk <= 'f') || (tk >= 'A' && tk <= 'F')))
                    ival = ival * 16 + (tk & 15) + (tk >= 'A' ? 9 : 0); 
            }
            else { while (*p >= '0' && *p <= '7') ival = ival * 8 + *p++ - '0'; } 
            tk = Num;
            return;
        }
        else if (tk == '/') {
            if (*p == '/') { ++p; while (*p != 0 && *p != '\n') ++p; }
            else { tk = Div; return; }
        }
        else if (tk == '\'' || tk == '"') { 
            pp = data;
            while (*p != 0 && *p != tk) { 
                if ((ival = *p++) == '\\') {
                    if ((ival = *p++) == 'n') ival = '\n'; 
                }
                if (tk == '"') *data++ = ival; 
            }
            ++p;
            if (tk == '"') ival = (int)pp; else tk = Num; 
            return;
        }
        else if (tk == '=') { if (*p == '=') { ++p; tk = Eq; } else tk = Assign; return; } 
        else if (tk == '+') { if (*p == '+') { ++p; tk = Inc; } else tk = Add; return; } 
        else if (tk == '-') { if (*p == '-') { ++p; tk = Dec; } else tk = Sub; return; } 
        else if (tk == '!') { if (*p == '=') { ++p; tk = Ne; } return; } 
        else if (tk == '<') { if (*p == '=') { ++p; tk = Le; } else if (*p == '<') { ++p; tk = Shl; } else tk = Lt; return; } 
        else if (tk == '>') { if (*p == '=') { ++p; tk = Ge; } else if (*p == '>') { ++p; tk = Shr; } else tk = Gt; return; } 
        else if (tk == '|') { if (*p == '|') { ++p; tk = Lor; } else tk = Or; return; } 
        else if (tk == '&') { if (*p == '&') { ++p; tk = Lan; } else tk = And; return; } 
        else if (tk == '^') { tk = Xor; return; } 
        else if (tk == '%') { tk = Mod; return; } 
        else if (tk == '*') { tk = Mul; return; } 
        else if (tk == '[') { tk = Brak; return; } 
        else if (tk == '?') { tk = Cond; return; } 
        else if (tk == '~' || tk == ';' || tk == '{' || tk == '}' || tk == '(' || tk == ')' || tk == ']' || tk == ',' || tk == ':') return; 
    }
}


void match(int t) { 
    if (tk == t) next(); 
    else { printf("%d: expected token: %d\n", line, tk); exit(-1); } 
}


void expr(int lev) 
{
    int t, *d;
    if (!tk) { printf("%d: unexpected eof in expression\n", line); exit(-1); }
    else if (tk == Num) { *++e = IMM; *++e = ival; match(Num); ty = INT; } 
    else if (tk == '"') { 
        *++e = IMM; *++e = ival; match('"');
        while (tk == '"') match('"');
        data = (char *)((int)data + sizeof(int) & -sizeof(int)); 
        ty = PTR;
    }
    else if (tk == Sizeof) { 
        match(Sizeof); match('(');
        ty = INT; if (tk == Int) match(Int); else if (tk == Char) { match(Char); ty = CHAR; }
        while (tk == Mul) { match(Mul); ty = ty + PTR; }
        if (tk == ')') match(')');
        *++e = IMM; *++e = (ty == CHAR) ? sizeof(char) : sizeof(int);
        ty = INT;
    }
    else if (tk == Id) { 
        d = id; match(Id); 
        if (tk == '(') { 
            match('(');
            t = 0; 
            while (tk != ')') { expr(Assign); *++e = PSH; ++t; if (tk == ',') match(','); } 
            match(')');
            if (d[Class] == Sys) *++e = d[Val]; 
            else if (d[Class] == Fun) { *++e = JSR; *++e = d[Val]; } 
            else { printf("%d: bad function call\n", line); exit(-1); }
            if (t) { *++e = ADJ; *++e = t; } 
            ty = d[Type];
        }
        else if (d[Class] == Num) { *++e = IMM; *++e = d[Val]; ty = INT; } 
        else { 
            if (d[Class] == Loc) { *++e = LEA; *++e = loc - d[Val]; } 
            else if (d[Class] == Glo) { *++e = IMM; *++e = d[Val]; } 
            else { printf("%d: undefined variable\n", line); exit(-1); }
            *++e = ((ty = d[Type]) == CHAR) ? LC : LI; 
        }
    }
    else if (tk == '(') {
        match('(');
        if (tk == Int || tk == Char) { 
            t = (tk == Int) ? INT : CHAR; match(tk);
            while (tk == Mul) { match(Mul); t = t + PTR; }
            match(')'); expr(Inc); 
            ty = t;
        }
        else { expr(Assign); match(')'); }
    }
    else if (tk == Mul) { 
        match(Mul); expr(Inc);
        if (ty > INT) ty = ty - PTR; else { printf("%d: bad dereference\n", line); exit(-1); }
        *++e = (ty == CHAR) ? LC : LI;
    }
    else if (tk == And) { 
        match(And);; expr(Inc);
        if (*e == LC || *e == LI) --e;  else { printf("%d: bad address-of\n", line); exit(-1); }
        ty = ty + PTR;
    }
    else if (tk == '!') { match('!'); expr(Inc); *++e = PSH; *++e = IMM; *++e = 0; *++e = EQ; ty = INT; } 
    else if (tk == '~') { match('~'); expr(Inc); *++e = PSH; *++e = IMM; *++e = -1; *++e = XOR; ty = INT; } 
    else if (tk == Add) { match(Add); expr(Inc); ty = INT; } 
    else if (tk == Sub) { 
        match(Sub); *++e = IMM;
        if (tk == Num) { *++e = -ival; match(Num); } else { *++e = -1; *++e = PSH; expr(Inc); *++e = MUL; }
        ty = INT;
    }
    else if (tk == Inc || tk == Dec) { 
        t = tk; match(tk); expr(Inc);
        if (*e == LC) { *e = PSH; *++e = LC; } else if (*e == LI) { *e = PSH; *++e = LI; } 
        else { printf("%d: bad lvalue in pre-increment\n", line); exit(-1); }
        *++e = PSH; 
        *++e = IMM; *++e = (ty > PTR) ? sizeof(int) : sizeof(char); 
        *++e = (t == Inc) ? ADD : SUB;
        *++e = (ty == CHAR) ? SC : SI; 
    }
    else { printf("%d: bad expression\n", line); exit(-1); }

    while (tk >= lev) { 
        t = ty; 
        if (tk == Assign) { 
            match(Assign);
            if (*e == LC || *e == LI) *e = PSH;
            else { printf("%d: bad lvalue in assignment\n", line); exit(-1); }
            expr(Assign); *++e = ((ty = t) == CHAR) ? SC : SI; 
        }
        else if (tk == Cond) { 
            match(Cond);
            *++e = BZ; d = ++e;
            expr(Assign);
            if (tk == ':') match(':'); else { printf("%d: conditional missing colon\n", line); exit(-1); }
            *d = (int)(e + 3); *++e = JMP; d = ++e;
            expr(Cond);
            *d = (int)(e + 1);
        }
        else if (tk == Lor) { match(Lor); *++e = BNZ; d = ++e; expr(Lan); *d = (int)(e + 1); ty = INT; } 
        else if (tk == Lan) { match(Lan); *++e = BZ;  d = ++e; expr(Or);  *d = (int)(e + 1); ty = INT; } 
        else if (tk == Or)  { match(Or); *++e = PSH; expr(Xor); *++e = OR;  ty = INT; } 
        else if (tk == Xor) { match(Xor); *++e = PSH; expr(And); *++e = XOR; ty = INT; } 
        else if (tk == And) { match(And); *++e = PSH; expr(Eq);  *++e = AND; ty = INT; } 
        else if (tk == Eq)  { match(Eq); *++e = PSH; expr(Lt);  *++e = EQ;  ty = INT; } 
        else if (tk == Ne)  { match(Ne); *++e = PSH; expr(Lt);  *++e = NE;  ty = INT; } 
        else if (tk == Lt)  { match(Lt); *++e = PSH; expr(Shl); *++e = LT;  ty = INT; } 
        else if (tk == Gt)  { match(Gt); *++e = PSH; expr(Shl); *++e = GT;  ty = INT; } 
        else if (tk == Le)  { match(Le); *++e = PSH; expr(Shl); *++e = LE;  ty = INT; } 
        else if (tk == Ge)  { match(Ge); *++e = PSH; expr(Shl); *++e = GE;  ty = INT; } 
        else if (tk == Shl) { match(Shl); *++e = PSH; expr(Add); *++e = SHL; ty = INT; } 
        else if (tk == Shr) { match(Shr); *++e = PSH; expr(Add); *++e = SHR; ty = INT; } 
        else if (tk == Add) { 
            match(Add); *++e = PSH; expr(Mul);
            if ((ty = t) > PTR) { *++e = PSH; *++e = IMM; *++e = sizeof(int); *++e = MUL;  }
            *++e = ADD;
        }
        else if (tk == Sub) { 
            match(Sub); *++e = PSH; expr(Mul);
            if (t > PTR && t == ty) { *++e = SUB; *++e = PSH; *++e = IMM; *++e = sizeof(int); *++e = DIV; ty = INT; }
            else if ((ty = t) > PTR) { *++e = PSH; *++e = IMM; *++e = sizeof(int); *++e = MUL; *++e = SUB; }
            else *++e = SUB;
        }
        else if (tk == Mul) { match(Mul); *++e = PSH; expr(Inc); *++e = MUL; ty = INT; } 
        else if (tk == Div) { match(Div); *++e = PSH; expr(Inc); *++e = DIV; ty = INT; } 
        else if (tk == Mod) { match(Mod); *++e = PSH; expr(Inc); *++e = MOD; ty = INT; } 
        else if (tk == Inc || tk == Dec) { 
            if (*e == LC) { *e = PSH; *++e = LC; }
            else if (*e == LI) { *e = PSH; *++e = LI; }
            else { printf("%d: bad lvalue in post-increment\n", line); exit(-1); }
            *++e = PSH; *++e = IMM; *++e = (ty > PTR) ? sizeof(int) : sizeof(char);
            *++e = (tk == Inc) ? ADD : SUB;
            *++e = (ty == CHAR) ? SC : SI;
            *++e = PSH; *++e = IMM; *++e = (ty > PTR) ? sizeof(int) : sizeof(char);
            *++e = (tk == Inc) ? SUB : ADD;
            match(tk);
        }
        else if (tk == Brak) { 
            match(Brak); *++e = PSH; expr(Assign); match(']');
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
    int *a, *b;
    if (tk == If) {
        match(If); match('('); expr(Assign); match(')');
        *++e = BZ;  b = ++e;
        stmt(); 
        if (tk == Else) {
            *b = (int)(e + 3); *++e = JMP; b = ++e;
            match(Else); stmt(); 
        }
        *b = (int)(e + 1);
    }
    else if (tk == While) {
        match(While);
        a = e + 1; 
        match('('); expr(Assign); match(')');
        *++e = BZ; b = ++e;
        stmt();
        *++e = JMP; *++e = (int)a;
        *b = (int)(e + 1);
    }
    else if (tk == Return) {
        match(Return);
        if (tk != ';') expr(Assign);
        *++e = LEV;
        match(';');
    }
    else if (tk == '{') { match('{'); while (tk != '}') stmt(); match('}'); }
    else if (tk == ';') { match(';'); }
    else { expr(Assign); match(';'); }
}


int parse() {
    int bt, ty, i;
    line = 1;
    next();
    while (tk) {
        bt = INT; 
        if (tk == Int) match(Int);
        else if (tk == Char) { match(Char); bt = CHAR; }
        else if (tk == Enum) { 
            match(Enum);
            if (tk != '{') match(Id);
            if (tk == '{') {
                match('{');
                i = 0; 
                while (tk != '}') {
                    if (tk != Id) { printf("%d: bad enum identifier %d\n", line, tk); exit(-1); }
                    next();
                    if (tk == Assign) { 
                        next();
                        if (tk != Num) { printf("%d: bad enum initializer\n", line); exit(-1); }
                        i = ival;
                        next();
                    }
                    id[Class] = Num; id[Type] = INT; id[Val] = i++; 
                    if (tk == ',') next();
                }
                match('}');
            }
        }
        while (tk != ';' && tk != '}') {
            ty = bt; 
            while (tk == Mul) { match(Mul); ty = ty + PTR; } 
            if (tk != Id) { printf("%d: bad global declaration\n", line); exit(-1); } 
            if (id[Class]) { printf("%d: duplicate global definition\n", line); exit(-1); } 
            match(Id); id[Type] = ty;
            if (tk == '(') { 
                id[Class] = Fun; id[Val] = (int)(e + 1); 
                match('(');
                i = 0;
                while (tk != ')') { 
                    ty = INT;
                    if (tk == Int) match(Int); else if (tk == Char) { match(Char); ty = CHAR; }
                    while (tk == Mul) { match(Mul); ty = ty + PTR; }
                    if (tk != Id) { printf("%d: bad parameter declaration\n", line); exit(-1); }
                    if (id[Class] == Loc) { printf("%d: duplicate parameter definition\n", line); exit(-1); }
                    id[HClass] = id[Class]; id[Class] = Loc;
                    id[HType]  = id[Type];  id[Type] = ty;
                    id[HVal]   = id[Val];   id[Val] = i++; 
                    match(Id); if (tk == ',') match(',');
                }
                match(')'); match('{');
                loc = ++i; 
                while (tk == Int || tk == Char) { 
                    bt = (tk == Int) ? INT : CHAR;
                    match(tk);
                    while (tk != ';') {
                        ty = bt;
                        while (tk == Mul) { match(Mul); ty = ty + PTR; }
                        if (tk != Id) { printf("%d: bad local declaration\n", line); exit(-1); }
                        if (id[Class] == Loc) { printf("%d: duplicate local definition\n", line); exit(-1); }
                        id[HClass] = id[Class]; id[Class] = Loc;
                        id[HType]  = id[Type];  id[Type] = ty;
                        id[HVal]   = id[Val];   id[Val] = ++i; 
                        match(Id); if (tk == ',') match(',');
                    }
                    match(';');
                }
                *++e = ENT; *++e = i - loc; 
                while (tk != '}') stmt(); 
                *++e = LEV; 
                id = sym; 
                while (id[Tk]) {
                    if (id[Class] == Loc) { id[Class] = id[HClass]; id[Type] = id[HType]; id[Val] = id[HVal]; }
                    id = id + Idsz;
                }
            }
            else { id[Class] = Glo; id[Val] = (int)data; data = data + sizeof(int); }
            if (tk == ',') match(',');
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
        if      (i == LEA) a = (int)(bp + *pc++);                             
        else if (i == IMM) a = *pc++;                                         
        else if (i == JMP) pc = (int *)*pc;                                   
        else if (i == JSR) { *--sp = (int)(pc + 1); pc = (int *)*pc; }        
        else if (i == BZ)  pc = a ? pc + 1 : (int *)*pc;                      
        else if (i == BNZ) pc = a ? (int *)*pc : pc + 1;                      
        else if (i == ENT) { *--sp = (int)bp; bp = sp; sp = sp - *pc++; }     
        else if (i == ADJ) sp = sp + *pc++;                                   
        else if (i == LEV) { sp = bp; bp = (int *)*sp++; pc = (int *)*sp++; } 
        else if (i == LI)  a = *(int *)a;                                     
        else if (i == LC)  a = *(char *)a;                                    
        else if (i == SI)  *(int *)*sp++ = a;                                 
        else if (i == SC)  a = *(char *)*sp++ = a;                            
        else if (i == PSH) *--sp = a;                                         

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
    int i, *t;
    char *in, *s; size_t n; ssize_t l;

    poolsz = 256 * 1024;
    sym = malloc(poolsz); e = malloc(poolsz); data = malloc(poolsz); sp = malloc(poolsz);
    memset(sym, 0, poolsz); memset(e, 0, poolsz); memset(data, 0, poolsz); memset(sp, 0, poolsz);

    p = "char else enum if int return sizeof while "
        "open read close printf malloc memset memcmp exit void main";
    i = Char; while (i <= While) { next(); id[Tk] = i++; }
    i = OPEN; while (i <= EXIT) { next(); id[Class] = Sys; id[Type] = INT; id[Val] = i++; }
    next(); id[Tk] = Char;
    next(); idmain = id;

    p = s = malloc(poolsz);
    in = "";
    while (1) {
        in = NULL; n = 0;
        l = getline(&in, &n, stdin); if (!strcmp("run\n", in)) break;
        for (char *tt = in; *tt; ) *s++ = *tt++;
    } *s = 0;
    parse();
    if (!(pc = (int *)idmain[Val])) { printf("main() not defined\n"); return -1; }
    sp = (int *)((int)sp + poolsz);
    *--sp = EXIT; *--sp = PSH; t = sp; *--sp = argc; *--sp = (int)argv; *--sp = (int)t;
    return eval();
}
