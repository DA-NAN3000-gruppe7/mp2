#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include<sys/wait.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>


#define LOKAL_PORT 80
#define BAK_LOGG 10 // Størrelse på for kø ventende forespørsler 
#define TAB "\t"
#define SPACE " "
#define MIME_NOT_FOUND "MIME_NOT_FOUND"
#define NO_EXT "NO_EXT"
#define EMPTY_FILE "EMPTY_FILE"
#define CANNOT_OPEN_MIME "CANNOT_OPEN_MIME"


void daemonizer();
void chroot_function();
void webserver();
char * parseUrl(char sIn[]);
char* get_mime(char *filename);

char *get_token(char *psrc, const char *delimit, void *psave);


void get_time();
void func(int signum); 

char mime_buf[200];

int main(int argc, char* argv[]) {

    //running as root
    setuid(0);
    setgid(0);

    close(1);
    close(2);

    int file = open("/var/log/debug.log", O_CREAT | O_RDWR | O_APPEND, 0644);

    dup2(file, 1);
    dup2(file, 2);
    if (1!=getppid())
        daemonizer();

    //this must be open do be able to read from mime-types after changing web-root
    FILE *fp = fopen("/etc/mime.types", "r");
    char buf[100];
    

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        strcat(mime_buf, buf);
    }

    fclose(fp);

    chroot_function();
    webserver();

    
    close(file);
    return 0;
}

