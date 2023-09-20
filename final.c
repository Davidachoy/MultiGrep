// File: final.c
// Creator:  David Achoy Yakimova, Earl


// import libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <regex.h>

#define MSG_KEY 1234 // Clave arbitraria para la cola de mensajes
#define BUFFER_SIZE 8192 // 8KB buffer
#define NUM_PROCESSES 10 // number of processes to create
char buffer[BUFFER_SIZE]; // buffer to store the file contents
#define MSGSZ sizeof(struct message)

struct message {
    long type;
    long position;  // Posición en el archivo
    char line[BUFFER_SIZE];  // Línea en la que se encontró una coincidencia
} msg;

//compile regex function
int compile_regex(regex_t *regex, const char *pattern);
void findRegex(regex_t *regex, FILE *file, int msgid, long position);



int main(int argc, char *argv[]){
    // READ ARGUMENTS
    if (argc != 3){
        fprintf(stderr," %s <expresion_regular> <archivo>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *pattern = argv[1];
    char *filename = argv[2];

    regex_t regex_compiled;
    // compile regex
    compile_regex(&regex_compiled, pattern);

    // open file
    FILE *fp = fopen(filename, "r");
    // error if file not found
    if (fp == NULL)
    {
        perror("Error opening file");
        regfree(&regex_compiled);  // Liberar memoria usada por la expresión regular compilada.
        exit(EXIT_FAILURE);
    }

    int msqid = msgget(MSG_KEY, IPC_CREAT | S_IRUSR | S_IWUSR); // Crear la cola de mensajes
    long current_position = 0;

    for (int i = 0; i < NUM_PROCESSES; i++) {
            pid_t pid = fork();
            if (pid == 0) {  // Proceso hijo
                findRegex(&regex_compiled, fp, msqid, current_position);
                exit(0);
            } else {  // Proceso padre
                msg.type = i + 1;
                msg.position = current_position;
                current_position += BUFFER_SIZE;  // Aumentar la posición para el siguiente hijo
                msgsnd(msqid, &msg, MSGSZ, 0);  // Enviar posición al hijo

                wait(NULL);  // Esperar a que termine el hijo
                msgrcv(msqid, &msg, MSGSZ, NUM_PROCESSES + i + 1, 0);  // Recibir líneas con coincidencias
                printf("Coincidencias en posición %ld: %s\n", msg.position, msg.line);  // Imprimir coincidencias
            }
        }

    for (int i = 0; i < NUM_PROCESSES; i++) {  // Esperar a que terminen todos los hijos
        wait(NULL);
    }


    msgctl(msqid, IPC_RMID, NULL);  // Eliminar la cola de mensajes

    fclose(fp);
    regfree(&regex_compiled);
    return 0;
}

//compile regex function
int compile_regex(regex_t *regex, const char *pattern)
{
    int ret;
    // compile regex
    ret = regcomp(regex, pattern, REG_EXTENDED);
    if (ret){
        fprintf(stderr, "Could not compile regex\n");
        exit(EXIT_FAILURE);
    }
    return ret;
}


void findRegex(regex_t *regex, FILE *file, int msgid, long position) {
    msgrcv(msgid, &msg, MSGSZ, getpid() % NUM_PROCESSES + 1, 0);
    fseek(file, msg.position, SEEK_SET);
    char buffer[BUFFER_SIZE];
    fread(buffer, 1, BUFFER_SIZE, file);

}
