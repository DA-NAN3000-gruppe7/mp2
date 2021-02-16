# mp2

MILEPÆL 2

Dette er markdown-dokumentet for milepæl 2 i DA-NAN3000. I dette dokumentet spesifiserers hva oppgaven går ut på, hva som er planen/krav, hvordan man bruker systemet og hva som er loggført.



## Oppgave
1. 	Utvid tjeneren fra milepæl 1, slik at den (minimum) kan leverere filer av følgende typer:

	* text/html,
	* text/plain,
	* image/png,
	* image/svg,
	* application/xml,
	* application/xslt+xml
	* text/css
	* application/json

	Identifiseringen av type skal gjøres ved hjelp av filendelsen. Eksempel: Hvis fila ender på .txt så er det av typen text/plain. Sammenheng mellom mime-typer og filendelser finner dere i filen /etc/mime.types.

2. 	Sørge for at tjeneren kjører i en busybox-basert container som skal opprettes ved hjelp av chroot(8) og unshare(1) Slik at tilgang til filer og prosesser begrenses.
	Filtreet i containern skal inneholde minst mulig. 
Ta gjerne utgangspunkt i eksempelet unshare-container

3.	Lag en web-side for gruppa som har minst ett bilde. Websiden skal være stilsatt, ved hjelp av en CSS-fil.

	*[2021-01-18 ma. 15:50]* Merk: Filene som websiden består av skal være lagret i webroten på webtjeneren dere lager, og skal "serveres" av denne tjeneren.


## Plan
1. 	I denne oppgaven skal det lages en funksjon som kan lese av mime-typer til filer som blir spesifisert. Dette skal brukes av webserver programmet når en http forespørsel blir gjort som da returnerer mime-typen som blir vist i http-headeren. Det skal sendes riktig http-meldinger ettersom hva som blir returnert av get_mime(). Dersom filen ikke eksisterer eller det er en forespørsel til en fil av type som ikke er i */etc/mime.type* skal det sendes en 404

2. 	Det skal lages et bash-skript som skal initialiserer og kjøre en container ved bruk av *chroot* og *unshare* kommandoene som gjør at roten bli i katalogen til milepæl 2 systemet. Dette skal også gjøre at man bare har oversikt over prosesser som er innenfor systemet.

3. 	Det skal lages en .html og .css fil som blir returnert når man sender en http-forespørsel til port 80 på maskinen som webtjeneren kjører på. Disse skal lastes inn under en enkel forespørsel og tjenern skal da gi tillgang til alle filer under webroten*(var/www/)*

## Instruksjoner

Programmet krever følgende:

*	**busybox**-binære filer for å kjøre. Installer **busybox** med den tillhørende packagemanageren på distroet ditt.*

For å kjøre programmet trenger man bare å kjøre ./run. Dette skriptet laster ned dumb-init til bin, legger til busybox symlenker i bin, kompilerer C-koden og så lager en ny namespace og endrer rota til katalogen til **mp2**. Fra der kjører init.sh som burker dumb-init til å tillordne den PID 1. Fra dette lille init-skriptet kjører webserverern og systemet kjøres i en busybox shell container som man kan arbeide fra.

Webserveren kjører på port 80, og man kan gjøre HTTP forespørsler til tjeneren. Så lang kan man bare forespørre til index.asis filen, som bare inneholder en kort tekst om at programmet funker.

Webserveren blir loggført i /var/log/debug.log

## Log
<br>

	16-02-2021
		MAGNUS: Har nå konfigurert run.sh slik at den laster ned dumb-init om den ikke finnes i bin, den legger til busybox symlenker i bin, og kompilerer webserver.c. I tillegg kjører den en ny namespace og enderer rota til systemet, og kjører init.sh med dumb-init som shebang. init.sh aktiverer webserveren og kjører systemet i et busybox skall.
		En viktig observasjon er at man bør i sterkeste grad bruke *umount $PWD/proc* når man er ferdig med containerern, ellers forblir alle prossesorene i proc katalogen i **mp2**.

	15-02-2021
		MAGNUS: det jeg har lagt til er å ha en funskjon *get_mime()* som tar en streng av et filnavn som argument og så finner ut MIME-typen.
		Dette gjør den ved å tokenisere filstrengen, der fileksensjonen blir lagret i en buffer *ext*, og så leser filen **/etc/mime.types**
		Her blir også innholdet av **mime.types** tokenisert der mime-typen blir en token og de tillørende ekstensjonene blir en annen token. Så sammenlignes *ext* med mime-ekstensjonene og dersom de er like returneres mime-typen. Om det er flere ekstensjoner blir de samenlignet i sekvens.
		Om filtypen ikke finnes i mime, om strengen er tom, eller om det ikke er noen ekstensjon, returneres feilmeldinger.