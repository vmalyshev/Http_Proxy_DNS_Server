#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char *parse(char *line) {
	char* p = strtok(line, "=");
	char* temp;
	while (p != NULL) {
		p = strtok(NULL, "=");
		if (p != NULL) {
			temp = p;
			strtok(NULL, "=");
		}
	}
	return temp;
}

char *save_to_string(char *value)
{
	char *temp = malloc(strlen(value)+1);
	strcpy(temp, parse(value));

	if (temp[strlen(temp)-1] == '\n') {
		temp[strlen(temp)-1] = '\0';
	}
	
	return temp;
}

int save_to_int(char *value)
{
	return atoi(parse(value));
}

char *save_url(char *value)
{
	char *temp = malloc(strlen(value));
	strcpy(temp, value);

	if (temp[strlen(temp)-1] == '\n') {
		temp[strlen(temp)-1] = '\0';
	}
	
	return temp;
}



















