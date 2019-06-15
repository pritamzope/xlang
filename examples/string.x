extern void printf(char*, int);

char* string;
string="HelloWorld\n";

int string_len()
{
  int index, len, val;
  index = 0;
  len = 0;
  val = string[index];
  while(val != 0){
    index++;
    len++;
    val = string[index];
  }
  return len;
}

void print_string(int len)
{
  int index, val;

  for(index = 0; index < len; index++){
    val = string[index];
    printf("%c", val);
  }
}

global void main()
{
  int len;
  len = string_len();
  printf("len = %d\n", len);
  print_string(len);
}

