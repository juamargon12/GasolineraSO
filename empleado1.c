#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <sys/msg.h>
#include <signal.h>  

//Definicion de constantes
#define TAM_BUFFER 6                // Tamaño máximo del buffer circular (memoria compartida)
#define NUM_ELEMENTOS 12            // Número total de elementos a procesar (6 bombas * 4 ciclos = 24 elementos)
#define TAM_SEG 100                 // Tamaño de cada segmento de memoria compartida
#define TAM_NOMBRE 50               // Longitud máxima para nombres de productos o textos
#define MAX_PRODUCTOS 4             // Cantidad máxima de productos que puede haber en el inventario
#define TAM_INV 50                  // Tamaño total de la estructura del inventario
#define TAM_PRODUCTOS (MAX_PRODUCTOS * TAM_NOMBRE)  // Espacio total para nombres de productos
#define TAM_INVENTARIO (TAM_PRODUCTOS + (MAX_PRODUCTOS * 4))  // Espacio total del inventario con cantidades
#define MAX_SEND_SIZE 80            // Tamaño máximo de mensajes en las colas
#define NUM_HORARIOS 4              // Número máximo de horarios disponibles para entrevistas


struct mymsgbuf { //define una estructura para la comunicacion mediante colas de mensajes
  long mtype; //tipo de mensaje
  char mtext[MAX_SEND_SIZE]; //texto del mensaje
};

int escr_msg(int qid,struct mymsgbuf *qbuf); 
int leer_msg(int qid,long type,struct mymsgbuf *qbuf);

int escr_msg(int qid,struct mymsgbuf *qbuf) //funcion para enviar mensaje a la cola
{ 
  int resultado;
  
  resultado=msgsnd(qid,qbuf,MAX_SEND_SIZE,0); //Envía un mensaje al identificador de cola qid donde qbuf contiene el mensaje a enviar
  
  return (resultado); //0 si tiene exito y -1 si hay error
} 

int leer_msg(int qid,long type,struct mymsgbuf *qbuf) //funcion para leer mensaje de la cola
{ 
  int resultado;
  
  resultado=msgrcv(qid,qbuf,MAX_SEND_SIZE,type,0); //lee un mensaje del tipo type de la cola qid y se guarda en qbuf
  
  return (resultado); //0 si tiene exito y -1 si hay err
} 


typedef struct { // se usa para gestionar productos y su cantidad disponible
  char productos[MAX_PRODUCTOS][TAM_NOMBRE]; //Almacena nombres de productos
  int inventario[MAX_PRODUCTOS]; //Almacena la cantidad disponible para cada producto
} Inventario;

void manejador_sigint() {
    printf("\nSaliendo...\n");
    exit(0); // Salir del programa
}

void inicializar_inventario(Inventario *inv) {
  const char *productos_iniciales[MAX_PRODUCTOS] = { 
    "Limpieza",
    "Cambio de aceite",
    "Revisión de neumáticos",
    "Lavado exterior"
  }; //lista de productos iniciales

  for (int i = 0; i < MAX_PRODUCTOS; i++) {
    strncpy(inv->productos[i], productos_iniciales[i], TAM_NOMBRE); //copia el nombre de cada producto al inventario
    inv->inventario[i] = 1; // asigna 1 unidad inicial para cada producto
  }
  printf("Inventario inicializado.\n");
}

