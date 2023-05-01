#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>


#define MAX_TOKENS 100
#define MAX_TOKEN_LENGTH 100

void red() {
    printf("\033[1;31m");
}

void Help(char *keyword) {
    red();
    if (keyword == NULL) {
        printf("Integrantes: David Barroso Rey, Juan José Muñóz Noda (C311)\n");
        printf("Funcionalidades:\n");
        printf("·Basic\n·Spaces\n·Help");
        printf("Funciónamiento General del Shell:\n");
        printf("Observación: Los comandos deben estar delimitados de al menos 1 espacio.");
        printf("Se obtiene la línea tecleada por el usuario utilizando fgets()\n");
        printf("Los comandos built-in se ejecutan mediante la función execv\n");

    } else if (strcmp(keyword, "spaces")) {
        printf("Mediante la función strtok se crea un array de punteros\n"
               "que almacena en cada uno de ellos las palabras separadas por espacios(se eliminan los espacios, en otras palabras,\n"
               "\n no importan los espacios demás\n");
    } else if (strcmp(keyword, "basic")) {
        printf("La función cd fue implementada utilizando la función de chdir()\n\n");

        printf("La función exit solamente hace un break en el método main lo cual termina el programa\n\n");
        printf("Para implementar los comandos \"<,>,>> \"se hace uso de la función open para abrir el archivo especificado y\n"
               "se le pasan los flags correspondientes en depencia del operador");
        printf("Para la implementación de pipes se crean dos procesos hijos, el primero se encargará de ejecutar\n"
               "el primer comando y pasar su salida a través del pipe al segundo y, finalmente, ejecuta el segundo comando.");
        printf("Si existen comentarios, o sea, si existe el símbolo # y se sustituirá con el escape de fin del\n"
               "string \\0 para definir que el fin de la cadena es en ese símbolo(también la última posición de la cadena\n"
               "que tiene \n es sustituido con el escape anterior)\n");

    }
}

void DirList() {
    DIR *dir;
    struct dirent *ent;
    dir = opendir(".");
    while ((ent = readdir(dir)) != NULL) {
        printf("%s ", ent->d_name);
    }
    closedir(dir);
    printf("\n");
}


void yellow() {
    printf("\033[1;33m");
}

void blue() {
    printf("\033[0;34m");
}


void print_prompt() {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    yellow();
    printf("%s $ ", cwd);
    blue();
}

void execute_command(char **tokens, int num_tokens) {
    int i;
    int input_fd = STDIN_FILENO;
    int output_fd = STDOUT_FILENO;
    char *input_file = NULL;
    char *output_file = NULL;
    int append_output = 0;
    int pipe_index = -1;


    for (i = 0; i < num_tokens; i++) {
        if (strcmp(tokens[i], ">") == 0) {
            // Output redirection
            output_file = tokens[i + 1];
            output_fd = open(output_file, O_WRONLY | O_CREAT | (append_output ? O_APPEND : O_TRUNC), 0666);
            if (output_fd == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }

            if (strcmp(tokens[i - 1], "dir") == 0) {

                dup2(output_fd, STDOUT_FILENO);


                DirList();


                close(output_fd);


                freopen("/dev/tty", "w", stdout);
            }


            break;


        } else if (strcmp(tokens[i], ">>") == 0) {
            // Output redirection (append)
            output_file = tokens[i + 1];
            append_output = 1;
            output_fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
            if (output_fd == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }

            if (strcmp(tokens[i - 1], "dir") == 0) {

                dup2(output_fd, STDOUT_FILENO);


                DirList();


                close(output_fd);


                freopen("/dev/tty", "w", stdout);
            }

            break;
        } else if (strcmp(tokens[i], "<") == 0) {
            // Input redirection
            input_file = tokens[i + 1];
            input_fd = open(input_file, O_RDONLY);
            if (input_fd == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }
            break;
        } else if (strcmp(tokens[i], "|") == 0) {
            // Pipe
            pipe_index = i;
            break;
        }
    }

    if (pipe_index != -1) {
        // Create pipe
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }


        pid_t pid1 = fork();
        if (pid1 == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid1 == 0) {
            // Child process 1
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);

            // Execute first command
            tokens[pipe_index] = NULL;
            execvp(tokens[0], tokens);
            perror("execvp");
            exit(EXIT_FAILURE);
        }


        pid_t pid2 = fork();
        if (pid2 == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid2 == 0) {
            // Child process 2
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);


            execvp(tokens[pipe_index + 1], &tokens[pipe_index + 1]);
            perror("execvp");
            exit(EXIT_FAILURE);
        }

        // Parent process
        close(pipefd[0]);
        close(pipefd[1]);
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
    } else {

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            if (input_fd != STDIN_FILENO) {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }
            if (output_fd != STDOUT_FILENO) {
                dup2(output_fd, STDOUT_FILENO);
                close(output_fd);
            }

            // Execute command
            execvp(tokens[0], tokens);
            perror("execvp");
            exit(EXIT_FAILURE);
        }

        // Parent process
        waitpid(pid, NULL, 0); // Wait for child process to finish
    }
}

int main() {
    char input[1024];
    char *tokens[MAX_TOKENS];
    int num_tokens;
    FILE *file;
    char file_content[1024];

    while (1) {
        print_prompt();


        if (fgets(input, sizeof(input), stdin) == NULL) {
            continue;
        }
        if(strcmp(input,"\n")==0)continue;
        //Colocar fin de string
        input[strcspn(input, "\n")] = '\0';

        // Ignorar comentarios
        char *comment = strchr(input, '#');
        if (comment != NULL) {
            *comment = '\0';
        }

        // Parser
        num_tokens = 0;
        char *token = strtok(input, " ");
        while (token != NULL) {
            tokens[num_tokens++] = token;
            token = strtok(NULL, " ");
        }
        tokens[num_tokens] = NULL;

        if (strcmp(tokens[0], "help") == 0) {
            if (tokens[1] == NULL) {
                Help(NULL);
            } else Help(tokens[1]);
        }
        //Built-in
        else if (strcmp(tokens[0], "cd") == 0) {

            if (num_tokens > 2 && strcmp(tokens[1], "<") == 0) {
                // Abrir el archivo en modo lectura
                file = fopen(tokens[2], "r");

                // Verificar si el archivo se abrió correctamente
                if (file == NULL) {
                    printf("No se pudo abrir el archivo\n");
                    exit(EXIT_FAILURE);
                }

                // Leer el archivo
                fscanf(file, "%s", file_content);

                // Cerrar el archivo
                fclose(file);

                // Cambiar el directorio actual a la ruta leída del archivo
                if (chdir(file_content) == -1) {
                    printf("No se pudo cambiar al directorio especificado.n");
                    exit(EXIT_FAILURE);
                }


            } else if (num_tokens > 2) {
                fprintf(stderr, "cd: too many arguments\n");

            } else if (num_tokens == 1) {
                chdir(getenv("HOME"));
            } else {
                if (chdir(tokens[1]) == -1) {
                    perror("cd");
                }
            }
        } else if (strcmp(tokens[0], "exit") == 0) {
            break;
        } else {
            execute_command(tokens, num_tokens);
        }
    }

    return 0;
}




