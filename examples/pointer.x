extern void printf(char*, int, int);

int **ptr2;

global void main()
{
  int *ptr, x, y;
  x = 1234;
  ptr = &x;
  y = *ptr;
  printf("ptr = %x, y = %d\n", ptr, y);
  ptr2 = &ptr;
  y = ***ptr2;
  printf("ptr2 = %x, y = %d\n", ptr2, y);
}
