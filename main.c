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
int msqid;  // ID de la cola de mensajes, puedes mover la declaración de msqid a nivel global si lo prefieres.
off_t global_offset = 0;  // Offset global para el archivo
int active_children = NUM_PROCESSES; // Número de procesos hijos activos
const char *word_to_search = "libro";  // Cambia "DonQuijote" por la palabra que deseas buscar

// define message for pipe
struct message
{
    int mtype;
    char text[BUFFER_SIZE];
    off_t file_offset;
};

int compile_regex(regex_t *regex);
void create_process(regex_t regex, FILE *fp); // function for create process
void coordinator(); // function for coordinator
void processBuffer(regex_t *regex, FILE *file, int msqid);



int main()
{
    regex_t regex;
    // regex pattern for find the word
    const char *pattern = "([a-zA-Z]+)";
    // call compile regex function with if error
    if (compile_regex(&regex) != 0)
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

    msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0666); 
    if (msqid == -1) {
        perror("Error creating message queue");
        exit(1);
    }
    
    // call create process function
    create_process(regex, fp);
    // free regex
    regfree(&regex);
    fclose(fp);
    return 0;
}




void coordinator() {
    struct message msg;
    while (active_children > 0) { // Se ejecuta mientras haya procesos hijos activos
        // Esperar un mensaje de cualquier hijo
        msgrcv(msqid, &msg, sizeof(msg) - sizeof(long), msg.mtype, 0);

        switch(msg.mtype) {
            case 1:  // Petición de lectura
                msg.file_offset = global_offset;  // Pasar el offset actual
                global_offset += BUFFER_SIZE;     // Actualizar el offset global

                // Devolver el mensaje con el offset al proceso hijo que hizo la petición
                msgsnd(msqid, &msg, sizeof(msg) - sizeof(long), 0);
                break;

            case 2:  // Petición para escribir en la salida
                // En este ejemplo, simplemente imprimimos el texto.
                printf("%s", msg.text);
                break;

            case 3:  // Mensaje de terminación de un proceso hijo
                active_children--;  // Decrementar el número de procesos hijos activos
                break;

            default:
                fprintf(stderr, "Tipo de mensaje desconocido recibido.\n");
                break;
        }
    }

    // Limpiar recursos al finalizar
    msgctl(msqid, IPC_RMID, NULL);  // Eliminar la cola de mensajes
}

// function for compile regex
int compile_regex(regex_t *regex)
{
    char pattern[100];
    sprintf(pattern, "\\b%s\\b", word_to_search); // Usamos \b para delimitadores de palabra para una búsqueda exacta
    int reti = regcomp(regex, pattern, 0); // Compilar la expresión regular.
    if (reti)
    {
        fprintf(stderr, "No se pudo compilar la expresión regular.\n");
        return -1;
    }
    return 0;
}

// function for create process
void create_process(regex_t regex, FILE *fp) {
    int status, i, n = NUM_PROCESSES;
    key_t msqkey = 999;
    int msqid = msgget(msqkey, IPC_CREAT | S_IRUSR | S_IWUSR); 
    pid_t pid;

    for (i = 0; i < n; i++) {
        pid = fork();
        if (pid == 0) { // Código de proceso hijo
            processBuffer(&regex, fp, msqid);
            exit(0);
        }
    }
    coordinator();


    // Código del coordinador
    for (i = 0; i < n; i++) {
        wait(&status);
    }
    msgctl(msqid, IPC_RMID, NULL); 
}

void processBuffer(regex_t *regex, FILE *file, int msqid) {
    struct message msg_request, msg_response;
    off_t offset;
    ssize_t bytes_read;
    char last_char;
    regmatch_t matches[2]; // Assuming one match group
    
    while (1) {
        // Send a read request to the coordinator
        msg_request.mtype = 1;
        msgsnd(msqid, &msg_request, sizeof(msg_request) - sizeof(long), 0);

        // Receive offset from the coordinator
        msgrcv(msqid, &msg_response, sizeof(msg_response) - sizeof(long), 0, 0);
        offset = msg_response.file_offset;

        fseek(file, offset, SEEK_SET);
        bytes_read = fread(buffer, 1, BUFFER_SIZE, file);
        if (bytes_read <= 0) {
            break;
        }

        last_char = buffer[bytes_read-1];
        while (last_char != '\n' && bytes_read > 0) {
            --bytes_read;
            last_char = buffer[bytes_read-1];
        }

        for (int i = 0; i < bytes_read; ) {
            if (regexec(regex, buffer + i, 2, matches, 0) == 0) {
                int start = matches[1].rm_so;
                int end = matches[1].rm_eo;
                int len = end - start;
                if (start != -1 && end != -1) {
                    strncpy(msg_response.text, buffer + i + start, len);
                    msg_response.text[len] = '\0';
                    msg_response.mtype = 2;
                    msgsnd(msqid, &msg_response, sizeof(msg_response) - sizeof(long), 0);
                }
                i += end;
            } else {
                break;
            }
        }
    }

    // Inform the coordinator that we are done
    msg_response.mtype = 3;
    msgsnd(msqid, &msg_response, sizeof(msg_response) - sizeof(long), 0);
}