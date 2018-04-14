 #include"header.h"
 // zmienne pomocniczne
int i, j;
//---------------------------------------------


// funkcja sprawdza co opisuje struktura podana jako argument,
// zwraca 1 jesli jest folderem, 0 jesli regular file, -1 jesli cos innego 
int mode(struct stat info)
{
    if(S_ISDIR(info.st_mode)) return 1;
    else
    if(S_ISREG(info.st_mode)) return 0;
    else return -1;
}
//---------------------------------------------
// funkcje dzialajace jako handlery, wysylajace logi gdy zostana wywolane
void fsleep(int sig)
{
    syslog(LOG_INFO,"demon obudził się z drzemki");
} 

void fsig(int sig)
{
    syslog(LOG_INFO,"obudzenie sygnałem SIGUSR1");
}

 

 
//Funcja sprawdza czy podana sciazka jest katalogiem
//tworzy strukture stat od sciezki i wywoluje funkcje mode

int if_Dir(char* path)
{
    struct stat info;
    if((stat(path, &info)) < 0) //jeśli nie udało się pobranie informacji o ścieżce, zwracana jest wartość -1
    {
        syslog(LOG_ERR,"nieudane pobranie informacji o pliku\n");
        return -1;
    }
    else return mode(info);
}
 
//FUNKCJA KOPIUJĄCA PLIKI
// no kopiowanie, co tu opisywac
 
int copy(char *source, char *target)
{
    int zrodlowy, docelowy, przeczytanych;
    char buf[BUFSIZE];
    zrodlowy = open(source, O_RDONLY);
    docelowy = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    syslog(LOG_INFO,"plik z %s został skopiowany do %s",source,target);
    do
    {
        przeczytanych = read(zrodlowy, buf, BUFSIZE);
        if((write(docelowy, buf, przeczytanych)) < 0)
        {
            syslog(LOG_ERR,"nieudane zapisanie do pliku");
            return -1;
        }
    }while(przeczytanych);
    close(zrodlowy);
    close(docelowy);
    return 0;
}
 
//FUNKCJA KOPIUJĄCA PLIKI (MAPOWANIE)
 // ten 3 argument to struktura stat o pliku
int copy_mmap(char *source, char *target, struct stat *st)
{
    int zrodlowy, docelowy;
    char* buf;
    zrodlowy = open(source, O_RDONLY);
    docelowy = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    
    syslog(LOG_INFO,"plik z %s został skopiowany do %s stosując metode mapowania",source,target);
    // http://man7.org/linux/man-pages/man2/mmap.2.html

    // On success, mmap() returns a pointer to the mapped area.  On error,
    // the value MAP_FAILED (that is, (void *) -1) is returned 

    // Pages may be read,  
    // Share this mapping.  

    // Updates to the mapping are visible to other processes mapping the same region
   
    // If addr is NULL, then the kernel chooses the address at which to create the mapping

    buf = mmap(0,st->st_size,PROT_READ, MAP_SHARED, zrodlowy, 0);

    if((write(docelowy, buf, st->st_size)) < 0)
    {
        syslog(LOG_ERR,"nieudane zapisanie do pliku");
        return -1;
    }
    close(zrodlowy);
    close(docelowy);
    return 0;
}
 
//SYNCHRONIZACJA KATALOGÓW
 
