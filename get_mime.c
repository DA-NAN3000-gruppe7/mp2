#include <stdio.h>
#include<stdlib.h>
#include<string.h>

static char* get_mime(char *filename);

int main(int argc, char* argv[]) {
	char* type = get_mime(argv[1]);
	printf("%s\n", type);

	return 0;
}


static char* get_mime(char *filename) {
	FILE *f;
	int i = 0;
	char buf[100];
	char *ext;

	ext = strrchr(filename, '.');
	if (!ext) {
		return "NO_EXT";
	} else {
   		ext = ext + 1;
	}

	f = fopen("etc/mime.types", "r");
	if(!f) {
		printf("ERROR: can't open mime.types\n");
		exit(EXIT_FAILURE);
	}
	while( fgets(buf, 50, f) != NULL ) {
		char *token = strtok(buf, "\t");
		char *type = malloc (sizeof (char) * 30);

		strcpy(type, token);
		token = strtok(NULL, "\t");
		token = strtok(token, " ");
		while (token != NULL)
		{
			char mime_ext[10];
			token[strcspn(token, "\n")] = 0;
			strcpy(mime_ext, token);

			if(strcmp(mime_ext, ext) == 0) {
				//char *ret = malloc (sizeof (type) * 30);
				return type;
			}
			token = strtok(NULL, " ");


		}

	}

		
	fclose(f);

	return "MIME_NOT_FOUND";

}