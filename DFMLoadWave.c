/*	DFMLoadWave.c
	
	An Igor external operation, DFMLoadWave, that imports data from various binary files.
	
	See the file DFMLoadWave Help for usage of DFMLoadWave XOP.
	
	HR, 10/15/93
		Added code to set S_waveNames output string.
		
	HR, 1/8/96 (Update for Igor Pro 3.0)
		Allowed zero and one point waves.
		Set up to require Igor Pro 3.0 or later.
	
	HR, 2/17/96
		Prior to version 3.0, there was one flag (/L) that specified the length of
		the input data and another flag (/F) that specified the format. Now, there
		is an additional flag (/T) that specifies both at once. For backward compatibility,
		the /L and /F flags are still supported.
		
		The program uses two different ways of specifying the input data type:
			inputBytesPerPoint and inputDataFormat		/L and /F flags
		or
			inputDataType								/T flag
		
		inputBytesPerPoint is 1, 2, 4 or 8
		inputDataFormat is IEEE_FLOAT, SIGNED_INT or UNSIGNED_INT
		
		inputDataType is one of the standard Igor data types:
			NT_FP64, NT_FP32
			NT_I32, NT_I16, NT_I8
			NT_I32 | NT_UNSIGNED, NT_I16 | NT_UNSIGNED, NT_I8 | NT_UNSIGNED
		
		The NumBytesAndFormatToNumType routine converts from the first representation
		to the second representation and the NumTypeToNumBytesAndFormat routine
		converts from the second to the first.
		
		The same is true for the output data type (the type of the waves that are created).
		The type is represented in these same two ways and the same routines are used
		to convert between them.
	
	HR, 2/20/96
		Fixed a bug in version 1.20. The interleave feature worked only when the
		waves created were 4 or 8 bytes per point.
		
	The requirement that we run with Igor Pro 3.0 or later comes from two reasons.
	First, we use CreateValidDataObjectName which requires Igor Pro 3.0. Second,
	we allow zero and one point waves, which Igor Pro 2.0 does not allow.
	
	HR, 10/96
		Converted to work on Windows as well as Macintosh.
		Changed to save settings in the IGOR preferences file rather than in
		the XOP's resource fork. This requires IGOR Pro 3.10 or later.
	
	HR, 980331
		Made changes to improve platform independence.
	
	HR, 980401
		Added a new syntax for the /I flag that allows the user to provide filters
		for both Macintosh and Windows.
	
	HR, 980403
		Changed to use standard C file I/O for platform independence.
	
	HR, 010528
		Started port to Carbon.
		
	HR, 020916
		Revamped for Igor Pro 5 using Operation Handler.
*/

#include "XOPStandardHeaders.h"		// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h

#include "DFMLoadWave.h"
#include "parseDFM.h"

// Global Variables (none).

/*	MakeAWave(waveName, flags, waveHandlePtr, points)

	MakeAWave makes a wave with specified number of points.
	It returns 0 or an error code.
	It returns a handle to the wave via waveHandlePtr.
	It returns the number of points in the wave via pointsPtr.
*/
static int
MakeAWave(char *waveName, int flags, waveHndl *waveHandlePtr, long points, int dataType)
{	
	int type, overwrite;
	long dimensionSizes[MAX_DIMENSIONS+1];
	
	type = dataType;
	overwrite = flags & OVERWRITE;
	MemClear(dimensionSizes, sizeof(dimensionSizes));
	dimensionSizes[0] = points;
	return MDMakeWave(waveHandlePtr, waveName, NIL, dimensionSizes, type, overwrite);
}

/*	AddWaveNameToHandle(waveName, waveNamesHandle)
	
	This routine accumulates wave names in a handle for later use in setting
	the S_waveNames file-loader variable in Igor.
	
	waveNamesHandle contains a C (null terminated) string to which we append
	the current name.
*/
static int
AddWaveNameToHandle(char* waveName, Handle waveNamesHandle)
{
	int len;
	char temp[256];
	
	len = strlen(waveName);
	SetHandleSize(waveNamesHandle, GetHandleSize(waveNamesHandle) + len + 1);
	if (MemError())
		return NOMEM;

	sprintf(temp, "%s;", waveName);
	strcat(*waveNamesHandle, temp);
	
	return 0;
}

