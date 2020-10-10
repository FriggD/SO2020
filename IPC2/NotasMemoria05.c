/*
 *  ALUNA: Gláucia Maria Beló Dias
 *  RA: 17242826
 *  Cálculo de médias usando IPC e Memória Compartilhada  
 */

/*
 * - SUMÁRIO
 * 1. Importações
 * 2. Estruturas
 * 3. Definições
 * 4. Função que escreve na memória 
 * 5. Função processes
 *  5.1. Cria um número _PROCESS_COUNT_ de processos filhos. 
 *    A carga de notas é dividida entre cada um dos filhos. 
 *    Cada um deles vai ler o numero de notas que foi designado e calcular as medias e vai escrever na memoria.
 * 6. Função principal (main)
 */

#include <stdio.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <wait.h>
#include <unistd.h>

#define ALUNOS_CONT 100 // Número de Alunos(Médias) a calcular
#define PROCESS_COUNT 10 // Número de processos

/*Estrutura aluno que conterá as notas de cada aluno*/
struct aluno {
  int id;
  float nota1, nota2, nota3;
  float media;
};

/*Definição da estrutura de memória compartilhada*/
typedef struct {
  struct aluno notas[ALUNOS_CONT]; //Vetor de estruturas do tipo aluno(contém as notas)
} mem;

int write_mem() {
  int shmid, key_id = 'S', id;
  float nota1, nota2, nota3;
  FILE *fp;

  shmid = shmget(IPC_PRIVATE, sizeof(mem), IPC_CREAT | 0666);

  /* Attach no segmento */
  mem *shared_memory = shmat(shmid, NULL, 0);

  if ((void *)shared_memory == (void *)-1) {
    printf("\nErro no attach");
    return -1;
  }

  /* Escreve no shared_memory */
  fp = fopen("notas.lst", "r");
  int i = 0;
  for (
    i=0;
    fscanf(fp, "%d %f %f %f\n", &shared_memory->notas[i].id, &shared_memory->notas[i].nota1, &shared_memory->notas[i].nota2, &shared_memory->notas[i].nota3) == 4; 
    i++);

  fclose(fp);

  if (shmdt(shared_memory) == -1) {
    printf("\nErro shmdt");
    return -1;
  }

  /*Retorna o segmento de memória criado*/
  return shmid;
}

void processes (int shmid) {
  int proc_medias_cont, aux;
  float nota1, nota2, nota3;
  pid_t proc;

  for (int i = 0; i < PROCESS_COUNT; i++) {
    if (i == PROCESS_COUNT - 1) {      
      // Último processo calcula as médias restantes
      proc_medias_cont = (ALUNOS_CONT / PROCESS_COUNT) + (ALUNOS_CONT % PROCESS_COUNT);
      fflush(stdout);
    } else {
      // printf("eoq\n");
      proc_medias_cont = ALUNOS_CONT / PROCESS_COUNT;
    }    
    aux = i * (ALUNOS_CONT / PROCESS_COUNT);
    proc = fork();

    if (proc == -1) {
      printf("\nFalha ao fazer um fork()");
      exit(-1);
    } else if (proc == 0) {
      // Filhos
      mem *shared_memory = shmat(shmid, NULL, 0);

      for (int j = aux; j < aux + proc_medias_cont + 1; j++) {
        nota1 = shared_memory->notas[j].nota1;
        nota2 = shared_memory->notas[j].nota2;
        nota3 = shared_memory->notas[j].nota3;
        shared_memory->notas[j].media = (nota1 + nota2 < 14.0) ? ((nota1 + nota2 + nota3) / 3.0) : ((nota1 + nota2) / 2.0);
      }

      // Dettach
      shmdt(shared_memory);

      exit(0);
    }
  }
}

int main(int argc, char *argv[]) {
  /* Variáveis Main */
  int shmid, status;  

  /*Função de criação de memória e escrita das notas*/

  /*Pega o valor de retorno da função, que é o segmento de memória*/
  shmid = write_mem();

  // Cria os processos filhos e processa todas as médias
  processes(shmid);

  // espera ocupada pelos filhos
  while ((wait(&status)) > 0);

  /* Reattach no segmento*/
  mem *shared_memory = shmat(shmid, NULL, 0);

  /*Exibição das notas*/
  for (int i = 0; i < ALUNOS_CONT; i++)  {
    printf("Aluno: %d ([%.2f, %.2f, %.2f] : %.2f)  \n", shared_memory->notas[i].id, shared_memory->notas[i].nota1, shared_memory->notas[i].nota2, shared_memory->notas[i].nota3, shared_memory->notas[i].media);
  }

  /*Detach no segmento*/
  if (shmdt(shared_memory) == -1) {
    printf("\nErro shmdt");
    exit(-1);
  }

  shmctl(shmid, IPC_RMID, NULL);
}

/*  
*  Foram utilizadas as seguintes referências:
*  https://www.geeksforgeeks.org/ipc-shared-memory
*  https://opensource.com/article/19/4/interprocess-communication-linux-storage 
*  http://www.inf.ufes.br/~rgomes/so_fichiers/aula15x4.pdf
*  http://users.cs.cf.ac.uk/Dave.Marshall/C/node27.html
*  http://www.csl.mtu.edu/cs4411.ck/www/NOTES/process/shm/example-2.html
*  Advanced Linux Programming - Capítulos 3, 4 e 5
*/