#include <sys/types.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <errno.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>

int main(int argc, char const *argv[])
{
   if (argc < 2)//Comprobamos que se haya llamado al programa de forma adecuada
   {
      printf("Error falta el numero de hijos que se desea crear.\n");
      printf("Llame al programa de la siguiente forma: (./Extra n) donde n es el numero de hijos que desea crear \n");
      exit(EXIT_FAILURE);
   }

   int id_Memoria;//Variable para almacenar el identificador de memoria compartida
	int id_Semaforo;//Variable para almacenar el identificador de memoria compartida del semaforo
   int *contador = NULL;//Puntero a la zona de memoria compartida
	sem_t *mutex = NULL;//Puntero a la zona de memoria compartida del semaforo
   int value;//Almacena el valor devuelto por shmdt al realizar la desconexión

   key_t Clave = ftok("Extra.c",3); //Usamos ftok para generar la clave de acceso a la zona de memoria compartida
   if (Clave == -1) //Comprobamos que la clave se haya generado correctamente
   {
      perror("Error al generar la clave");
		printf("Valor de errno=%d, definido como %s\n", errno, strerror(errno));  
		exit(EXIT_FAILURE);
   }

   id_Memoria = shmget (Clave, sizeof(int), IPC_CREAT | SHM_R | SHM_W);//Solicitamos un segmento de memoria compartida
	if (id_Memoria == -1)//Comprobamos que se la solicitud se haya completado satisfactoriamente
	{
		printf("Main() de E1... No consigo ID para la memoria compartida.\n");
		printf("Valor de errno=%d, definido como %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

   contador = (int *)shmat(id_Memoria, NULL, 0);//Enganchamos el segmento con el proceso
	if (contador == NULL)
	{
		printf("Main() de demo1... No consigo enlace a la memoria compartida.\n");
		printf("Valor de errno=%d, definido como %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	id_Semaforo = shmget (Clave+1, sizeof(sem_t), IPC_CREAT | SHM_R | SHM_W);//Solicitamos un segmento de memoria compartida
	if (id_Semaforo == -1)//Comprobamos que se la solicitud se haya completado satisfactoriamente
	{
		printf("Main() de E1... No consigo ID para la memoria compartida.\n");
		printf("Valor de errno=%d, definido como %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	mutex = (sem_t *)shmat(id_Semaforo, NULL, 0);//Enganchamos el segmento con el proceso
	if (mutex == NULL)
	{
		printf("Main() de demo1... No consigo enlace a la memoria compartida.\n");
		printf("Valor de errno=%d, definido como %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	sem_init(mutex, 1, 1);//Inicializamos el semaforo que controla el acceso a la zona crítica
  	int status; //Creamos la variable que almacena el número de hijos y la que almacena el estado de salida del hijo
  	pid_t pid[2], childpid; //Estas variables almacenan el id de los procesos hijos. 
  	//Pid almacena el valor devuelto al padre tras el fork y chilpid el valor devuelto al padre por la función wait cuando termina de esperar al hijo 
  	printf("Soy %d el padre de todos\n", getpid()); //El proceso padre imprime su id
  	for (int i = 0; i < atoi(argv[1]); i++) //Cuando hacemos el fork la variable i es distinta en cada caso 
  	{//Se crean bucles diferentes e independientes
   	pid[i] = fork(); // Aqúi el proceso tiene su hijo. En el padre pid valdrá el id del hijo y en el hijo pid valdrá 0
   	switch(pid[i]) //En base al valor de pid cada proceso ejecutará una función
   	{
   		case 0: //El fork se ha realizado corractamente
   	   for (int i = 0; i < 100000; i++)
   	   {//No hace falta linkearlo ya que con el fork hereda el puntero contador del padre
         	sem_wait(mutex);
				printf("Soy %d: \n", getpid()); //El hijo se identifica 
         	printf("El valor de la variable es: %d\n", *contador);//Imprime e incrementa el valor de la variable de la zona compartida
				++*contador;
				sem_post(mutex);
        	}
        	value = shmdt ((char *)contador);
	     	if (value == -1)//Comprobamos el valor devuelto por shmdt al realizar la desconexión
	     	{
	     		printf("Error en shmdt...\n");
	     		printf("Valor de errno=%d, definido como %s\n", errno, strerror(errno));
	     		exit(EXIT_FAILURE);
	     	}
	  		value = shmdt ((sem_t *)mutex);
	     	if (value == -1)//Comprobamos el valor devuelto por shmdt al realizar la desconexión
	     	{
	     		printf("Error en shmdt...\n");
	     		printf("Valor de errno=%d, definido como %s\n", errno, strerror(errno));
	     		exit(EXIT_FAILURE);
	     	}
        	exit(EXIT_SUCCESS); //El hijo muere

      	case -1: //Ha ocurrido un error al realizar el fork
      		printf("Error al crear el proceso hijo\n"); //Se informa al usuario
      	 	exit(EXIT_FAILURE); //Indica que ha ocurrido un fallo en la ejecución

      	default:
      		printf("Esperando a que acabe mi hijo nº %d\n", i+1);
   	 }//Como el padre no ha hecho exit continua con el for y crea otro hijo
  	}//Una vez el padre ha terminado de crear los hijos que le hemos solicitado empieza a esperarlos
	sleep(1);
	kill(pid[0], SIGKILL);
  	while ( (childpid=waitpid(-1, &status, WUNTRACED | WCONTINUED)) > 0 )//Si lo hacemos así en vez de con wait podemos saber si el proceso ha sido pausado y poniendo -1 en el primer parametro de waitpid esperamos a cualquier hijo
	{//Este bucle se repetirá mientas haya hijos que esperar cuando no haya mas wait devolverá -1
		if (WIFEXITED(status)) 
		{//Entrará en el caso de que el hijo haya finaizado correctamente ya que WIFEXITED(status) devolverá true
			printf("Proceso padre %d, hijo con PID %ld finalizado, status = %d\n", getpid(), (long int)childpid, WEXITSTATUS(status));
		} 
		else if (WIFSIGNALED(status))
		{//Entrará en el caso de que el proceso haya finalizado debido a una señar externa ya sea de finalizar o matar 
			printf("Proceso padre %d, hijo con PID %ld finalizado al recibir la señal %d\n", getpid(), (long int)childpid, WTERMSIG(status));
		}//La macro WTERMSIG nos dice que señal ha sido la que ha recibido el proceso que ha producido que acabe 
	}
	if (childpid==(pid_t)-1 && errno==ECHILD) 
	{//Entra cuando vuelve al while y no hay más hijos que esperar porque en ese caso chilpid valdrá -1 y erno 10 que es el valor que devuelce ECHILD cuando no hay mas procesos hijo
		printf("Proceso padre %d, no hay mas hijos que esperar. Valor de errno = %d, definido como: %s\n", getpid(), errno, strerror(errno));
	}	//strerror devuelve una cadena de caracteres que nos permite identificar el valor de la variable errno
	else
	{//Solo entra si se ha producido un error con wait 
		printf("Error en la invocacion de wait o waitpid. Valor de errno = %d, definido como: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE); //Acaba el proceso padre con error
	} 
  	printf("El valor de la variable es: %d\n", *contador);//Imprimimos el valor de la variable
   value = shmctl (id_Memoria, IPC_RMID, (struct shmid_ds *)NULL);//Como no vamos a volver a usar la memoria compartida, la marcamos para borrado.
   if (value == -1)//Comprobamos el valor devuelto por shmctl
	{
		printf("Error en shmctl...\n");
		printf("Valor de errno=%d, definido como %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	 value = shmctl (id_Semaforo, IPC_RMID, (struct shmid_ds *)NULL);//Como no vamos a volver a usar la memoria compartida, la marcamos para borrado.
   if (value == -1)//Comprobamos el valor devuelto por shmctl
	{
		printf("Error en shmctl...\n");
		printf("Valor de errno=%d, definido como %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
   value = shmdt ((char *)contador);//Desconectamos el proceso padre de la zona de memoria compartida
   if (value == -1)//Comprobamos el valor de la desconexion
	{
		printf("Error en shmdt...\n");
		printf("Valor de errno=%d, definido como %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
  exit(EXIT_SUCCESS); //Como todo ha ido bien el proceso padre acaba exitosamente
}