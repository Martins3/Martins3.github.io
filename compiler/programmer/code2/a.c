extern int shared;
int main(void){
  int a = 100;
  swap(&a, &shared);
  return 0;
}
