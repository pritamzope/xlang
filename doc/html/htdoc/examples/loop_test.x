extern void printf(char*, int);

int array[10] = {1,2,3,4,5,6,7,8};

global void main()
{
  int i, j, k, x;

  printf("for loop test:\n", 0);
  for(i = 0; i < 3; i++){
   for(j = 7; j >= 0; j--){
     printf("%d ", array[j]);
   }
   printf("\n", 0);
  }

  printf("while loop test:\n", 0);
  i = 0;
  j = 0;
  while(i < 3){
    while(j < 3){
      printf("A", 0);
      j++;
    }
    printf("\n", 0);
    i++;
    j = 0;
  }

  printf("do-while loop test:\n", 0);
  i = 0;
  do{
    j = 0;
    do{
      printf("%d ", array[j]);
      j++;
    }while(j < 8);
    printf("\n", 0);
    i++;
  }while(i < 3);

  printf("nested for/while/do-while loop test:\n", 0);
  for(i = 0; i < 3; i++){
    j = 0;
    while(j < 3){
      k = 0;
      do{
        printf("%d ", array[k]);
        k++;
      }while(k < 3);
      j++;
      printf("\n", 0);
    }
  }

}