void procesar_bombas() {
  char *nsemaforo[3] = {"huecos", "elementos", "mutex"};
  sem_t *huecosID = NULL;
  sem_t *elementosID = NULL;
  sem_t *mutexID = NULL;

  // Abrir semáforos
  huecosID = sem_open(nsemaforo[0], 0); //semaforos para espacios libres
  elementosID = sem_open(nsemaforo[1], 0); //semaforo para elementos disponibles
  mutexID = sem_open(nsemaforo[2], 0); //semaforo mutex (exclusion mutua)

  if (!huecosID || !elementosID || !mutexID) {
    printf("ERROR: No se pudieron abrir los semáforos");
  }

  // Conectar memoria compartida
  key_t clave = ftok(".", 'S');
  int shmid = shmget(clave, TAM_SEG, 0); //obtiene el ID del segmento de memoria compartida
  if (shmid == -1) {
    printf("ERROR: No se pudo acceder al segmento de memoria compartida");
  }

  int *addr = (int *)shmat(shmid, NULL, 0); //mapeado del segmento de memoria compartida al espacio de direcciones del proceso
  if (addr == (int *)-1) {
    printf("ERROR: No se pudo mapear el segmento de memoria compartida");
  }

  for (int i = 0; i < NUM_ELEMENTOS; i++) {
    // Consumir un elemento
    if (sem_wait(elementosID) != 0) { //Decrementa el semáforo o si el valor es 0 espera hasta que haya elementos disponibles
      printf("ERROR: No se pudo decrementar el semáforo elementos");
    }
    if (sem_wait(mutexID) != 0) { //entra en seccion critica
      printf("ERROR: No se pudo decrementar el semáforo mutex");
    }
	
    int j=addr[addr[TAM_BUFFER+1]];  
    printf("Empleado: Atendida la bomba numero %d de la posición %d\n",j+1,addr[TAM_BUFFER+1]+1);
    addr[TAM_BUFFER+1]=(addr[TAM_BUFFER+1]+1)%TAM_BUFFER; //avanza posicion en el buffer
	
    if (sem_post(mutexID) != 0) { //sale de la seccion critica
      printf("ERROR: No se pudo incrementar el semáforo mutex");
    }
    sleep(1);
    if (sem_post(huecosID) != 0) { //libera un espacio del buffer
      printf("ERROR: No se pudo incrementar el semáforo huecos");
    }
  }
}