void webserver() {

    struct sockaddr_in  lok_adr;
    int sd, ny_sd;
    // Setter opp socket-strukturen

    sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // For at operativsystemet ikke skal holde porten reservert etter tjenerens død
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    // Initierer lokal adresse
    lok_adr.sin_family      = AF_INET;
    lok_adr.sin_port        = htons((u_short)LOKAL_PORT);
    lok_adr.sin_addr.s_addr = htonl(         INADDR_ANY);

    // Kobler sammen socket og lokal adresse
    if ( 0==bind(sd, (struct sockaddr *)&lok_adr, sizeof(lok_adr)) ) {
        get_time();
        fprintf(stderr, "Prosess %d er knyttet til port %d.\n", getpid(), LOKAL_PORT);
    }
    else
        exit(1);

    // Venter på forespørsel om forbindelse

    listen(sd, BAK_LOGG);

    setuid(1000);
    setgid(985);
    int nroot_sd = dup(sd);
    close(sd);

    while(1) { 
        // Aksepterer mottatt forespørsel
        ny_sd = accept(nroot_sd, NULL, NULL);    

        if(0==fork()) {

                // Setter opp for å lese fil
                int read_size;
                char client_message[2000];
                char inPage[128] = "";

                // Leser inn fil, parser filnavn
                if((read_size = recv(ny_sd , client_message , 2000 , 0)) > 0) {

                    char *returned_str = parseUrl(client_message);
                    strcpy(inPage, returned_str);

                    if(strcmp(returned_str, "/") == 0)
                        returned_str = "index.html";

                    char *mime_type = get_mime(returned_str);


                    
                    if(strcmp(mime_type, EMPTY_FILE) == 0 ||
                        strcmp(mime_type, NO_EXT) == 0) {

                        dup2(ny_sd, 1);

                        printf("HTTP/1.1 400 Bad Request\n");
                        printf("\n");
                        printf("%s\n", mime_type);

                        get_time();
                        fprintf(stderr, "%s\n", mime_type);
                        fflush(stdout);
                    }
                    else if(strcmp(mime_type, MIME_NOT_FOUND) == 0) {

                        dup2(ny_sd, 1);

                        printf("HTTP/1.1 404 Not Found\n");
                        printf("\n");
                        printf("%s\n", mime_type);

                        get_time();
                        fprintf(stderr, "%s\n", mime_type);
                        fflush(stdout);
                    }
                    else if(strcmp(mime_type, CANNOT_OPEN_MIME) == 0) {

                        dup2(ny_sd, 1);

                        printf("HTTP/1.1 500 Internal Server Error\n");
                        printf("\n");
                        printf("%s\n", mime_type);

                        get_time();
                        fprintf(stderr, "%s\n", mime_type);
                        fflush(stdout);

                    }
                    /*else if(strcmp(returned_str, "teapot") == 0) {
                        dup2(ny_sd, 1);
                        printf("HTTP/1.1 418 I'm a teapot\n");
                        printf("Content-Type: teapot\n");
                        printf("\n");

                    }*/
                    else if(strcmp(mime_type, "ASIS") == 0) {
                         // Leser filforespørsel fra klient                           
                        FILE* fp = fopen(returned_str, "r");
                        if(fp) {
                            
                            char buf[1024];

                            while (fgets(buf, sizeof(buf), fp) != NULL) {
                                
                                if (send(ny_sd, buf, strlen(buf), 0) < 0) {
                                    get_time();
                                    fprintf(stderr, "Kunne ikke sende melding..\n");                                
                                } else {
                                    // Sender suksess
                                    
                                }
                            }
                            
                            fclose(fp);
                        
                        } else {
                            printf("HTTP/1.1 404 Not Found\n");
                            get_time();
                            fprintf(stderr, "Kan ikke lese fil %s.\n", returned_str);
                            fflush(stdout);
                        }

                    
                     } else {

                        dup2(ny_sd, 1);

                        
                        // Leser filforespørsel fra klient                             
                        FILE* fp = fopen(returned_str, "r");
                        if(fp) {
                            struct stat st;
                            stat(returned_str, &st);
                            printf("HTTP/1.1 200 OK\n");
                            printf("Content-Type: %s\n", mime_type);
                            printf("Content-Length: %d\n", st.st_size);
                            printf("\n");

                            
                            char buf[1024];
                            while (fgets(buf, sizeof(buf), fp) != NULL)
                                printf("%s", buf);

                            fflush(stdout);
                            fclose(fp);

                        } else {
                            printf("HTTP/1.1 404 Not Found\n");
                            printf("Content-Type: %s\n", mime_type);
                            printf("\n");
                            get_time();
                            fprintf(stderr, "Kan ikke lese fil %s.\n", returned_str);
                            fflush(stdout);
                        }
                    }
                }

            // Sørger for å stenge socket for skriving og lesing
            // NB! Frigjør ingen plass i fildeskriptortabellen
            shutdown(ny_sd, SHUT_RDWR);
            exit(0);
        }

        else {
            signal(SIGCHLD, func); 

            close(ny_sd);
        }
      }
}

void daemonizer() {
    //create a process ID variable
    pid_t pid;

    //creates a fork of the process, a parent and a child
    //the code should run identical in practice for both child and parent
    //but they each have a different value assigned to the pid variable
    //a child has pid = 0 and the parent pid > 0. a pid < 0 means the forking failed
    //this is so that the process can run in the background and so that it is not a process group leader
    pid = fork();


    if (pid < 0)
        exit(EXIT_FAILURE);
    //exits the parent process. Now only the child is running
    if (pid > 0)
        exit(EXIT_SUCCESS);

    //creates a session from the child process and assigns it the lead session ID
    //if this process fails(returns > 0) the child process terminates 
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    //this is to ignore the SIGHUP signal which is sent whenever a session leader process is terminated
    signal(SIGHUP, SIG_IGN);

    //forks the process again to ensure that it's no longer a session leader and cannot be accessed through an available terminal
    pid = fork();

    if (pid < 0)
        exit(EXIT_FAILURE);

    if (pid > 0)
        exit(EXIT_SUCCESS);
}

void chroot_function() {
    //calling the chroot process and sets "." as the root directory
    //if error code -1 is returned, the code exits

    

    chdir("/var/www/.");
    if(chroot("/var/www/") != 0) {
        get_time();
        perror("chroot /var/www/");
        exit(EXIT_FAILURE);

    }

}

