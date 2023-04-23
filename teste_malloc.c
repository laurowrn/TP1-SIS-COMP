#include <stdio.h>
#include <stdlib.h>

int main(){
    int *num;
    printf("Endereço: %p\n", num);
    num = (int *) malloc(sizeof(int));

    *num = 2;
    printf("Endereço: %p\n", num);
    printf("Conteúdo: %d\n", *num);


    num = (int *) malloc(sizeof(int));
    *num = 3;
    printf("Endereço: %p\n", num);
    printf("Conteúdo: %d\n", *num);
    return 0;
}