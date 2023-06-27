#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <argp.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/wait.h>
#include <signal.h>
#include <argz.h>

// Enums for color and font style
enum color {BLACK, RED, GREEN, YELLOW, BLUE, PURPLE, CYAN, WHITE};
enum style {REGULAR = 0, BOLD = 1, UNDERLINE = 4};
#define RESET_COLOR "\e[0m"

// Macros for sizes of arrays and buffers
#define USER_LEN 32
#define CWD_LEN 256
#define CMD_PARSER_BUFSIZE 64
#define CMD_PARSER_DELIM " \t\n\r\a"
// #define QUOTE_PARSER_BUFSIZE 256

// Global variables
bool __STOP__ = false;
char __USER__[USER_LEN];
char __CWD__[CWD_LEN];
int __LAST_EXIT_STATUS__ = 0;

// Builtins
int byteshell_cd(int, char**);
int byteshell_help(int, char**);
int byteshell_exit(int, char**);
int byteshell_pwd(int, char**);
int byteshell_builtin(int, char**);
int byteshell_command(int, char**);
int byteshell_echo(int, char**);
int byteshell_enable(int, char**);

char* builtin_str[] = {
    "cd",
    "help",
    "exit",
    "pwd",
    "builtin",
    "command",
    "echo",
    "enable"
};

int (*builtin_func[]) (int, char**) = {
    &byteshell_cd,
    &byteshell_help,
    &byteshell_exit,
    &byteshell_pwd,
    &byteshell_builtin,
    &byteshell_command,
    &byteshell_echo,
    &byteshell_enable
};

bool is_builtin_enabled[] = {
    true,   // cd
    false,   // help
    true,   // exit
    true,   // pwd
    false,   // builtin
    false,   // command
    false,   // echo
    false    // enable
};

int num_builtins() {
    unsigned long int str_len = sizeof(builtin_str)/sizeof(char*), func_len = sizeof(builtin_func)/sizeof(int*), enabled_len = sizeof(is_builtin_enabled)/sizeof(bool);
    if((str_len==func_len)&&(str_len==enabled_len)) return str_len;
    else {
        fprintf(stderr, "buitin arrays not initialized properly [%lu %lu %lu]\n", str_len, func_len, enabled_len);
        exit(EXIT_FAILURE);
    }
}

// Function definitions
void sigint_handler(int);
void byteshell(void);
char* get_user(void);
char* style_font(int, int, char*);
char* read_line(void);
int exec_line(char*);
char* extract_command(char*);
int pipe_exec(char*);
char** split_command(char*, int*);
int launch(char**);
int exec_cmd(int, char**);
// struct token* extract_tokens_quotes(struct token*, int*);

// Variables for argp
// enum {NO_LOGIN = 500} arg_map;
const char *argp_program_version = "ByteShell v0.1";
static int parse_opt(int key, char *arg, struct argp_state *state) {
    switch(key) {
        case 'c':
            printf("%s\n",arg);
            break;
    }
    return 0;
}
static struct argp_option options[] = {
    {0, 'c', "COMMAND_STRING", 0, "Executes the command string"},
    { 0 }
};
static struct argp argp = {options, parse_opt, 0, 0};

void byteshell(void) {
    char *line;
    char **args;
    int status=1;

    if(getcwd(__CWD__, CWD_LEN) == NULL) {
        strcpy(__CWD__, "<unknown>");
    }

    do {
    printf("%s:%s > ", style_font(BOLD, GREEN, __USER__), style_font(BOLD, BLUE, __CWD__));
    line = read_line();
    status = exec_line(line);
    } while(status);
}

char* get_user(void) {  //getenv("USER") might have been easier but I like a challenge
    register struct passwd *pw;
    pw = getpwuid(geteuid());
    if(pw) {
        return pw->pw_name;
    }
    else {
        return "<unknown>";
    }
}

char* style_font(int style, int color, char *c) {
    int size = 300;
    char *ret = (char*)malloc(size * sizeof(char));
    if(!ret) return c;

    snprintf(ret, size, "\033[%d;3%dm%s\033[0m", style, color, c);
    return ret;
}

char* read_line(void) {
    static char* line;
    size_t bufsize;

    if(getline(&line, &bufsize, stdin) == -1) {
        if(feof(stdin)) {
            printf("\nGoodbye! :)\n");
            exit(EXIT_SUCCESS);
        } else {
            fprintf(stderr, "byteshell: Input error\n");
            exit(EXIT_SUCCESS);
        }
    }

    return line;
}

int exec_line(char* line) {
    char *command = extract_command(line);
    int status = 1;
    while(command!=NULL) {
        if(strstr(command,"|")) {
            status = pipe_exec(command);
            if(status!=1) return status;
        }
        else {
            int cmd_argc = 0;
            char **args = split_command(command, &cmd_argc);
            status = exec_cmd(cmd_argc, args);
            if(status!=1) return status;
        }
        command = extract_command(NULL);
    }
    return 1;
}

