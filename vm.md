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

* `LEA` : load local variable's address into `a`;

* `IMM` : load global address or immediate intoto `a` from `pc`, then advance `pc`;

* `JMP` : jump to `*pc` unconditionally;

* `JSR` : jump to subroutine, push `pc+1` onto stack, then jump to `*pc` unconditionally;

* `BZ` : branch if zero, if `a` is zero, jump tp `*pc`, otherwise jumo to `pc+1`;

* `BNZ` : branch if not zero, if `a` is not zero, jump tp `*pc`, otherwise jumo to `pc+1`;

* `ENT` : enter subroutine, push `bp` onto stack, then make new call frame for parameters and inner automatic variables;

* `ADJ` : adjust stack, remove frame arguments (when leave subroutine);

* `LEV` : leave subroutine, unwind `sp` to `bp` (callee) address;

* `LC` : load character value to `a` from address in `a`;

* `LI` : load integer value to `a` from address in `a`;

* `SC` : store character value, `*sp=a`, then pop stack;

* `SI` : store integer value, `*sp=a`, then pop stack;

* `PSH` : push the value of `a` onto stack;

  ​

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

  ​

* `EXIT` : exit program;

* `OPEN` : open file;

* `CLOS` : close file;

* `READ` : read file;

* `PRTF` : standard print out;

* `MALC` : allocate memory block;

* `MSET` : set memory block;

* `MCMP` : compare memory block.