/*	FinishWave(wavH, point, points, flags, message)

	FinishWave trims unneeded points from the end of the specified wave.
	point is the number of the first unused point in the wave.
	points is the total number of points in the wave.
	flags is the flags passed to the MakeWave operation.
	message is NIL for no message at all, "" for standard message of "whatever" for custom message.
	It returns 0 or an error code.
*/
static int
FinishWave(waveHndl wavH, long point, long points, char *message)
{
	char buffer[80];
	char waveName[MAX_OBJ_NAME+1];
	int result;
	
	WaveName(wavH, waveName);					// Get wave name.
	
	/*	HR, 1/8/96
		Removed test requiring at least two points. In Igor Pro 3.0, waves
		can contain any number of points, including zero.
	*/
	
	/*	Cut back unused points at end of wave.
		This is not used in DFMLoadWave. It is useful when loading text
		files which may have blank values at the end of a column.
	*/
	if (points-point) {							// Are there extra points at end of wave ?
		if (result = ChangeWave(wavH, point, WaveType(wavH)))
			return result;
	}
	
	sprintf(buffer, "%s loaded, %ld points\015", waveName, point);
	WaveHandleModified(wavH);

	if (message == 0) {		// Want message ?
		if (*message)							// Want custom message ?
			strcpy(buffer, message);
		XOPNotice(buffer);
	}
	return 0;
}

/*	TellFileType(fileMessage, fileName, flags, totalBytes)

	Shows type of file being loaded in history.
*/
static void
TellFileType(char *fileMessage, char *fileName, int flags, long totalBytes)
{
	char temp[MAX_PATH_LEN + 100];
	

	sprintf(temp, "%s \"%s\" (%ld total bytes)\015", fileMessage, fileName, totalBytes);
	XOPNotice(temp);

}

#define MAX_MESSAGE 80
static void
AnnounceWaves(ColumnInfoPtr caPtr, long numColumns)
{
	ColumnInfoPtr ciPtr;
	char temp[MAX_MESSAGE+1+1];		// Room for message plus null plus CR.
	long column;

	sprintf(temp, "Data length: %ld, waves: ", caPtr->points);
	ciPtr = caPtr;
	column = 0;
	do {
		while (strlen(temp) + strlen(ciPtr->waveName) < MAX_MESSAGE) {
			strcat(temp, ciPtr->waveName);
			strcat(temp, ", ");
			column += 1;
			ciPtr += 1;
			if (column >= numColumns)
				break;
		}
		temp[strlen(temp)-2] = 0;			// Remove last ", ".
		strcat(temp, "\015");
		XOPNotice(temp);
		strcpy(temp, "and: ");				// Get ready for next line.
	} while (column < numColumns);
}

static void
GetWaveStuff(waveHndl waveHandle, int *dataTypePtr, void **data)
{
	*dataTypePtr = WaveType(waveHandle);
	*data = WaveData(waveHandle);
}


