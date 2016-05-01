# C-Compiler
Tiny C interpreter inspired by [c4](https://github.com/rswier/c4).



**xc.c** is much like the original c4, whereas it has plenty of easy-to-understand comments. Self-interpreting is supported of course.

Usage:

```shell
> make
> ./xc test.c
> ./xc xc.c test.c
```



**xci.c** is a rough interpreter playing with input.

Usage:

```c
> ./xci
> int a;
> int mian() {
> 	a = 1;
>	printf("%d\n", a);
> }
> run
```

