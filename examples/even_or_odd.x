extern void scanf(char*, int);
extern void printf(char*, int);

void print_string(char* str)
{
  printf(str, 0);
}

global int main()
{
  int x, result;

  print_string("Enter a number: ");
  scanf("%d", &x);

  result = x % 2;

  if(result == 0){
    printf("%d is even\n", x);
  }else{
    printf("%d is odd\n", x);
  }

}
