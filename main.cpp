#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <sys/stat.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 255
#endif


void printUsage(char *programm_name)
{
    printf("Type this: %s [-R] [-i] searchpath filename1 [filename2]...[filenameN]\n\n\n", programm_name);
    return;
}

//searchfunktion die mithilfe der mitgegegebenen Flags alle Optionen durchgeht

//4 verschiedene Suchoptionen in einer search
//Rflag und iflag werden mitübergeben damit man weiß wie gesucht werden soll
//um zugriff auf das verzeichnis zu erhalten muss der DIR Datentyp verwendet werden
//dirent.h verwenden -> opendir zum verzeichnis öffnen
//strcasecmp = vergleicht die Zeichenketten ohne zwischen Groß und Kleinschreibung zu unterscheiden
int mySearch(const char *filename, const char *firstPath, int Rflag, int iflag)
{
    DIR *dir = opendir(firstPath); //öffnen des gewünschten ordners
    struct dirent *entry;

    //max Path = 255 (char array)
    char path[PATH_MAX];

    //wenn sich der Ordner nicht öffnen lässt -> suche vorbei ansonsten loslegen
    if (dir)
    {
        while ((entry = readdir(dir)) != NULL) //auslesen
        {

            //Einträge "." und "." werden ignoriert/übersprungen
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            // Pfad erschaffen in dem gesucht wird
            strcpy(path, firstPath);
            strcat(path, "/");
            strcat(path, entry->d_name);


            //nicht mehr weitersuchen wenn bereits gefunden 
            if(mySearch(filename, path, Rflag, iflag) != 0)
            {
                return 1;
            }

            //vergleich: gefundener d_name = gesuschter filename?
            //wurde der  iflag gesezt -> suche mit strcasecmp = strcmp aber ohne beachtung von großkleinschreibung
            if (iflag != 0)
            {
                if (strcasecmp(entry->d_name, filename) == 0)
                {

                    pid_t pid = getpid(); //prozess ID des aktuellen Kindpprozess zurückgegen
                    fprintf(stdout, "Pid: '%ld' Yeah Datei '%s' gefunden in' %s'  \n", (long)pid, filename, path);
                    return 1;
                }
            }
            else //kein iflag gesetzt -> normale suche mit strcmp
            {
                //vergleich: gefundener d_name = geuschter filename?
                if (strcmp(entry->d_name, filename) == 0)
                {
                    pid_t pid = getpid(); //prozess ID des aktuellen Kindpprozess zurückgegen
                    fprintf(stdout, "Pid: '%ld' Yeah Datei '%s' gefunden in' %s'  \n", (long)pid, filename, path);
                    return 1; //file gefunden -> 1 = true zurückgeben
                }
            }


            if (Rflag != 0) //rlfag gesetzt -> rekursiv weitermachen
            {
                mySearch(filename, path, Rflag, iflag); //erneut rekursiv aufrufen
            }
            //sonst leider pech keine rekursive suche
        }
    }
    else
    {
        return 0; //vorbei Ordner konnte nicht geöffnet werden
    }
    closedir(dir); //schließen der files

   
}

int main(int argc, char *argv[])
{

    int option;          //prüfen der eingegebenen werte
    char *programm_name; //programmname - immer bei usage ausgegeben
    //welche option wurde gewählt -> flag setzen
    int Rflag = 0;
    int iflag = 0;

    programm_name = argv[0]; //das erste argument im terminal ist der programm name

    //alles außer -R und -i ist ein invalid Input

    while ((option = getopt(argc, argv, "Ri")) != -1)
    {
        switch (option)
        {

        case 'R':    //rekursive Suche
            Rflag++; //flag eröhen
            break;
        case 'i': //suche unabhängig von Groß und Kleinschreibung
            iflag++;
            break;
        case '?': //ungültiges Argument
            printUsage(programm_name);
            exit(1);
            break;
        default: //unmöglich
            assert(0);
        }
    }

    //mithilfe der gesetzen Flags prüfen ob die Eingabe richtig war:

    //1. prüfen ob genug Argumente eingegeben wurden
    //minimum = programmcode + searchpath + filename
    if ((argc - Rflag - iflag) < 3)
    {
        printf("Nicht genügend Argumente\n");
        printUsage(programm_name);
    }

    // 2. prüfen ob jede Auswahlmöglichkeit auch nur max 1x gewählt wurde:

    if ((Rflag > 1) || (iflag > 1))
    {
        printf("Jedes Argument kann nur 1x aufgerufen werden\n");
        printUsage(programm_name);
        exit(1);
    }

    //wenn also soweit alles richtig eingegeben wurde- kann endlich begonnen werden...
    //optind = der Inhalt des ersten Elements dass keine option ist (kein -R oder -i)
    //4 Optionen müssen geprüft werden: -R und -i eingebeben, nur -R, nur -i, keins von beiden

    //for schleife läuft so oft durch wie es filenames gibt -> bedeutet arc- gesetzte flags -1(für den programmnamen)
    //damit bleiben nur noch die filenames über nach denen gesucht werden sollen

    for (int i = 0; i < (argc - Rflag - iflag - 1); ++i)
    {
        //erster optind = 2 = searchpath
        char *firstPath = argv[optind];
        //zweites argument = filename1, filename2 ... (aktuelles File nach dem gesucht werden soll)
        char *currentFile = argv[optind + 1 + i];

        char current_path[PATH_MAX];
        realpath(argv[optind], current_path);

        //Kindrpozess -> fork
        pid_t myPid = fork();

        //wenn alles geklappt hat entsteht ein Kindprozess mit pid = 0
        //der Kindprozess sucht nach der File
        if (myPid == 0)
        {
            //aufruf der searchfunktion 
            if (mySearch(currentFile, current_path, Rflag, iflag) != 1) //prüfen ob file gefunden wurde
            {
                pid_t childPID = getpid();
                printf("Konnte die Datei %s leider nicht finden", currentFile);
            }
            exit(0); 
        }
        else if (myPid == -1) //Kindprozess konnte nicht generiert werden
        {
            printf("Oh nein Kindprozess erstellen fehlgeschlagen");
            exit(1);
        }
    }

    //Elternprozess muss warten bis ale Kindprozesse abgeschlossen sind
    for (int i = 0; i < argc - Rflag - iflag - 1; ++i)
    {
        wait(NULL);
    }
    //endlich ..alle Kindprozesse sind fertig - genug gewartet
    printf("\n..Fertig\n");
}
