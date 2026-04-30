#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <sys/msg.h>

//Definicion de constantes
#define TAM_BUFFER 6 //Tamaño del buffer circular de bombas
#define TAM_SEG 100 //Tamaño de los segmentos de memoria compartida (100 bytes).
#define NUM_ELEMENTOS 24  // Número total de elementos que las bombas procesarán (6 bombas * 4 ciclos = 24 elementos)
#define TAM_INV 50 //Limite del inventario
#define MAX_SEND_SIZE 80 //Tamaño máximo de los mensajes enviados por las colas.

struct mymsgbuf {
  long mtype; //tipo de mensaje
  char mtext[MAX_SEND_SIZE];//contenido del mensaje
};

int escr_msg(int qid,struct mymsgbuf *qbuf); 
int leer_msg(int qid,long type,struct mymsgbuf *qbuf);

int escr_msg(int qid,struct mymsgbuf *qbuf) //qid: Identificador de la cola de mensajes, qbuf: Puntero al mensaje que se enviará
{ 
  int resultado;
  
  resultado=msgsnd(qid,qbuf,MAX_SEND_SIZE,0); //Envía el mensaje a la cola identificada por qid
  
  return (resultado);

} 

int leer_msg(int qid,long type,struct mymsgbuf *qbuf) 
{ 
  int resultado;
  
  resultado=msgrcv(qid,qbuf,MAX_SEND_SIZE,type,0); //Lee un mensaje del tipo type en la cola identificada por qid
  
  return (resultado);
} 

void modo_surtidores();
void modo_servicios();
void modo_entrevistas();

int main() {
  int opcion;

  while (1) {
    printf("\n--- Menú Principal ---\n");
    printf("1. Modo Servicios\n");
    printf("2. Modo Surtidores\n");
    printf("3. Modo Entrevista\n");
    printf("4. Salir\n");
    printf("Seleccione una opción: ");
    scanf("%d", &opcion);

    switch (opcion) {
    case 1:
      modo_servicios();
      break;
    case 2:
      modo_surtidores();
      break;
    case 3:
      modo_entrevistas();
      break;
    case 4:
      printf("Saliendo del programa...\n");
      exit(0);
    default:
      printf("Opción no válida. Intente de nuevo.\n");
    }
  }
  return 0;
}

void modo_surtidores() {
  char *nsemaforo[3] = {"huecos", "elementos", "mutex"};
  sem_t *huecosID = NULL;
  sem_t *elementosID = NULL;
  sem_t *mutexID = NULL;

  // Abrir semáforos
  huecosID = sem_open(nsemaforo[0], 0);
  elementosID = sem_open(nsemaforo[1], 0);
  mutexID = sem_open(nsemaforo[2], 0);

  if (!huecosID || !elementosID || !mutexID) {
    perror("ERROR: No se pudieron abrir los semáforos");
       
  }

  // Conectar memoria compartida
  key_t clave = ftok(".", 'S');
  int shmid = shmget(clave, TAM_SEG, 0);
  if (shmid == -1) {
    perror("ERROR: No se pudo acceder al segmento de memoria compartida");
  }

  int *addr = (int *)shmat(shmid, NULL, 0);
  if (addr == (int *)-1) {
    perror("ERROR: No se pudo mapear el segmento de memoria compartida");
  }

  for (int i = 0; i < NUM_ELEMENTOS; i++) {
    // Producir un elemento
    if (sem_wait(huecosID) != 0) {
      perror("ERROR: No se pudo decrementar el semáforo huecos");
    }
    if (sem_wait(mutexID) != 0) {
      perror("ERROR: No se pudo decrementar el semáforo mutex");
    }

    int posicion = addr[TAM_BUFFER]; // Posición actual del buffer
    addr[posicion] = i;             // Escribe el elemento en el buffer
    addr[TAM_BUFFER] = (posicion + 1) % TAM_BUFFER; // Actualizar la posicion del productor
    printf("Cliente_%d: Ocupada la bomba de la posición %d\n", i+1, posicion + 1);

    if (sem_post(mutexID) != 0) {
      printf("ERROR: No se pudo incrementar el semáforo mutex");
    }
    if (sem_post(elementosID) != 0) {
      printf("ERROR: No se pudo incrementar el semáforo elementos");
    }
  }
   
}

