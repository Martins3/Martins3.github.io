#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void die(const char *what)
{
    perror(what);
    exit(EXIT_FAILURE);
}

static void show(const char *label)
{
    int tty = open("/dev/tty", O_RDWR | O_NOCTTY);
    int saved_errno = errno;
    char *input = ttyname(STDIN_FILENO);

    printf("%-20s PID=%ld PPID=%ld SID=%ld PGID=%ld\n", label,
           (long)getpid(), (long)getppid(), (long)getsid(0), (long)getpgrp());
    printf("  fd 0 terminal: %s\n", input ? input : "not a terminal");
    if (tty >= 0) {
        printf("  /dev/tty: available; foreground PGID=%ld\n",
               (long)tcgetpgrp(tty));
        close(tty);
    } else {
        printf("  /dev/tty: unavailable (%s)\n", strerror(saved_errno));
    }
}

int main(int argc, char **argv)
{
    setbuf(stdout, NULL);
    if (argc == 2 && strcmp(argv[1], "--after-exec") == 0) {
        show("after exec");
        return EXIT_SUCCESS;
    }
    show("parent");
    pid_t child = fork();
    if (child == -1)
        die("fork");
    if (child == 0) {
        show("child after fork");
        if (setsid() == -1)
            die("setsid");
        show("child after setsid");
        puts("  Notice: setsid() did not close fd 0/1/2.");
        execl("/proc/self/exe", "session.out", "--after-exec", (char *)NULL);
        die("execl");
    }
    int status;
    if (waitpid(child, &status, 0) == -1)
        die("waitpid");
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return EXIT_FAILURE;
    show("parent unchanged");
    return EXIT_SUCCESS;
}
