#include "XOPStandardHeaders.h"		// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h

#include "DFMLoadWave.h"

// Operation template: DFMLoadWave /DFM=string:dfmParamStr /B[=number:lowByteFirst] /CSTR /P=name:dataPathName /O /I[={string:macFilterStr,string:winFilterStr}] /ST=number:sampleTime /DFMP=name:dfmPathName [string:fileParamStr]

// Runtime param structure for DFMLoadWave operation.
#pragma pack(2)     // All structures passed to Igor are two-byte aligned.
struct DFMLoadWaveRuntimeParams {
	// Flag parameters.
	
	// Parameters for /DFM flag group.
	int DFMFlagEncountered;
	Handle dfmParamStr;
	int DFMFlagParamsSet[1];
	
	// Parameters for /B flag group.
	int BFlagEncountered;
	double lowByteFirst;					// Optional parameter.
	int BFlagParamsSet[1];
	
	// Parameters for /CSTR flag group.
	int CSTRFlagEncountered;
	// There are no fields for this group because it has no parameters.
	
	// Parameters for /P flag group.
	int PFlagEncountered;
	char dataPathName[MAX_OBJ_NAME+1];
	int PFlagParamsSet[1];
	
	// Parameters for /O flag group.
	int OFlagEncountered;
	// There are no fields for this group because it has no parameters.
	
	// Parameters for /I flag group.
	int IFlagEncountered;
	Handle macFilterStr;					// Optional parameter.
	Handle winFilterStr;					// Optional parameter.
	int IFlagParamsSet[2];
	
	// Parameters for /ST flag group.
	int STFlagEncountered;
	double sampleTime;
	int STFlagParamsSet[1];
	
	// Parameters for /DFMP flag group.
	int DFMPFlagEncountered;
	char dfmPathName[MAX_OBJ_NAME+1];
	int DFMPFlagParamsSet[1];
	
	// Main parameters.
	
	// Parameters for simple main group #0.
	int fileParamStrEncountered;
	Handle fileParamStr;					// Optional parameter.
	int fileParamStrParamsSet[1];
	
	// These are postamble fields that Igor sets.
	int calledFromFunction;					// 1 if called from a user function, 0 otherwise.
	int calledFromMacro;					// 1 if called from a macro, 0 otherwise.
};
typedef struct DFMLoadWaveRuntimeParams DFMLoadWaveRuntimeParams;
typedef struct DFMLoadWaveRuntimeParams* DFMLoadWaveRuntimeParamsPtr;
#pragma pack()          // Reset structure alignment to default.

static int
InitSettings(LoadSettingsPtr lsp)
{
	MemClear(lsp, sizeof(LoadSettings));

	lsp->flags = 0;
	lsp->lowByteFirst = 0;
		
	return 0;
}

static int
ExecuteDFMLoadWave(DFMLoadWaveRuntimeParamsPtr p)
{
	int err = 0;
	char symbolicPathName[MAX_OBJ_NAME+1];
	char fileParam[MAX_PATH_LEN+1];
	char dfmSymbolicPathName[MAX_OBJ_NAME+1];
	char dfmFileParam[MAX_PATH_LEN+1];
	LoadSettings ls;

	InitSettings(&ls);							// Set default values.
	*symbolicPathName = NULL;
	*dfmSymbolicPathName = NULL;
	*fileParam = NULL;
	*dfmFileParam = NULL;


	// Flag parameters.

	if (p->DFMFlagEncountered) {
		if (p->dfmParamStr == NULL)
			return USING_NULL_STRVAR;
		if (err = GetCStringFromHandle(p->dfmParamStr, dfmFileParam, sizeof(dfmFileParam)-1))
			return err;
	}

	if (p->BFlagEncountered) {
		if (p->BFlagParamsSet[0]) {
			// Optional parameter: p->lowByteFirst
			ls.lowByteFirst=p->lowByteFirst;
		}
	}
	
	if (p->CSTRFlagEncountered) {
		ls.flags |= CHAR2CSTR;
	}

	if (p->PFlagEncountered) {
		if (*p->dataPathName != 0) {				// Treat /P=$"" as a NOP.
			strcpy(symbolicPathName, p->dataPathName);
			ls.flags |= PATH;
		}
	}

	if (p->OFlagEncountered) {
		ls.flags |= OVERWRITE;
	}

	if (p->IFlagEncountered) {
		ls.flags |= INTERACTIVE;				// Need dialog to get file name.
		if (p->IFlagParamsSet[0]) {
			if (p->macFilterStr == NULL) {
				return USING_NULL_STRVAR;
			}
			else {
				#ifdef MACIGOR
					if (err = GetCStringFromHandle(p->macFilterStr, ls.filterStr, sizeof(ls.filterStr)-1))
						return err;
				#endif
			}
		}
		
		if (p->IFlagParamsSet[1]) {
			if (p->winFilterStr == NULL) {
				return USING_NULL_STRVAR;
			}
			else {
				#ifdef WINIGOR
					char* t;

					if (err = GetCStringFromHandle(p->winFilterStr, ls.filterStr, sizeof(ls.filterStr)-1))
						return err;
						
					// To make this string suitable for XOPOpenFileDialog, we need to replace tabs with nulls.
					t = ls.filterStr;
					while(*t != 0) {
						if (*t == '\t')
							*t = 0;
						t += 1;						
					}
				#endif
			}
		}
	}

	if (p->STFlagEncountered) {
		// Parameter: p->sampleTime
		ls.sampleTime = p->sampleTime;
	}

	if (p->DFMPFlagEncountered) {
		if (*p->dfmPathName != 0) {				// Treat /P=$"" as a NOP.
			strcpy(dfmSymbolicPathName, p->dfmPathName);
			ls.flags |= DFMPATH;
		}

	}

	// Main parameters.
	if (p->fileParamStrEncountered) {
		if (p->fileParamStr == NULL)
			return USING_NULL_STRVAR;
		if (err = GetCStringFromHandle(p->fileParamStr, fileParam, sizeof(fileParam)-1))
			return err;
	}

	// Do the operation's work here.
	
	err = LoadWave(&ls, symbolicPathName, fileParam, dfmSymbolicPathName, dfmFileParam, p->calledFromFunction);
	return err;
}


int
RegisterDFMLoadWave(void)
{
	const char* cmdTemplate;
	const char* runtimeNumVarList;
	const char* runtimeStrVarList;
	
	// NOTE: If you change this template, you must change the DFMLoadWaveRuntimeParams structure as well.
	cmdTemplate = "DFMLoadWave /DFM=string:dfmParamStr /B[=number:lowByteFirst] /CSTR /P=name:dataPathName /O /I[={string:macFilterStr,string:winFilterStr}] /ST=number:sampleTime /DFMP=name:dfmPathName [string:fileParamStr]";
	runtimeNumVarList = "V_flag;";
	runtimeStrVarList = "S_path;S_fileName;S_waveNames;";
	return RegisterOperation(cmdTemplate, runtimeNumVarList, runtimeStrVarList, sizeof(DFMLoadWaveRuntimeParams), (void*)ExecuteDFMLoadWave, 0);
}