int synchronize(char *src, char *dst, int recursion, long int size)
{

    // dirent to taka struktura reprezentujaca foldery
    // stat to struktura przechowujaca informacje o plikach itd...
    // https://linux.die.net/man/2/stat
    // http://pubs.opengroup.org/onlinepubs/7908799/xsh/dirent.h.html
    // http://man7.org/linux/man-pages/man3/readdir.3.html

    DIR *catSrc;
    DIR *catDst;
    struct dirent *dit;
    char srcpath[30];
    char dstpath[30];
    struct stat srcfileinfo;
    struct stat dstfileinfo;

    // jeśli nie udało się otworzyć tych folderów to błąd i wychodzi
    if (((catSrc = opendir(src)) == NULL) || ((catDst = opendir(dst)) == NULL))
    {
        syslog(LOG_ERR,"Blad otwarcia katalogu\n");
        return -1;
    }
    // jesli sie udalo utworzyc struktury to zaczynamy synchronizacje

    syslog(LOG_INFO,"synchronizacja dwóch katalogów %s %s",src,dst);
    // dopóki src nie jest nullem
    while ((dit = readdir(catSrc)) != NULL)
    {
        // pomijamy . i .., przechodzimy dalej
        if( (strcmp(dit->d_name,".")==0) || (strcmp(dit->d_name,"..")==0) ) continue;
        //modyfikacje ostatnia ustawiamy na 0
        dstfileinfo.st_mtime = 0;
        // wrzucam sciezki do srcpath i dstpath
        strcpy(srcpath,src);
        strcpy(dstpath,dst);
        // srcpath i dstpath jest wycinana od "/" i d_name,
        // czyli nazwy kolejnego folderu/file itd.
        strcat(srcpath,"/");
        strcat(srcpath,dit->d_name);
        strcat(dstpath,"/");
        strcat(dstpath,dit->d_name);

        //stat() stats the file pointed to by path and fills in buf.

        stat(srcpath,&srcfileinfo);
        stat(dstpath,&dstfileinfo);

        switch (mode(srcfileinfo)) //sprawdzamy czym jest ścieżka
        {
            case 0: //jeśli ścieżka jest zwykłym plikiem
                if(srcfileinfo.st_mtime > dstfileinfo.st_mtime) //jeśli data modyfikacji pliku w katalogu źródłowym jest późniejsza
                {
                    if (srcfileinfo.st_size > size) //jeśli rozmiar pliku przekracza zadany rozmiar
                    copy_mmap(srcpath,dstpath,&srcfileinfo); //kopiowanie przez mapowanie
                    else copy(srcpath,dstpath); //zwykłe kopiowanie
                    i++; //licznik operacji
                }
                break;
            case 1: //jesli ścieżka jest folderem
                if (recursion == 1) //jeśli użytkownik wybral rekurencyjną synchronizacje
                {
                    if (stat(dstpath,&dstfileinfo) == -1) //jeśli w katalogu docelowym brak folderu z katalogu źródłowego
                    {
                        mkdir(dstpath,srcfileinfo.st_mode); //utworz w katalogu docelowym folder
                        synchronize(srcpath,dstpath,recursion,size); //rekurencyjne wywolanie synchronize
                    }
                    else synchronize(srcpath,dstpath,recursion,size); //jesli jest to tylko zsynchronizuj
                }
                break;
            default: break;
        }
    }
    // log z licznikiem i zamykanie zmiennych etc.
    syslog(LOG_INFO,"readdir() found a total of %i files", i);
    closedir(catDst);
    closedir(catSrc);
    free(dit);
    return 0;
}
 
//FUNCKCJA USUWA PLIKI Z KATALOGU DOCELOWEGO
 
void deleteExtras(char *src, char *dst, int recursion)
{

    // dirent to taka struktura reprezentujaca foldery
    // stat to struktura przechowujaca informacje o plikach itd...
    // https://linux.die.net/man/2/stat
    // http://pubs.opengroup.org/onlinepubs/7908799/xsh/dirent.h.html
    // http://man7.org/linux/man-pages/man3/readdir.3.html

    DIR *catSrc;
    DIR *catDst;
    struct dirent *dit;
    char srcpath[30];
    char dstpath[30];
    struct stat srcfileinfo;
    struct stat dstfileinfo;
    dit=malloc(sizeof(struct dirent));
    // jesli wskaznik jest nullem to dajemy loga
    if (((catDst = opendir(dst)) == NULL))
    {
        syslog(LOG_ERR,"Blad otwarcia katalogu\n");
    }
    //jesli nie to dopoki nie jest nullem
    while((dit = readdir(catDst))!=NULL)
    {
         // pomijamy . i .., przechodzimy dalej
        if( (strcmp(dit->d_name,".")==0) || (strcmp(dit->d_name,"..")==0) ) continue;
        
        // wrzucam sciezke do srcpath
        strcpy(srcpath,src);
        // przycinam sciezki o / i aktualna nazwe folderu/pliku
        strcat(srcpath,"/");
        strcat(srcpath,dit->d_name);
        // to samo dla dst
        strcpy(dstpath,dst);
        strcat(dstpath,"/");
        strcat(dstpath,dit->d_name);

        //lstat() is identical to stat(), except that if path is a symbolic link,
        //then the link itself is stat-ed, not the file that it refers to.
        // no czyli to samo co w synchronize, struktura z informacjami o pliku
        lstat(dstpath,&dstfileinfo);

        if(mode(dstfileinfo) == 0) //jesli to regularny plik
        {
            if(lstat(srcpath,&srcfileinfo)==0) // jesli istnieje w zrodlowym
            {
                if(mode(srcfileinfo) != 0)// jesli nie jest regularnym plikiem
                {
                    //usuwamy
                    unlink(dstpath);
                    syslog(LOG_INFO,"plik %s został usuniety",dstpath);
                }
            }
            else //nie ma takiego pliku w zrodlowym
            {
                //usuwamy
                unlink(dstpath);
                syslog(LOG_INFO,"plik %s został usuniety",dstpath);
            }
        }
        if(mode(dstfileinfo) == 1 && recursion)//jesli to katalog i oprcja -R
        {
            if(lstat(srcpath,&srcfileinfo)==0) // jesli istnieje w zrodlowym
            {
                if(mode(srcfileinfo) == 1) //katalog
                deleteExtras(srcpath, dstpath,recursion); //wywolanie deleteExtras dla tego folderu
            }
            else//nie ma takiego katalogu w zrodlowym
            {
                deleteExtras(srcpath, dstpath,recursion); //wywolanie deleteExtras dla tego folderu
                rmdir(dstpath); // usuwamy katalog
            }
        }
    }
    //zamykamy strumienie itd.
    closedir(catDst);
    free(dit);
}