// Postcondition: clean file-url sent back from input-string (file)
char * parseUrl(char sIn[]) {
    
    /* data to store client query, warning, should not be big enough to handle all cases */
    char query[1024] = "";
    static char page[128] = ""; // !! Bør endres til noe annet enn static!
    char host[128] = "";
    
    /* read query */
    if (sIn > 0)
    {
        char *tok;
        char sep[] = "\r\n";
        char tmp[128];
        /* cut query in lines */
        tok = strtok(sIn, sep);

        /* process each line */
        while (tok)
        {
            /* See if line contains 'GET' */
            if (1 == sscanf(tok, "GET %s HTTP/1.1", tmp))
            {
                
                strcpy(page, tmp);
            }
            /* See if line contains 'Host:' */
            else if (1 == sscanf(tok, "Host: %s", tmp))
            {
                strcpy(host, tmp);
            }
            /* get next line */
            tok = strtok(query, sep);
        }
        /* print got data */
        get_time();
        fprintf(stderr, "Wanted page is: %s%s\n", host, page);

        
    } 
    else 
    {
        /* handle the error (-1) or no data to read (0) */
    }

    return page;
}

char* get_mime(char *filename) {
    char *ext;

    //if the string is empty
    if(filename[0] == '\0')
        return EMPTY_FILE;

    //gets the extention of the string
    ext = strrchr(filename, '.');
    if (!ext)
        return NO_EXT;

    else
        ext = ext + 1;

    if(strcmp("asis", ext) == 0)
        return "ASIS";


    //parses the mime.types file
    char *buf = strtok(strdup(mime_buf), "\n");
    while(buf != NULL) {

        char saveptr[32];
        char tmp[50];
        strcpy(tmp, buf);

        //initializes buffers to store mime-type and extension
        char *token = get_token(tmp, TAB, &saveptr);
        char *type = malloc (sizeof (char) * 30);

        strcpy(type, token);
        token = get_token(NULL, TAB, &saveptr);


        //it's not assumed that there are mimetypes associated with more than 5 extensions
        //this used to be able to iterate and terminate at NULL with strtok(),
        //but as that implementation from MP1 became VERY difficult(and frustrating) in MP2,
        //it has been discarded and replaced with a less convenient, but atleast functional, method
        char mime_ext[10];
        do{
            token[strcspn(token, "\n")] = 0;
            strcpy(mime_ext, token);

            //condition is met if the mime extension matches the extension of the file
            //respective mime-type is returned

            if(strcmp(mime_ext, ext) == 0) {
                return type;
            }
            token = get_token(NULL, TAB, &saveptr);

            //if inspecting the log-file, you will notice that the mime_ext tokens are rather messy when parsing for the next ones.
            //I really don't know any workaround on this. It's probably adding a 100th of a miliseconds of delay, so it's not really crucial in this application
            //fprintf(stderr, "%s\n", token);



        }while(strcmp(mime_ext, token) != 0);
        fflush(stderr);
        buf = strtok(NULL, "\n");


    }


    return MIME_NOT_FOUND;

}

char *get_token(char *psrc, const char *delimit, void *psave) 
{
    static char sret[512];
    register char *ptr = psave;
    memset(sret, 0, sizeof(sret));

    if (psrc != NULL) strcpy(ptr, psrc);
    if (ptr == NULL) return NULL;

    int i = 0, nlength = strlen(ptr);
    for (i = 0; i < nlength; i++)
    {
        if (ptr[i] == delimit[0]) break;
        if (ptr[i] == delimit[1]) 
        {
            ptr = NULL;
            break;
        }
        sret[i] = ptr[i];
    }
    if (ptr != NULL) strcpy(ptr, &ptr[i+1]);

    return sret;
}

//gets the current time, used for logging
void get_time() {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    fprintf(stderr, "[%d-%02d-%02d %02d:%02d:%02d]: ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void func(int signum) { 
    wait(NULL); 
} 

