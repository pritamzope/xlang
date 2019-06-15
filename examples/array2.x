extern void printf(char*, int, int);

int array[10] = {0,1,2,3,4,5,6,7,8,9};

char buffer1[100], buffer2[100];

//copy n elements from from buffer1 to buffer2
void memcpy(int n)
{
  char val;
  int old_n;
  old_n = n;

  if(n < 0){
    return;
  }

  while(n >= 0){
    val = buffer1[n];
    buffer2[n] = val;
    n--;
  }
  buffer2[old_n] = 0;
}

global void main()
{
  int index, val;

  for(index = 0; index < 10; index++){
    val = array[index];
    printf("array[%d] = %d\n", index, val);
  }
}