char* extract_command(char* line) {
    static char *save_ptr;
    char *command = strtok_r(line, ";", &save_ptr);
    return command;
}

int pipe_exec(char* command) {
    return 0;
}

char** split_command(char *command, int *num_tokens) {
    int bufsize = CMD_PARSER_BUFSIZE, pos = 0;
    char **tokens = malloc(bufsize*sizeof(char*)), *save_ptr;
    char* token;
    if(!tokens) {
        fprintf(stderr, "byteshell: Allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok_r(command, CMD_PARSER_DELIM, &save_ptr);
    while(token!=NULL) {
        tokens[pos++] = token;
        if(pos>=bufsize) {
            bufsize += CMD_PARSER_BUFSIZE;
            tokens = realloc(tokens, bufsize*sizeof(char*));
            if(!tokens) {
                fprintf(stderr, "byteshell: Allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok_r(NULL, CMD_PARSER_DELIM, &save_ptr);
    }
    *num_tokens = pos;
    tokens[pos] = NULL;
    return tokens;
}

int launch(char **args) {
    pid_t pid, wpid;
    int status;

    pid = fork();
    if(pid==0) {
        if(execvp(args[0], args) == -1) {
            perror("byteshell");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0) {
        perror("byteshell");
    } else {
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while(!WIFEXITED(status) && !WIFSIGNALED(status));
        __LAST_EXIT_STATUS__ = WEXITSTATUS(status);
    } 
    return 1;
}

int exec_cmd(int argc, char **argv) {
    if(argv[0]==NULL) {
        return 1;
    }
    for(int i=0; i<num_builtins(); i++) {
        if(strcmp(argv[0], builtin_str[i])==0 && is_builtin_enabled[i]) {
            return (*builtin_func[i])(argc, argv);
        }
    }
    return launch(argv);  
}

int byteshell_cd(int argc, char** argv) {
    if(argc==1) {
        if(chdir(getenv("HOME"))<0) {
            fprintf(stderr, "byteshell: cd: %s: No such file or directory\n", getenv("HOME"));
            __LAST_EXIT_STATUS__ = 1;
            return 1;
        }
        else {
            if(getcwd(__CWD__, CWD_LEN) == NULL) {
                strcpy(__CWD__, "<unknown>");
            }
            __LAST_EXIT_STATUS__ = 0;
            return 1;
        }
    }
    if(!strcmp(argv[1], "--help")) {
        printf("Usage: cd [--help|DIR]\n\
Change the shell current working directory.\n\
\n\
Change the current directory to DIR. The default DIR is the value of the \n\
HOME shell variable.\n\
\n\
OPTIONS:\n\
--help \t Prints this help menu\n");
        __LAST_EXIT_STATUS__ = 0;
        return 1;
    }

    if(argv[1][0]=='-') {
        fprintf(
            stderr, 
            "byteshell: cd: %s: invalid option \n\
Usage: cd [--help|DIR]\n", 
            argv[1]
        );
        __LAST_EXIT_STATUS__ = 1;
        return 1;
    }
    if(argc>2) {
        fprintf(stderr, "byteshell: cd: too many arguments\n");
        __LAST_EXIT_STATUS__ = 1;
        return 1;
    }

    if(chdir(argv[1])<0) {
        fprintf(stderr, "byteshell: cd: %s: No such file or directory\n", argv[1]);
        __LAST_EXIT_STATUS__ = 1;
        return 1;
    }
    else {
        if(getcwd(__CWD__, CWD_LEN) == NULL) {
            strcpy(__CWD__, "<unknown>");
        }
        __LAST_EXIT_STATUS__ = 0;
        return 1;
    }
    __LAST_EXIT_STATUS__ = 1;
    return 1;
}

int byteshell_help(int argc, char** argv) {
    return 0;
}

int byteshell_exit(int argc, char** argv) {
    if(argc>2) {
        fprintf(stderr, "byteshell: exit: too many arguments\n");
        __LAST_EXIT_STATUS__ = 1;
        return 1;
    }
    else if(argc==1) {
        if(__LAST_EXIT_STATUS__==EXIT_SUCCESS) printf("Goodbye! :)\n");
        exit(__LAST_EXIT_STATUS__);
    }
    bool is_arg_num = true;
    for(int i=0; i<strlen(argv[1]); i++) {
        is_arg_num = is_arg_num && isdigit(argv[1][i]);
    }
    if(!is_arg_num) {
        fprintf(stderr, "byteshell: exit: invalid argument\n");
        __LAST_EXIT_STATUS__ = 1;
        return 1;
    }
    if(atoi(argv[1])==EXIT_SUCCESS) printf("Goodbye! :)\n");
    exit(atoi(argv[1]));
    return 0;
}

int byteshell_pwd(int argc, char** argv) {
    if(!strcmp(__CWD__, "<unknown>")) {
        fprintf(stderr, "byteshell: pwd: error getting current directory\n");
        __LAST_EXIT_STATUS__ = 1;
        return 1;
    }
    else {
        printf("%s\n",__CWD__);
        __LAST_EXIT_STATUS__ = 0;
        return 1;
    }
}

int byteshell_builtin(int argc, char** argv) {
    return 0;
}

int byteshell_command(int argc, char** argv) {
    return 0;
}

int byteshell_echo(int argc, char** argv) {
    return 0;
}

int byteshell_enable(int argc, char** argv) {
    return 0;
}

// struct token* extract_tokens_quotes(struct token *tokens, int *sz) {
//     int ret_tokens_idx = 0, token_idx = 0;
//     bool in_quotes = false;
//     struct token *ret_tokens = (struct token*)malloc(*sz * sizeof(struct token));
//     char quote, *token = (char*)malloc(QUOTE_PARSER_BUFSIZE*sizeof(char));
//     for(int i=0; i<strlen(tokens[0].token_str); i++) {
//         if(!in_quotes && (tokens[0].token_str[i] == '"'||tokens[0].token_str[i] == '\'')) {
//             if(i!=0) {
//                 if(token_idx>=strlen(token)) token = realloc(token, (strlen(token)+QUOTE_PARSER_BUFSIZE)*sizeof(char));
//                 token[token_idx] = '\0';
//                 if(ret_tokens_idx>=*sz) ret_tokens = realloc(ret_tokens, ++(*sz)*sizeof(struct token));
//                 ret_tokens[ret_tokens_idx++].is_in_quotes = in_quotes;
//                 ret_tokens[ret_tokens_idx++].token_str = token;
//                 free(token);
//                 token = (char*)malloc(QUOTE_PARSER_BUFSIZE*sizeof(char));
//                 token_idx = 0;
//             }
//             in_quotes = true;
//             quote = tokens[0].token_str[i];
//         }
//         else if(in_quotes && tokens[0].token_str[i]==quote) {
//             if(token_idx>=strlen(token)) token = realloc(token, (strlen(token)+QUOTE_PARSER_BUFSIZE)*sizeof(char));
//             token[token_idx] = '\0';
//             if(ret_tokens_idx>=*sz) ret_tokens = realloc(ret_tokens, ++(*sz)*sizeof(struct token));
//             ret_tokens[ret_tokens_idx++].is_in_quotes = in_quotes;
//             ret_tokens[ret_tokens_idx++].token_str = token;
//             free(token);
//             token = (char*)malloc(QUOTE_PARSER_BUFSIZE*sizeof(char));
//             token_idx = 0;
//             in_quotes = false;
//         }
//         else if(i==strlen(tokens[0].token_str)-1) {
//             if(token_idx>=strlen(token)) token = realloc(token, (strlen(token)+QUOTE_PARSER_BUFSIZE)*sizeof(char));
//             token[token_idx] = '\0';
//             if(ret_tokens_idx>=*sz) ret_tokens = realloc(ret_tokens, ++(*sz)*sizeof(struct token));
//             token_init(&ret_tokens[ret_tokens_idx++], token, in_quotes);
//             free(token);
//         }
//         else {
//             if(token_idx>=strlen(token)) token = realloc(token, (strlen(token)+QUOTE_PARSER_BUFSIZE)*sizeof(char));
//             token[token_idx++] = tokens[0].token_str[i];
//         }
//     }

//     // for(int i=0; i<inp_size; i++) {
//     //     if(tokens[i].is_in_quotes) continue;
//     //     if(tokens[i].token_str[0] == quote) in_quotes = true;
//     //     char *token = strtok(tokens[i].token_str, &quote);
//     //     while(token!=NULL){
//     //         if(idx >= *sz) ret_tokens = realloc(ret_tokens, ++(*sz) * sizeof(struct token));
//     //         if(idx>0 && in_quotes && ret_tokens[idx-1].token_str[strlen(ret_tokens[idx-1].token_str)-1] == ' ') {
//     //             char *temp = (char*)malloc(2 * sizeof(char));
//     //             strcpy(temp, " ");
//     //             token = strcat(temp, token);
//     //         } 
//     //         if(idx>0 && !in_quotes && token[0] == ' ') ret_tokens[idx-1].token_str = strcat(ret_tokens[idx-1].token_str, " ");
//     //         token_init(&ret_tokens[idx++], token, in_quotes);
//     //         in_quotes = !in_quotes;
//     //         token = strtok(NULL, &quote);
//     //     }
//     // }
//     if(in_quotes) {
//         free(ret_tokens);
//         return NULL;
//     }
//     for(int i=0; i<*sz; i++) {
//         printf("%s  ", ret_tokens[i].token_str);
//     }
//     return ret_tokens;
// }

void sigint_handler(int sig_num) {
    signal(SIGINT, sigint_handler);
    printf("\n");
    printf("%s:%s > ", style_font(BOLD, GREEN, __USER__), style_font(BOLD, BLUE, __CWD__));
    fflush(stdout);
}

int main(int argc, char **argv) {
    num_builtins();
    signal(SIGINT, sigint_handler);
    argp_parse(&argp, argc, argv, 0, 0, 0);
    strcpy(__USER__, get_user());

    byteshell();

    return EXIT_SUCCESS;
}