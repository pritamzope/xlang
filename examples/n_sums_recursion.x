extern void scanf(char*, int);
extern void printf(char*, int, int);

void print_string(char* str)
{
  printf(str, 0, 0);
}

int sum_of_natural_no(int x)
{
  int sum;
  sum = 0;

  if(x <= 0){
    return 0;
  }else{
    sum = sum_of_natural_no(x - 1);
    sum = sum + x;
    return sum;
  }

}

global void main()
{
  int num, sum;
  sum = 0;

  print_string("Enter a number: ");
  scanf("%d", &num);

  sum = sum_of_natural_no(num);

  printf("sum of first %d natual numbers = %d\n", num, sum);
}

