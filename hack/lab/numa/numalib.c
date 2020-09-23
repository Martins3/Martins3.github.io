#include <stdio.h>
#include <numa.h>

int main(int argc, char *argv[]) { 
  printf("%d\n", numa_available());
  printf("%d\n" ,numa_max_possible_node());
  printf("%d\n", numa_num_possible_nodes());
  return 0; }
