#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

static const char *const commands[] = {
    "help", "echo", "history", "quit", NULL,
};

/* Readline repeatedly calls this function until it returns NULL.
 * Each match must be allocated: Readline owns and frees the returned string. */
static char *command_generator(const char *text, int state)
{
    static size_t index;

    if (state == 0)
        index = 0;

    while (commands[index]) {
        const char *command = commands[index++];

        if (strncmp(command, text, strlen(text)) == 0)
            return strdup(command);
    }

    return NULL;
}

static char **complete(const char *text, int start, int end)
{
    int first_word = 1;

    (void)end;
    for (int i = 0; i < start; i++) {
        if (!isspace((unsigned char)rl_line_buffer[i])) {
            first_word = 0;
            break;
        }
    }

    if (first_word) {
        /* Do not fall back to filenames for an unknown command. */
        rl_attempted_completion_over = 1;
        return rl_completion_matches(text, command_generator);
    }

    /* NULL with this flag unset enables Readline's filename completion. */
    rl_attempted_completion_over = 0;
    return NULL;
}

static void print_help(void)
{
    puts("Commands:\n"
         "  help          Show this help\n"
         "  echo TEXT     Print TEXT (arguments support filename completion)\n"
         "  history       Show this session's history\n"
         "  quit          Exit (or press Ctrl+D on an empty line)\n"
         "Editing: Left/Right, Up/Down, Ctrl+A/E, Ctrl+R, Tab.");
}

int main(void)
{
    char *line;

    setlocale(LC_ALL, "");
    rl_readline_name = "readline-demo";
    rl_attempted_completion_function = complete;
    using_history();
    stifle_history(100);

    puts("GNU Readline demo: type help, or try he<Tab>.");
    while ((line = readline("demo> ")) != NULL) {
        char *command = line;
        char *args;
        int should_quit = 0;

        while (isspace((unsigned char)*command))
            command++;
        if (!*command) {
            free(line);
            continue;
        }

        /* add_history copies the string; save it before splitting it. */
        add_history(line);
        args = command;
        while (*args && !isspace((unsigned char)*args))
            args++;
        if (*args)
            *args++ = '\0';
        while (isspace((unsigned char)*args))
            args++;

        if (strcmp(command, "help") == 0) {
            print_help();
        } else if (strcmp(command, "echo") == 0) {
            puts(args);
        } else if (strcmp(command, "history") == 0) {
            HIST_ENTRY **entries = history_list();

            for (int i = 0; entries && entries[i]; i++)
                printf("%4d  %s\n", history_base + i, entries[i]->line);
        } else if (strcmp(command, "quit") == 0) {
            should_quit = 1;
        } else {
            printf("Unknown command: %s (try help)\n", command);
        }

        /* readline returns allocated storage, without the final newline. */
        free(line);
        if (should_quit)
            break;
    }

    clear_history();
    puts("Bye!");
    return 0;
}
