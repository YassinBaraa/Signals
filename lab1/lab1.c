//PRIORITETI SIGNALA SU 1.SIGTERM,2.SIGINT,3.SIGUSR1
#define _POSIX_C_SOURCE 199309L
#define _BSD_SOURCE

#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

struct timespec t0; /* vrijeme pocetka programa */

/*struct kontekst{*/
static int k_z[3] = {0,0,0}; //kontrolne zastavice
static int t_p = 0; //prioritet prekida(1,2,3)
static int sus_stog[3] = {0,0,0};
/*};*/
int prosli_prioritet=0;

/*funkcija koja prima signal*/
void obrada(int sig);
/* funkcije za obradu signala, navedene ispod main-a */
void obradi(int sig);

/* postavlja trenutno vrijeme u t0 */
void postavi_pocetno_vrijeme()
{
	clock_gettime(CLOCK_REALTIME, &t0);
}

/* dohvaca vrijeme proteklo od pokretanja programa */
void vrijeme(void)
{
	struct timespec t;

	clock_gettime(CLOCK_REALTIME, &t);

	t.tv_sec -= t0.tv_sec;
	t.tv_nsec -= t0.tv_nsec;
	if (t.tv_nsec < 0) {
		t.tv_nsec += 1000000000;
		t.tv_sec--;
	}

	printf("%03ld.%03ld:\t", t.tv_sec, t.tv_nsec/1000000);
}

/* ispis kao i printf uz dodatak trenutnog vremena na pocetku */
#define PRINTF(format, ...)       \
do {                              \
  vrijeme();                      \
  printf(format, ##__VA_ARGS__);  \
}                                 \
while(0)

/*
 * spava zadani broj sekundi
 * ako se prekine signalom, kasnije nastavlja spavati neprospavano
 */
void spavaj(time_t sekundi)
{
	struct timespec koliko;
	koliko.tv_sec = sekundi;
	koliko.tv_nsec = 0;

	while (nanosleep(&koliko, &koliko) == -1 && errno == EINTR){
		//PRINTF("Bio prekinut, nastavljam\n");
        }
}

void inicijalizacija()
{
	struct sigaction act;
	act.sa_handler = obrada;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NODEFER;
    //act.sa_flags = 0;
	act.sa_handler = obrada;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGUSR1, &act, NULL);

	postavi_pocetno_vrijeme();
}

int main()
{
    inicijalizacija();
    vrijeme();
    printf("Program s PID=%ld krenuo s radom\n", (long)getpid());
    printf("\t\tK_Z=000, T_P=%d, stog: -\n",t_p);

    //cekanje prekida
    while(1){
        pause();
    }

    PRINTF("G: Kraj glavnog programa\n");
}

void obrada(int sig)
{
    //struct kontekst kon;
    int prioritet = 0;

    if(sig == SIGUSR1){
        prioritet = 3;
    }
    else if(sig  == SIGINT){
        prioritet = 2;
    }
    else if(sig == SIGTERM){
        prioritet = 1;
    }

/////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    //stavljanje na k_z
    k_z[prioritet-1] = 1;
    //provjera prioriteta
    //ako je novi signal veceg prioriteta treba se prekinuti onaj koji se obraduje i pohraniti njegov kontekst

    if(t_p == 0)
    {
        prosli_prioritet =  t_p;
        t_p = prioritet;
        //(ne stavljamo na stog)

        //prekid je def prihvacen, mice se sa k_z
        k_z[t_p-1] = 0;
        //obrada novog prioriteta
        obradi(t_p);
    }
    else if (t_p == prioritet)
    {
        vrijeme();
        printf("Pojavio se prioritet jednakog stupnja od onog koji se obraduje, sprema se na k_z\n");
        printf("\t\tK_Z=%d%d%d, T_P=%d, stog: %d%d%d\n",k_z[0],k_z[1],k_z[2], t_p,sus_stog[0],sus_stog[1],sus_stog[2]);
        k_z[prioritet-1] =1;
    }
    else if(t_p < prioritet && t_p != 0)
    {
        //pohrana prijasnjg prioriteta na sus_stog
        sus_stog[t_p - 1] = 1;

        prosli_prioritet =  t_p;
        t_p = prioritet;
        //prekid je def prihvacen, mice se sa k_z
        k_z[prioritet-1] = 0;
        //obrada novog prioriteta
        obradi(t_p);

        int naj_prio_stog = -1;
        for(int i=0;i<3;i++){
            if(sus_stog[i] == 1){
                naj_prio_stog = i+1;
            }
        }
        int naj_prio_zas = -1;
        for(int i=0;i<3;i++){
            if(k_z[i] == 1){
                naj_prio_zas = i+1;
            }
        }

        //obraduje prioritete koji su na zastavicama (veceg prioriteta neg oni na stogu)
        if(naj_prio_zas > naj_prio_stog)
        {
            t_p = naj_prio_zas;
            k_z[naj_prio_zas-1] = 0;
            obradi(naj_prio_zas);
        }

    
        sus_stog[prosli_prioritet - 1] = 0;
    }
    //ako je signal istog ili manjeg prioriteta nego onaj koji se trenutno obraduje
    else if(t_p > prioritet && t_p != 0)
    {
       //ostavljamo zahtjev na k_z
       printf("Pojavio se prioritet manjeg stupnja od onog koji se obraduje, sprema se na k_z\n");
       printf("\t\tK_Z=%d%d%d, T_P=%d, stog: %d%d%d\n",k_z[0],k_z[1],k_z[2], t_p,sus_stog[0],sus_stog[1],sus_stog[2]);
       
    }
    
}

void obradi(int sig)
{ 
 
    vrijeme();
    printf("Pocetak obrade signala %d. prioriteta\n", sig);
    printf("\t\tK_Z=%d%d%d, T_P=%d, stog: %d%d%d\n",k_z[0],k_z[1],k_z[2], t_p,sus_stog[0],sus_stog[1],sus_stog[2]);
    spavaj(20);


    vrijeme();
    printf("Kraj obrade signala %d. prioriteta\n", sig);


    int naj_prio_zas = 0;
    for(int i=0;i<3;i++){
        if(k_z[i] == 1){
            naj_prio_zas = i+1;
        }
    }
//rubni slucaj kad je najmanji prioritet na zastavicama
    if(sus_stog[0] == 0 && sus_stog[1] == 0 && sus_stog[2] == 0)
    {
            k_z[naj_prio_zas-1] = 0;
            t_p = naj_prio_zas;
            obradi(naj_prio_zas);
    }
    
}
