// File: main.c
// Creator:  David Achoy Yakimova, Earl

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <regex.h>

#define BUFFER_SIZE 8192 // 8KB buffer
#define NUM_PROCESSES 10 // number of processes to create
char buffer[BUFFER_SIZE]; // buffer to store the file contents

// define message for pipe
struct message
{
    int mtype;
    char text[BUFFER_SIZE];
    off_t file_offset;


};

int compile_regex(regex_t *regex, const char *pattern);
void create_process(regex_t regex, FILE *fp); // function for create process
void coordinator(); // function for coordinator
void processBuffer(regex_t *regex, FILE *file); // function for process



int main()
{
    regex_t regex;
    // regex pattern for find the word
    const char *pattern = "([a-zA-Z]+)";
    // call compile regex function with if error
    if (compile_regex(&regex, pattern) != 0)
    {
        return 1;
    }
    // open file
    FILE *fp = fopen("DonQuijote.txt", "r");
    // error if file not found
    if (fp == NULL)
    {
        perror("Error opening file");
        return (-1);
    }
    // call create process function
    create_process(regex, fp);
    // free regex
    regfree(&regex);
    fclose(fp);
    return 0;
}














// function for compile regex
int compile_regex(regex_t *regex, const char *pattern)
{
    int reti = regcomp(regex, pattern, 0); // Compilar la expresión regular.
    if (reti)
    {
        fprintf(stderr, "No se pudo compilar la expresión regular.\n");
        return -1;
    }
    return 0;
}
// function for create process
void create_process(regex_t regex, FILE *fp)
{
    int status, i, n = NUM_PROCESSES;
    key_t msqkey = 999;
    int msqid = msgget(msqkey, IPC_CREAT | S_IRUSR | S_IWUSR); // Crear la cola de mensajes
    pid_t pid;
    struct message msg;
    for (i = 0; i < n; i++)
    {
        pid = fork();
        if (pid == 0)
        {
            printf("Hijo %d\n", i + 1);
            // exit
            exit(0);
        }
        else
        {
            printf("\n");
        }
    }
    for (i = 0; i < n; i++)
    { // Esperar a que terminen los hijos
        wait(&status);
    }
    msgctl(msqid, IPC_RMID, NULL); // Eliminar la cola de mensajes
    exit(0);
}

/* Tarea1 - 4
if (pid == 0)
        {
            msgrcv(msqid, &msg, MSGSZ, i + 1, 0);
            msg.sum = msg.num1 + msg.num2;
            msg.type = n + i + 1;
            msgsnd(msqid, &msg, MSGSZ, 0);  // Enviar la suma al padre
            exit(0);

        }
        else
        {

            msg.type = i+1;
            msg.num1 = rand() % 30;
            msg.num2 = rand() % 30;
            msgsnd(msqid, &msg, MSGSZ, 0); // Enviar los números aleatorios a los hijos
            wait(&status); // Esperar a que terminen los hijos
            msgrcv(msqid, &msg, MSGSZ, n + i + 1, 0);
            printf("Suma de %d y %d es: %d\n", msg.num1, msg.num2, msg.sum); // Imprimir la suma de los números aleatorios
        }
*/