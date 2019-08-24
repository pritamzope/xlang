extern void printf(char*, int);

int array[10] = {7,4,9,1,2,5,6,8,3};

int partition(int p, int r)
{
  int x, i, j, v, a1, a2;

  x = array[r];
  i = p - 1;

  for(j = p; j < r; j++){
    v = array[j];
    if(v <= x){
      i = i + 1;
      //swap array elements
      a1 = array[i];
      a2 = array[j];
      array[i] = a2;
      array[j] = a1;
    }
  }

  v = i + 1;
  a1 = array[v];
  a2 = array[r];
  array[v] = a2;
  array[r] = a1;

  return i + 1;
}

void quick_sort(int p, int r)
{
  int q;
  if(p < r){
    q = partition(p, r);
    quick_sort(p, q - 1);
    quick_sort(q + 1, r);
  }
}

void print_array()
{
  int i;
  for(i = 0; i < 9; i++){
    printf("%d ", array[i]);
  }
  printf("\n",0);
}

global void main()
{
  int i;
  printf("Before Sorting: ", 0);
  print_array();
  quick_sort(0, 9);
  printf("After Sorting: ", 0);
  print_array();
}

