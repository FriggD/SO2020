
/*
        UNIVERSIDADE ESTADUAL DE PONTA GROSSA
        SETOR DE CIÊNCIAS AGRÁRIAS E DE TECNOLOGIA
        DEPARTAMENTO DE INFORMÁTICA

        Aluna Gláucia Dias
        Sistemas Operacionais
        Prof Dierone Foltran
*/

/*
Implementar um programa para:
	- calcular de forma concorrente as médias individuais de um grupo de alunos;
	- o processo filho deverá: receber um ID inicial e a quantidade de notas que deverá calcular;
		- passagem de parâmetro para o processo filho!
	- devolver as médias ao processo PAI através de PIPE();
		- poderá utilizar FIFO()!
                - ou qualquer outro mecanismo descrito como IPC;
*/

/*
    *SUMÁRIO
    1. Includes necessários para o desenvolvimento
        1.1 <stdio.h>
        1.2 <stdlib.h>
        1.3 <unistd.h>
        1.4 <math.h>
        1.5 <sys/stat.h>
        1.6 <sys/types.h>
        1.7 <errno.h>
        1.8 <fcntl.h>

    2. Definição de constantes
        2.1 NUM_WORKERS
        2.2 NUM_ALUNOS  

    3. Definição do tipo de estrutura ALUNO 

    4. Função void writter
        A função void writter abre o arquivo "Fifo_Alunos", em somente escrita;
        Inicializa os alunos;
        Para cada nota de cada aluno atribui um valor randomico de 0 a 100;
        Insere os valores na fifo(), se houver algum erro, ele retornará o valor -1;
        Depois que foi escrito, aguarda as respostas dos "Workers".  

    5. Função void workers 
        Cria n processos concorrentes e divide a tarefa entre eles;
        Abre o arquivo "Fifo_Alunos", em somente leitura;
        Leem o numero de médias que eles precisam calcular;
        Calculam as médias;
        Escrevem no "Fifo_Médias" 

    6. Função main   
        Faz um fork para dois processos;
        Um deles vai ser o writter e outro vai ser os workers
*/

//Includes necessários para o desenvolvimento
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>

//definição de constantes
#define NUM_WORKERS 50
#define NUM_ALUNOS 10000

//  Tipo de Dados do Aluno
typedef struct {
    int RA;  
    int media; 
    int notas[4];
} aluno;

/*
    Troca de Dados entre dois Processos
    1 - Writter: Envia os alunos para que sejam calculados as médias. E depois fica ouvindo a FIFO para as respostas
    2 - Workers: Aceita a requisição e devolve as respostas no FIFO
*/

void writter () {
    //abre o arquivi em modo somente escrita 
    int fd = open("Fifo_Alunos", O_WRONLY);

    //insere um valor randomico para cada nota de cada aluno
    aluno alunosArr[NUM_ALUNOS];
    for (int aluno_idx=0; aluno_idx<NUM_ALUNOS; aluno_idx++) {
        for (int nota_idx=0; nota_idx<4; nota_idx++) {
            alunosArr[aluno_idx].RA = aluno_idx;
            alunosArr[aluno_idx].notas[nota_idx] = rand() % 100;
        }        
    }
    //tratamento de erro caso ocorra erro na escrita da fifo()
    if (write(fd, alunosArr, sizeof(aluno) * NUM_ALUNOS) == -1) {
        printf("Erro ao escrever no FIFO");
    }
    close(fd);

    //tratamento de erro caso ocorra erro na abertura do arquivo
    int fd_medias = open("Fifo_Medias", O_RDONLY);
    if (fd_medias ==-1) { 
        printf("Erro ao abrir FIFO!");
    }

    //Espera as respostas dos workers
    //sempre que é escrita na fifo algum valor, esse valor é lido
    for(int cont=0; cont<NUM_ALUNOS; cont++) {
        int readStatus = 0;
        do {
            readStatus = read(fd_medias, &alunosArr[cont], sizeof(aluno));
            if (readStatus <0) {
                printf("Erro ao ler o FIFO!");
            }
        } while(readStatus==0);

        printf("\n%d Aluno com RA %d possui média: %d", cont, alunosArr[cont].RA, alunosArr[cont].media);
       
        
        // alunosArr[cont] = cur_aluno;
    }    
    close(fd_medias);
    
    // return &alunosArr;
}

