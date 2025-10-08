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
  if (line == NULL)
    return;

  static char last_command[MAXLINE] = "";

  // Handle !! history
  char current_line[MAXLINE];
  if (equal(line, "!!")) {
    if (equal(last_command, "")) {
      printf("No commands in history.\n");
      return;
    } else {
      printf("%s\n", last_command);
      strncpy(current_line, last_command, MAXLINE);
    }
  } else {
    strncpy(current_line, line, MAXLINE);
    strncpy(last_command, line, MAXLINE); // save history
  }

  size_t L = strlen(current_line);
  if (L > 0 && current_line[L - 1] == '\n')
    current_line[L - 1] = '\0';

  // --- Parse command segments separated by ';' or '&' ---
  char *p = current_line;
  while (*p != '\0') {
    // Skip leading spaces
    while (*p == ' ' || *p == '\t')
      p++;
    if (*p == '\0')
      break;

    // Find the end of this command (either ';', '&', or end of string)
    char *start = p;
    bool background = false;
    while (*p != '\0' && *p != ';' && *p != '&')
      p++;

    // Mark end of command and check for background symbol
    char sep = *p;
    if (*p != '\0') {
      *p = '\0';
      p++;
      if (sep == '&')
        background = true;
    }

    // Trim trailing spaces
    char *end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t'))
      *end-- = '\0';

    // Skip empty commands
    if (*start == '\0')
      continue;

    // --- Tokenize this command ---
    char *args[MAXLINE / 2 + 1];
    int argc = 0;
    char *saveptr = NULL;
    char *token = strtok_r(start, " \t\r\n", &saveptr);
    while (token != NULL && argc < (MAXLINE / 2)) {
      args[argc++] = token;
      token = strtok_r(NULL, " \t\r\n", &saveptr);
    }
    args[argc] = NULL;
    if (argc == 0)
      continue;

    // --- Fork and exec ---
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      return;
    }
    if (pid == 0) {
      execvp(args[0], args);
      fprintf(stderr, "%s: command not found or exec failed: %s\n", args[0],
              strerror(errno));
      _exit(1);
    } else {
      if (!background) {
        int status;
        if (waitpid(pid, &status, 0) < 0)
          perror("waitpid");
      } else {
        printf("[bg] %d\n", pid);
      }
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
