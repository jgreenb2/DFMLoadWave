#include "XOPStandardHeaders.h"		// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "parseDFM.h"
int getIgorType(char *s);

int parseDFM(char *fileName, int *ddSize, dataDef_t **ddh)
{	FILE *dfmp;
	char buffer[MAXBUFSIZE];
	char *token;
	int n=0;
	int ntokens=0;
	typedef char token_t[MAXVARNAMELEN];
	token_t tokenArray[4];
	dataDef_t *ddp;
	char strsep[]=" \t[];\n";

	// open the file

	dfmp = fopen(fileName,"r");
	if (dfmp == NULL) {
		return -errno;
	}

	// loop through and parse each line
	fgets(buffer, MAXBUFSIZE,dfmp);
	while (!feof(dfmp)) {
		// reallocate a new slot in the data def array
		n++;
		*ddh = (dataDef_t *) realloc(*ddh,sizeof(dataDef_t)*n);
		ddp = *ddh + n - 1;	// pointer to current data def slot

		// loop through the tokens on each line
		token = strtok(buffer,strsep);
		ntokens=0;
		while (token != NULL) {
			strcpy(tokenArray[ntokens++],token);
			token = strtok(NULL,strsep);
		}

		// depending on how many tokens we found we can 
		// fill the data def slot
		switch (ntokens) {
			// ignore eol or blank lines
			case 0:
			break;
			
			// scalar definition
			case 2:
				strcpy(ddp->type,tokenArray[0]);
				ddp->IgorTypeID = getIgorType(ddp->type);
				strcpy(ddp->varname,tokenArray[1]);
				ddp->nElems=1;
			break;
			
			// array definition
			case 3:
				strcpy(ddp->type,tokenArray[0]);
				ddp->IgorTypeID = getIgorType(ddp->type);
				strcpy(ddp->varname,tokenArray[1]);
				ddp->nElems=atoi(tokenArray[2]);
			break;
			
			// error
			default:
				return n;
			break;
		}

		fgets(buffer,MAXBUFSIZE,dfmp);
	}

	fclose(dfmp);

	*ddSize=n;

	return 0;
}

int getIgorType(char *s)
{
	if (strcmp(s,"int")==0) {
		return NT_I32;
	} else if (strcmp(s,"double")==0) {
		return NT_FP64;
	} else if (strcmp(s,"short")==0) {
		return NT_I16;
	} else if (strcmp(s,"float")==0) {
		return NT_FP32;
	} else if (strcmp(s,"char")==0) {
		return TEXT_WAVE_TYPE;
	} else {
		return -1;
	}
}