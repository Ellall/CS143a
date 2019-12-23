// ulib.c and system call declarations and typedefs
#include "types.h"
#include "user.h"
#include "fcntl.h"

#define ECHO    1
#define GREP    2
#define CAT     3
#define OUTPUT  4
#define INPUT   5
#define INNOUT  6
#define PIPE    7 
#define WC      8


struct execcmd {
  int type;
  char argv[50];
  char eargv[50];
};

struct redircmd {
  int type;
  int exec_cmd_index;
  int redir_cmd_index;
  char left[50];
  char right[50];
};

struct pipecmd {
  int type;
  int eleft;
  int eright;
  int rleft;
  int rright;
};


// from Kernighan and Ritchie C book pg 103-104
void next_token(char** traveler, char* token){
  while (**traveler == ' ' || **traveler=='\t') (*traveler)++;  //ignores spaces or tabs in the beginning
  int i = 0;
  for (; **traveler!=' ' && **traveler!='\t' && **traveler!='\0' && **traveler!='\n'; ++(*traveler)){
    token[i++]=**traveler;
  }
  token[i] = '\0';
}

void remove_newline(char* str){
  char * shulk = strchr(str, '\n');
  if (shulk!=0){
    *shulk = '\0';
  }
}

void to_the_left(char delimiter, char * str, char* left){
  int i=0;
  while(*str != delimiter){
    left[i++]=*str;
    ++str;
  }
  left[--i] = '\0';
}


//return -1 for invalid exec command otherwise return index in exec array
int parse_exec(char *str, struct execcmd * exec, int * exec_i){
  char token[50];
  memset(token, 0, sizeof(token));
  next_token(&str, token);
  if (strcmp("echo", token)==0){
    ++str;
    exec[++(*exec_i)].type = ECHO;
    remove_newline(str);
    strcpy(exec[*exec_i].argv , str);
    return *exec_i;
  }
  else if (strcmp("grep", token)==0){
    exec[++(*exec_i)].type = GREP;
    next_token(&str, exec[*exec_i].argv);
    next_token(&str, exec[*exec_i].eargv);
    return *exec_i;
  }
  else if (strcmp("cat", token)==0){
    exec[++(*exec_i)].type = CAT;
    next_token(&str, exec[*exec_i].eargv);
    return *exec_i;
  }
  else if (strcmp("wc", token)==0){
    exec[++(*exec_i)].type = WC;
    next_token(&str, exec[*exec_i].eargv);
    return *exec_i;
  }
  return -1;
}

//return -1 for has input/output otherwise returns index in redir array
int parse_redir(char *str, struct redircmd * redir, int * redir_i, struct execcmd * exec, int * exec_i){
  char * input = strchr(str, '<');
  char * output = strchr(str, '>');
  if (output==0 && input==0){
    return -1;
  }
  if (input==0){
    redir[++(*redir_i)].type = OUTPUT;
    char left[strlen(str)-strlen(output)];
    to_the_left('>', str, left);
    output += 2; //right
    int p = parse_exec(left, exec, exec_i);
    if(p==-1){
      strcpy(redir[*redir_i].left, left);
      strcpy(redir[*redir_i].right, output);
    }
    else{
      redir[*redir_i].exec_cmd_index = p;
      strcpy(redir[*redir_i].right, output);
    }
    return *redir_i;
  }
  if (output==0){
    redir[++(*redir_i)].type = INPUT;
    char left[strlen(str)-strlen(input)];
    to_the_left('<', str, left);
    input += 2; //right
    int p = parse_exec(left, exec, exec_i);
    if(p==-1){
      strcpy(redir[*redir_i].left, left);
      strcpy(redir[*redir_i].right, input);
    }
    else{
      redir[*redir_i].exec_cmd_index = p;
      strcpy(redir[*redir_i].right, input);
    }
    return *redir_i;
  }
  
  redir[++(*redir_i)].type = INPUT;
  char left[strlen(str)-strlen(input)];
  to_the_left('<', str, left);
  int p = parse_exec(left, exec, exec_i);
  char middle[strlen(output)-strlen(input)];
  input += 2;
  to_the_left('>', input, middle);
  output += 2; //right
  if(p==-1){
    strcpy(redir[*redir_i].left, left);
    strcpy(redir[*redir_i].right, middle);
  }
  else{
    redir[*redir_i].exec_cmd_index = p;
    strcpy(redir[*redir_i].right, middle);
  }

  redir[++(*redir_i)].type = INNOUT;
  redir[*redir_i].redir_cmd_index = *redir_i - 1;
  strcpy(redir[*redir_i].right, output);

  return *redir_i;

}

