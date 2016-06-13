/*	DFMLoadWaveDialog.c -- dialog handling routines for the DFMLoadWave XOP.

*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h

#include "DFMLoadWave.h"

#ifdef WINIGOR
	#include "resource.h"
#endif

void SetPort(GrafPtr);

// Global Variables

// Equates
#define DIALOG_TEMPLATE_ID 1260

enum {								// These are both Macintosh item numbers and Windows item IDs.
	DOIT_BUTTON=1,
	CANCEL_BUTTON,					// Cancel button ID must be 2 on Windows for else the escape key won't work.
	TOCMD_BUTTON,
	TOCLIP_BUTTON,
	CMD_BOX,
	HELP_BUTTON,
	
	INPUT_FILE_BOX,					// Item 7
	
	DFM_PATH_POPUP_TITLE,
	DFM_PATH_POPUP,
	DFM_FILE_BUTTON,
	DFM_FILE_NAME,
	BYTE_ORDER_POPUP_TITLE,
	BYTE_ORDER_POPUP,

	DATA_PATH_POPUP_TITLE,
	DATA_PATH_POPUP,
	DATA_FILE_BUTTON,
	DATA_FILE_NAME,
	
	OUTPUT_WAVES_BOX,				// Item 18

	OVERWRITE_WAVES,
	SAMPLE_TIME_TITLE,
	SAMPLE_TIME,
	CHAR_IS_CSTRING
};


// Structures

struct DialogStorage {			// See InitDialogStorage for a discussion of this structure.
	char filePath[MAX_PATH_LEN+1];			// Room for full file specification.
	char dfmFilePath[MAX_PATH_LEN+1];
	int pointOpenFileDialog;				// Determines when we point the open file dialog at a particular directory.
	int dfmPointOpenFileDialog;
};
typedef struct DialogStorage DialogStorage;
typedef struct DialogStorage *DialogStoragePtr;

static void
SaveDialogPrefs(XOP_DIALOG_REF theDialog, DialogStorage* dsp)
{
	DFMLoadInfoHandle dfmHandle;
	int itemNumber;
	char temp[256];
	double sampleTime=0.0;

	/* save dialog settings for next time */
	dfmHandle = (DFMLoadInfoHandle)NewHandle((long)sizeof(DFMLoadInfo));
	if (dfmHandle != NULL) {
		MemClear((char *)(*dfmHandle), (long)sizeof(DFMLoadInfo));
		(*dfmHandle)->version = DFMLoadInfo_VERSION;

		GetPopMenu(theDialog, DFM_PATH_POPUP, &itemNumber, temp);
		if (strlen(temp) < sizeof((*dfmHandle)->dfmSymbolicPathName))
			strcpy((*dfmHandle)->dfmSymbolicPathName, temp);
		
		GetPopMenu(theDialog, DATA_PATH_POPUP, &itemNumber, temp);
		if (strlen(temp) < sizeof((*dfmHandle)->symbolicPathName))
			strcpy((*dfmHandle)->symbolicPathName, temp);
		
		GetPopMenu(theDialog, BYTE_ORDER_POPUP, &itemNumber, temp);
		(*dfmHandle)->bytesSwapped = itemNumber > 1;
		
		(*dfmHandle)->overwrite = GetCheckBox(theDialog, OVERWRITE_WAVES);
		(*dfmHandle)->char_is_cstring = GetCheckBox(theDialog, CHAR_IS_CSTRING);

		if (GetDDouble(theDialog,SAMPLE_TIME,&sampleTime)==0) {
			(*dfmHandle)->sampleTime = sampleTime;
		} else {
			(*dfmHandle)->sampleTime = 0.0;
		}
		
		
		SaveXOPPrefsHandle((Handle)dfmHandle);		// Does nothing prior to IGOR Pro 3.1.
		DisposeHandle((Handle)dfmHandle);
	}
}

