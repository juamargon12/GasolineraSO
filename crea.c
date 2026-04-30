#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <string.h>

//Definicion de constantes

#define TAM_BUFFER 6 //Tamano del buffer
#define TAM_SEG 100 //Tamano para los segmentos de memoria compartida (100 bytes)
#define TAM_INV 50 //Tamano del inventario en memoria compartida (50 bytes).
#define MAX_SEND_SIZE 80 //Tamano maximo del mensaje a enviar en la cola (80 bytes).

struct mymsgbuf { //Estructura para los mensajes en la cola
  long mtype; //Tipo del mensaje
  char mtext[MAX_SEND_SIZE]; //Texto del mensaje
};

int escr_msg(int qid, struct mymsgbuf *qbuf) { //Envia un mensaje a la cola identificada por qid
                                               //(qid es el id de la cola y qbuf el puntero al que se envia el mensaje)
  int resultado;
  resultado=msgsnd(qid,qbuf,MAX_SEND_SIZE,0);//funcion que coloca el mensaje en la cola (devuelve 0 sin errores y -1 con error)
  return (resultado);
}

int main() {
  sem_t *semaforos[4] = {NULL, NULL, NULL, NULL}; //Array de punteros para manejar los semaforos.
  char *nombres[4] = {"huecos", "elementos", "mutex", "mutex_inv"}; //Array de nombres para los semaforos (huecos: numero de espacios disponibles en el buffer.
                                                                    //elementos: numero de elementos presentes en el buffer.
                                                                    //mutex y mutex_inv: semaforos para exclusion mutua.)
  int valores[4] = {TAM_BUFFER, 0, 1, 1}; //valores iniciales de los semaforos

  // Crear semaforos
  
  for (int i = 0; i < 4; i++) { //recorre los semaforos, creandolos con sem_open
    semaforos[i] = sem_open(nombres[i], O_CREAT, 0600, valores[i]);
    if (semaforos[i] != NULL) {
      printf("Semáforo creado correctamente: %s (valor: %d)\n", nombres[i], valores[i]);
    } else {
      printf("ERROR: No se pudo crear el semáforo %s\n", nombres[i]);
    }
  }

  // Crear memoria compartida para buffer
  
  key_t clave_buffer = ftok(".", 'S'); //Genera una clave unica basada en el archivo actual (".") y un identificador ('S').
  int shmid_buffer = shmget(clave_buffer, TAM_SEG, IPC_CREAT | IPC_EXCL | 0660); //Crea un segmento de memoria compartida
  if (shmid_buffer == -1) {
    printf("ERROR: No se pudo crear la memoria compartida para el buffer\n");
  } else {
    printf("Memoria compartida para buffer creada\n");
  }

  // Crear memoria compartida para inventario
  
  key_t clave_inv = ftok(".", 'I'); 
  int shmid_inv = shmget(clave_inv, TAM_INV, IPC_CREAT | IPC_EXCL | 0660);
  if (shmid_inv == -1) {
    printf("ERROR: No se pudo crear la memoria compartida para inventario\n");
  } else {
    printf("Memoria compartida para inventario creada\n");
  }

  // Crear memoria compartida para tarea
  
  key_t clave_tarea = ftok(".", 'M');
  int shmid_tarea = shmget(clave_tarea, TAM_SEG, IPC_CREAT |IPC_EXCL | 0660);
  if (shmid_tarea == -1) {
    printf("ERROR: No se pudo crear la memoria compartida para tarea\n");
  } else {
    printf("Memoria compartida para tarea creada\n");
  }

  // Crear memoria compartida para espuesta
  
  key_t clave_respuesta = ftok(".", 'R');
  int shmid_respuesta = shmget(clave_respuesta, TAM_SEG, IPC_CREAT |IPC_EXCL | 0660);
  if (shmid_respuesta == -1) {
    printf("ERROR: No se pudo crear la memoria compartida para respuesta\n");
  } else {
    printf("Memoria compartida para respuesta creada\n");
  }

  //Colas de mensajes
  
  key_t clave;
  int msgqueue_id;
  long tipo = 1;
  char *testigo = "testigo";
  struct mymsgbuf qbuffer;

  clave = ftok(".", 'm'); //crea una clave para la cola
  printf("Creando cola...\n");
  if ((msgqueue_id = msgget(clave, IPC_CREAT | 0660)) == -1) { //crea la cola de mensajes
    printf("Error al iniciar la cola");
  }

  // Crear el testigo
  printf("Añadiendo el testigo a la cola...\n");
  qbuffer.mtype = tipo;  // asigna tipo 1 al mensaje
  strncpy(qbuffer.mtext, testigo, MAX_SEND_SIZE - 1); //copia el texto "testigo" al mensaje

  if (escr_msg(msgqueue_id,&qbuffer) == -1) { //envia el mensaje a la cola
    printf("Error al colocar el testigo en la cola");
  }

  printf("Testigo creado y colocado en la cola.\n");

    
  return 0;
}
