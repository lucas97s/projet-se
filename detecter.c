/**
 * \file detecter.c
 * \Author Lucas SCHOTT et Hussein TAHER
 * \brief
 * usage : detecter [-t <format de date>] [-i <intervalle en millisecondes>]
 *                   [-l <nb d'executions>] [-c (code de retour)]
 *                   prog arg1 ... argn
 */


#define _BSD_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>

#define TAILLE_BUFFER 1024
#define TAILLE_DATE 64


//MACRO////////////////////////////////////////////////////////////////////////


/**
 * \def PRIM(r, msg)
 * \brief macro executant la primitive systeme et testant son code de retour
 * \a r primitive systeme
 * \a msg chaine de caractere indiqant la fonction qui fait l'erreur
 */
#define PRIM(r, msg) do{if((r)==-1){perror(msg);exit(EXIT_FAILURE);}}while(0)


//TYPEDEF//////////////////////////////////////////////////////////////////////


/**
 * \typedef Options
 * \brief options au lancement du programme
 * \a format_date format lisible par strftime()
 * \a intervalle en microsecondes
 * \a limite en nombre d'executions
 * \a nbArgs nombre d'arguments utilisés
 */
typedef struct {
    char * format_date;
    long int intervalle; //en micro-secondes
    int limite;
    bool retour;
    int nbArgs;
} Options;


/**
 * \typedef Resultat_exec
 * \brief sortie std du programme sous forme de chaine
 * et taille de la chaine
 * \a buf sortie std
 * \a taille de la chaine
 */
typedef struct {
    char * buf;
    int taille;
} Resultat_exec;


//VARIABLES GLOBALES///////////////////////////////////////////////////////////


/**\var Options options;
 * \brief options à l'execution*/
Options options;

/**\var char ** prog_argv=NULL;
 * \brief tableau argv de la commande a executer avec execvp*/
char ** prog_argv=NULL;

/**\var char * fichier=NULL;
 * \brief chaine representant le fichier de la commande a executer avec execvp*/
char * fichier=NULL;

/**\var int tube[2];
 * \brief tube resuperant la sortie std de execvp*/
int tube[2];

/**\var char * buf_exec_prec=NULL;
 * \brief buffer contenant les resultats de l'execution precedente*/
char * buf_exec_prec=NULL;

/**\var int taille_buf_prec=0;
 * \brief taille du buffer de l'execution precedente*/
int taille_buf_prec=0;

/**\var int raison_precedente=0;
 * \brief code de retour de l'exection precedente*/
int raison_precedente=1;


//FONCTIONS////////////////////////////////////////////////////////////////////


/**
 * \fn void erreur_prim(char * primitive)
 * \brief fonction a declencher en cas d'erreur de primitive systeme
 * un message d'erreur est envoyé sur stderr
 * \a primitive le nom de la primitive qui a échoué
 * \post la fonction quitte la programme
 */
void erreur_prim(char * primitive)
{
    perror(primitive);
    exit(EXIT_FAILURE);
    return;
}


/**
 * \fn getOptions(int argc, char * argv[])
 * \brief fonction qui recupere les options du programme avec getopt
 * et qui les stock dans une structure de type Options
 * \a argc la valeur argc du main
 * \a argv la tableau des arguments du main
 * \return une structure de type Options avec les données sotcké
 */
void getOptions(int argc, char * argv[])
{
    options.format_date = NULL;
    options.intervalle = 10000000; //10000000 usec = 10000 msec
    options.limite = 0;
    options.retour = false;
    options.nbArgs = 0;
    int c_option;
    extern char *optarg;
    extern int optind;
    while( (c_option = getopt(argc, argv, "+t:i:l:c")) != EOF )
    {
        switch(c_option)
        {
            case 't':
                //format
                options.format_date = optarg;
                options.nbArgs+=2;
                break;
            case 'i':
                //intervalle
                options.intervalle = atoi(optarg)*1000;
                if(options.intervalle<=0)
                {
                    fprintf(stderr,"la limite ne peut ni negative ni nulle\n");
                    exit(EXIT_FAILURE);
                }
                options.nbArgs+=2;
                break;
            case 'l':
                //limite
                options.limite = atoi(optarg);
                if(options.limite<0)
                {
                    fprintf(stderr,"la limite ne peut pas etre negative\n");
                    exit(EXIT_FAILURE);
                }
                options.nbArgs+=2;
                break;
            case 'c':
                //changement de code de retour
                options.retour = true;
                options.nbArgs++;
                break;
            case '?':
                break;
            default:
                break;
        }
    }
    return;
}


