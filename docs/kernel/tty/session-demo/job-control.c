#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 700
#include <errno.h>
#include <pty.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

static void die(const char *what)
{
    perror(what);
    exit(EXIT_FAILURE);
}

static void require(int condition, const char *what)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", what);
        exit(EXIT_FAILURE);
    }
}

static void disposition(int signo, void (*handler)(int))
{
    struct sigaction action = { .sa_handler = handler };
    sigemptyset(&action.sa_mask);
    if (sigaction(signo, &action, NULL) == -1)
        die("sigaction");
}

static void send_signal(pid_t pid, int signo)
{
    if (kill(pid, signo) == -1)
        die("kill");
}

static int wait_for(pid_t child, int flags)
{
    int status;
    pid_t result;
    do {
        result = waitpid(child, &status, flags);
    } while (result == -1 && errno == EINTR);
    if (result == -1)
        die("waitpid");
    return status;
}

static void show(const char *label)
{
    printf("%s: PID=%ld SID=%ld PGID=%ld TPGID=%ld TTY=%s\n", label,
           (long)getpid(), (long)getsid(0), (long)getpgrp(),
           (long)tcgetpgrp(STDIN_FILENO), ttyname(STDIN_FILENO));
}

/* This child is the job. Only it tries reading the private terminal. */
static void worker(void)
{
    disposition(SIGTTOU, SIG_DFL);
    if (setpgid(0, 0) == -1)
        die("setpgid");
    show("worker / background");
    /* A stop/wait handshake ensures setpgid completes before the experiment. */
    raise(SIGSTOP);
    char input[80];
    ssize_t count = read(STDIN_FILENO, input, sizeof(input) - 1);
    if (count <= 0)
        die("worker read");
    input[count] = '\0';
    printf("worker read: %s", input);
    require(strcmp(input, "hello from PTY master\n") == 0, "input contents");
    show("worker / foreground");
    puts("[interrupt] worker is ready for Ctrl+C");
    for (;;)
        pause();
}

/* forkpty establishes a new session and a new controlling terminal here. */
static void session_controller(void)
{
    sigset_t empty;
    sigemptyset(&empty);
    if (sigprocmask(SIG_SETMASK, &empty, NULL) == -1)
        die("sigprocmask");
    disposition(SIGINT, SIG_DFL);
    disposition(SIGTTIN, SIG_DFL);
    disposition(SIGCHLD, SIG_DFL);
    disposition(SIGALRM, SIG_DFL);
    /* Like a shell, the controller must reclaim the tty while background. */
    disposition(SIGTTOU, SIG_IGN);
    alarm(10);
    struct termios settings;
    if (tcgetattr(STDIN_FILENO, &settings) == -1)
        die("tcgetattr");
    settings.c_lflag |= ISIG | ICANON;
    settings.c_lflag &= ~(ECHO | TOSTOP);
    settings.c_iflag |= ICRNL;
    settings.c_cc[VINTR] = 3;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &settings) == -1)
        die("tcsetattr");
    show("controller / foreground");
    pid_t child = fork();
    if (child == -1)
        die("fork");
    if (child == 0)
        worker();

    int status = wait_for(child, WUNTRACED);
    require(WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP,
            "worker startup handshake");
    puts("1. Resume worker in BACKGROUND; it attempts read(tty).");
    send_signal(child, SIGCONT);
    status = wait_for(child, WUNTRACED);
    require(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTTIN,
            "background read must stop with SIGTTIN");
    puts("   PASS: kernel stopped worker with SIGTTIN.");

    puts("2. tcsetpgrp(tty, worker_pgid), then SIGCONT (like shell fg).");
    if (tcsetpgrp(STDIN_FILENO, child) == -1)
        die("tcsetpgrp worker");
    show("controller / now background");
    send_signal(child, SIGCONT);
    puts("[input] terminal is ready for a line of input");

    status = wait_for(child, 0);
    require(WIFSIGNALED(status) && WTERMSIG(status) == SIGINT,
            "Ctrl+C must terminate foreground worker with SIGINT");
    puts("3. PASS: terminal delivered SIGINT to foreground worker; controller survived.");
    if (tcsetpgrp(STDIN_FILENO, getpgrp()) == -1)
        die("tcsetpgrp controller");
    show("controller / foreground again");
    puts("PASS: job control complete; worker reaped, private PTY will close.");
    exit(EXIT_SUCCESS);
}

static void write_all(int fd, const char *data, size_t size)
{
    while (size) {
        ssize_t count = write(fd, data, size);
        if (count == -1 && errno == EINTR)
            continue;
        if (count <= 0)
            die("write");
        data += count;
        size -= (size_t)count;
    }
}

int main(void)
{
    setbuf(stdout, NULL);
    int master;
    pid_t controller = forkpty(&master, NULL, NULL, NULL);
    if (controller == -1)
        die("forkpty");
    if (controller == 0)
        session_controller();

    /* The outer process acts as a terminal emulator, supplying bytes only.
     * In particular it never uses kill(SIGINT) to simulate Ctrl+C. */
    char output[8192] = {0};
    size_t used = 0;
    int input_sent = 0, interrupt_sent = 0;
    for (;;) {
        require(used < sizeof(output) - 1, "transcript fits buffer");
        ssize_t count = read(master, output + used, sizeof(output) - used - 1);
        if (count == -1 && errno == EINTR)
            continue;
        /* Linux PTY master returns EIO after the last slave fd closes. */
        if (count == 0 || (count == -1 && errno == EIO))
            break;
        if (count == -1)
            die("read master");
        write_all(STDOUT_FILENO, output + used, (size_t)count);
        used += (size_t)count;
        output[used] = '\0';
        if (!input_sent && strstr(output, "[input]")) {
            const char text[] = "hello from PTY master\n";
            write_all(master, text, sizeof(text) - 1);
            input_sent = 1;
        }
        if (!interrupt_sent && strstr(output, "[interrupt]")) {
            puts("PTY master: writing byte 0x03 (Ctrl+C).");
            write_all(master, "\003", 1);
            interrupt_sent = 1;
        }
    }
    close(master);
    int status = wait_for(controller, 0);
    require(WIFEXITED(status) && WEXITSTATUS(status) == 0,
            "controller completed successfully");
    require(input_sent && interrupt_sent, "both terminal inputs sent");
    return EXIT_SUCCESS;
}
