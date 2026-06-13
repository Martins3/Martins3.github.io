#include <iostream>


struct Dog{
  int age;
  static int legs;
  virtual void get_age(){
    printf("Dog\n");
  }
};

int Dog::legs = 12;

class Sampel{
  virtual void get_age() = 0;
public:
  virtual void fuck(){
    printf("concrete method");
  }
};

class SampelSon: public Sampel{
public:
  void get_age(){
    printf("fuck");
  }
};

struct DDog{
  int age;
  virtual void get_age(){
    printf("Dog\n");
  }
};

struct G: public Dog, public DDog{
  int age;

  void get_age(){
    printf("G\n");
  }
};

void get_age(struct Dog * d){
  d->get_age();
}

using namespace std;
int main(int argc, const char *argv[]) {
  SampelSon a;
  a.get_age();
  a.fuck();
  Dog d;
  Dog p;
  cout << ++d.legs << endl;
  cout << p.legs << endl;
  return 0;
}
