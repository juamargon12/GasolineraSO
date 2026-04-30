# ⛽ Sistema de Gestión de Servicios de una Gasolinera

## 📖 Descripción del Proyecto
Este trabajo consiste en el diseño e implementación de un sistema operativo simulado en C que integra varios servicios de una estación de servicio: mantenimiento de vehículos, uso de surtidores y solicitudes de entrevistas de trabajo. 

El objetivo principal es crear un entorno concurrente donde múltiples procesos interactúan simultáneamente sin interferir entre sí, gestionando la disponibilidad de recursos mediante **mecanismos IPC (Inter-Process Communication)**.

## ✨ Modos de Funcionamiento
El sistema cuenta con tres modos principales accesibles mediante un menú interactivo:

* **Modo Surtidores:** Simula un sistema de bombas de combustible donde los clientes llenan sus vehículos. Utiliza un buffer circular con capacidad para 6 elementos controlado por el modelo Productor-Consumidor (clientes y empleados).
* **Modo Servicios:** Gestiona solicitudes de mantenimiento (cambios de aceite, revisión de neumáticos, etc.). El cliente envía la solicitud y el servidor la procesa y responde.
* **Modo Entrevistas:** Sistema para que los candidatos apliquen a puestos de trabajo. Gestiona las peticiones, revisa la disponibilidad de horarios y confirma las citas.

## 🛠️ Arquitectura y Tecnologías
* **Lenguaje:** C.
* **Sincronización y Comunicación (IPC):**
    * **Semáforos:** Utilizados para coordinar el acceso a recursos comunes y evitar condiciones de carrera (ej. `huecos`, `elementos`, `mutex` para el buffer, y `mutex_inv` para el inventario).
    * **Memoria Compartida:** Permite a los procesos compartir información en tiempo real, como el estado de las tareas solicitadas y las respuestas del servidor.
    * **Colas de Mensajes:** Usadas principalmente en el Modo Entrevistas para el intercambio de solicitudes y confirmaciones mediante el paso de un "testigo" (mtype = 1).
* **Manejo de Señales:** Implementación de un manejador de señales (`SIGINT` / Ctrl+C) en el servidor para anunciar cancelaciones o cierres seguros.

## 📂 Estructura de Ficheros
1. **`crea.c`:** Inicializa el entorno creando los semáforos, reservando los segmentos de memoria compartida y generando la cola de mensajes.
2. **`elimina.c`:** Script de limpieza que libera los recursos IPC (`sem_unlink`, `shmctl`, `msgctl`) para evitar que queden ocupados en el sistema tras finalizar la ejecución.
3. **`cliente.c`:** Contiene el menú principal interactivo. Actúa como el proceso que genera solicitudes (productor en surtidores, solicitante en servicios y candidato en entrevistas).
4. **`empleado.c`:** Actúa como el servidor o trabajador. Consume los elementos de los surtidores, procesa los servicios solicitados enviando respuestas, y asigna los horarios de las entrevistas manejando el inventario de recursos.

## ⚙️ Compilación y Ejecución
Para que el sistema funcione correctamente en un entorno Linux, es fundamental respetar el orden de creación y eliminación de recursos IPC compartidos.

**1. Compilación de los archivos**
Abre una terminal y compila los cuatro ficheros (se recomienda enlazar la librería de hilos para el uso de semáforos POSIX):
```bash
gcc crea.c -o crea -pthread
gcc elimina.c -o elimina -pthread
gcc cliente.c -o cliente -pthread
gcc empleado.c -o empleado -pthread
```

**2. Inicialización del entorno**
Antes de arrancar la gasolinera, debes crear la memoria compartida, los semáforos y la cola de mensajes ejecutando el fichero de creación:
```bash
./crea
```

**3. Ejecución del sistema (Cliente y Empleado)**
Al ser un sistema concurrente, necesitarás abrir **dos terminales distintas**:
*   En la **Terminal 1**, ejecuta el servidor/empleado para que esté a la escucha de peticiones:
    ```bash
    ./empleado
    ```
*   En la **Terminal 2**, ejecuta la interfaz del cliente para acceder al menú y realizar solicitudes:
    ```bash
    ./cliente
    ```

**4. Cierre y Limpieza de recursos (Muy importante)**
Una vez hayas terminado de probar el programa, debes liberar los semáforos y segmentos de memoria compartida que quedaron reservados en el sistema operativo ejecutando el archivo de eliminación:
```bash
./elimina
```
