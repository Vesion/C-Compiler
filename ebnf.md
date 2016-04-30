# EBNF sepc of tinyc

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
| old BP        | <- bp
+---------------+
| local_1       |    bp - 1
+---------------+
| local_2       |    bp - 2
+---------------+
|    ....       |  low address
```

