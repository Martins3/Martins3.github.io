extern int shared;
void swap(int *a, int *b);

int main(){
  int a = 1000;
  swap(&a, &shared);
  return 0;
}


