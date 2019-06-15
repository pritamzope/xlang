extern void printf(char*, int);
extern void scanf(char*, int);

global int main()
{
  int num, c, result;
 
  printf("Enter a number: ", 0);
  scanf("%d", &num);
 
  if(num == 2){
    printf("%d is prime\n", num);
  }else{
    for(c = 2; c < num; c++){
      result = num % c;
      if(result == 0){
        break;
      }
    }
    if (c != num){
      printf("%d is not prime\n", num);
    }else{
       printf("%d is prime\n", num);
    }
  }
}