/*	LoadData(fileRef, lsp, caPtr, numColumns, waveNamesHandle)

	Loads data into waves.
	lsp is a pointer to a structure containing settings for the LoadWave operation.
	caPtr points to locked array of column info (see .h file).
	numColumns is number of "columns" (arrays of data to be loaded into waves) in file.
	The names of waves loaded are stored in waveNamesHandle for use in creating the S_waveNames output string.
*/
static int
LoadData(XOP_FILE_REF fileRef, LoadSettingsPtr lsp, ColumnInfoPtr caPtr, int numColumns, Handle waveNamesHandle, int nTextWaves)
{
	ColumnInfoPtr ciPtr;
	long column;
	long numPoints;
	int dataType;
	char* strbuf;
	int maxCharFieldLen=0;
	char* readDataPtr;
	int nBytes;
	int result, err;
	long nRec;
	int formatPtr,isComplex;
	int i,nt=0;
	Handle *textHandles;
	
	numPoints = caPtr->points;
	
	// for each text wave, create a text handle to hold the data
	//textHandles = (Handle *)NewHandle((long)(sizeof(Handle *)*nTextWaves));
	textHandles = (Handle *) NewPtr((long)(sizeof(Handle *)*nTextWaves));
	if (textHandles == NIL)
		return NOMEM;
	MemClear((void *) textHandles, GetPtrSize((Ptr)textHandles));	

	for (column=0;column< numColumns;column++) {
		ciPtr = caPtr + column;
		GetWaveStuff(ciPtr->waveHandle, &dataType, &ciPtr->dataPtr);
		if (dataType == TEXT_WAVE_TYPE) {
			textHandles[nt] = NewHandle(ciPtr->nChars);
			if (textHandles[nt] == NIL)
				return NOMEM;
			MemClear((void *) *(textHandles[nt]), GetHandleSize(textHandles[nt]));
			if (ciPtr->nChars > maxCharFieldLen) 
				maxCharFieldLen = ciPtr->nChars;
			nt++;
		}
		//HSetState(ciPtr->waveHandle, ciPtr->hState);
	}
	//	allocate a string buffer in case we need to convert char fields
	// to c-strings
	
	strbuf = malloc(maxCharFieldLen);
	if (strbuf == NULL)
		return NOMEM;
	
	for (nRec=0;nRec<numPoints;nRec++) {
		nt=0;
		for (column = 0; column < numColumns; column++) {
			ciPtr = caPtr + column;
			dataType=WaveType(ciPtr->waveHandle);
			if (dataType == TEXT_WAVE_TYPE) {
				result = XOPReadFile(fileRef, ciPtr->nChars, strbuf, NULL);
				// if we're treating the char fields as c-strings, blank fill the
				// textHandle after the terminating null
				if (lsp->flags & CHAR2CSTR) {
					PutCStringInHandle(strbuf, textHandles[nt]);
				} else {
					memcpy(*(textHandles[nt]),strbuf,ciPtr->nChars);
				}
				result = MDSetTextWavePointValue(ciPtr->waveHandle,&nRec,textHandles[nt]);
				nt++;
			} else {
				NumTypeToNumBytesAndFormat(dataType,&nBytes,&formatPtr,&isComplex);
				readDataPtr = (char *) ciPtr->dataPtr + nRec*nBytes;
				result = XOPReadFile(fileRef, nBytes, readDataPtr, NULL);
			}
			if (result==0) {
			// See if we need to reverse bytes.
				#ifdef XOP_LITTLE_ENDIAN
					if (lsp->lowByteFirst == 0)
						FixByteOrder(readDataPtr, nBytes, 1);
				#else
					if (lsp->lowByteFirst != 0)
						FixByteOrder(readDataPtr, nBytes, 1);
				#endif

			}
			if (result)
				break;
		}
		if (result)
			break;
	}
	
	free(strbuf);

	for (i=0;i<nTextWaves;i++) {
		DisposeHandle(textHandles[i]);
	}
	DisposePtr((Ptr) textHandles);
	
	if (XOPAtEndOfFile(fileRef))
		result = 0;
	
	// Clean up waves.
	for (column = 0; column < numColumns; column++) {
		ciPtr = caPtr + column;
		if (ciPtr->waveHandle != NIL) {
			AddWaveNameToHandle(ciPtr->waveName, waveNamesHandle);		// Keep track of waves loaded so that we can set S_waveNames.
			if (err = FinishWave(ciPtr->waveHandle, numPoints, numPoints, "")) {
				if (result == 0)
					result = err;
			}
		}
	}
		
	return result;
}

