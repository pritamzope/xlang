extern void scanf(char*, int);
extern void printf(char*, int);

void print_string(char* str)
{
  printf(str, 0);
}

int power_of_3(int num)
{
  return (num * num * num);
}

int is_armstrong_number(int num)
{
  int orig_num, remainder, result, cube;
  result = 0;
  cube = 0;

  orig_num = num;

  while(orig_num > 0){
    remainder = orig_num % 10;
    cube = power_of_3(remainder);
    result = result + cube;
    orig_num = orig_num / 10;
  }

  if(num == result){
     return 1;
  }else{
     return 0;
  }
}

global int main()
{
  int num, is_armstrong;

  print_string("Enter a number: ");
  scanf("%d", &num);

  is_armstrong = is_armstrong_number(num);

  if(is_armstrong == 1){
     printf("%d is armstrong number\n", num);
  }else{
     printf("%d is not armstrong number\n", num);
  }

}



