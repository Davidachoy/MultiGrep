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


#define BUFFER_SIZE 8192 // 8KB buffer
#define NUM_PROCESSES 10 // number of processes to create

char buffer[BUFFER_SIZE]; // buffer to store the file contents

//define message for pipe
struct message {
    int length;
    char text[BUFFER_SIZE];
} msg;

int main() {
    int status,i,n=NUM_PROCESSES;
    key_t msqkey = 999;
    int msqid = msgget(msqkey, IPC_CREAT | S_IRUSR | S_IWUSR); // Crear la cola de mensajes
    for (i = 0; i < n; i++) { 
        pid_t pid = fork();

        if (pid == 0) { // Crear un proceso hijo
            //use greep to find the word "main" in the file
            char *argv[] = {"grep", "Character", "./DonQuijote.txt", NULL};
            execvp(argv[0], argv);
            printf("Hello world!\n");
            return 0;
        }
    }  // Crear NUM_PROCESSES procesos hijos
    for (i = 0; i < n; i++) { 
        wait(&status); // Esperar a que terminen los procesos hijos
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