# A Tiny C Interpreter
This is a tiny C interpreter inspired by [c4](https://github.com/rswier/c4).



### Interpreters

**xc.c** is much like the original c4, whereas it has plenty of easy-to-understand comments. Self-interpreting is supported of course.

```shell
> make
> ./xc test.c
> ./xc xc.c test.c
```



**xci.c** is a rough interpreter playing with input.

```c
> ./xci
> int a;
> int mian() {
> 	a = 1;
>	printf("%d\n", a);
> }
> run
```



### Subset of C

- Ignore all includes and macros;
- Only `int`, `char` and (multilevel) pointers `*` to int or char types are supported;
- Do not support `typedef`, `struct`, `union`, so no user-defined-classes;
- Ignore the name of `enum`;
- Treat `void` return type as `char` type;
- Variables (globals and locals) must be declared without initializing expression before reference;
- Scopes are only global and local, do not support block scopes;
- Do not support array type, but reference with subscript `[]` is allowed;
- Do not support `for`, `do-while` and `switch` statements;
- Do not support `goto`, `break` and `continue` statements;
- Do not support foward declaration, so mutal recursions are not allowed;
- No type checking, although coercions are supported;



### Language Spec

```c
program ::= {global_declaration}+

global_declaration ::= enum_decl | variable_decl | function_decl

enum_decl ::= 'enum' [id] '{' id ['=' num] {',' id ['=' num]} '}'

variable_decl ::= type {'*'} id { ',' {'*'} id } ';'

function_decl ::= type {'*'} id '(' parameter_decl ')' '{' body_decl '}'

parameter_decl ::= type {'*'} id {',' type {'*'} id}

body_decl ::= {variable_decl}, {statement}

statement ::= non_empty_statement | empty_statement

non_empty_statement ::= if_statement | while_statement | '{' statement '}'
                     | 'return' expression | expression ';'

if_statement ::= 'if' '(' expression ')' statement ['else' non_empty_statement]

while_statement ::= 'while' '(' expression ')' non_empty_statement
```



