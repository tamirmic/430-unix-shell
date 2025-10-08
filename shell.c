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

  // remove trailing newline if present (fetchline already does that, but safe)
  size_t L = strlen(line);
  if (L > 0 && line[L - 1] == '\n')
    line[L - 1] = '\0';

  // simple tokenization on whitespace
  char *args[MAXLINE / 2 + 1];
  int argc = 0;
  char *saveptr = NULL;
  char *tok = strtok_r(line, " \t\r\n", &saveptr);
  while (tok != NULL && argc < (MAXLINE / 2)) {
    args[argc++] = tok;
    tok = strtok_r(NULL, " \t\r\n", &saveptr);
  }
  args[argc] = NULL;

  if (argc == 0)
    return; // blank line

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return;
  }
  if (pid == 0) {
    // child
    execvp(args[0], args);
    // if execvp returns, it failed
    fprintf(stderr, "%s: command not found or exec failed: %s\n", args[0],
            strerror(errno));
    _exit(1);
  } else {
    // parent: wait for child to finish
    int status;
    if (waitpid(pid, &status, 0) < 0) {
      perror("waitpid");
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
