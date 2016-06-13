/*
 *	DFMLoadWave.h
 *		equates for DFMLoadWave XOP
 *
 */

/* DFMLoadWave custom error codes */

#define IMPROPER_FILE_TYPE 1 + FIRST_XOP_ERR	/* not the type of file this XOP loads */
#define NO_DATA_FOUND 2 + FIRST_XOP_ERR			/* file being loaded contains no data */
#define EXPECTED_GB_FILE 3 + FIRST_XOP_ERR		/* expected name of loadable file */
#define EXPECTED_BASENAME 4 + FIRST_XOP_ERR		/* expected base name for new waves */
#define EXPECTED_FILETYPE 5 + FIRST_XOP_ERR		/* expected file type */
#define TOO_MANY_FILETYPES 6 + FIRST_XOP_ERR	/* too many file types */
#define BAD_DATA_LENGTH 7 + FIRST_XOP_ERR		/* data length in bits must be 8, 16, 32 or 64 */
#define BAD_NUM_WAVES 8 + FIRST_XOP_ERR			/* number of waves must be >= 1 */
#define NOT_ENOUGH_BYTES 9 + FIRST_XOP_ERR		/* file contains too few bytes for specified ... */
#define BAD_DATA_TYPE 10 + FIRST_XOP_ERR		/* bad data type value */
#define OLD_IGOR 11 + FIRST_XOP_ERR				/* Requires Igor Pro x.x or later */
#define BAD_FP_FORMAT_CODE 12 + FIRST_XOP_ERR	/* Valid floating point formats are 1 (IEEE) and 2 (VAX). */

#define LAST_GBLOADWAVE_ERR BAD_FP_FORMAT_CODE

#define ERR_ALERT 1258

/* #defines for GBLoadWave flags */

#define OVERWRITE 1						/* /O means overwrite */
#define INTERACTIVE 2					/* /I means interactive -- use open dialog */
#define PATH 4							/* /P means use symbolic path */
#define DFMPATH 8
#define FROM_MENU 16					/* LoadWave summoned from menu item */
#define CHAR2CSTR 32

/* structure used in reading file */
struct ColumnInfo {
	char waveName[MAX_OBJ_NAME+1];		/* Name of wave for this column. */
	waveHndl waveHandle;				/* Handle to this wave. */
	int wavePreExisted;					/* True if wave existed before this command ran. */
	long points;						/* Total number of points in wave. */
	int hState;							/* To save locked/unlocked state of wave handle. */
	int nChars;							/* 0 for numeric data, number of characters in text fields */
	void *dataPtr;						/* To save pointer to start of wave data. */
};
typedef struct ColumnInfo ColumnInfo;
typedef struct ColumnInfo *ColumnInfoPtr;
typedef struct ColumnInfo **ColumnInfoHandle;

// This structure is used in memory only - not saved to disk.
struct LoadSettings {
	long flags;						// Flag bits are defined in DFMLoadWave.h.
	int lowByteFirst;				// 0 = high byte first (Motorola); 1 = low byte first (Intel). Set by /B flag.
	double sampleTime;				// sample time used for wave scaling.
	char filterStr[256];			// Set by /I= flag.
};
typedef struct LoadSettings LoadSettings;
typedef struct LoadSettings *LoadSettingsPtr;

/* Since the DFMLoadInfo structure is saved to disk, we make sure that it is 2-byte-aligned. */
#pragma pack(2) // All structures passed to Igor must use 2-byte alignment

/* structure used to save GBLoadWave settings */
struct DFMLoadInfo {
	short version;						/* version number for structure */
	short bytesSwapped;
	short overwrite;
	double sampleTime;
	
	/* fields below added in version #2 of structure */
	char symbolicPathName[32];	
	char dfmSymbolicPathName[32];
	short char_is_cstring;
};
typedef struct DFMLoadInfo DFMLoadInfo;
typedef struct DFMLoadInfo *DFMLoadInfoPtr;
typedef struct DFMLoadInfo **DFMLoadInfoHandle;

#define DFMLoadInfo_VERSION 4			// HR, 980401: Incremented to 4 because of adding filterStr field.

#pragma pack()	// Reset structure alignment to default.


/* miscellaneous #defines */

#define MAX_IGOR_UNITS 4				/* max chars allowed in Igor unit string */
#define MAX_USER_LABEL 3
#define MAX_WAVEFORM_TITLE 18

/* Prototypes */

// In DFMLoadWave.c
HOST_IMPORT int XOPMain(IORecHandle ioRecHandle);
int LoadWave(LoadSettings* lsp, const char* symbolicPathName, const char* fileParam, const char* dfmSymbolicPathName, const char* dfmFileParam, int runningInUserFunction);

// In DFMLoadWaveOperation.c
int RegisterDFMLoadWave(void);

// In DFMLoadWaveDialog.c
int DFMLoadWaveDialog(void);
int GetLoadFile(const char* initialDir, const char* fileFilterStr, char *fullFilePath);