// return 0 if invalid pipe inputs
// return -1 for no pipe
// return 1 for pipe
int pipe_her(char *str, struct pipecmd * pipe_struct, struct redircmd * redir, int * redir_i, struct execcmd * exec, int * exec_i){
  char * pipe_ptr = strchr(str, '|');
  if (pipe_ptr==0) return -1;

  pipe_struct->type = PIPE;
  char left[strlen(str)-strlen(pipe_ptr)];
  to_the_left('|', str, left);
  pipe_ptr += 2;

  int p = parse_redir(left, redir, redir_i, exec, exec_i);
  if (p!=-1){
    pipe_struct->rleft = p;
    pipe_struct ->eleft = -1;
  }
  else{
    p = parse_exec(left, exec, exec_i); 
    if (p!=-1){
      pipe_struct->eleft = p;
    }
    else{
      return 0;
    }
  }
  

  p = parse_redir(pipe_ptr, redir, redir_i, exec, exec_i);
  if (p!=-1){
    pipe_struct->rright = p;
    pipe_struct ->eright = -1;
  }
  else{
    p = parse_exec(pipe_ptr, exec, exec_i);
    if (p!=-1){
      pipe_struct->eright = p;
    }
    else{
      return 0;
    }
  }

  return 1;
}

//exec the execcmd
void run_exec(struct execcmd e){
  if (e.type==ECHO){
    char * args[] = {"echo", e.argv, 0};
    if (strcmp("", e.argv)!=0)
      exec("echo", args);
  }
  else if (e.type==GREP){
    char * args[] = {"grep", e.argv,  e.eargv, 0};
    exec("grep", args);
  }
  else if (e.type==WC){
    char * args[] = {"wc", e.eargv, 0};
    exec("wc", args);
  }
  else{
    char * args[] = {"cat", e.eargv, 0};
    exec("cat", args);
  }
  exit();
}

//exec the run_redir
void run_redir(struct redircmd * redir, int * redir_i, struct execcmd * execc){
  if (redir[*redir_i].type==OUTPUT){
    remove_newline(redir[*redir_i].right);
    close(1);
    open(redir[*redir_i].right, O_WRONLY|O_CREATE);
    run_exec(execc[redir[*redir_i].exec_cmd_index]);
    close(1);
  }
  else if (redir[*redir_i].type==INPUT){
    remove_newline(redir[*redir_i].right);
    strcpy(execc[redir[*redir_i].exec_cmd_index].eargv, redir[*redir_i].right);
    run_exec(execc[redir[*redir_i].exec_cmd_index]);
  }
  else{
    remove_newline(redir[*redir_i].right);
    close(1);
    open(redir[*redir_i].right, O_WRONLY|O_CREATE);
    run_redir(redir, &redir[*redir_i].redir_cmd_index, execc);
    close(1);
  }
  exit();
}

void run_pipe(struct pipecmd * pipe_struct, struct redircmd * redir, struct execcmd * execc){
  int p[2];
  pipe(p);
  if(fork() == 0){
    close(1);
    dup(p[1]);
    close(p[0]);
    close(p[1]);
    if(pipe_struct->eleft==-1){
      run_redir(redir, &(pipe_struct->rleft), execc);
    }
    else{
      run_exec(execc[pipe_struct->eleft]);
    }
  }
  if(fork() == 0){
    close(0);
    dup(p[0]);
    close(p[0]);
    close(p[1]);
    if (pipe_struct->eright==-1){
      if (fork()==0){
        close(1);
        open(".temp", O_WRONLY|O_CREATE);
        char b[100];
        while(strcmp(gets(b, sizeof(b)),"")!=0){
          printf(1, b);
        }
        close(1);
      }
      wait();
      strcpy(execc[redir[pipe_struct->rright].exec_cmd_index].eargv, ".temp");
      run_redir(redir, &(pipe_struct->rright), execc);
      unlink(".temp");
    }
    else{
      if(execc[pipe_struct->eright].type==CAT){
        run_exec(execc[pipe_struct->eleft]);
      }
      else if(execc[pipe_struct->eright].type==ECHO){
        run_exec(execc[pipe_struct->eright]);
      }
    }

  }
  close(p[0]);
  close(p[1]);
  wait();
  wait();
  exit();
}

