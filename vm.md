# VM Specification

A one-register [stack machine](https://en.wikipedia.org/wiki/Stack_machine) with accumulator is used in xc.

###Registers

```c
int *pc, *sp, *bp, a, cycle;
```

- `pc` : program counter register, store next instruction;
- `sp` : stack pointer register, maintain the address of stack top, decrease while pushing stack;
- `bp` : base (or frame) pointer register, for calling function;
- `a` : accumulator register;
- `cycle` : counter of instructions.



###Instructions

```c
enum { 
  LEA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,
  OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
  OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT 
};
```

* `LEA` : `a = (int)(bp + *pc++);` load local variable's address into `a`;
* `IMM` : `a = *pc++;` load global address or immediate intoto `a` from `pc`, then advance `pc`;
* `JMP` : `pc = (int*)*pc;` jump to `*pc` unconditionally;
* `JSR` : `*--sp = (int)pc+1; pc = (int*)*pc;` jump to subroutine, push `pc+1` onto stack, then jump to `*pc` unconditionally;
* `BZ` : `pc = a ? pc+1 : (int*)*pc;` branch if zero, if `a` is zero, jump tp `*pc`, otherwise jumo to `pc+1`;
* `BNZ` : `pc = a ? (int*)*pc : pc+1;` branch if not zero, if `a` is not zero, jump tp `*pc`, otherwise jumo to `pc+1`;
* `ENT` : `*--sp = (int)bp; bp = sp; sp = sp - *pc++;` enter subroutine, push `bp` onto stack, then make new call frame for parameters and inner automatic variables;
* `ADJ` : `sp = sp + *pc++;` adjust stack, remove frame arguments (when leave subroutine);
* `LEV` : `sp = bp; bp = (int*)*sp++; pc = (int*)sp++;` leave subroutine, unwind `sp` to `bp` (callee) address;
* `LC` : `a = *(char*)a;` load character value to `a` from address in `a`;
* `LI` : `a = *(int*)a;` load integer value to `a` from address in `a`;
* `SC` : `a = *(char*)*sp++ = a;` store character value, hen pop stack;
* `SI` : `*(int*)*sp++ = a;` store integer value, then pop stack;
* `PSH` : `*--sp = a;` push the value of `a` onto stack;
* `OR` : `|`
* `XOR` : `^`
* `AND` : `&`
* `EQ` : `==`
* `NE` : `!=`
* `LT` : `<`
* `LE` : `<=`
* `GT` : `>`
* `GE` : `>=`
* `SHL` : `<<`
* `SHR` : `>>`
* `ADD` : `+`
* `SUB` : `-`
* `MUL` : `*`
* `DIV` : `/`
* `MOD` : `%`
* `EXIT` : exit program;
* `OPEN` : open file;
* `CLOS` : close file;
* `READ` : read file;
* `PRTF` : standard print out;
* `MALC` : allocate memory block;
* `MSET` : set memory block;
* `MCMP` : compare memory block.



### Activation Record (Frame)

```c
|    ....       | high address
+---------------+
| arg: param_a  |    bp + 3
+---------------+
| arg: param_b  |    bp + 2
+---------------+
|return address |    bp + 1
+---------------+
| old bp        | <- bp
+---------------+
| local_1       |    bp - 1
+---------------+
| local_2       |    bp - 2
+---------------+
|    ....       |  low address
```
