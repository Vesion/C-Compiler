#include <stdio.h>
char g1, *g2;
enum {e};
char f(char* a) { return *a; }
int main() { printf("hello world\n"); g1 = 1; g2 = &g1; f(g2); return 0; }