static void
GetDialogPrefs(XOP_DIALOG_REF theDialog, DialogStorage* dsp)
{
	DFMLoadInfoHandle dfmHandle;
	char temp[256];

	GetXOPPrefsHandle((Handle*)&dfmHandle);			// Returns null handle prior to IGOR Pro 3.1.

	if (dfmHandle && (*dfmHandle)->version==DFMLoadInfo_VERSION) {
		
		strcpy(temp, (*dfmHandle)->dfmSymbolicPathName);
		SetPopMatch(theDialog, DFM_PATH_POPUP, temp);
		
		strcpy(temp, (*dfmHandle)->symbolicPathName);
		SetPopMatch(theDialog, DATA_PATH_POPUP, temp);
		
		// (*dfmHandle)->filterStr) is no longer used.
				
		SetPopItem(theDialog, BYTE_ORDER_POPUP, (*dfmHandle)->bytesSwapped ? 2:1);
		
		SetCheckBox(theDialog, OVERWRITE_WAVES, (int)(*dfmHandle)->overwrite);
		SetCheckBox(theDialog, CHAR_IS_CSTRING, (int) (*dfmHandle)->char_is_cstring);
		SetDDouble(theDialog,SAMPLE_TIME ,&(*dfmHandle)->sampleTime);
		
		
	}
	
	if (dfmHandle != NULL)
		DisposeHandle((Handle)dfmHandle);
}

/*	InitDialogStorage(dsp)

	We use a DialogStorage structure to store working values during the dialog.
	In a Macintosh application, the fields in this structure could be local variables
	in the main dialog routine. However, in a Windows application, they would have
	to be globals. By using a structure like this, we are able to avoid using globals.
	Also, routines that access these fields (such as this one) can be used for
	both platforms.
*/
static int
InitDialogStorage(DialogStorage* dsp)
{
	MemClear(dsp, sizeof(DialogStorage));
	return 0;
}

static void
DisposeDialogStorage(DialogStorage* dsp)
{
	// Does nothing for this dialog.
}

static int
InitDialogPopups(XOP_DIALOG_REF theDialog)
{
	int err;

	err = 0;
	do {
		if (err = CreatePopMenu(theDialog, DFM_PATH_POPUP, DFM_PATH_POPUP_TITLE, "_none_", 1))
			break;
		FillPathPopMenu(theDialog, DFM_PATH_POPUP, "*", "", 1);
		
		if (err = CreatePopMenu(theDialog, DATA_PATH_POPUP, DATA_PATH_POPUP_TITLE, "_none_", 1))
			break;
		FillPathPopMenu(theDialog, DATA_PATH_POPUP, "*", "", 1);
		
		
		if (err = CreatePopMenu(theDialog, BYTE_ORDER_POPUP, BYTE_ORDER_POPUP_TITLE, "High byte first (Motoroloa);Low byte first (Intel)", 1))
			break;
	} while(0);
	
	return err;
}

/*	UpdateDialogItems(theDialog, dsp)

	Shows, hides, enables, disables dialog items as necessary.
*/
static void
UpdateDialogItems(XOP_DIALOG_REF theDialog, DialogStorage* dsp)
{
	
}

/*	InitDialogSettings(theDialog, dsp)

	Called when the dialog is first displayed to initialize all items.
*/
static int
InitDialogSettings(XOP_DIALOG_REF theDialog, DialogStorage* dsp)
{
	int err;
	double sampleTime=0.0;

	err = 0;
	
	InitPopMenus(theDialog);
	
	do {
		if (err = InitDialogPopups(theDialog))
			break;
		
		SetDText(theDialog, DATA_FILE_NAME, "");
		SetDText(theDialog, DFM_FILE_NAME, "");
	
		SetCheckBox(theDialog, OVERWRITE_WAVES, 0);
		SetCheckBox(theDialog, CHAR_IS_CSTRING, 0);
		SetDDouble(theDialog,SAMPLE_TIME,&sampleTime);
		SetPopItem(theDialog, BYTE_ORDER_POPUP, 1);
		GetDialogPrefs(theDialog, dsp);
		
		UpdateDialogItems(theDialog, dsp);
	} while(0);
	
	if (err)
		KillPopMenus(theDialog);
	
	return err;
}