int main(void){
  //open 3 file descriptors from sh.c
  int fd;
  while((fd = open("console", O_RDWR)) >= 0){
    if(fd >= 3){
      close(fd);
      break;
    }
  }
  char buf[100];
  while(1){
    printf(1, "@ ");
    memset(buf, 0, sizeof(buf));
    gets(buf, sizeof(buf));
    if(buf[0] == 0) break;

    int and = 0;
    if (strchr(buf, '&')!=0){
      *strchr(buf, '&') = '\0';
      and = 1;
    }

    if (and){
      if (fork()==0){
      char * buf_mover = buf;
      while (buf_mover!=0){

        char buf_ptr[100];
        if (strchr(buf_mover, ';')){
          to_the_left(';', buf_mover, buf_ptr);
          buf_mover = strchr(buf_mover, ';');
          buf_mover++;
        }
        else{
          strcpy(buf_ptr, buf_mover);
          buf_mover = 0;
        }

        struct pipecmd pipe_struct;
        struct redircmd redir_arr[3];
        struct execcmd exec_arr[5];

        int redir_arr_i = -1;
        int exec_arr_i = -1;

        pipe_struct.type=0;

        if (pipe_her(buf_ptr, &pipe_struct, redir_arr, &redir_arr_i, exec_arr, &exec_arr_i)==-1){
          if (parse_redir(buf_ptr, redir_arr, &redir_arr_i, exec_arr, &exec_arr_i)==-1){
            parse_exec(buf_ptr, exec_arr, &exec_arr_i);
          }
        }

        if (pipe_struct.type==PIPE){
          if(fork()==0){
            run_pipe(&pipe_struct, redir_arr, exec_arr);
          }
          wait();
        }
        else{
          if (redir_arr_i != -1){
            if (fork()==0){
              run_redir(redir_arr, &redir_arr_i, exec_arr);
            }
            wait();
          }
          else{ 
            if (exec_arr_i !=-1){
              if (fork()==0){
                run_exec(exec_arr[exec_arr_i]);
              }
              wait();
            }
            else{
              char * temp = buf;
              char token[50];
              memset(token, 0, sizeof(token));
              next_token(&temp, token);
              printf(2, "exec: fail\nexec ");
              printf(2, token);
              printf(2, " failed\n");
            }
          }
        }
      }
      exit();
      exit();
      }
    }
    else{

      char * buf_mover = buf;
      while (buf_mover!=0){

        char buf_ptr[100];
        if (strchr(buf_mover, ';')){
          to_the_left(';', buf_mover, buf_ptr);
          buf_mover = strchr(buf_mover, ';');
          buf_mover++;
        }
        else{
          strcpy(buf_ptr, buf_mover);
          buf_mover = 0;
        }

        struct pipecmd pipe_struct;
        struct redircmd redir_arr[3];
        struct execcmd exec_arr[5];

        int redir_arr_i = -1;
        int exec_arr_i = -1;

        pipe_struct.type=0;

        if (pipe_her(buf_ptr, &pipe_struct, redir_arr, &redir_arr_i, exec_arr, &exec_arr_i)==-1){
          if (parse_redir(buf_ptr, redir_arr, &redir_arr_i, exec_arr, &exec_arr_i)==-1){
            parse_exec(buf_ptr, exec_arr, &exec_arr_i);
          }
        }

        if (pipe_struct.type==PIPE){
          if(fork()==0){
            run_pipe(&pipe_struct, redir_arr, exec_arr);
          }
          wait();
        }
        else{
          if (redir_arr_i != -1){
            if (fork()==0){
              run_redir(redir_arr, &redir_arr_i, exec_arr);
            }
            wait();
          }
          else{ 
            if (exec_arr_i !=-1){
              if (fork()==0){
                run_exec(exec_arr[exec_arr_i]);
              }
              wait();
            }
            else{
              char * temp = buf;
              char token[50];
              memset(token, 0, sizeof(token));
              next_token(&temp, token);
              printf(2, "exec: fail\nexec ");
              printf(2, token);
              printf(2, " failed\n");
            }
          }
        }
      }
    }

  }
  exit();
}
