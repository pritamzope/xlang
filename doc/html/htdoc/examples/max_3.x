extern void scanf(char*, int, int, int);
extern void printf(char*, int);

void print_string(char* str)
{
  printf(str, 0);
}

global int main()
{
  int num1, num2, num3, max;

  print_string("Enter 3 numbers: ");
  scanf("%d%d%d", &num1, &num2, &num3);

  if(num1 > num2){
    max = num1;
  }else{
    max = num2;
  }

  if(num1 > num3){
    max = num1;
  }else{
    max = num3;
  }

  if(num2 > num3){
    max = num2;
  }else{
    max = num3;
  }
    
  printf("Maximum among all three numbers = %d\n", max);

}