/**
 * \fn char ** recup_argv(int debut, int argc, char * argv[])
 * \brief fonction qui recupere le tableau d'arguments de prog,
 * contenu dans argv, au format utilisable par execvp :
 * [<prog>;<arg1>;...;<argn>;NULL]
 * \a debut indice du debut du nouveau tableau
 * \a argc le nombre d'arguments de l'execution du programme
 * \a argv le tableau d'arguments de l'execution du programme
 * \return nouveau tableau des arguments de prog
 */
void recup_prog_argv(int debut, int argc, char * argv[])
{
    int i;
    prog_argv = malloc(sizeof(char*)*(argc-debut+2));
    if(prog_argv==NULL)
    {
        fprintf(stderr, "erreur allocation memoire prog_argv\n");
        exit(EXIT_FAILURE);
    }
    for(i=0; i<=argc-debut+1; i++)
    {
        prog_argv[i]=argv[i+debut];
    }
    return;
}

/**
 * \n void print_exit(int status)
 * \brief affiche sur la sortie std le code de retour du prog execute
 * \return voi
 */
void print_exit(bool changement_code_de_retour,int status)
{
    if(options.retour && changement_code_de_retour)
    {
        char buffer[TAILLE_BUFFER];
        sprintf(buffer, "exit %d\n", status);
        PRIM(write(1, buffer, strlen(buffer)),"write");
    }
}


/**
 * \fn void print_time(void)
 * \brief fonction affichant la date courante au format de strftime()
 * \return void
 */
void print_time(void)
{
    if (options.format_date!=NULL)
    {
        time_t raw_time;
        char buffer_date[TAILLE_DATE];
        struct tm * tm_info;
        PRIM(time(&raw_time),"time");
        tm_info = localtime(&raw_time);
        if (strftime(buffer_date, TAILLE_DATE, options.format_date, tm_info))
            puts(buffer_date);
    }
    return;
}


/**
 * \fn Resultat_exec lecture_de_l'affichage(void)
 * \brief lecture de l'affichage de la derniere execution
 * \return resultat de l'affichage et taille du buffer dans
 * la structure Resultat_exec
 */
Resultat_exec lecture_de_l_affichage(void)
{
    char buf_sortie_tube[TAILLE_BUFFER];
    int count,i;
    Resultat_exec resultat_exec;
    resultat_exec.buf=NULL;
    resultat_exec.taille=0;
    while((count=read(tube[0],buf_sortie_tube,TAILLE_BUFFER))>0)
    {
        if((resultat_exec.buf=realloc(resultat_exec.buf,
                        resultat_exec.taille+count))==NULL)
        {
            fprintf(stderr,"erreur de reallocation memoire\n");
            exit(EXIT_FAILURE);
        }
        for(i=0;i<count;i++)
        {
            resultat_exec.buf[resultat_exec.taille+i]=buf_sortie_tube[i];
        }
        resultat_exec.taille+=count;
    }
    if(count==-1)
        erreur_prim("read");
    return resultat_exec;
}


/**
 * \fn void afficher_resultat(bool changement_code_de_retour,
 * Resultat_exec resultat_exec, int raison_precedente);
 * \brief fonction affichant le resultat du programme
 * \a changement_code_de_retour booleen 
 * \a resultat_exec resulatat du programme executé
 * \a raison_precedente code de retour du programme
 * \return void
 * \post liberation de buf_exec_prec
 */
