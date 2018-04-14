#include"header.h"
#include "functions.c"

// PRZYKŁADOWE ODPALENIE PROGRAMU: ./program -R -T 4 test1 test2

// -------------------------------------------
// ZMIENNE POMOCNICZE
// -------------------------------------------
int i, j;
int isNum = 0;
// -------------------------------------------
// ŚCIEŻKI
// -------------------------------------------
char src[30]; // sciezka source - zrodla
char dst[30]; //sciezka destination - docelowa
// -------------------------------------------
// PARAMETRY PROGRAMU
// -------------------------------------------
int recursion = 0; // parametr -R - rekurencja
int sleeplength = 5; //długość drzemki demona w sekundach
int size = 100000; // maxsize

int main(int argc,char* argv[])
{
    // -------------------------------------------
    // OBIEKTY SIGACTION
    // -------------------------------------------
    struct sigaction sleeper; // sigaction odpowiadajace za budzenie demona
    struct sigaction listener; // sigaction odpowiadajcy za nasluchiwanie sygnału SIGUSR1

    // -------------------------------------------
    // DEMONIZACJA
    // -------------------------------------------
    openlog("demon", LOG_PID, LOG_LOCAL1);
    daemon(3, 0); // demonizacja


    // -------------------------------------------
    // POBIERANIE ARGUMENTÓW
    // -------------------------------------------
    if (( argc < 3 ) || ( argc > 6 )) //sprawdzam liczbę parametrów funkcji
    {
        syslog(LOG_ERR,"niewłasciwe wywolanie funkcji");
        exit(1);
    }

    // szukam ścieżki źródłowej - pierwszy argument ktory nie jest -R, -T LICZBA, -S LICZBA uznaje za sciezke zrodlową
    while (src && !src[0]) {
        for(i=1;i<argc;i++) {
            if (argv[i][0] != '-') {
                strcpy(src, argv[i]); // umieszczam w zmiennej src
                i++;
                break;
            } else {
                if(argv[i][1] != 'R') { // bo -R jest bez argumentu pomocniczego
                    i++; // pomijam liczbę która nie jest ścieżką
                }
            }
        }
        
    }

    while (dst && !dst[0]) { // to samo co przy ścieżce źródłowej, tylko że szukam ścieżki docelowej
        for(i;i<argc;i++) { // samo i, a nie i=1 bo szukam od miejsca w którym znalazłem ścieżkę źródłową
            if (argv[i][0] != '-') {
                strcpy(dst, argv[i]);
                break;
            } else {
                i++;
            }
        }
    }  
    if (access(src,0) == -1 || access(dst,0) == -1 || if_Dir(src) != 1 || if_Dir(dst) != 1) //czy jest dostęp i czy jest katalogiem
    {
        syslog(LOG_ERR,"Nieprawidłowe argumenty - brak ścieżki lub nie jest katalogiem \n");
        syslog(LOG_ERR,"Sciezka1 - '%s'\n",src);
        syslog(LOG_ERR,"Sciezka2 - '%s'\n",dst);
        exit(1);
    }

    // sprawdzam parametr rekurencji '-R'
    for(i=1;i<argc;i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'R') {
            recursion = 1;
            break;
        }
    }
    // sprawdzam parametr czasu '-T'
    for(i=1;i<argc;i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'T') {
            isNum = 1;
            for (j=0; j < strlen(argv[i+1]); ++j)
            {
                if (!(isdigit(argv[i+1][j])))
                {
                    isNum = 0;
                    break;
                }
            }
            if(isNum == 1) sleeplength = atoi(argv[i+1]); //atoi zamienia argument na liczbę
            break;
        }
    }
    // sprawdzam parametr rozmiaru '-S'
    for(i=1;i<argc;i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'S') {
            isNum = 1;
            for (j=0; j < strlen(argv[i+1]); ++j)
            {
                if (!(isdigit(argv[i+1][j])))
                {
                    isNum = 0;
                    break;
                }
            }
            if(isNum == 1) size = atoi(argv[i+1]); //atoi zamienia argument na liczbę
            break;
        }
    }


    // -------------------------------------------
    // KONFIGURACJA SYGNAŁÓW 
    // -------------------------------------------
    
    sleeper.sa_handler = &fsleep; // ustawiam handler
    sigaction(SIGALRM, &sleeper, NULL);  // ustawiam akcję w przypadku sygnału alarmu (funckja alarmu wywołana jest niżej)
    listener.sa_handler = &fsig;  // ustawiam handler
    sigaction(SIGUSR1, &listener, NULL); // ustawiam akcję w przypadku sygnału SIGUSR1, budzę demona

    // -------------------------------------------
    // GŁÓWNA PĘTLA PROGRAMU
    // -------------------------------------------
    while(1 == 1)
    {
        syslog(LOG_INFO,"demon śpi");
        alarm(sleeplength); // funkcja wywołuje sygnał SIGALRM po sleeplength
        pause();
        syslog(LOG_INFO,"demon budzi sie");
        synchronize(src, dst, recursion, size);
        deleteExtras(src, dst, recursion);
    }
    closelog ();
    return 0;
}
