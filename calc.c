#include <stdio.h>
#include <stdlib.h>

/* use top-down BNF to implement a simple calculator supporting + - * / ()
 * 
 * basic:
 
        <expr> ::= <expr> + <term>
                | <expr> - <term>
                | <term>

        <term> ::= <term> * <factor>
                | <term> / <factor>
                | <factor>

        <factor> ::= ( <expr> )
                    | Num

 * elimination of left-recursion:
 
        <expr> ::= <term> <expr_tail>
        <expr_tail> ::= + <term> <expr_tail>
                    | - <term> <expr_tail>
                    | <empty>

        <term> ::= <factor> <term_tail>
        <term_tail> ::= * <factor> <term_tail>
                    | / <factor> <term_tail>
                    | <empty>

        <factor> ::= ( <expr> )
                   | Num

 */

enum { Num };
int token;
int token_val;
char *line = NULL;
char *src = NULL;

void next() {
    while (*src == ' ' || *src == '\t') ++src;
    token = *src++;
    if (token >= '0' && token <= '9' ) {
        token_val = token - '0';
        token = Num;
        while (*src >= '0' && *src <= '9') token_val = token_val * 10 + *src++ - '0';
        return;
    }
}

void match(int tk) { // check if token is what we want
    if (token != tk) {
        printf("expected token: %d(%c), got: %d(%c)\n", tk, tk, token, token);
        exit(-1);
    }
    next();
}

int expr();

int factor() {
    int value = 0;
    if (token == '(') {
        match('(');
        value = expr();
        match(')');
    } else {
        value = token_val;
        match(Num);
    }
    return value;
}

int term_tail(int lvalue) {
    if (token == '*') {
        match('*');
        int value = lvalue * factor();
        return term_tail(value);
    } else if (token == '/') {
        match('/');
        int value = lvalue / factor();
        return term_tail(value);
    } else {
        return lvalue;
    }
}

int term() {
    int lvalue = factor();
    return term_tail(lvalue);
}

int expr_tail(int lvalue) {
    if (token == '+') {
        match('+');
        int value = lvalue + term();
        return expr_tail(value);
    } else if (token == '-') {
        match('-');
        int value = lvalue - term();
        return expr_tail(value);
    } else {
        return lvalue;
    }
}

int expr() {
    int lvalue = term();
    return expr_tail(lvalue);
}

int main(int argc, char *argv[])
{
    size_t linecap = 0;
    while (getline(&line, &linecap, stdin) > 0) {
        src = line;
        next();
        printf("%d\n", expr());
    }
    return 0;
}