void workers () {
    // ############ Pega todos os Alunos ############
    int fd_alunos = open("Fifo_Alunos", O_RDONLY);
    //tratamento de erro 
    if (fd_alunos ==-1) { 
        printf("Erro ao abrir FIFO!");
    }   

    aluno alunos[NUM_ALUNOS];  
    int readResponse = read(fd_alunos, &alunos, sizeof(aluno) * NUM_ALUNOS);
    if (readResponse == -1) {
        printf("Erro ao ler do Fifo");
        close(fd_alunos);
        return;
    }   

    // ############ Criação dos Workers #############    
    int numMediasResolver;
    int inicio;
    fflush(stdout);

    int pid = fork();
    for(int i=0;i<(NUM_WORKERS - 1);i++) {
        if (pid != 0 && getppid() != 1) {
            pid = fork();

           numMediasResolver = NUM_ALUNOS % NUM_WORKERS + floor(NUM_ALUNOS / NUM_WORKERS); 
            inicio = NUM_ALUNOS - numMediasResolver;          
        }
        else {
            numMediasResolver = floor(NUM_ALUNOS / NUM_WORKERS);
            inicio = numMediasResolver*i;            
            break;
        }   
	}
    if (pid != 0 && getppid() != 1) {
        // Processo pai
        exit(0);
    } else {
        // n Processos filhos
    }
      
    
    // ############ Worker #############
    printf("\nPreciso Resolver %d", numMediasResolver);
    fflush(stdout);
    aluno alunosCalculados[numMediasResolver];   

    for (int cont = 0; cont < numMediasResolver; cont++) {
        aluno cur_aluno = alunos[inicio + cont];
        cur_aluno.media = (cur_aluno.notas[0] +  cur_aluno.notas[1] +  cur_aluno.notas[2] +  cur_aluno.notas[3]) / 4;
        // printf("\nRA: %d Media: %d", cur_aluno.RA, cur_aluno.media);
        alunosCalculados[cont] = cur_aluno;
    }

    close(fd_alunos);

    int fd_medias = open("Fifo_Medias", O_WRONLY);
    if (fd_medias ==-1) { 
        printf("Erro ao abrir FIFO!");
    }

    for(int cont=0; cont<numMediasResolver; cont++) {
        // printf("\nEscrevendo RA: %d Media: %d", alunosCalculados[cont].RA, alunosCalculados[cont].media);
        if (write(fd_medias, &alunosCalculados[cont], sizeof(aluno)) == -1) {
            printf("Erro ao escrever no FIFO");
        } 
    }
    
    close(fd_medias);

    exit(1);
}

int main(int argc, char const *argv[]) {    
    // Abrir Fifo
    if (mkfifo("Fifo_Alunos", 0777) == -1) {
        if (errno != EEXIST) {
            // Erro Inesperado
            printf("Erro ao criar FIFO");
            return 1;
        }
    }

    if (mkfifo("Fifo_Medias", 0777) == -1) {
        if (errno != EEXIST) {
            // Erro Inesperado
            printf("Erro ao criar FIFO");
            return 1;
        }
    }

    fflush(stdout);
    int id = fork();

    if (id != 0) {
        writter();
    } else {
        workers();
        exit(0);
    }

    return 0;    
}

/*
    *Principais referencias:
        1. https://www.youtube.com/playlist?list=PLfqABt5AS4FkW5mOn2Tn9ZZLLDwA3kZUY
*/