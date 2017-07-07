#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

//Constantes
#define tamano 1024
#define longnombre 50

static void mimanejador(int signal){
  int estado;
  wait(&estado);
}

int main(int argc, char const *argv[]) {

  if (argc != 2) {
    printf("Uso: %s <nombre_fifo>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Pone el manejador para la señal SIGCHLD
  signal(SIGCHLD, mimanejador);
  umask(0);

  //Creamos el archivo de bloqueo
  if(creat("bloqueo", S_IRWXU) < 0){
    perror("Error al crear el archivo de bloqueo\n");
    exit(EXIT_FAILURE);
  }

  // Nombre de los fifos
  char nombrefifoe[longnombre],
       nombrefifos[longnombre],
       nombre[longnombre];

  // Compone los nombres de los FIFOs conocidos a partir del parametro,
  // uno de entrada y otro de salida (desde el punto de vista del servidor).
  sprintf(nombrefifoe,"%se",argv[1]);
  sprintf(nombrefifos,"%ss",argv[1]);

  // Creo cauce para entrada
  if((mkfifo(nombrefifoe, S_IRWXU)) == -1){
    perror("Error en mkfifo\n");
    exit(EXIT_FAILURE);
  }
  // Creamos cauce para la salida
  if((mkfifo(nombrefifos, S_IRWXU)) == -1){
    perror("Error en mkfifo\n");
    exit(EXIT_FAILURE);
  }

  int dfifos, dfifoe, dfifoproxy;

  //Abrimos los cauces
  if ((dfifos = open(nombrefifos, O_RDWR)) < 0) {
    perror("Error en open\n");
    exit(EXIT_FAILURE);
  }
  if ((dfifoe = open(nombrefifoe, O_RDWR)) < 0) {
    perror("Error en open\n");
    exit(EXIT_FAILURE);
  }

  pid_t pid;
  size_t numbytes;
  int buff[tamano];

  // El hijo ejecutará el proxy que será una
  // especie de monitor que controlará la Escritura
  // y lectura en emporal.
  //
  // El padre escribirá en en la en el fifo el pid
  // correspondiente a su hijo creado y
  // continuará leyendo el fifo e (pid )
  //

  while (1) {
    numbytes = read(dfifoe, &buff, tamano);
    printf("Ha entrado una solicitud\n");

    if ((pid = fork()) < 0) {
      perror("Error en el fork\n");
      exit(EXIT_FAILURE);
    }

    if(pid == 0){
      pid = getpid();

      // Formo el nombre con el fifo y el pid para
      // abrir cada archivo. Creo el fifo con el nombre
      // de cada pid y lo abro.
      sprintf(nombre, "fifo.%d", pid);

      if((mkfifo(nombre, 0777)) == -1){
        perror("Error en mkfifo\n");
        exit(EXIT_FAILURE);
      }

      if(write(dfifos, &pid, sizeof(pid)) != sizeof(pid)){
        perror("Error al escribir en el ");
        exit(EXIT_FAILURE);
      }

      if ((dfifoproxy=open(nombre, O_RDONLY)) < 0) {
        perror("Error en write\n");
        exit(EXIT_FAILURE);
      }

      // Cambio la entrada a cada fichero proxy
      // Y ejecuto el programa proxy que funcionará como
      // un paso intermedio entre el cliente y el servidor
      // para organizar las lecturas y escrituras.
      dup2(dfifoproxy, STDIN_FILENO);
      execlp("./proxy", "proxy", NULL);

      perror("Error al ejecutar proxy\n");
      exit(0);
    }
  }

  unlink(nombrefifoe);
  unlink(nombrefifos);
  
  exit(EXIT_FAILURE);
}