static void
ShutdownDialogSettings(XOP_DIALOG_REF theDialog, DialogStorage* dsp)
{
	SaveDialogPrefs(theDialog, dsp);
	KillPopMenus(theDialog);
}

/*	SymbolicPathPointsToFolder(symbolicPathName, folderPath)

	symbolicPathName is the name of an Igor symbolic path.

	folderPath is a full path to a folder. This is a native path with trailing
	colon (Mac) or backslash (Win).
	
	Returns true if the symbolic path refers to the folder specified by folderPath.
*/
static int
SymbolicPathPointsToFolder(const char* symbolicPathName, const char* folderPath)
{
	char symbolicPathDirPath[MAX_PATH_LEN+1];	// This is a native path with trailing colon (Mac) or backslash (Win).
	
	if (GetPathInfo2(symbolicPathName, symbolicPathDirPath))
		return 0;		// Error getting path info.
	
	return CmpStr(folderPath, symbolicPathDirPath) == 0;
}

static int
DataTypeItemNumberToDataTypeCode(dataTypeItemNumber)
{
	int result;
	
	switch(dataTypeItemNumber) {
		case 1:
			result = NT_FP64;
			break;
		case 2:
			result = NT_FP32;
			break;
		case 3:
			result = NT_I32;
			break;
		case 4:
			result = NT_I16;
			break;
		case 5:
			result = NT_I8;
			break;
		case 6:
			result = NT_I32 | NT_UNSIGNED;
			break;
		case 7:
			result = NT_I16 | NT_UNSIGNED;
			break;
		case 8:
			result = NT_I8 | NT_UNSIGNED;
			break;
		default:
			result = NT_FP64;
			break;	
	}
	return result;
}

