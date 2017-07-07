#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

//Constantes
#define tamanio 1024
#define longnombre 50

// Función para la competitición de expclusión mutua.
void block_desblock(int fd, int type){
  struct flock cerrojo;

  cerrojo.l_type = type;
  cerrojo.l_whence = SEEK_SET;
  cerrojo.l_start = 0;
  cerrojo.l_len = 0;

  // Aplico sobre el descriptor el cerrojo ajustado con las
  // opciones de más arriba.
  if (fcntl(fd, F_SETLKW, &cerrojo) == -1) {
    perror ("Error: al bloquear para imprimir");
		exit(EXIT_FAILURE);
  }
}

int main(int argc, char const *argv[]) {
  size_t numbytes;
  int buf[tamanio];


  int dbloqueo;
  if ((dbloqueo = open("bloqueo", O_RDWR)) < 0) {
    perror("Error al crear el archivo bloqueo.\n");
    exit(EXIT_FAILURE);
  }

  FILE *tmp = tmpfile();        // intancia única de ejecunción libro, consultar

  // Mientras lea de la entrada standard la escribiré
  // en el archivo temporal
  while ((numbytes = read(STDIN_FILENO, &buf, sizeof(buf))) > 0) {
    if ((fwrite(&buf, sizeof(char), sizeof(buf), tmp)) < 0) {
      perror("Error en la lectura en del proxy.\n");
      exit(EXIT_FAILURE);
    }
  }

  //Volvemos al inicio del descriptor del archivo
  rewind(tmp);

  block_desblock(dbloqueo, F_WRLCK);
  while ((numbytes = fread(&buf, sizeof(char), sizeof(buf), tmp)) > 0){
    printf("\n[%s]\n", buf);
  }
  block_desblock(dbloqueo, F_UNLCK);

  fclose(tmp);

  char nombrefifo[longnombre];
  sprintf(nombrefifo, "fifo.%d", getpid());
  unlink(nombrefifo);

  exit(0);
}
