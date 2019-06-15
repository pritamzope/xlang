extern void printf(char*, int, int, int);

int x,y;

global int main()
{
  int result;
  x = 10;
  y = 5;

  result = x + y;
  printf("%d + %d = %d\n", x,y,result);

  result = x - y;
  printf("%d - %d = %d\n", x,y,result);

  result = x * y;
  printf("%d * %d = %d\n", x,y,result);

  result = x / y;
  printf("%d / %d = %d\n", x,y,result);

  result = x % y;
  printf("%d % %d = %d\n", x,y,result);

  result = x & y;
  printf("%d & %d = %d\n", x,y,result);

  result = x | y;
  printf("%d | %d = %d\n", x,y,result);

  result = x ^ y;
  printf("%d ^ %d = %d\n", x,y,result);

  result = x << 2;
  printf("%d << %d = %d\n", x,2,result);

  result = x >> 2;
  printf("%d >> %d = %d\n", x,2,result);

  printf("~%d = %d\n", x, ~x, 0);


}