/*	GetCmd(theDialog, dsp, cmd)

	Generates a DFMLoadWave command based on the dialog's items.
	
	Returns 0 if the command is OK or non-zero if it is not valid.
	Returns -1 if cmd contains error message that should be shown in dialog.
*/
static int
GetCmd(XOP_DIALOG_REF theDialog, DialogStorage* dsp, char cmd[MAXCMDLEN+1])
{
	char temp[256+2];
	char fullFilePath[MAX_PATH_LEN+1];
	char dfmFullFilePath[MAX_PATH_LEN+1];

	char fileDirPath[MAX_PATH_LEN+1];
	char dfmFileDirPath[MAX_PATH_LEN+1];

	char fileName[MAX_FILENAME_LEN+1];
	char dfmFileName[MAX_FILENAME_LEN+1];

	int pathItemNumber, useSymbolicPath;
	int dfmPathItemNumber, dfmUseSymbolicPath;

	char symbolicPathName[MAX_OBJ_NAME+1];
	char dfmSymbolicPathName[MAX_OBJ_NAME+1];

	int overwrite;
	int byteOrderItemNumber;

	double sampleTime;
	char sampleTimeStr[32];

	*cmd = 0;
	
	if (*dsp->filePath == 0 || *dsp->dfmFilePath == 0)
		return 1;
	
	strcpy(fullFilePath, dsp->filePath);
	if (GetDirectoryAndFileNameFromFullPath(fullFilePath, fileDirPath, fileName) != 0) {
		sprintf(cmd, "*** GetDirectoryAndFileNameFromFullPath Error ***");
		return -1;		// Should never happen.
	}
	strcpy(dfmFullFilePath, dsp->dfmFilePath);
	if (GetDirectoryAndFileNameFromFullPath(dfmFullFilePath, dfmFileDirPath, dfmFileName) != 0) {
		sprintf(cmd, "*** GetDirectoryAndFileNameFromFullPath Error ***");
		return -1;		// Should never happen.
	}
	strcpy(cmd, "DFMLoadWave");

	/*	We use a symbolic path if the user has chosen one and if it points to the
		folder containing the file that the user selected.
	*/
	useSymbolicPath = 0;
	GetPopMenu(theDialog, DATA_PATH_POPUP, &pathItemNumber, symbolicPathName);
	if (pathItemNumber > 1) {					// Item 1 is "_none_".
		if (SymbolicPathPointsToFolder(symbolicPathName, fileDirPath)) {
			strcat(cmd, "/P=");
			strcat(cmd, symbolicPathName);
			useSymbolicPath = 1;
		}
	}

	dfmUseSymbolicPath = 0;
	GetPopMenu(theDialog, DFM_PATH_POPUP, &dfmPathItemNumber, dfmSymbolicPathName);
	if (dfmPathItemNumber > 1) {					// Item 1 is "_none_".
		if (SymbolicPathPointsToFolder(dfmSymbolicPathName, dfmFileDirPath)) {
			strcat(cmd, "/DFMP=");
			strcat(cmd, dfmSymbolicPathName);
			dfmUseSymbolicPath = 1;
		}
	}

	{
		char *dfmp;
		
		if (dfmUseSymbolicPath) {
			dfmp = dfmFileName;
		} else {
			WinToMacPath(dfmFullFilePath);
			dfmp=dfmFullFilePath;
		}

		strcat(cmd,"/DFM=\"");
		strcat(cmd,dfmp);
		strcat(cmd,"\"");
	}
	
	if (GetDDouble(theDialog, SAMPLE_TIME, &sampleTime) != 0) {
		sampleTime=0.0;
	}
	sprintf(sampleTimeStr,"%g",sampleTime);
	strcat(cmd,"/ST=");
	strcat(cmd,sampleTimeStr);

	if (overwrite = GetCheckBox(theDialog, OVERWRITE_WAVES))
		strcat(cmd, "/O");
	
	if (GetCheckBox(theDialog, CHAR_IS_CSTRING))
		strcat(cmd,"/CSTR");
	
	GetPopMenu(theDialog, BYTE_ORDER_POPUP, &byteOrderItemNumber, temp);
	if (byteOrderItemNumber > 1) {					// Low byte first?
		strcat(cmd, "/B=1");
	} else {
		strcat(cmd, "/B=0");
	}
	
	// Add file name or full path to the command.
	{
		char* p1;

		if (useSymbolicPath) {
			p1 = fileName;
		}
		else {
			/*	In generating Igor commands, we always use Macintosh-style paths,
				even when running on Windows. This avoids a complication presented
				by the fact that backslash is an escape character in Igor.
			*/
			WinToMacPath(fullFilePath);
			p1 = fullFilePath;
		}
	
		if (strlen(cmd) + strlen(p1) > MAXCMDLEN) {	/* command too long ? */
			strcpy(cmd, "*** Command is too long to fit in command buffer ***");
			return -1;
		}
		
		strcat(cmd, " \"");
		strcat(cmd, p1);
		strcat(cmd, "\"");
	}
	
	return(0);
}

/*	ShowCmd(theDialog, dsp, cmd)

	Displays DFMLoadWave cmd in dialog cmd box.
	Returns 0 if cmd is OK, non-zero otherwise.
	
	Also, enables or disables buttons based on cmd being OK or not.
*/
static int
ShowCmd(XOP_DIALOG_REF theDialog, DialogStorage* dsp, char cmd[MAXCMDLEN+1])
{
	int result, enable;

	result = GetCmd(theDialog, dsp, cmd);
	if (result) {
		*cmd = 0;
		enable = 0;
	}
	else {
		enable = 1;
	}
	
	HiliteDControl(theDialog, DOIT_BUTTON, enable);
	HiliteDControl(theDialog, TOCLIP_BUTTON, enable);
	HiliteDControl(theDialog, TOCMD_BUTTON, enable);
	
	DisplayDialogCmd(theDialog, CMD_BOX, cmd);

	return result;
}

