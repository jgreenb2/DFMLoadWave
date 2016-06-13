// parseDFM.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define MAXVARNAMELEN 256
#define MAXBUFSIZE 512

typedef struct {
	char varname[MAXVARNAMELEN+1];
	char type[4];
	int  nElems;
} dataDef_t;

int parseDFM(char *,int *, dataDef_t **);

int _tmain(int argc, _TCHAR* argv[])
{
	dataDef_t *ddp=NULL;
	int nVars;
	int i;
	char fileName[]="C:\\Documents and Settings\\jgreenb2\\My Documents\\2009 Misc\\universal hmi\\context\\mlab_data\\context.dfm";

	parseDFM(fileName,&nVars,&ddp);

	for (i=0;i<nVars;i++) {
		printf("%s %s %d\n",ddp[i].varname,ddp[i].type,ddp[i].nElems);
	}
	return 0;
}

int parseDFM(char *fileName, int *nVars, dataDef_t **ddh)
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
		return errno;
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
				strcpy(ddp->varname,tokenArray[1]);
				ddp->nElems=1;
			break;
			
			// array definition
			case 3:
				strcpy(ddp->type,tokenArray[0]);
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

	*nVars=n;

	return 0;
}