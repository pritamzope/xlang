extern void printf(char*, int, int);

int array[10];

global void main()
{
  int index, val;

  for(index = 0; index < 10; index++){
    array[index] = index * 10;
  }

  for(index = 0; index < 10; index++){
    val = array[index];
    printf("array[%d] = %d\n", index, val);
  }
}
