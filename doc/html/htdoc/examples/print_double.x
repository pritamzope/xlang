extern void printf(char*, double);
double pd;
float array[10] = {1.234, 2.345, 3.456, 4.567, 5.678};

void print_double(char* str)
{
  asm{
    "\tpush dword[pd + 4]",
    "\tpush dword[pd]",
    "\tpush %0"["=m"(str):""()],
    "\tcall printf",
    "\tadd esp, 12"
  }
  pd = 0.0;
}

void print_array()
{
  int index;
  float val;

  for(index = 0; index < 5; index++){
    val = array[index];
    pd = val;
    printf("array[%d] = ", index);
    print_double("%.3f\n");
  }
}

global void main()
{
  double d1, d2;
  d1 = 17.2345374;
  d2 = 24.347833;
  pd = d1 + d2 * 149.924;
  print_double("result: %.8f\n");
  print_array();
}


