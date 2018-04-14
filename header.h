#ifndef HEADER_H_INCLUDED
#define HEADER_H_INCLUDED
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <syslog.h>
#include <signal.h>
#include <sys/syslog.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFSIZE 1024

 
int mode(struct stat info); //funkcja zwraca wartosc ktora reprezentuje rodzaj: 1-folder, 0-plik, - 1 inne
void fsleep(int sig); //funkcja dla wywołania sygnału SIGALRM
void fsig(int sig); //funkcja dla wywołania sygnału SIGUSR1
int if_Dir(char* path); //sprawdza, czy dana ścieżka jest katalogiem
int copy(char *source, char *target); //funkcja kopiujaca pliki
int copy_mmap(char *source, char *target, struct stat *st);//funkcja kopiująca pliki poprzez mapowanie
int synchronize(char *src, char *dst, int rec, long int size);//funkcja synchronizuje podkatalogi
void deleteExtras(char *src, char *dst, int rec);//funkcja usuwa zbędne foldery z katalogu docelowego
 
#endif // HEADER_H_INCLUDED