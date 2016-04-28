#define _BSD_SOURCE
#define _SVID_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
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
void vota(int, int);
void cria_cabine(int);
void verifica_erro(int, const char[]);
void encerra_cabine(void);
void gera_relatorio(char[],char[]);
void inicia_relatorio(void);
void inicializa_semaforos(void);

pid_t *pids;
unsigned int it_pids;

sem_t *sem_senador, *sem_deputados, *sem_declarar_voto, *sem_entrada;

char *votos;
unsigned int *it_votos;

int shrm_id;

FILE * file;

int main(){
	int retorno;
	unsigned int i;
	unsigned int qtd_deputados, qtd_senadores, qtd_vereadores, qtd_politicos;
	unsigned int quantidade[2] = {0,0};

	inicia_relatorio();

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

	encerra_cabine();


	printf("\nVotação final: \n");
	for (i = 0; i < *it_votos; ++i){
		quantidade[(int)votos[i]]++;
	}
   
	fprintf(file, "\nVotos SIM : %d\n", quantidade[SIM]);
	printf("\tVotos SIM : %d\n", quantidade[SIM]);
	
	fprintf(file, "\nVotos NÃO : %d\n", quantidade[NAO]);
	printf("\tVotos NÃO : %d\n", quantidade[NAO]);
	
	fclose(file);
	
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
			sem_wait(sem_senador);
			vota(rand()%2, SENADOR);
			sem_post(sem_senador);
			break;

		case DEPUTADO:
			sem_wait(sem_deputados);
			sem_wait(sem_senador);
			sem_post(sem_senador);
			vota(rand()%2, DEPUTADO);
			sem_post(sem_deputados);
			break;

		case VEREADOR:
			sem_wait(sem_senador);
			sem_post(sem_senador);
			vota(rand()%2, VEREADOR);
			break;
	}
}

void vota(int voto, int parlamentar){
	char resposta[][5] = {"SIM", "NÃO"};
	char nomes[][15] = {"senador", "deputado", "vereador"};

	usleep(50);

	sem_wait(sem_declarar_voto);
	votos[(*it_votos)++] = voto;
	printf("Eu, na condição de %s, voto %s!\n", nomes[parlamentar], resposta[voto]);
	gera_relatorio(resposta[voto], nomes[parlamentar]);
	sem_post(sem_declarar_voto);
}

void verifica_erro(int valor, const char funcao[]){
	if(valor == -1){
		perror(funcao);
		exit(1);
	}
}

void cria_cabine(int n_politicos){
	void * shm_address;
	int key;
	srand((int)clock());
	key  = ftok("sem_project.c", rand());
	verifica_erro(key, "ftok");

	shrm_id = shmget(key, n_politicos + sizeof(unsigned int) + 3*sizeof(sem_t), 0666 | IPC_CREAT);
	verifica_erro(key, "shmget");

	shm_address = shmat(shrm_id, NULL, 0);
	verifica_erro((long long)shm_address, "shmat");

	it_votos = (unsigned int *) shm_address;
	*it_votos = 0;

	sem_senador = (sem_t *)(it_votos+1);
	sem_deputados = sem_senador+1;
	sem_declarar_voto = sem_deputados+1;

	votos = (char *)(sem_declarar_voto+1);

	inicializa_semaforos();
}

void encerra_cabine(){
	int retorno;

	retorno = shmctl(shrm_id, IPC_RMID, NULL);
	verifica_erro(retorno, "shmctl");
}

void inicia_relatorio(){
	file = fopen("resultado_final.txt", "w+");

	fprintf(file, "Hora            | Parlamentar   | Voto\n");
	fprintf(file, "-------------------------------------------\n");
	fclose(file);
	file = fopen("resultado_final.txt", "a+");
}

void gera_relatorio(char voto[], char parlamentar[]){
	struct timeval t;
	time_t hora_atual;
	struct tm *hora;

	gettimeofday(&t, NULL);
	hora_atual = t.tv_sec;
	hora = localtime(&hora_atual);

	fprintf(file, "%02d:%02d:%02d.%06d | %s\t    | %s\n", hora->tm_hour, hora->tm_min, hora->tm_sec, (int)t.tv_usec, parlamentar, voto);
	fprintf(file, "-------------------------------------------\n");
}

void inicializa_semaforos(){
	int retorno;

	retorno = sem_init(sem_senador, 1, 1);
	verifica_semaforo(retorno);

	retorno = sem_init(sem_deputados, 1, 5);
	verifica_semaforo(retorno);

	retorno = sem_init(sem_declarar_voto, 1, 1);
	verifica_semaforo(retorno);
}