void modo_servicios() {
  key_t clave_tarea = ftok(".", 'S');
  int shmid_tarea = shmget(clave_tarea, TAM_SEG, 0); //obtiene el ID del segmento de memoria compartida asociado a la clave
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

  char *buffer_tarea = (char *)shmat(shmid_tarea, NULL, 0); //Mapea los segmentos de memoria compartida en el espacio de memoria del proceso.
  char *buffer_respuesta = (char *)shmat(shmid_respuesta, NULL, 0);

  if (buffer_tarea == (char *)-1 || buffer_respuesta == (char *)-1) { //buffer_tarea apunta al área de memoria compartida para solicitudes.
                                                                      //buffer_respuesta apunta al área de memoria compartida para respuestas
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

  printf("Inventario inicializado.\n");
  printf("Servicio listo para recibir solicitudes.\n");

  
  do {
    sem_wait(mutex);  // Decrementa el valor del semáforo (mutex), bloqueando si el valor es 0.
                      //Esto asegura acceso exclusivo al segmento de memoria compartida

    if (*buffer_tarea != '\0') { //Comprueba si hay una solicitud en el buffer (buffer_tarea no está vacío) y si existe lo imprime
      printf("Solicitud recibida: %s\n", buffer_tarea);

      
      sleep(2);
      snprintf(buffer_respuesta, TAM_SEG, "Servicio de %s completado", buffer_tarea); //Escribe una respuesta en el buffer_respuesta
                                                                                     //indicando que el servicio fue completado
      printf("Solicitud procesada.\n");

      memset(buffer_tarea, 0, TAM_SEG); // Limpia el contenido del buffer de tareas para indicar que esta libre
    } else {
      struct sigaction act;  
      act.sa_handler= manejador_sigint;   // Maneja la señal  
      act.sa_flags= 0;           // Ninguna accion especial  
      sigemptyset(&act.sa_mask); // Ninguna señal bloqueada  
    
      sigaction(SIGINT, &act, NULL);
      printf("Esperando solicitud...\n");
    }

    sem_post(mutex);  // Incrementa el valor del semáforo (mutex), permitiendo que otros procesos accedan a la memoria compartida.

    sleep(1); 
  } while (1); //El ciclo continua indefinidamente, revisando solicitudes

  //Se desconectan los segmentos de memoria compartida (shmdt) y se cierra el semáforo
  shmdt(buffer_tarea);
  shmdt(buffer_respuesta);
  sem_close(mutex);
}

void modo_entrevistas() { // se utiliza una cola de mensajes para gestionar solicitudes de entrevistas, asignando horarios disponibles
  key_t clave; //clave para identificar la cola de mensajes
  int msgqueue_id; //ID de la cola de mensajes.
  struct mymsgbuf solicitud, confirmacion, testigo; //Variables para manejar mensajes en la cola.
  char horarios[NUM_HORARIOS][20] = {"Lunes 10:00 AM", "Lunes 11:00 AM", "Martes 10:00 AM", "Martes 11:00 AM"};
  int horario_actual = 0; //indice que rastrea los horarios disponibles

  // Generar clave para la cola
  clave = ftok(".", 'm');
  printf("Abriendo cola...\n");
  if ((msgqueue_id = msgget(clave, 0)) == -1) { //acceder a la cola de mensajes
    printf("Error al iniciar la cola");
  }

  while (1) { //ciclo infinito para procesar solicitudes de entrevistas
    printf("Esperando testigo...\n");
    if (leer_msg(msgqueue_id, 1, &testigo) == -1) { //Espera el "testigo" (un mensaje especial de tipo 1 que indica que puede procesar solicitudes)
      printf("Error al recibir el testigo");
    }
    printf("Testigo recibido. Procesando solicitudes...\n");

    // Leer solicitud de entrevista
    if (leer_msg(msgqueue_id, 2, &solicitud) == -1) { //Lee un mensaje de tipo 2 (solicitud de entrevista)
      printf("Error al recibir solicitud");
      printf("Devolviendo testigo...\n");
      escr_msg(msgqueue_id, &testigo);  // Devolver testigo
      continue;
    }

    printf("Solicitud recibida: %s\n", solicitud.mtext); //Imprime el contenido de la solicitud

    // Preparar confirmación
    confirmacion.mtype = 3;  // Prepara una respuesta (mtype = 3 para respuestas)
    
    // Si hay horarios disponibles
    if (horario_actual < NUM_HORARIOS) {
      snprintf(confirmacion.mtext, MAX_SEND_SIZE,"Cita asignada %.19s para %.40s", // Si hay horarios disponibles, asigna uno y lo incluye en el mensaje
	       horarios[horario_actual], solicitud.mtext);
      horario_actual++;
    } else {
      snprintf(confirmacion.mtext, MAX_SEND_SIZE,"No hay horarios disponibles para %.40s", solicitud.mtext); //Si no hay horarios, informa que no hay disponibilidad
    }

    // Enviar confirmación
    if (escr_msg(msgqueue_id, &confirmacion) == -1) { //Envía la confirmación de la entrevista y verifica si se envió correctamente
      printf("Error al enviar la confirmación");
    } else {
      printf("Confirmación enviada: %s\n", confirmacion.mtext);
    }

    // Devolver testigo
    printf("Devolviendo testigo...\n");
    escr_msg(msgqueue_id, &testigo); //Devuelve el "testigo" a la cola para que otros procesos puedan usarlo
  }

}


int main() {
  int opcion;

  while (1) {
    printf("\n--- Menú Empleado ---\n");
    printf("1. Procesar Servicios\n");
    printf("2. Procesar Bombas\n");
    printf("3. Modo Entrevista\n");
    printf("4. Salir\n");
    printf("Seleccione una opción: ");
    scanf("%d", &opcion);

    switch (opcion) {
    case 1:
      modo_servicios();
      break;
    case 2:
      procesar_bombas();
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