/*	Load(fileRef, lsp, fileName, baseName, numWavesLoadedPtr, waveNamesHandle)
	
	Loads custom file and returns error code.
	fileRef is the ref number for the file's data fork.
	lsp is a pointer to a structure containing settings for the LoadWave operation.
	baseName is base name for new waves or "" to auto name.
	The names of waves loaded are stored in waveNamesHandle for use in creating the S_waveNames output string.
*/
static int
Load(XOP_FILE_REF fileRef, LoadSettingsPtr lsp, char *fileName, dataDef_t *ddp, int ddSize, int* numWavesLoadedPtr, Handle waveNamesHandle)
{
	int column;
	ColumnInfoHandle ciHandle;					// See DFMLoadWave.h.
	ColumnInfoPtr ciPtr;
	char tempName[MAX_OBJ_NAME+1];			
	char temp[128];
	int suffixNum;
	int nameChanged, doOverwrite;
	unsigned long fileBytes;
	int result;
	long nDataPoints;
	int i,j,nWaves=0,nBytesPerRecord=0,dataBytes;
	int nTextWaves=0;
	
	*numWavesLoadedPtr = 0;
	
	// compute the number of waves to load
	for (i=0;i<ddSize;i++) {
		switch (ddp[i].IgorTypeID) {
			case NT_FP64:
				dataBytes=8;
				nWaves+=ddp[i].nElems;
			break;

			case NT_FP32:
			case NT_I32:
				dataBytes=4;
				nWaves+=ddp[i].nElems;
			break;

			case NT_I16:
				dataBytes=2;
				nWaves+=ddp[i].nElems;
			break;

			case TEXT_WAVE_TYPE:
				dataBytes=1;
				nWaves++;
			break;
		}
		nBytesPerRecord+=dataBytes*ddp[i].nElems;
	}


	// Find the total number of bytes in the file.
	if (result = XOPNumberOfBytesInFile(fileRef, &fileBytes))
		return result;
	nDataPoints = fileBytes/nBytesPerRecord;

	strcpy(temp, "DFM binary file load from");
	TellFileType(temp, fileName, lsp->flags, fileBytes);
		
	
	// Go to start of data.
	if (result = XOPSetFilePosition(fileRef,0, -1))
		return result;

	/*	Make data structure used to keep track of each wave being made.
		Because this structure originated in an XOP that loaded text files,
		we call each wave a "column".
	*/
	ciHandle = (ColumnInfoHandle)NewHandle((long)(sizeof(ColumnInfo)*nWaves));
	if (ciHandle == NIL)
		return NOMEM;
	MemClear((void *)*ciHandle, GetHandleSize((Handle)ciHandle));	
	
		
	// Make a wave for each array in the file.
	suffixNum = 0;								// Used by CreateValidDataObjectName to create unique wave names.
	column=0;
	for (i = 0; i < ddSize; i++) {
		//	In DFMLoadWave, wave names come from the DFM file (through the dataDef_t pointer)
		suffixNum=0;
		for (j=0;j < ddp[i].nElems;j++) {
			ciPtr = *ciHandle + column;
			ciPtr->points = nDataPoints;
			strncpy(tempName, ddp[i].varname,MAX_OBJ_NAME);
			tempName[MAX_OBJ_NAME]=0;
			ciPtr->nChars = (ddp[i].IgorTypeID == TEXT_WAVE_TYPE ? ddp[i].nElems : 0);

			/*	CreateValidDataObjectName takes care of any illegal or conflicting names.
				CreateValidDataObjectName requires Igor Pro 3.00 or later.
			*/
			result = CreateValidDataObjectName(NULL, tempName, ciPtr->waveName, &suffixNum, WAVE_OBJECT,
				1, lsp->flags & OVERWRITE, (ddp[i].nElems>1) && !ciPtr->nChars, 1, &nameChanged, &doOverwrite);
            if (ddp[i].nElems > 1) strncat(ciPtr->waveName, "_", MAX_OBJ_NAME);
			ciPtr->wavePreExisted = doOverwrite;	// Used below in case an error occurs and we want to kill any waves we've made.
			
			if (result == 0) {
				if (doOverwrite) {
					waveHndl wh;
					wh = FetchWave(ciPtr->waveName);
					if (wh != NULL) {
						result = KillWave(wh);
					}
				}
				if (!result) {
					result = MakeAWave(ciPtr->waveName, lsp->flags, &ciPtr->waveHandle, nDataPoints, ddp[i].IgorTypeID);
				}
			}
			
			if (result) {							// Couldn't make wave (probably low memory) ?
				int thisColumn;
				thisColumn = column;				// Kill waves that we just made.
				for (column = 0; column < thisColumn; column++) {
					ciPtr = *ciHandle + column;
					if (!ciPtr->wavePreExisted)		// Is this a wave that we just made?
						KillWave(ciPtr->waveHandle);
				}
				DisposeHandle((Handle)ciHandle);
				return result;
			}
			column++;
			
			// text arrays are treated as single text waves
			if (ciPtr->nChars) {
				nTextWaves++;
				break;
			}
		}
	}
	
	// Load data.
	result = LoadData(fileRef, lsp, *ciHandle, nWaves, waveNamesHandle, nTextWaves);
	if (result == 0) {
		double zero=0;
		*numWavesLoadedPtr = nWaves;
		
		AnnounceWaves(*ciHandle, nWaves);

		// scale the waves

		for (column=0;column<nWaves;column++) {
			ciPtr = *ciHandle + column;
			MDSetWaveScaling(ciPtr->waveHandle,0,&lsp->sampleTime,&zero);
		}
	}
		
	DisposeHandle((Handle)ciHandle);
	free(ddp);
	
	if (result == 0 && nWaves == 0)
		result = NO_DATA_FOUND;
	
	return result;
}

