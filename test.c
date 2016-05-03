#include <stdio.h>
char g1, *g2;
enum {e = 1};
void f(char a) {
    char b;
    a = 1;
    b = 2;
    printf("hello\n");
}
int main() { 
    f(0);
    return 0; 
}

