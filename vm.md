# VM Spec of tinyc

###Registers

```c
int *pc, *sp, *bp, ax, cycle;
```

- `pc` : program counter register, store the address of next instruction;
- `sp` : stack pointer register, maintain the address of stack top, decrease while pushing stack;
- `bp` : base pointer register, for function call usage;
- `ax` : general register, store the result of one instruction;
- `cycle` : counter of instructions.



###Instructions

```c
enum {
    LEA, IMM, JMP, CALL, JZ, JNZ, ENT, ADJ, LEV, LI, LC, SI, SC, PUSH,
    OR, XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD,
    OPEN, READ, CLOS, PRTF, MALC, MSET, MCMP, EXIT
};
```

* `IMM` : load immediate value to `ax` from `pc`, then `pc` point to next instruction;

* `LC` : load character value to `ax` from address in `ax`;

* `LI` : load integer value to `ax` from address in `ax`;

* `SC` : let address in `sp` save character value of `ax`, then pop stack;

* `SI` : let address in `sp` save integer value of `ax`, then pop stack;

* `PUSH` : push the value of `ax` onto stack;

* `JMP` : advance `pc`, jump to next address unconditionally;

* `JZ` : jump if `ax` is zero;

* `JNZ` : jump if `ax` is not zero;

* `CALL` : call subroutine, push callee address(`pc+1`) onto stack for return;

* `ENT` : enter subroutine, push `bp` onto stack, then make new call frame for parameters and inner automatic variables;

* `ADJ` : remove call frame; (poor `ADD`)

* `LEV` : return to callee address; (lack of `POP`)

* `LEA` : load arguments to ax; (poor `ADD`)

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