/*	LoadFile(fullFilePath, lsp, baseName, calledFromFunction)

	LoadFile loads data from the file specified by fullFilePath.
	See the discussion at the top of this file for the specifics on fullFilePath and wavename.
	
	It returns 0 if everything goes OK or an error code if not.
*/
static int
LoadFile(const char* fullFilePath, LoadSettingsPtr lsp, dataDef_t *ddp, int ddSize, int calledFromFunction)
{
	XOP_FILE_REF fileRef;
	int updateSave;
	int numWavesLoaded;				// Number of waves loaded from file.
	Handle waveNamesHandle;
	char fileName[MAX_FILENAME_LEN+1];
	char fileDirPath[MAX_PATH_LEN+1];
	int err;
	
	WatchCursor();
	
	if (err = GetDirectoryAndFileNameFromFullPath(fullFilePath, fileDirPath, fileName))
		return err;
	
	// Initialize file loader global output variables.
	if (err = SetFileLoaderOperationOutputVariables(calledFromFunction, fullFilePath, 0, ""))
		return err;

	// Open file.
	if (err = XOPOpenFile(fullFilePath, 0, &fileRef))
		return err;
		
	numWavesLoaded = 0;
	waveNamesHandle = NewHandle(1L);
	if (waveNamesHandle == NIL)
		return NOMEM;
	**waveNamesHandle = 0;		// We build up a C string in this handle.

	PauseUpdate(&updateSave);						// Don't update windows during this.
	err = Load(fileRef, lsp, fileName, ddp, ddSize, &numWavesLoaded, waveNamesHandle);
	ResumeUpdate(&updateSave);						// Go back to previous update state.

	XOPCloseFile(fileRef);

	if (err == 0) {
		err = SetFileLoaderOperationOutputVariables(calledFromFunction, fullFilePath, numWavesLoaded, *waveNamesHandle);
	}
	DisposeHandle(waveNamesHandle);

	ArrowCursor();

	return err;
}

