record test{
  int x;
  char ch;
  float f;
  double d;
  test next;
  int array[3][3];
}

global int x;
x=100;

record node{
  int data;
  char* chptr;
  node* next;
}

test n1;

extern void printf(char*, int);

global void main()
{
  x = sizeof(test);
  printf("sizeof test: %d\n", x);
  
  printf("sizeof node: %d\n", sizeof(node));

  asm{
    "\tmov dword[n1 + test.x], 1234",
    "\tmov eax, dword[n1 + test.x]",
    "\tmov %, %" ["=m"(x):"a"()]
  }
  printf("x = %d\n", x);

}

