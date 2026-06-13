typedef struct Student {
  int age;
} S;

void Student() {} // 正确，定义后 "Student" 只代表此函数

// void S() {} // 错误，符号 "S" 已经被定义为一个 "struct Student" 的别名

int main() {
  Student();
  struct Student me; // 或者 "S me";
  return 0;
}
