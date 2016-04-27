#define _BSD_SOURCE
#define _SVID_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/shm.h>

#define CHILD 0

enum{
	SENADOR,
	DEPUTADO,
	VEREADOR	
};

enum{
    SIM,
    NAO
};

void verifica_leitura(int);
void verifica_semaforo(int);
void novo_politico(int);
void medita(void);
void entra(int);
void vota(int);
void cria_cabine(int);
void verifica_erro(int, const char[]);

pid_t *pids;
unsigned int it_pids;

sem_t sem_senador, sem_deputados, sem_declarar_voto;

char *votos;
unsigned int *it_votos;

int main(){
	int retorno;
	unsigned int i;
	unsigned int qtd_deputados, qtd_senadores, qtd_vereadores, qtd_politicos;
	unsigned int quantidade[2] = {0,0};

	retorno = sem_init(&sem_senador, 1, 1);
	verifica_semaforo(retorno);

	retorno = sem_init(&sem_deputados, 1, 5);
	verifica_semaforo(retorno);

	retorno = sem_init(&sem_declarar_voto, 1, 1);
	verifica_semaforo(retorno);

	printf("Número de senadores: ");
	retorno = scanf("%u", &qtd_senadores);
	verifica_leitura(retorno);

	printf("Número de deputados: ");
	retorno = scanf("%u", &qtd_deputados);	
	verifica_leitura(retorno);

	printf("Número de vereadores: ");
	retorno = scanf("%u", &qtd_vereadores);
	verifica_leitura(retorno);

	qtd_politicos = qtd_senadores + qtd_deputados + qtd_vereadores;
	cria_cabine(qtd_politicos);
	pids = (pid_t *) calloc(qtd_politicos, sizeof (pid_t));
	it_pids = 0;

	for(i = 0; i < qtd_senadores; ++i){
		novo_politico(SENADOR);
	}

	for(i = 0; i < qtd_deputados; ++i){
		novo_politico(DEPUTADO);
	}

	for(i = 0; i < qtd_vereadores; ++i){
		novo_politico(VEREADOR);
	}

	for(i = 0; i < it_pids; ++i){
		waitpid(pids[i], &retorno, 0);
	}

	printf("\nVotação final: \n");
	for (i = 0; i < *it_votos; ++i){
		quantidade[(int)votos[i]]++;
	}	
		printf("Votos SIM : %d\n", quantidade[SIM]);
		printf("Votos NAO : %d\n", quantidade[NAO]);

	sem_destroy(&sem_senador);
	sem_destroy(&sem_deputados);
	sem_destroy(&sem_declarar_voto);

	shmdt(it_votos);

	return 0;
}

void verifica_leitura(int retorno){
	if(retorno != 1){
		printf("Erro na leitura de dados\n");
		exit(1);
	}
}

void verifica_semaforo(int retorno){
	if(retorno != 0){
		printf("Erro na inicialização do semáforo\n");
		exit(1);
	}
}

void novo_politico(int cargo){
	int pid = fork();
	if(pid == CHILD){
		medita();
		entra(cargo);
		exit(0);
	}
	
	pids[it_pids++] = pid;
}

void medita(){
	int meditation_time;
	
	srand((int) clock());
	meditation_time = rand()%1001;
	
	usleep(meditation_time);
}

void entra(int cargo){
	srand((int) clock());	
	
	switch(cargo){
		
		case SENADOR:
			sem_wait(&sem_senador);
			vota(rand()%2);
			sem_post(&sem_senador);
			break;

		case DEPUTADO:
			sem_wait(&sem_senador);
			sem_post(&sem_senador);
			sem_wait(&sem_deputados);
			vota(rand()%2);
			sem_post(&sem_deputados);
			break;

		case VEREADOR:
			sem_wait(&sem_senador);
			sem_post(&sem_senador);
			vota(rand()%2);
			break;
	}
}

void vota(int voto){
	char resposta[][5] = {"SIM", "NÃO"};
	sem_wait(&sem_declarar_voto);
	usleep(50);
	votos[(*it_votos)++] = voto;
	printf("Eu votei %s\n", resposta[voto]);
	sem_post(&sem_declarar_voto);
}

void verifica_erro(int valor, const char funcao[]){
     if(valor == -1){
        perror(funcao);
        exit(1);
    }
}

void cria_cabine(int n_politicos){
    void * shm_address;
    int shrm_id;
    int key;
    srand((int)clock());
    key  = ftok("sem_project.c", rand());

    verifica_erro(key, "ftok");

    shrm_id = shmget(key, n_politicos + sizeof(int), 0666 | IPC_CREAT);
    verifica_erro(key, "shmget");
    
    shm_address = shmat(shrm_id, NULL, 0);
    verifica_erro((long long)shm_address, "shmat");
   	it_votos = (unsigned int *) shm_address;
   	*it_votos = 0;
    votos = (char *)(it_votos+1);
}

