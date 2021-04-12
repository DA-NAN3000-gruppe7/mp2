// Viktig: kjør programmet som root med sudo.
// Kommando "ps aux | grep a.out" for å finne tjenerprosess.
// Kommando kill pid fra forrige søk for å avslutte prosessen.

#include <unistd.h> 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

// Definer konstanter.
#define LOKAL_PORT 80
#define BAK_LOGG 10 // Størrelse på for kø ventende forespørsler.

// Deklarer funksjoner.
static void tjener();
static void deamon();
static void nyrot();

// Main-funksjon.
int main()
{
  // Opprett variabler.
  int log;
  
  // Steng STDERR.
  close(2);

  // Fjern gammel loggfil.
  remove("../var/log/debug.log");

  // Opprett loggfil.
  log = open("../var/log/debug.log", O_CREAT | O_RDWR | O_APPEND, 0644);

  // Send STDERR til loggfil.
  dup2(log, 2);
  
  // Demoniserer.
  if (1!=getpid())
    {
      deamon();
    }

  // Endrer rotkatalog.
  nyrot();

  // Kjører tjener.
  tjener();

  // Lukk loggfilen.
  fprintf(stderr, "Lukker loggfil.\n");
  close(log);

  // Returverdi.
  return 0;
}

// Tjenerfunksjon.
static void tjener()
{
  // Deklarer variabler.
  struct sockaddr_in  lok_adr;
  int sd, ny_sd, sd2;
  char request[2048], file[128], buf[1024], mbuf[1024], mime[128], tmp[128], filsti[128];
  char *token, *type, *mtoken, *tok;
  FILE *fp, *mfp;

  // Setter opp socket-strukturen.
  sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  // For at OS ikke skal holde porten reservert etter tjenerens død.
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

  // Initierer lokal adresse.
  lok_adr.sin_family      = AF_INET; // Type socket.
  lok_adr.sin_port        = htons((u_short)LOKAL_PORT); // Oversette portnummer.
  lok_adr.sin_addr.s_addr = htonl(         INADDR_ANY); // Lytte på alle IPer.

  // Kobler sammen socket og lokal adresse.
  if (0 == bind(sd, (struct sockaddr *)&lok_adr, sizeof(lok_adr)))
    {
      // Skriver melding hvis ok.
      fprintf(stderr, "Prosess %d er knyttet til port %d.\n", getpid(), LOKAL_PORT);
    }
  else
    {
      // Avslutter prosess med feil.
      fprintf(stderr, "Bind feilet: %s\n", strerror(errno));
      exit(1);
    }

  // Bytt fra rotbruker.
  setuid(1000);
  setgid(1000);

  // Kopier socket og steng den gamle.
  sd2 = dup(sd);
  close(sd);

  // Venter på forespørsel om forbindelse
  listen(sd2, BAK_LOGG);

  // Evig serverløkke.
  while (1)
    {      
      // Aksepterer mottatt forespørsel
      ny_sd = accept(sd2, NULL, NULL);

      // Egen prosess for forespørsel.
      if (0 == fork())
	{
	  // Les forespørsel fra klient.
	  recv(ny_sd, request, sizeof(request), 0);

	  // Del opp forespørsel.
	  if (request > 0)
	    {
	      // Opprett separator.
	      token = strtok(request, "\r\n");

	      // Del opp tekst linje for linje.
	      while (token != NULL)
		{
		  // Hent filnavn, returnerer antall variabler.
		  if (sscanf(token, "GET %s HTTP/1.1", tmp) == 1)
		    {
		      // Lagre filnavn.
		      strcpy(file, tmp);
		    }

		  // Neste linje.
		  token = strtok(NULL, "\r\n");
		}
	    }
	  else
	    {
	      fprintf(stderr, "Tom forespørsel: %s\n", strerror(errno));
	    }

	  // Hent fil-type.
	  type = strrchr(file, '.');
	  fprintf(stderr, "Fil: %s Ending: %s\n", file, type);

	  // Sjekk om filen har ending.
	  if (type)
	    {
	      // Fjern punktum.
	      type++;

	      // Åpne mime.types.
	      mfp = fopen("/../etc/mime.types", "r");
	      if (!mfp)
		{
		  fprintf(stderr, "Kunne ikke åpne mimefil: %s\n", strerror(errno));
		}

	      // Bla gjennom mime.types linje for linje.
	      while (fgets(mbuf, sizeof(mbuf), mfp) != NULL)
		{		  
		  // Søk etter ending/filtype.
		  if (strstr(mbuf, type))
		    {
		      // Hent første token: mimetype.
		      mtoken = strtok(mbuf, "\t");

		      // Lagre mimetype i temp-fil.
		      strcpy(tmp, mtoken);

		      // Hent neste token: filtype.
		      mtoken = strtok(NULL, "\t");

		      // Hent første filtypetoken.
		      mtoken = strtok(mtoken, " ");

		      // Bla gjennom filtypetokens.
		      while (mtoken != NULL)
			{
			  // Sammenlign filtype med forespurt type.
			  if (strcmp(mtoken, type) == 0)
			    {
			      // Lagre mimetype.
			      strcpy(mime, tmp);
			      
			      // Print match.
			      fprintf(stderr, "Match! Mimetype: %s Filtype: %s\n", mime, mtoken);

			      // Bryt ut av løkkene ved match for å spare ressurser.
			      goto mainmime;
			    }
			  else
			    {
			      fprintf(stderr, "Ikke match: %s\n", mtoken);
			    }

			  // Neste token.
			  mtoken = strtok(NULL, " ");
			}
		    }
		}
	    }
	  else
	    {
	      // Feilmelding: filen har ikke ending.
	      fprintf(stderr, "%s har ingen ending/filtype.\n", file);

	      // Åpne feilmelding.
	      fp = fopen("../var/www/400.asis", "r");
	      if (!fp)
		{
		  fprintf(stderr, "Kunne ikke åpne feilmelding: %s\n", strerror(errno));
		}
	    }

	  // Hent mime-hovedtype.
	mainmime:
	  {
	    // Hent første del av mimetypen.
	    tok = strtok(mime, "/");
	    fprintf(stderr, "Hovedmime: %s\n", tok);
	  }

	  // Steng mime.types.
          fclose(mfp);

	  // Sjekk om filen støttes.
	  if ((strcmp("asis", type) == 0) ||
	      (strcmp("text", tok) == 0) ||
	      (strcmp("image", tok) == 0) ||
	      (strcmp("application", tok) == 0))
	    {
	      // Slå sammen til full filsti.
	      strcat(filsti, "../var/www");
	      strcat(filsti, file);
	      fprintf(stderr, "Filsti: %s\n", filsti);

	      // Sjekk om filen eksisterer.
	      if (access(filsti, F_OK) == 0)
		{		  
		  // Filen eksisterer, åpne filen.
		  fp = fopen(filsti, "r");
		  fprintf(stderr, "%s eksisterer.\n", file);
		}
	      else
		{
		  // Feilmelding: filen eksisterer ikke.
		  fp = fopen("../var/www/404.asis", "r");
		  fprintf(stderr, "%s eksisterer ikke: %s\n", file, strerror(errno));
		}
	    }
	  else
	    {
	      // Feilmelding: filtypen støttes ikke.
	      fp = fopen("../var/www/400.asis", "r");
	      fprintf(stderr, "%s har feil filtype: %s\n", file, strerror(errno));
	    }

	  // Les fil inn i buffer linje for linje.
	  while (fgets(buf, sizeof(buf), fp) != NULL)
	    {
	      // Sender filen linje for linje til klient.
	      if (send(ny_sd, buf, strlen(buf), 0) < 0)
		{
		  // Feilmelding: kunne ikke sende fil.
		  fprintf(stderr, "Kunne ikke sende fil: %s\n", strerror(errno));
		}
	    }

	  // Steng fil.
	  fclose(fp);
	  
	  // Tøm bufre.
	  memset(request, 0, sizeof(request));
	  memset(file, 0, sizeof(file));
	  memset(buf, 0, sizeof(buf));
	  memset(mbuf, 0, sizeof(mbuf));
	  memset(mime, 0, sizeof(mime));
	  memset(tmp, 0, sizeof(tmp));
	  
	  // Sørger for å stenge socket for skriving og lesing
	  // NB! Frigjør ingen plass i fildeskriptortabellen
	  shutdown(ny_sd, SHUT_RDWR);

	  // Avslutter prosessen.
	  exit(0);
	}
      else
	{
	  // Foreldren lukker socketen og kjører accept på nytt.
	  close(ny_sd);

	  // Venter til barneprosessen terminerer.
	  wait(NULL);
	}
    }
}

