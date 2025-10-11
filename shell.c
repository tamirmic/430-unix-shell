#include "shell.h"

int main(int argc, char **argv) {
  if (argc == 2 && equal(argv[1], "--interactive")) {
    return interactiveShell();
  } else {
    return runTests();
  }
}

// interactive shell to process commands
int interactiveShell() {
  bool should_run = true;
  char *line = calloc(1, MAXLINE);
  while (should_run) {
    printf(PROMPT);
    fflush(stdout);
    int n = fetchline(&line);
    printf("read: %s (length = %d)\n", line, n);
    // ^D results in n == -1
    if (n == -1 || equal(line, "exit")) {
      should_run = false;
      continue;
    }
    if (equal(line, "")) {
      continue;
    }
    processLine(line);
  }
  free(line);
  return 0;
}

void processLine(char *line) {
  if (line == NULL) {
    return;
  }

  static char last_command[MAXLINE] = "";
  char current_line[MAXLINE];

  // Handle !!
  if (equal(line, "!!")) {
    if (equal(last_command, "")) {
      printf("No commands in history.\n");
      return;
    }
    printf("%s\n", last_command);
    strncpy(current_line, last_command, MAXLINE);
  } else {
    strncpy(current_line, line, MAXLINE);
    strncpy(last_command, line, MAXLINE);
  }

  size_t L = strlen(current_line);
  if (L > 0 && current_line[L - 1] == '\n') {
    current_line[L - 1] = '\0';
  }

  // Split by ';' or '&'
  char *p = current_line;
  while (*p != '\0') {
    while (*p == ' ' || *p == '\t') {
      p++;
    }
    if (*p == '\0') {
      break;
    }
    char *start = p;
    bool background = false;
    while (*p != '\0' && *p != ';' && *p != '&') {
      p++;
    }
    char sep = *p;
    if (*p != '\0') {
      *p = '\0';
      p++;
      if (sep == '&') {
        background = true;
      }
    }

    char *end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t')) {
      *end-- = '\0';
    }
    if (*start == '\0') {
      continue;
    }
    // Split by pipes '|'
    char *commands[32];
    int num_cmds = 0;
    char *saveptr = NULL;
    char *token = strtok_r(start, "|", &saveptr);
    while (token && num_cmds < 32) {
      while (*token == ' ' || *token == '\t') {
        token++;
      }
      commands[num_cmds++] = token;
      token = strtok_r(NULL, "|", &saveptr);
    }

    int prev_fd = -1;
    pid_t pids[32];

    for (int i = 0; i < num_cmds; i++) {
      int pipefd[2];
      if (i < num_cmds - 1 && pipe(pipefd) < 0) {
        perror("pipe");
        return;
      }

      // Tokenize all the arguments
      char *args[MAXLINE / 2 + 1];
      int argc = 0;
      char *in_file = NULL, *out_file = NULL;
      char *tok, *saveptr2 = NULL;
      tok = strtok_r(commands[i], " \t\r\n", &saveptr2);
      while (tok && argc < MAXLINE / 2) {
        if (equal(tok, "<")) {
          in_file = strtok_r(NULL, " \t\r\n", &saveptr2);
        } else if (equal(tok, ">")) {
          out_file = strtok_r(NULL, " \t\r\n", &saveptr2);
        } else {
          args[argc++] = tok;
        }
        tok = strtok_r(NULL, " \t\r\n", &saveptr2);
      }
      args[argc] = NULL;
      if (argc == 0) {
        continue;
      }
      pid_t pid = fork();
      if (pid < 0) {
        perror("fork");
        return;
      }

      if (pid == 0) {
        // Handle input redirection
        if (in_file) {
          int fd = open(in_file, O_RDONLY);
          if (fd < 0) {
            perror(in_file);
            _exit(1);
          }
          dup2(fd, STDIN_FILENO);
          close(fd);
        }

        // Handle output redirection
        if (out_file) {
          int fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
          if (fd < 0) {
            perror(out_file);
            _exit(1);
          }
          dup2(fd, STDOUT_FILENO);
          close(fd);
        }

        // Pipe input
        if (prev_fd != -1) {
          dup2(prev_fd, STDIN_FILENO);
          close(prev_fd);
        }

        // Pipe output
        if (i < num_cmds - 1) {
          close(pipefd[0]);
          dup2(pipefd[1], STDOUT_FILENO);
          close(pipefd[1]);
        }

        execvp(args[0], args);
        fprintf(stderr, "%s: command failed: %s\n", args[0], strerror(errno));
        _exit(1);
      } else {
        pids[i] = pid;
        if (prev_fd != -1) {
          close(prev_fd);
        }
        if (i < num_cmds - 1) {
          close(pipefd[1]);
          prev_fd = pipefd[0];
        }
      }
    }

    if (!background) {
      for (int i = 0; i < num_cmds; i++) {
        waitpid(pids[i], NULL, 0);
      }
    } else {
      printf("[bg] %d\n", pids[num_cmds - 1]);
    }
  }
}

int runTests() {
  printf("*** Running basic tests ***\n");
  char lines[7][MAXLINE] = {
      "ls",      "ls -al", "ls & whoami ;", "ls > junk.txt", "cat < junk.txt",
      "ls | wc", "ascii"};
  for (int i = 0; i < 7; i++) {
    printf("* %d. Testing %s *\n", i + 1, lines[i]);
    processLine(lines[i]);
  }

  return 0;
}

// return true if C-strings are equal
bool equal(char *a, char *b) { return (strcmp(a, b) == 0); }

// read a line from console
// return length of line read or -1 if failed to read
// removes the \n on the line read
int fetchline(char **line) {
  size_t len = 0;
  size_t n = getline(line, &len, stdin);
  if (n > 0) {
    (*line)[n - 1] = '\0';
  }
  return n;
}
