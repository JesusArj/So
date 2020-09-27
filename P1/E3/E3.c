#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

//Da un warning al compilar que no se como quitar pero funciona :)

int main(int argc, char const *argv[])
{
  int status;
  int pid[2];
  printf("Soy %d el padre de todos\n", getpid());
  for (int i = 0; i < 2; i++)
  {
    switch(fork())
    {
      case 0:
        printf("Soy %d el hijo nº %d del proceso: %d\n", getpid(), (i+1), getppid());
        if (i==0)
        {
            execl("/usr/bin/gnome-calculator", "gnome-calculator", NULL);
        }
        else
        {
            execvp(argv[2], argv+2);
        }
        return 0;

      case -1:
        printf("Error al crear el proceso hijo\n");
        return -1;

      default:
        printf("Esperando a que acabe mi hijo nº %d\n", i+1);
    }
  }
  pid[0] = wait(&status);
  printf("Ya ha acabado mi hijo %d con el codigo de salida: %d\n", pid[0], WEXITSTATUS(status));
  pid[1] = wait(&status);
  printf("Ya ha acabado mi hijo %d con el codigo de salida: %d\n", pid[1], WEXITSTATUS(status));
  return 0;
}
