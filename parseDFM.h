#define MAXVARNAMELEN 256
#define MAXBUFSIZE 512

typedef struct {
	char varname[MAXVARNAMELEN+1];
	char type[6];
	int  IgorTypeID;
	int  nElems;
} dataDef_t;

int parseDFM(char *,int *, dataDef_t **);

