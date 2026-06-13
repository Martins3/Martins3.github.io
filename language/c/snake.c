// 用C语言，能在100行之内实现贪吃蛇吗？ - 黄亮anthony的回答 - 知乎
// https://www.zhihu.com/question/360814879/answer/1009064057
#include <locale.h>  /* setlocale, 使用特殊字符 */
#include <ncurses.h> /* 绘图 */
#include <stdlib.h>  /* 随机数 */
#define DRAW(pos, c) !mvprintw(pos / W, pos % W * 2, c)
int const W = 30, H = 30, MSIZE = W * H,
          MoveHints[4] = {W, 2 * W + 1, 2 + W, 1};
int Board[MSIZE] = {0}, Snake[MSIZE] = {W / 2 + H * W / 2};
int main() { /* 使用特殊字符，初始化屏幕, 隐藏光标 使用光标键控制, 不缓冲按键,
                随机种子 */
  setlocale(0, ""), initscr(), curs_set(0), keypad(stdscr, TRUE),
      nodelay(stdscr, TRUE), srand(0);
  int food = Snake[0] - 1, h = 0, t = 0, x = 0, moveDirection = 0;
  for (int i = 0, j = 1; i != MSIZE;
       i += j, j = (i < W || i >= MSIZE - W) ? 1 : W - j)
    Board[i] = DRAW(i, "■");
  do {
    int key = (KEY_LEFT == x) + (KEY_RIGHT == x) * 3 + (KEY_UP == x) * 4 +
              (KEY_DOWN == x) * 2;
    if (key && (key - 1 & 1) != (moveDirection & 1))
      moveDirection = key - 1;
    int next = Snake[h] + MoveHints[moveDirection] - W - 1;
    if (Board[next])
      break; /* end game */
    Board[Snake[h = (h == MSIZE - 1) ? 0 : h + 1] = next] = DRAW(next, "■");
    if (next != food)
      Board[Snake[t]] = !DRAW(Snake[t], " "), t = (t == MSIZE - 1) ? 0 : t + 1;
    while (Board[food] || !DRAW(food, "◆"))
      food = rand() / (RAND_MAX / ((W - 1) * (H - 1))) + W + 1;
    timeout(50); /* 等待按键的最大时间 */
  } while ((x = getch()) != 'q');
  endwin();
}
