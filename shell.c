#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"

int main(int argc, char **argv)
{
  // Load config files, if any

  // Run command loop
  lsh_loop();

  // Perform any shutdown/cleanup

  return EXIT_SUCCESS;
}

void lsh_loop(void)
{
  char *line;
  char **args;
  int  status;

  do {
    printf("> ");
    line   = lsh_read_line(); // Read
    args   = lsh_split_line(line); // Parse
    status = lsh_execute(args); // Execute
    
    free(line);
    free(args);
  }while(status);
}

#define LSH_RL_BUF_SIZE 1024
char *lsh_read_line(void)
{
  int buf_size = LSH_RL_BUF_SIZE;
  int position = 0;
  char *buffer = (char*)malloc(sizeof(char)*buf_size);
  int c;

  if(!buffer){
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while(1){
    // Read a character
    c = getchar();

    // If we hit EOF, replace it with a null character and return
    if(c == EOF || c == '\n'){
      buffer[position] = '\0';
      return buffer;
    }else{
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate
    if(position >= buf_size){
      buf_size += LSH_RL_BUF_SIZE;
      buffer = (char*)realloc(buffer, buf_size);
      if(!buffer){
        fprintf(stderr, "lsh: allocation error\n");
	exit(EXIT_FAILURE);
      }
    }
  }
}

#define LSH_TOK_BUF_SIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
char **lsh_split_line(char *line)
{
  int buf_size = LSH_TOK_BUF_SIZE;
  int position = 0;
  char **tokens = (char**)malloc(sizeof(char*)*buf_size);
  char *token;

  if(!tokens){
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while(token != NULL){
    tokens[position] = token;
    position++;

    if(position >= buf_size){
      buf_size += LSH_TOK_BUF_SIZE;
      tokens = (char**)realloc(tokens, sizeof(char*)*buf_size);
      if(!tokens){
        fprintf(stderr, "lsh: allocation error\n");
	exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }

  tokens[position] = NULL;
  return tokens;
}

int lsh_launch(char **args)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  if(pid == 0){
    // Child process
    if(execvp(args[0], args) == -1){
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  }else if(pid < 0){
    // Error forking
    perror("lsh");
  }else{
    // Parent process
    do{
      wpid = waitpid(pid, &status, WUNTRACED);
    }while(!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/*
 * Function Declarations for buildint shell commands
 */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

/*
 * List of buildin commands, followed by their corresponding functions
 */
char *buildin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*buildin_func[])(char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit
};

int lsh_num_buildins()
{
  return sizeof(buildin_str)/sizeof(char*);
}

/*
 * Buildin function implementations
 */
int lsh_cd(char **args)
{
  if(args[1] == NULL){
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  }else{
    if(chdir(args[1]) != 0){
      perror("lsh");
    }
  }
  return 1;
}

int lsh_help(char **args)
{
  int i;
  printf("MY LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are build in:\n");

  for(int i=0; i<lsh_num_buildins(); i++){
    printf("  %s\n", buildin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int lsh_exit(char **args)
{
  return 0;
}

int lsh_execute(char **args)
{
  int i;

  if(args[0] == NULL){
    // An empty command was entered.
    return 1;
  }

  for(i=0; i<lsh_num_buildins(); i++){
    if(strcmp(args[0], buildin_str[i]) == 0){
      return (*buildin_func[i])(args);
    }
  }

  return lsh_launch(args);
}
