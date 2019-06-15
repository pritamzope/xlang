int x;
int array[10];

asm{"; global inline assembly"}

void print_string(char* str, int len)
{
  asm{
    "\tmov eax, 4",
    "\tmov ebx, 1",
    "\tmov ecx, %" ["=m"(str):],
    "\tmov edx, %" ["=m"(len):],
    "\tint 0x80"
  }
}

global void main()
{
  int a;
  short s;
  char c;
  double d;

  print_string("hello world\n", 12);

label:

 /*
    a  eax
    b  ebx
    c  ecx
    d  edx
    S  esi
    D  edi
    m  memory
    i  immediate value
    % or %n  where n = 0, 1, ...
 */

  asm{"\tmov %, %" ["=a"():"i"(0xfffffff0)],
      "\tmov ax, %" ["=m"(s):],
      "\tmov al, %" ["=m"(c):],
      "\tfld %" ["=m"(x):],
      "\txor %, %" ["=c"():"c"()],
      "\tmov dword[array + %0 * 4], %1" ["=c"(): "a"()]
     }

  asm{"\tcmp %0, %1" ["=m"(a):"a"()],
      "\tje .label"}

}

