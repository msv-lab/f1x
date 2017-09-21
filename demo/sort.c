#include <stdio.h>
#include <stdlib.h>

/* The buggy program and the input are obtained from
     https://stackoverflow.com/questions/3444382/quicksort-partition-algorithm
   (translated from Java to C)
*/

int Partition(int a[], int p, int r) {
  int x = a[r];
  
  int i = p-1;
  int temp=0;
  
  for(int j=p; j<r-1; j++) {
    if(a[j]<=x) {
      i++;
      temp = a[i];
      a[i] = a[j];
      a[j] = temp;
    }
  }
  
  temp = a[i+1];
  a[i+1] = a[r];
  a[r] = temp;
  return (i+1);
}

void qSort(int a[], int p, int r) {
  if(p<r) {
    int q = Partition(a, p, r);
    qSort(a, p, q-1);
    qSort(a, q + 1, r);
  }
}

int main(int argc, char *argv[]) {
  int n;
  int numbers[1000];
  int tmp;

  while (scanf("%d", &tmp) == 1) {
    numbers[n++] = tmp;
  }

  for (int i=0; i<n; i++)
    scanf("%d", &numbers[i]);

  qSort(numbers, 0, n-1);

  for (int i=0; i<n; i++)
    printf("%d ", numbers[i]);

  return 0;
}