/*	GetFileToLoad(theDialog, pointOpenFileDialog, fullFilePath)

	Displays open file dialog. Returns 0 if the user clicks Open or -1 if
	the user cancels.
	
	If the user clicks Open, returns via fullFilePath the full native path
	to the file to be loaded.
	
	If pointOpenFileDialog is true then the open file dialog will initially
	display the directory associated with the symbolic path selected in the
	path popup menu.
*/
static int
GetFileToLoad(XOP_DIALOG_REF theDialog, int pointOpenFileDialog, char fullFilePath[MAX_PATH_LEN+1])
{
	char symbolicPathName[MAX_OBJ_NAME+1];
	char initialDir[MAX_PATH_LEN+1];
	int pathItemNumber;
	int result;
	
	// Assume that the last path shown in the open file dialog is what we want to show.
	*initialDir = 0;
	
	// If the user just selected a symbolic path, show the corresponding folder in the open file dialog.
	if (pointOpenFileDialog) {
		GetPopMenu(theDialog, DATA_PATH_POPUP, &pathItemNumber, symbolicPathName);
		if (pathItemNumber > 1)					// Item 1 is "_none_".
			GetPathInfo2(symbolicPathName, initialDir);
	}
	
	// Display the open file dialog.
	result = XOPOpenFileDialog("Looking for a dfm binary file", "", NULL, initialDir, fullFilePath);

	#ifdef WINIGOR				// We need to reactivate the parent dialog.
		SetWindowPos(theDialog, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	#endif
	
	// Set the dialog FILE_NAME item.
	if (result == 0) {
		char fileDirPath[MAX_PATH_LEN+1];
		char fileName[MAX_FILENAME_LEN+1];
		
		if (GetDirectoryAndFileNameFromFullPath(fullFilePath, fileDirPath, fileName) != 0)
			strcpy(fileName, "***Error***");				// Should never happen.

		SetDText(theDialog, DATA_FILE_NAME, fileName);
	}
	
	return result;
}
/*	GetFileToLoad(theDialog, pointOpenFileDialog, fullFilePath)

	Displays open file dialog. Returns 0 if the user clicks Open or -1 if
	the user cancels.
	
	If the user clicks Open, returns via fullFilePath the full native path
	to the file to be loaded.
	
	If pointOpenFileDialog is true then the open file dialog will initially
	display the directory associated with the symbolic path selected in the
	path popup menu.
*/
static int
GetDFMFileToLoad(XOP_DIALOG_REF theDialog, int pointOpenFileDialog, char fullFilePath[MAX_PATH_LEN+1])
{
	char symbolicPathName[MAX_OBJ_NAME+1];
	char initialDir[MAX_PATH_LEN+1];
	int pathItemNumber;
	int result;
	
	// Assume that the last path shown in the open file dialog is what we want to show.
	*initialDir = 0;
	
	// If the user just selected a symbolic path, show the corresponding folder in the open file dialog.
	if (pointOpenFileDialog) {
		GetPopMenu(theDialog, DFM_PATH_POPUP, &pathItemNumber, symbolicPathName);
		if (pathItemNumber > 1)					// Item 1 is "_none_".
			GetPathInfo2(symbolicPathName, initialDir);
	}
	
	// Display the open file dialog.
	{
		#ifdef MACIGOR
		char filterStr[]="Text Files:TEXT:.txt,.dfm;All Files:****:;";
		#endif
		#ifdef WINIGOR
		char filterStr[]="DFM Files (*.dfm)\0*.dfm\0Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0";
		#endif
		result = XOPOpenFileDialog("Looking for a dfm file", filterStr, NULL, initialDir, fullFilePath);
	}
	#ifdef WINIGOR				// We need to reactivate the parent dialog.
		SetWindowPos(theDialog, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	#endif
	
	// Set the dialog FILE_NAME item.
	if (result == 0) {
		char fileDirPath[MAX_PATH_LEN+1];
		char fileName[MAX_FILENAME_LEN+1];
		
		if (GetDirectoryAndFileNameFromFullPath(fullFilePath, fileDirPath, fileName) != 0)
			strcpy(fileName, "***Error***");				// Should never happen.

		SetDText(theDialog, DFM_FILE_NAME, fileName);
	}
	
	return result;
}
/*	HandleItemHit(theDialog, itemID, dsp)
	
	Called when the item identified by itemID is hit.
	Carries out any actions necessitated by the hit.
*/
static void
HandleItemHit(XOP_DIALOG_REF theDialog, int itemID, DialogStorage* dsp)
{
	int selItem;
	
	if (ItemIsPopMenu(theDialog, itemID))
		GetPopMenu(theDialog, itemID, &selItem, NULL);
	
	switch(itemID) {
		case BYTE_ORDER_POPUP:
			// Nothing more needs to be done for these popup.
			break;

		case DFM_FILE_BUTTON:
			GetDFMFileToLoad(theDialog, dsp->dfmPointOpenFileDialog, dsp->dfmFilePath);
			dsp->pointOpenFileDialog = 0;
			break;

		case DFM_PATH_POPUP:
			dsp->dfmPointOpenFileDialog = 1;		// Flag that we want to point open file dialog at a particular directory.
			break;

		case DATA_FILE_BUTTON:
			GetFileToLoad(theDialog, dsp->pointOpenFileDialog, dsp->filePath);
			dsp->pointOpenFileDialog = 0;
			break;

		case DATA_PATH_POPUP:
			dsp->pointOpenFileDialog = 1;		// Flag that we want to point open file dialog at a particular directory.
			break;

		case OVERWRITE_WAVES:
			ToggleCheckBox(theDialog, itemID);
			break;
			
		case CHAR_IS_CSTRING:
			ToggleCheckBox(theDialog, itemID);
		break;
				
		case HELP_BUTTON:
			// This does nothing prior to Igor Pro 3.13B03.
			XOPDisplayHelpTopic("DFMLoadWave Help", "DFMLoadWave XOP[The Load DFM Binary Dialog]", 1);
			break;

		case DOIT_BUTTON:
		case TOCMD_BUTTON:
		case TOCLIP_BUTTON:
			{
				char cmd[MAXCMDLEN+1];
				
				GetCmd(theDialog, dsp, cmd);
				switch(itemID) {
					case DOIT_BUTTON:
						FinishDialogCmd(cmd, 1);
						break;
					case TOCMD_BUTTON:
						FinishDialogCmd(cmd, 2);
						break;
					case TOCLIP_BUTTON:
						FinishDialogCmd(cmd, 3);
						break;
				}
			}
			break;
	}

	UpdateDialogItems(theDialog, dsp);
}

#ifdef MACIGOR		// Macintosh-specific code [

/*	DFMLoadWaveDialog()

	DFMLoadWaveDialog is called when user selects 'Load General Binary...'
	from 'Load Waves' submenu.
	
	Returns 0 if OK, -1 for cancel or an error code.
*/

int
DFMLoadWaveDialog(void)
{
	DialogStorage ds;
	DialogPtr theDialog;
	GrafPtr savePort;
	short itemHit;
	char cmd[MAXCMDLEN+1];
	int err;
	
	if (err = InitDialogStorage(&ds))
		return -1;

	theDialog = GetXOPDialog(DIALOG_TEMPLATE_ID);
	savePort = SetDialogPort(theDialog);
	SetDialogBalloonHelpID(DIALOG_TEMPLATE_ID);					// NOP on Windows. Now also a NOP under Carbon.

	// As of Carbon 1.3, this call messes up the activation state of the Do It button. This appears to be a Mac OS bug.
	SetDialogDefaultItem(theDialog, DOIT_BUTTON);
	SetDialogCancelItem(theDialog, CANCEL_BUTTON);
	SetDialogTracksCursor(theDialog, 1);
	
	if (err = InitDialogSettings(theDialog, &ds)) {
		DisposeDialogStorage(&ds);
		DisposeXOPDialog(theDialog);
		SetPort(savePort);
		return err;
	}

	ShowCmd(theDialog, &ds, cmd);
	SelEditItem(theDialog, SAMPLE_TIME);
	
	ShowDialogWindow(theDialog);

	do {
		DoXOPDialog(&itemHit);
		switch(itemHit) {
			default:
				HandleItemHit(theDialog, itemHit, &ds);
				break;
		}
		ShowCmd(theDialog, &ds, cmd);
	} while (itemHit<DOIT_BUTTON || itemHit>TOCLIP_BUTTON);
	
	ShutdownDialogSettings(theDialog, &ds);

	DisposeDialogStorage(&ds);
	
	DisposeXOPDialog(theDialog);
	SetPort(savePort);

	SetDialogBalloonHelpID(-1);										/* reset resource ID for balloons */

	return itemHit==CANCEL_BUTTON ? -1 : 0;
}

#endif					// Macintosh-specific code ]


#ifdef WINIGOR		// Windows-specific code [

static BOOL CALLBACK
DialogProc(HWND theDialog, UINT msgCode, WPARAM wParam, LPARAM lParam)
{
	char cmd[MAXCMDLEN+1];
	int itemID, notificationMessage;
	BOOL result; 						// Function result

	static DialogStoragePtr dsp;

	result = FALSE;
	itemID = LOWORD(wParam);						// Item, control, or accelerator identifier.
	notificationMessage = HIWORD(wParam);
	
	switch(msgCode) {
		case WM_INITDIALOG:
			PositionWinDialogWindow(theDialog, NULL);		// Position nicely relative to Igor MDI client.
			
			dsp = (DialogStoragePtr)lParam;
			if (InitDialogSettings(theDialog, dsp) != 0) {
				EndDialog(theDialog, IDCANCEL);				// Should never happen.
				return FALSE;
			}
			
			ShowCmd(theDialog, dsp, cmd);

			SetFocus(GetDlgItem(theDialog, DATA_PATH_POPUP));
			result = FALSE; // Tell Windows not to set the input focus			
			break;
		
		case WM_COMMAND:
			switch(itemID) {
				case DOIT_BUTTON:
				case TOCMD_BUTTON:
				case TOCLIP_BUTTON:
				case CANCEL_BUTTON:
					HandleItemHit(theDialog, itemID, dsp);
					ShutdownDialogSettings(theDialog, dsp);
					EndDialog(theDialog, itemID);
					result = TRUE;
					break;				
				
				default:
					if (!IsWinDialogItemHitMessage(theDialog, itemID, notificationMessage))
						break;					// This is not a message that we need to handle.
					HandleItemHit(theDialog, itemID, dsp);
					ShowCmd(theDialog, dsp, cmd);
					break;
			}
			break;
	}
	return result;
}

/*	DFMLoadWaveDialog()

	DFMLoadWaveDialog is called when user selects 'Load General Binary...'
	from 'Load Waves' submenu.
	
	Returns 0 if OK, -1 for cancel or an error code.
*/
int
DFMLoadWaveDialog(void)
{
	DialogStorage ds;
	int result;
	
	if (result = InitDialogStorage(&ds))
		return result;

	result = DialogBoxParam(XOPModule(), MAKEINTRESOURCE(DIALOG_TEMPLATE_ID), IgorClientHWND(), DialogProc, (LPARAM)&ds);

	DisposeDialogStorage(&ds);

	if (result != CANCEL_BUTTON)
		return 0;
	return -1;					// Cancel.
}

#endif					// Windows-specific code ]
