// gcc notasmemoria.c -o notas

#include <stdio.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <wait.h>
#include <unistd.h>
#include <semaphore.h>

#define IPC_RESULT_ERROR (-1)

#ifndef N_NOTAS
#define N_NOTAS 100 //Valor de notas default
#endif

/*Definição do número de Processos*/
#ifndef PROCESS
#define PROCESS 10
#endif

typedef struct
{            //Estrutura utilizada para comunicação dos processos
  pid_t pid; //process id
  int id;    //id recebido
} vet_filhos;

vet_filhos proc_filho[PROCESS];

typedef struct
{
  int id;
  float n1, n2, n3;
  float media;
} read_notas;

read_notas *notas;

int proporcao = N_NOTAS / PROCESS;

int main(int argc, char *argv[])
{
  int shmid;
  char *path = "notas.lst";
  int key_id = 'S';
  key_t key = ftok(path, key_id);

  FILE *fp;
  fp = fopen("notas.lst", "r"); //testar rb depois
  fseek(fp, 0, SEEK_END);
  int lengthOfFile = ftell(fp);
  fclose(fp);
  const int shared_segment_size = lengthOfFile; //Bloco de memória

  pid_t proc;
  int id, i, status, aux;
  float n1, n2, n3;

  for (int i = 0; i < PROCESS; i++)
  {
    aux = i * (proporcao) + 1;         //Variável auxiliar para ajudar no laço do bloco
    proc = proc_filho[i].pid = fork(); //Criação do processo filho
    if (proc > 0)
    {
      /* Aloca o segmento de memória */
      //printf("\nKey: %d", key);
      shmid = shmget(key * i, sizeof(lengthOfFile / N_NOTAS), IPC_CREAT | 0666);
      //printf("\nShmid: %d ", shmid);

      if (shmid == -1){
        printf("\nDeu ruim na alocação");
        exit(-1);
      }
      /* Attach no segmento */
      read_notas *shared_memory = shmat(shmid, NULL, 0);

      if ((void *)shared_memory == (void *)-1)
      {
        printf("\nErro no attach");
        exit(-1);
      }

      /*Escrevendo na memória*/
      fp = fopen("notas.lst", "r");

      while (!feof(fp) && id < aux - 1)
      { //Enquanto não chegar no final do arquivo e enquanto id < aux-1
        fscanf(fp, "%d %f %f %f\n", &id, &n1, &n2, &n3);
      }

      for (int j = aux; j < aux + proporcao + 1; j++)
      {
        fscanf(fp, "%d %f %f %f\n", &id, &n1, &n2, &n3);
        shared_memory[i].id = id;
        shared_memory[i].n1 = n1;
        shared_memory[i].n2 = n2;
        shared_memory[i].n3 = n3;
        printf("\nId testando: %d", shared_memory[i].id);
      }
      fclose(fp);
      if (shmdt(shared_memory) == -1)
      {
        printf("\nErro no detach");
        exit(-1);
      }
      sleep(1);
    }
    else if (proc == 0)
    {
      shmid = shmget(key * i, sizeof(lengthOfFile / N_NOTAS), IPC_CREAT | 0666);
      /* Reattach no segmento*/
      read_notas *shared_memory = shmat(shmid, NULL, 0);

      if ((void *)shared_memory == (void *)-1)
      {
        printf("\nErro no attach (Filho)");
        exit(-1);
      }

      printf("\nshared memory reattached at address %p\n", shared_memory);
      for (int j = aux; j < aux + proporcao + 1; j++)
      {
        printf("\naux %d", aux);
        n1 = shared_memory[i].n1;
        n2 = shared_memory[i].n2;
        n3 = shared_memory[i].n3;
        shared_memory[i].media = (n1 + n2 < 14.0) ? ((n1 + n2 + n3) / 3.0) : ((n1 + n2) / 2.0); //Se n1+n2 <= 14, então soma n3 e divide por 3, senão, n1+n2/2
        //printf("\nId: %d || Média: %.2f", shared_memory[i].id, shared_memory[i].media);
      }

      /*Detach no segmento*/
      if (shmdt(shared_memory) == -1)
      {
        printf("\nErro no detach (Filho)");
        exit(-1);
      }
      /*Desaloca memória*/
      // shmctl (shmid, IPC_RMID, NULL);
      // printf("\nMemória desalocada com sucesso!");
    }
  }

  // shmid = shmget (key*i, sizeof(N_NOTAS), IPC_CREAT | 0666);
  // /* Reattach no segmento*/
  // read_notas *shared_memory = shmat(shmid, NULL, 0);
  // printf("|Aluno| - |Nota|\n");
  // for (int i = 0; i < N_NOTAS; i++) { //Para cada PROCESS
  //   //printf("\nId: %d || Média: %.2f", shared_memory[i].id, shared_memory[i].media);
  //   waitpid(proc_filho[i].pid, &status, 0); //Suspende a execução do processo e aguarda até que o filho encerre
  // }
}