/*	LoadWave(lsp, baseName, symbolicPathName, fileParam, calledFromFunction)

	LoadWave does the actual work of DFMLoadWave.
	
	symbolicPathName may be an empty string.
	
	fileParam may be full path, partial path, just a file name, or an empty string.

	LoadWave returns 0 if everything went alright or an error code if not.
*/
int
LoadWave(LoadSettings* lsp, const char* symbolicPathName, const char* fileParam, const char* dfmSymbolicPathName, const char* dfmFileParam, int calledFromFunction)
{
	char symbolicPathPath[MAX_PATH_LEN+1];		// Full path to the folder that the symbolic path refers to. This is a native path with trailing colon (Mac) or backslash (Win).
	char nativeFilePath[MAX_PATH_LEN+1];			// Full path to the file to load. Native path.
	char dfmSymbolicPathPath[MAX_PATH_LEN+1];		// Full path to the folder that the symbolic path refers to. This is a native path with trailing colon (Mac) or backslash (Win).
	char dfmNativeFilePath[MAX_PATH_LEN+1];			// Full path to the file to load. Native path.
	int err;
	char errormsg[320];
	dataDef_t *ddp=0;
	int ddSize;

	*dfmSymbolicPathPath = 0;
	if (*dfmSymbolicPathName != 0) {
		if (err = GetPathInfo2(dfmSymbolicPathName, dfmSymbolicPathPath))
			return err;
	}
	GetFullPathFromSymbolicPathAndFilePath(dfmSymbolicPathName, dfmFileParam, dfmNativeFilePath);
	
#ifdef MACIGOR
	err = HFSToPosixPath(dfmNativeFilePath, dfmNativeFilePath, 0);
	if (err) {
		sprintf(errormsg,"Failed to parse DFM path");
		XOPNotice(errormsg);
	}
#endif
	
	// parse the dfm file
	err = parseDFM(dfmNativeFilePath, &ddSize, &ddp);
	if (err<0) {
		sprintf(errormsg,"Error opening file %s: %s",dfmNativeFilePath,strerror(err));
		XOPNotice(errormsg);
		return err;
	} else if (err > 0) {
		sprintf(errormsg,"Error parsing file %s: at line #%d",dfmNativeFilePath,err);
		return err;
	}

	*symbolicPathPath = 0;
	if (*symbolicPathName != 0) {
		if (err = GetPathInfo2(symbolicPathName, symbolicPathPath))
			return err;
	}

	if (GetFullPathFromSymbolicPathAndFilePath(symbolicPathName, fileParam, nativeFilePath) != 0)
		lsp->flags |= INTERACTIVE;				// Need dialog to get file name.
		
	if (!FullPathPointsToFile(nativeFilePath))	// File does not exist or path is bad?
		lsp->flags |= INTERACTIVE;				// Need dialog to get file name.

	if (lsp->flags & INTERACTIVE) {
		if (err = XOPOpenFileDialog("Looking for a binary file", lsp->filterStr, NULL, symbolicPathPath, nativeFilePath))
			return err;
	}

	// We have the parameters, now load the data.
	err = LoadFile(nativeFilePath, lsp, ddp, ddSize, calledFromFunction);
	
	return err;
}

static int
RegisterOperations(void)		// Register any operations with Igor.
{
	int result;
	
	if (result = RegisterDFMLoadWave())
		return result;
	
	// There are no more operations added by this XOP.
		
	return 0;
}

/*	MenuItem()

	MenuItem() is called when XOP's menu item is selected.
	Puts up the Load General Binary dialog.
*/
static int
MenuItem(void)
{
	return DFMLoadWaveDialog();						// This is in DFMLoadWaveDialog.c.
}

/*	XOPEntry()

	This is the the routine that Igor calls to pass all messages other than INIT.

	You must get the message (GetXOPMessage) and get any parameters (GetXOPItem)
	before doing any callbacks to Igor.
*/
static void
XOPEntry(void)
{	
	long result = 0;
	
	switch (GetXOPMessage()) {
		case CLEANUP:								// XOP about to be disposed of.
			break;
		
		case MENUITEM:								// XOPs menu item selected.
			result = MenuItem();
			break;
	}
	SetXOPResult(result);
}

/*	main()

	This is the initial entry point at which the host application calls XOP.
	The message sent by the host must be INIT.

	main does any necessary initialization and then sets the XOPEntry field of the
	XOPRecHandle to the address to be called for future messages.
*/
HOST_IMPORT int XOPMain(IORecHandle ioRecHandle) // XOPMain requires Igor Pro 6.20 or later
{
	int result;
	
	XOPInit(ioRecHandle);				// Do standard XOP initialization.
	SetXOPEntry(XOPEntry);				// Set entry point for future calls.

	if (igorVersion < 620) {
		SetXOPResult(IGOR_OBSOLETE);	
		return EXIT_FAILURE;
	}

	if (result = RegisterOperations()) {
		SetXOPResult(result);
		return EXIT_FAILURE;
	}

	SetXOPResult(0L);
    return EXIT_SUCCESS;
}