static void deamon()
{
  // Deklarer variabler.
  pid_t pid;

  // Lag barneprosess.
  pid = fork();

  // Pid < 0 betyr at fork feilet.
  if (pid < 0)
    {
      // Avslutt prosess med feil.
      fprintf(stderr, "Deamon fork 1 feilet: %s\n", strerror(errno));
      exit(1);
    }

  // Pid > 0 er forelder, barn = 0.
  if (pid > 0)
    {
      // Avslutt foreldreprosessen for å ha tjener i bakgrunnen.
      exit(0);
    }

  // Lager ny sesjon. Gjør barneprosessen til sesjons- og prosessgruppeleder.
  // Setsid setter ny pid, setsid < 0 betyr at den feilet.
  if (setsid() < 0)
    {
      // Avslutt prosess med feil.
      fprintf(stderr, "Deamon setsid feilet: %s\n", strerror(errno));
      exit(1);
    }

  // Ignorer SIGHUP ved terminering i neste trinn.
  signal(SIGHUP, SIG_IGN);

  // Lag ny barneprosess.
  pid = fork();

  // Pid < 0 betyr at fork feilet.
  if (pid < 0)
    {
      // Avslutt prosess med feil.
      fprintf(stderr, "Deamon fork 2 feilet: %s\n", strerror(errno));
      exit(1);
    }

  // Pid > 0 er forelder, barn = 0.
  if (pid > 0)
    {
      // Avslutt foreldreprosessen for å hindre tjener som sesjonsleder.
      exit(0);
    }
}

// Funksjon for å endre rot.
static void nyrot()
{
  // Setter . som ny rot.
  if (chroot("..") == -1)
    {
      fprintf(stderr, "Chroot feilet: %s\n", strerror(errno));
      exit(1);
    }
}