void afficher_resultat(bool different,Resultat_exec resultat_exec)
{
    if(different)
    {
        PRIM(write(1,resultat_exec.buf,resultat_exec.taille),"write");
        free(buf_exec_prec);
        buf_exec_prec=resultat_exec.buf;
        taille_buf_prec=resultat_exec.taille;
    }
    else
    {
        free(buf_exec_prec);
        buf_exec_prec=resultat_exec.buf;
        taille_buf_prec=resultat_exec.taille;
    }
    return;
}


/**
 * \fn void comparer(int raison)
 * \brief fonction qui compare la sortie du tube1 avec celle du tube2
 * et qui compare aussi leur code de retour
 * la focntion affiche le contenu du tube2 s'il est différent de
 * du tube1
 * \pre utilisation des variables globales : tube1 tube2, options
 * et raison_precedente
 */
void comparer_resultat(void)
{
    int raison,i;
    bool different=false;
    bool changement_code_de_retour=false;
    Resultat_exec resultat_exec;

    //lecture de l'affichage
    resultat_exec = lecture_de_l_affichage();
    PRIM(close(tube[0]),"close");
    PRIM(wait(&raison),"wait");

    //comparer les codes de retour
    if(options.retour)
    {
        if(WIFEXITED(raison))
        {
            if(WEXITSTATUS(raison)!=raison_precedente)
            {
                raison_precedente=WEXITSTATUS(raison);
                different=true;
                changement_code_de_retour=true;
            }
        }
        else if(WIFSIGNALED(raison ))
        {
            fprintf (stderr,"le programme a quitté avec le signal %d\n"
                    , WTERMSIG (raison )) ;
            exit(EXIT_FAILURE);
        }
        else
        {
            fprintf (stderr,"le programme a quitté pour raison inconnue\n") ;
            exit(EXIT_FAILURE);
        }
    }

    //comparer l'affichage
    if(!different)
    {
        if(taille_buf_prec!=resultat_exec.taille)
            different=true;
        else
        {
            for(i=0;i<resultat_exec.taille && !different;i++)
            {
                if(buf_exec_prec[i]!=resultat_exec.buf[i])
                    different=true;
            }
        }
    }

    print_time();
    afficher_resultat(different,resultat_exec);
    print_exit(changement_code_de_retour,WEXITSTATUS(raison));
    return;
}


/**
 * \fn void switch_fork(void)
 * \brief fonction qui execute le fork et qui gere l'action du fils
 * et celle du pere
 * \return void
 * \pre utilisation des variables globales : tube1, tube2, options,
 * raison_precedente, prog_argv et fichier
 */
void switch_fork(void)
{
    PRIM(pipe(tube),"pipe(tube1)");
    switch(fork())
    {
        case -1:
            erreur_prim("fork");
            break;
        case 0:
            PRIM(dup2(tube[1],1),"dup2(tube[1],1)");
            PRIM(close(tube[1]),"close(tube[1])");
            PRIM(close(tube[0]),"close(tube[0])");
            execvp(fichier,prog_argv);
            erreur_prim("execvp");
            break;
        default:
            PRIM(close(tube[1]),"close(tube[1])");
            comparer_resultat();
            usleep(options.intervalle);
            break;
    }
    return;
}


/**
 * \fn int main(int argc, char * argv[])
 * \a argc, nb d'argument du programme
 * \a argv tableau des arguments a l'execution
 * \return 0 si le programme s'est fini avec succes
 * 1 sinon
 */
int main(int argc, char * argv[])
{
    if(argc<2)
    {
        fprintf(stderr, "usage : detecter [-t <format de date>]\n\
[-i <intervalle en millisecondes>] [-l <nb d'executions>]\n\
[-c (code de retour)] prog arg1 ... argn\n");
        exit(EXIT_FAILURE);
    }

    getOptions(argc, argv);
    recup_prog_argv(options.nbArgs+1, argc,  argv);
    fichier = prog_argv[0];
    int i;

    if(options.limite>0)
    {
        for(i=0; i<options.limite; i++)
        {
            switch_fork();
        }
    }
    else
    {
        while(true)
        {
            switch_fork();
        }
    }
    free(buf_exec_prec);
    free(prog_argv);
    return 0;
}
