#include <stdio.h>
#include<stdlib.h>
#include<string.h>

#define TAB "\t"
#define SPACE " "
static char* get_mime(char *filename);

int main(int argc, char* argv[]) {
	if (argc > 1) {
		char* type = get_mime(argv[1]);
		printf("%s\n", type);
	}
	return 0;
}


static char* get_mime(char *filename) {

	FILE *f;
	char buf[100];
	char *ext;

	//if the string is empty
	if(filename[0] == '\0')
		return "EMPTY_FILE";

	//gets the extention of the string
	ext = strrchr(filename, '.');
	if (!ext) {
		return "NO_EXT";
	} else {
   		ext = ext + 1;
	}

	f = fopen("etc/mime.types", "r");
	if(!f) {
		return "CANNOT_OPEN_MIME.TYPES";
	}
	//parses the mime.types file
	while( fgets(buf, 50, f) != NULL ) {

		//initializes buffers to store mime-type and extension
		char *token = strtok(buf, TAB);
		char *type = malloc (sizeof (char) * 30);

		strcpy(type, token);
		token = strtok(NULL, TAB);
		token = strtok(token, SPACE);

		//iterates through all extensions of the mime-type
		while (token != NULL)
		{
			char mime_ext[10];
			token[strcspn(token, "\n")] = 0;
			strcpy(mime_ext, token);

			//condition is met if the mime extension matches the extension of the file
			//respective mime-type is returned
			if(strcmp(mime_ext, ext) == 0) {
				return type;
			}
			token = strtok(NULL, SPACE);


		}

	}

		
	fclose(f);

	return "MIME_NOT_FOUND";

}