void modo_servicios() {
  key_t clave_tarea = ftok(".", 'S');
  int shmid_tarea = shmget(clave_tarea, TAM_SEG, 0);
  if (shmid_tarea == -1) {
    printf("ERROR: No se pudo acceder a la memoria compartida para tareas");
    return;
  }

  key_t clave_respuesta = ftok(".", 'R');
  int shmid_respuesta = shmget(clave_respuesta, TAM_SEG, 0);
  if (shmid_respuesta == -1) {
    printf("ERROR: No se pudo acceder a la memoria compartida para respuestas");
    return;
  }
  //para escribir las solicitudes de servicios
  char *buffer_tarea = (char *)shmat(shmid_tarea, NULL, 0); //shmat conecta el segmento de memoria al espacio de direcciones del proceso.
  //Contendra las respuestas de los servicios
  char *buffer_respuesta = (char *)shmat(shmid_respuesta, NULL, 0);

  if (buffer_tarea == (char *)-1 || buffer_respuesta == (char *)-1) {
    printf("ERROR: No se pudo mapear la memoria compartida");
    return;
  }

  sem_t *mutex = sem_open("mutex", 0);
  if (mutex == SEM_FAILED) {
    perror("ERROR: No se pudo abrir el semáforo mutex");
    shmdt(buffer_tarea);
    shmdt(buffer_respuesta);
    return;
  }

  int opcion;
  char *tareas[] = {"Limpieza", "Cambio de aceite", "Revisión de neumáticos", "Lavado exterior"};

  do { //muestra las tareas disponibles
    printf("\nMenú de servicios:\n");
    for (int i = 0; i < 4; i++) {
      printf("%d. %s\n", i + 1, tareas[i]);
    }
    printf("0. Salir\nSeleccione una opción: ");
    scanf("%d", &opcion);

    if (opcion >= 1 && opcion <= 4) {
      sem_wait(mutex);  // Bloquea el semáforo para garantizar que este proceso tiene acceso exclusivo a la memoria compartida

      // Escribir tarea
      strncpy(buffer_tarea, tareas[opcion - 1], TAM_SEG); //Copia la tarea seleccionada al segmento de memoria compartida para tareas.
      printf("Solicitud enviada: %s\n", tareas[opcion - 1]);

      sem_post(mutex);  // Libera el acceso al semáforo para que otros procesos puedan usar la memoria.

      // Esperar respuesta
      while (*buffer_respuesta == '\0') { //El proceso verifica si el segmento de respuestas está vacío
	usleep(100000); // Esperar en microsegundos
      }
      //Cuando recibe una respuesta, la imprime y limpia el segmento de memoria compartida para futuras respuestas
      printf("Respuesta: %s\n", buffer_respuesta);
      memset(buffer_respuesta, 0, TAM_SEG); // Limpiar respuesta
    } else if (opcion != 0) {
      printf("Opción no válida.\n");
    }
  } while (opcion != 0);
  //se sale del modo servicios
  shmdt(buffer_tarea);
  shmdt(buffer_respuesta);
  sem_close(mutex);
}


void modo_entrevistas() {
  key_t clave;
  int msgqueue_id;
  struct mymsgbuf solicitud, confirmacion;
  char nombre[40], puesto[40];

  // Generar clave para la cola
  clave = ftok(".", 'm');
  if ((msgqueue_id = msgget(clave, 0)) == -1) {
    printf("Error al acceder a la cola");
  }
    
  // Solicita los datos al usuario
  printf("Ingrese su nombre: ");
  scanf("%s", nombre);
  //fgets(nombre, 40, stdin);
  nombre[strcspn(nombre, "\n")] = '\0';

  printf("Ingrese el puesto al que aplica: ");
  scanf("%s", puesto);
  //fgets(puesto, 40, stdin);
  puesto[strcspn(puesto, "\n")] = '\0';

  // Preparar solicitud
  solicitud.mtype = 2;  // Tipo 2 para solicitudes de candidatos
  snprintf(solicitud.mtext, MAX_SEND_SIZE, "%s,%s", nombre, puesto); //Escribe el mensaje en el formato nombre,puesto y lo almacena en mtext

  // Enviar solicitud
  if (escr_msg(msgqueue_id, &solicitud) == -1) { //envia el mensaje a la cola identificada por msgqueue_id
    printf("Error al enviar la solicitud");
  }

  printf("Solicitud enviada: %s\n", solicitud.mtext);

  // Recibir confirmación
  if (leer_msg(msgqueue_id, 3, &confirmacion) == -1) { //Usa leer_msg para recibir mensajes del tipo 3 (confirmaciones)
    printf("Error al recibir la confirmación");
  }

  printf("Confirmación recibida: %s\n", confirmacion.mtext);
   
}
