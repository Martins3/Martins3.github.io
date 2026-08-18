#include <GL/glut.h>
#include <stdio.h>

void display()
{
	glClear(GL_COLOR_BUFFER_BIT);

	glBegin(GL_TRIANGLES);
	glColor3f(1.0f, 0.0f, 0.0f); // Red
	glVertex2f(0.0f, 0.5f);
	glColor3f(0.0f, 1.0f, 0.0f); // Green
	glVertex2f(-0.5f, -0.5f);
	glColor3f(0.0f, 0.0f, 1.0f); // Blue
	glVertex2f(0.5f, -0.5f);
	glEnd();

	glFlush(); // 确保所有OpenGL命令立即执行
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);

   // 只请求 RGBA 和单缓冲，不指定任何 OpenGL 版本
    glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
    glutInitWindowSize(500, 500); // 设置窗口大小，有时能帮助避免FBConfig问题
    // ===================================================

    glutCreateWindow("Mesa OpenGL Demo - Simple Triangle");
    glutDisplayFunc(display);

    printf("✅ OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("✅ Renderer: %s\n", glGetString(GL_RENDERER));

    glutMainLoop();
    return 0;
}
