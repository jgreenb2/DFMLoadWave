/*	This file contains routines that are Macintosh-specific.
	This file is used only when compiling for Macintosh.
*/

#include "XOPStandardHeaders.h"			// Include ANSI headers, Mac headers, IgorXOP.h, XOP.h and XOPSupport.h

// IsMacOSX has been removed. Use #ifdef MACIGOR.

void
debugstr(const char* message)			// Sends debug message to low level debugger (e.g., Macsbug).
{
	char ctemp[256];
	unsigned char ptemp[256];
	int len;
	
	len = strlen(message);
	if (len >= sizeof(ctemp))
		len = sizeof(ctemp)-1;
	strncpy(ctemp, message, len);
	ctemp[len] = 0;
	CopyCStringToPascal(ctemp, ptemp);
	DebugStr(ptemp);
}

/*	paramtext(param0, param1, param2, param3)
	
	Thread Safety: paramtext is not thread-safe.
*/
void
paramtext(const char* param0, const char* param1, const char* param2, const char* param3)
{
	unsigned char p0[256];
	unsigned char p1[256];
	unsigned char p2[256];
	unsigned char p3[256];

	CopyCStringToPascal(param0, p0);
	CopyCStringToPascal(param1, p1);
	CopyCStringToPascal(param2, p2);
	CopyCStringToPascal(param3, p3);
	ParamText(p0, p1, p2, p3);
}

/*	Resource Routines -- for dealing with resources

	These routines are for accessing Macintosh resource forks.
	There are no Windows equivalent routines, so these routines can't be used in
	Windows or cross-platform XOPs.
*/

/*	XOPRefNum()

	Returns XOP's resource file reference number.
	
	Thread Safety: XOPRefNum is thread-safe but there is little that you can do with it that is thread-safe.
*/
int
XOPRefNum(void)
{
	return((*(*XOPRecHandle)->stuffHandle)->xopRefNum);
}

/*	GetXOPResource(resType, resID)

	Tries to get specified handle from XOP's resource fork.
	Does not search any other resource forks and does not change curResFile.
	
	Thread Safety: GetXOPResource is not thread-safe.
*/
Handle
GetXOPResource(int resType, int resID)
{
	Handle rHandle;
	int curResNum;
	
	curResNum = CurResFile();
	UseResFile(XOPRefNum());
	rHandle = Get1Resource(resType, resID);
	UseResFile(curResNum);
	return(rHandle);
}

/*	GetXOPNamedResource(resType, name)

	Tries to get specified handle from XOP's resource fork.
	Does not search any other resource forks and does not change curResFile.
	
	Thread Safety: GetXOPNamedResource is not thread-safe.
*/
Handle
GetXOPNamedResource(int resType, const char* name)
{
	Handle rHandle;
	unsigned char pName[256];
	int curResNum;
	
	curResNum = CurResFile();
	UseResFile(XOPRefNum());
	CopyCStringToPascal(name, pName);
	rHandle = Get1NamedResource(resType, pName);
	UseResFile(curResNum);
	return rHandle;
}

#ifndef NO_MAC_WINDOW_SUPPORT		// [ These routines are not supported in Xcode 6

/*	The rest of the routines in this file are not supported in Xcode 6 because
	they rely on the old Macintosh Dialog Manager which is not supported by recent Apple SDK's.
	If you need to implement a dialog using these XOP Toolkit routines, drop back to Xcode 4.
	Alternatively you can create Cocoa-based dialogs as illustrated in use XOP Toolkit 7.
*/


/*	Window Routines

	These are Macintosh window routines that have no Windows equivalent.
*/

/*	GetXOPWindow(windowID, wStorage, behind)

	This routine is for Macintosh only.

	Creates a window using the WIND resource with resource ID=windowID.
	This resource must be in the XOPs resource fork.
	wStorage and behind work as for the toolbox trap GetNewWindow.
	
	Returns a window pointer if everything OK or NULL if couldn't get window.
	Sets the windowKind field of the window equal to the XOPs refNum.
	YOU MUST NOT CHANGE THE windowKind FIELD.
	
	HR, 980317: Changed GetNewWindow to GetNewCWindow.
	
	Thread Safety: GetXOPWindow is not thread-safe.
*/
WindowPtr
GetXOPWindow(int windowID, char* wStorage, WindowPtr behind)
{
	int curResNum;
	Handle wHandle;
	WindowPtr wPtr = NULL;
	
	curResNum = CurResFile();
	UseResFile(XOPRefNum());
	wHandle = Get1Resource('WIND', windowID);	/* make sure resource exists in XOP file */
	if (wHandle) {
		wPtr = GetNewCWindow(windowID, wStorage, behind);
		if (wPtr)
			SetWindowKind(wPtr, XOPRefNum());
		ReleaseResource(wHandle);
	}
	UseResFile(curResNum);
	return(wPtr);
}

#endif	// NO_MAC_WINDOW_SUPPORT ]
