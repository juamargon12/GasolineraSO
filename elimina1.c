#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>

//Definicion de constantes
#define TAM_SEG 100 //tamano de un segmento de memoria compartida (100 bytes)
#define TAM_INV 50 //tamano de la memoria compartida para el inventario (50 bytes).

int main() {
  char *nombres[4] = {"huecos", "elementos", "mutex", "mutex_inv"}; //array que contiene los nombres de los semaforos que se eliminaran

  // Eliminar semaforos
  
  for (int i = 0; i < 4; i++) {
    if (sem_unlink(nombres[i]) == 0) { //elimina el semaforo indicado por su nombre, devuelve 0 sin error y -1 con error
      printf("Semáforo eliminado correctamente: %s\n", nombres[i]);
    } else {
      printf("ERROR: No se pudo eliminar el semáforo %s\n", nombres[i]);
    }
  }

  // Eliminar memoria compartida del buffer
    
  key_t clave_buffer = ftok(".", 'S');
  int shmid_buffer = shmget(clave_buffer, TAM_SEG, 0); //obtiene el ID del segmento de memoria compartida asociado a la clave
  if (shmid_buffer != -1) {
    if (shmctl(shmid_buffer, IPC_RMID, NULL) == 0) { //IPC_RMID indica que el segmento debe eliminarse
      printf("Segmento de memoria compartida para buffer eliminado\n");
    } else {
      printf("ERROR: No se pudo eliminar el segmento de memoria compartida para buffer\n");
    }
  }

  // Eliminar memoria compartida del inventario
  key_t clave_inv = ftok(".", 'I');
  int shmid_inv = shmget(clave_inv, TAM_INV, 0);
  if (shmid_inv != -1) {
    if (shmctl(shmid_inv, IPC_RMID, NULL) == 0) {
      printf("Segmento de memoria compartida para inventario eliminado\n");
    } else {
      printf("ERROR: No se pudo eliminar el segmento de memoria compartida para inventario\n");
    }
  }

  // Eliminar memoria compartida de la tarea
  key_t clave_tarea = ftok(".", 'M');
  int shmid_tarea = shmget(clave_tarea, TAM_SEG, 0);
  if (shmid_tarea != -1) {
    if (shmctl(shmid_tarea, IPC_RMID, NULL) == 0) {
      printf("Segmento de memoria compartida para tarea eliminado\n");
    } else {
      printf("ERROR: No se pudo eliminar el segmento de memoria compartida para tarea\n");
    }
  }


  // Eliminar memoria compartida de la respuesta
  key_t clave_respuesta = ftok(".", 'R');
  int shmid_respuesta = shmget(clave_respuesta, TAM_SEG, 0);
  if (shmid_respuesta != -1) {
    if (shmctl(shmid_respuesta, IPC_RMID, NULL) == 0) {
      printf("Segmento de memoria compartida para respuesta eliminado\n");
    } else {
      printf("ERROR: No se pudo eliminar el segmento de memoria compartida para respuesta\n");
    }
  }

  // Eliminamos la cola de mensajes, como no sabemos el id de la cola que se vaya a crear, eliminamos varios y hacemos que no salgan errores
    system("ipcrm -q 0 2>/dev/null");
    system("ipcrm -q 1 2>/dev/null");
    system("ipcrm -q 2 2>/dev/null");
    system("ipcrm -q 3 2>/dev/null");
    system("ipcrm -q 4 2>/dev/null");
    system("ipcrm -q 5 2>/dev/null");
    printf("Cola eliminada\n");
  
  return 0;
}
