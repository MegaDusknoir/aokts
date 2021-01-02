/*
	AOK Trigger Studio / SWGB Trigger Studio
	by David Tombs (aka DiGiT): cyan.spam@gmail.com
	WINDOWS VERSION.

	-------------------------------------------------------------------------------

	Copyright (C) 2007 David Tombs

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

	----------------------------------------------------------------------------

	aokts.cpp -- Defines the core functions of AOKTS

	VIEW/CONTROLLER
*/

#include "../util/utilio.h"
#include "../util/zlibfile.h"
#include "../util/settings.h"
#include "../model/scen.h"
#include "../model/TrigScrawlVisitor.h"
#include "../model/TrigXmlVisitor.h"
#include "../model/TrigXmlReader.h"
#include "editors.h"
#include "mapview.h"
#include "../util/winugly.h"
#include "utilui.h"
#include "../util/MemBuffer.h"

#include <commdlg.h>
#include "LCombo.h"
#include "../res/resource.h" // must be included after Windows stuff
#include <ctype.h>
#include <fstream>
#include <sstream>
#include "Shlwapi.h"

/* Microsoft-specific stuff */
#ifdef _MSC_VER
#include <crtdbg.h>
#endif

/** External Globals (see respective headers for descriptions) **/

Scenario scen;
PropSheetData propdata;
Setts setts;
ESDATA esdata;

/** Internal Globals **/

HINSTANCE aokts;

/*
	The system-defined DialogProc for property sheets will
	be stored here when it is replaced with SetWindowLongPtr().
	The replacement DialogProc will call this for any messages
	it does not process.
*/
DLGPROC pproc;

/*
	These are the IDs of the standard Property Sheet buttons.
	They are disabled and hidden since AOKTS uses essentially
	a hack of a Property Sheet. (The menu covers these
	functions, instead.)
*/
const WORD PropSheetButtons[] =
{ IDOK, IDCANCEL, IDHELP };

/* Each editor's property page proc (in order of Dialog ID). */
DLGPROC procs[NUM_PAGES] =
{
	&PlayersDlgProc,
	&IMsgsDlgProc,
	&PlayerDlgProc,
	&AIDlgProc,
	&CTYDlgProc,
	&VictDlgProc,
	&DisDlgProc,
	&MapDlgProc,
	&UnitDlgProc,
	&TrigDlgProc,
	&TrigtextDlgProc,
	&CampaignDlgProc
};

/* Strings */
const char * askSaveChanges =
"是否保存更改？";

const char *szTitle = "触发工作室";
const char welcome[] =
"欢迎使用AOKTS！请打开一个场景或创建一个新场景。";
const char extOpen[] =
"帝国时代 2 场景 (*.scn, *.scx, *.scx2, *.aoe2scenario)\0*.scn;*.scx;*.scx2;*.aoe2scenario\0星球大战场景 (*.scx, *.sc1)\0*.scx;*.sc1\0所有文件 (*.*)\0*.*\0";
const char extSave[] =
"国王时代场景 (*.scn)\0*.scn\0征服者 1.0c 场景 (*.scx)\0*.scx\0征服者 Userpatch 1.5RC 场景 (*.scx)\0*.scx\0征服者 ETP 1.7 场景 (*.scx)\0*.scx\0帝国时代 HD 场景 (*.scx)\0*.scx\0失落帝国场景 (*.scx2)\0*.scx2\0非洲王朝场景 (*.aoe2scenario)\0*.aoe2scenario\0星球大战场景 (*.scx)\0*.scx\0克隆战争场景 (*.sc1)\0*.sc1\0帝国时代 HD 4.3 (2F) (*.scx)\0*.scx\0失落帝国 4.3 (2F) (*.scx2)\0*.scx2\0所有文件 (*.*)\0*.*\0";
const char datapath_aok[] = "data_aok.xml";
const char datapath_wk[] = "data_wk.xml";
const char datapath_swgb[] = "data_swgb.xml";

/** Functions **/

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")

const char *getFilenameFromPath(const char *path)
{
	const char *ret;

	ret = strrchr(path, '\\') + 1;
	if (!ret)
	{
		ret = strrchr(path, '/') + 1;

		if (!ret)
			ret = path;
	}

	return ret;
}

/*
	SetSaveState: Sets state of Save/Save As buttons on the File menu.

	HWND window: Handle to the window that owns the menu.
	UINT state: The new state for each.
*/
inline void SetSaveState(HWND window, UINT state)
{
	HMENU menu = GetMenu(window);
	EnableMenuItem(menu, ID_FILE_REOPEN, state);
	EnableMenuItem(menu, ID_TS_FILE_SAVE, state);
	EnableMenuItem(menu, ID_TS_FILE_SAVE_AS, state);
	EnableMenuItem(menu, ID_TS_FILE_SAVE_AS2, state);
	EnableMenuItem(menu, ID_TS_FILE_CLOSE, state);
}

/*
	UpdateRecentMenu: Updates both the state and the text of the recent file menu items.
*/
void UpdateRecentMenu(HMENU menu)
{
	RecentFile *rfile;
	WORD item;
	MENUITEMINFO menuitem;
	char buffer[_MAX_FNAME];

	rfile = setts.recent_first;
	item = 0;

	menuitem.cbSize = sizeof(MENUITEMINFO);
	menuitem.fMask = MIIM_TYPE | MIIM_STATE;	//can't use MIIM_STRING, too new
	menuitem.fType = MFT_STRING;	//must set this with MIIM_TYPE.
	menuitem.dwTypeData = buffer;
	menuitem.fState = MFS_ENABLED;

	while (rfile && item < MAX_RECENT)
	{
		sprintf(buffer, "&%d %s", item + 1, rfile->display);
		SetMenuItemInfo(menu, ID_FILE_RECENT1 + item++, FALSE, &menuitem);
		rfile = rfile->next;
	}
}

/*
	FileSave: Handles a request to save the currently open file.

	Parameters:
	HWND sheet: Handle to the property sheet.
	bool as:		Save or Save As?
*/
void FileSave(HWND sheet, bool as, bool write)
{
	int error;			//error value from Scenario::save()
	HWND cpage;			//the current property page
	HCURSOR previous;	//the mouse cursor before/after save operation
	OPENFILENAME ofn;
	char titleBuffer[100];
	Game startver;
	Game conv = NOCONV;
    SaveFlags::Value flags = SaveFlags::NONE;

    char w1[] = "该场景是受保护的。";
    char w2[] = "场景受保护";
    if (setts.disabletips) {
        MessageBox(sheet, w1, w2, MB_ICONWARNING);
        return;
    }

	//init
	cpage = PropSheet_GetCurrentPageHwnd(sheet);

	//Save As: Ask for filename.
	if (as || *setts.ScenPath == '\0')
	{
		char dir[_MAX_PATH];
		strcpy(dir, setts.BasePath);
		strcat(dir, "Scenario");

		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = sheet;
		ofn.lpstrFilter = extSave;
		ofn.lpstrCustomFilter = NULL;
		ofn.lpstrFile = setts.ScenPath;
		ofn.nMaxFile = _MAX_PATH;
		ofn.lpstrFileTitle = NULL;
		ofn.lpstrInitialDir = dir;
		ofn.lpstrTitle = NULL;
		ofn.Flags = OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT;

	    startver = scen.game;

		if (scen.header.header_type == HT_AOE2SCENARIO) {
		    ofn.nFilterIndex =	7;
		    ofn.lpstrDefExt =	"aoe2scenario";
		} else {
		    switch (scen.game) {
		    case AOK:
		        ofn.nFilterIndex =	1;
		        ofn.lpstrDefExt =	"scn";
		        break;
		    case AOC:
		        ofn.nFilterIndex =	2;
		        ofn.lpstrDefExt =	"scx";
		        break;
		    case UP:
		        ofn.nFilterIndex =	3;
		        ofn.lpstrDefExt =	"scx";
		        break;
		    case ETP:
		        ofn.nFilterIndex =	4;
		        ofn.lpstrDefExt =	"scx";
		        break;
		    case AOHD:
		    case AOHD4:
		        ofn.nFilterIndex =	5;
		        ofn.lpstrDefExt =	"scx";
		        break;
		    case AOF:
		    case AOF4:
		        ofn.nFilterIndex =	6;
		        ofn.lpstrDefExt =	"scx2";
		        break;
		    case SWGB:
		        ofn.nFilterIndex =	8;
		        ofn.lpstrDefExt =	"scx";
		        break;
		    case SWGBCC:
		        ofn.nFilterIndex =	9;
		        ofn.lpstrDefExt =	"sc1";
		        break;
		    case AOHD6:
		        ofn.nFilterIndex =	10;
		        ofn.lpstrDefExt =	"scx";
		        break;
		    case AOF6:
		        ofn.nFilterIndex =	11;
		        ofn.lpstrDefExt =	"scx2";
		        break;
		    }
		}

		if (!GetSaveFileName(&ofn))
			return;

		switch (ofn.nFilterIndex) {
		case 1:
		    conv = AOK;
		    break;
		case 2:
		    conv = AOC;
		    break;
		case 3:
		    conv = UP;
		    break;
		case 4:
		    conv = ETP;
		    break;
		case 5:
		    conv = AOHD4;
		    break;
		case 6:
		    conv = AOF4;
		    break;
		case 7:
	        scen.header.header_type = HT_AOE2SCENARIO;
		    conv = startver;
		    break;
		case 8:
		    conv = SWGB;
		    break;
		case 9:
		    conv = SWGBCC;
		    break;
		case 10:
		    conv = AOHD6;
		    break;
		case 11:
		    conv = AOF6;
		    break;
		}

		if (!*scen.origname)
			strcpy(scen.origname, setts.ScenPath + ofn.nFileOffset);

		/* Update window title since filename has probably changed */
		_snprintf(titleBuffer, sizeof(titleBuffer),
			"%s - %s", szTitle, setts.ScenPath + ofn.nFileOffset);
		SetWindowText(sheet, titleBuffer);
	} else {
	    conv = scen.game;
	}

    if (isHD(startver) && conv == UP) {
        if (setts.asktoconverteffects &&
            MessageBox(sheet, "同时转换 HD 效果到 UserPatch ？", "转换", MB_YESNOCANCEL) == IDYES) {
            flags = (SaveFlags::Value)(flags | SaveFlags::CONVERT_EFFECTS);
        }
    }

    if (startver == UP && isHD(conv)) {
        if (setts.asktoconverteffects &&
            MessageBox(sheet, "同时转换 UserPatch 效果到 HD ？", "转换", MB_YESNOCANCEL) == IDYES) {
            flags = (SaveFlags::Value)(flags | SaveFlags::CONVERT_EFFECTS);
        }
    }

	//update scenario data
	SendMessage(cpage, AOKTS_Saving, 0, 0);

	//perform before-saving operations
	previous = SetCursor(LoadCursor(NULL, IDC_WAIT));
	// Pretend we're "closing" the scenario because it may change during the
	// save.
	SendMessage(cpage, AOKTS_Closing, 0, 0);
	scen.clean_triggers();

	//save the scenario
	try
	{
 		error = scen.save(setts.ScenPath, setts.TempPath, write, conv, flags);
		SetCursor(previous);

	    struct RecentFile *file = NULL;	//the file info will be stored here one way or another
		struct RecentFile *r_parse;

		/* Now check if file is already on recent list. */
		r_parse = setts.recent_first;
		while (r_parse)
		{
			if (!strcmp(r_parse->path, setts.ScenPath))
			{
				file = r_parse;
				break;
			}
			r_parse = r_parse->next;
		}

        if (file) {
            file->game = (int)conv;
        } else {
		    file = setts.recent_getnext();
		    strcpy(file->path, setts.ScenPath);
		    strcpy(file->display, PathFindFileName(setts.ScenPath));
		    file->game = (int)conv;
	        setts.recent_push(file);
	    }
	    UpdateRecentMenu(propdata.menu);
	}
	catch (std::exception &ex)
	{
		// TODO: better atomic cursor handling?
		SetCursor(previous);
		MessageBox(sheet, ex.what(), "场景保存错误", MB_ICONWARNING);
	}

	//perform after-saving operations
	SendMessage(cpage, AOKTS_Loading, 0, 0);

	//report any errors
	if (error)
	{
		SetWindowText(propdata.statusbar, scen.msg);
		//MessageBox(sheet, scen.msg, "Save Error", MB_OK);
	}
}

#define MAP_OFFSET 10
HWND MakeMapView(HWND sheet, int cmdshow)
{
	HWND ret;
	RECT rect;

	GetWindowRect(sheet, &rect);
	ret = CreateMapView(sheet, rect.right + MAP_OFFSET, rect.top, &scen);
	ShowWindow(ret, cmdshow);

	return ret;
}

/*
	FileOpen: Handles a request to open a file. (Either by menu or generated by the app.)

	Parameters:
	HWND sheet: Handle to the property sheet.
	bool ask:	Should AOKTS ask the user which file?
	int recent:	Optionally open from one of the recent file entries. (-1 to disable.)
*/
void FileOpen(HWND sheet, bool ask, int recent)
{
	OPENFILENAME ofn;
	struct RecentFile *file = NULL;	//the file info will be stored here one way or another
	char titleBuffer[100];
	const char *filename;
	Game version = scen.game;

	HWND page = PropSheet_GetCurrentPageHwnd(sheet);

	//save the scenario if changes have been made (NOT FUNCTIONAL)
	if (scen.needsave())
	{
		int sel = MessageBox(sheet, askSaveChanges, "保存", MB_YESNOCANCEL);

		if (sel == IDYES)
			FileSave(sheet, false, true);

		else if (sel == IDCANCEL)
			return;	//stop closing
	}

    // Hint about whether to open as AOC or SGWB
	if (setts.recent_first) {
	     scen.game = (Game)setts.recent_first->game;
	}

	/* Using a recent file... */
	if (recent >= 0)
	{
		ofn.Flags = 0;	//make sure no random flags set
		file = setts.recent_first;

		/* Parse the linked list to find the one we want */
		while (recent--)
		{
			if (file)
				file = file->next;
			else
			{
				MessageBox(sheet,
					"警告：打开最近文件失败。",
					"打开警告", MB_ICONWARNING);
			}
		}

		strcpy(setts.ScenPath, file->path);
		version = (Game)file->game;
	}
	/* Prompt the user for a filename. */
	else if (ask)
	{
		struct RecentFile *r_parse;
		char dir[_MAX_PATH];
		strcpy(dir, setts.BasePath);
		strcat(dir, "场景");

		ofn.lStructSize =	sizeof(OPENFILENAME);
		ofn.hwndOwner =		sheet;
		//ofn.hInstance unused
		ofn.lpstrFilter =	extOpen;
		ofn.lpstrCustomFilter = NULL;	//user should not set custom filters
		ofn.lpstrFile =		setts.ScenPath;
		ofn.nMaxFile =		_MAX_PATH;
		ofn.lpstrFileTitle = NULL;
		ofn.lpstrInitialDir = dir;
		ofn.lpstrTitle =	NULL;
		ofn.Flags =			OFN_FILEMUSTEXIST | OFN_NONETWORKBUTTON | OFN_NOCHANGEDIR;

		if (scen.header.header_type == HT_AOE2SCENARIO) {
		    ofn.nFilterIndex =	1;
		    ofn.lpstrDefExt =	"aoe2scenario";
		} else {
		    switch (scen.game) {
		    case AOK:
		    case AOC:
		    case AOHD:
		    case AOF:
			case UP:
			case ETP:
		    case AOHD4:
		    case AOF4:
		    case AOHD6:
		    case AOF6:
		        ofn.nFilterIndex =	1;
		        ofn.lpstrDefExt =	"scx";
		        break;
		    case SWGB:
		    case SWGBCC:
		        ofn.nFilterIndex =	2;
		        ofn.lpstrDefExt =	"scx";
		        break;
			default:
		        ofn.nFilterIndex =	1;
		        ofn.lpstrDefExt =	"scx";
		    }
		}

		if (!GetOpenFileName(&ofn))
			return;

		switch (ofn.nFilterIndex) {
		case 1:
		    version = AOC;
		    printf_log("Selected %d, AOE 2.\n", ofn.nFilterIndex);
		    break;
		case 2:
		    version = SWGB;
		    printf_log("Selected %d, Star Wars.\n", ofn.nFilterIndex);
		    break;
		}

		/* Now check if selected file is already on recent list. */
		r_parse = setts.recent_first;
		while (r_parse)
		{
			if (!strcmp(r_parse->path, setts.ScenPath))
			{
				file = r_parse;
				break;
			}
			r_parse = r_parse->next;
		}
	}

	/* Open it! */
	SendMessage(page, AOKTS_Closing, 0, 0);
	// set hourglass, might take more than 1ms
	HCURSOR previous = SetCursor(LoadCursor(NULL, IDC_WAIT));
	scen.reset();
	try
	{
		version = scen.open(setts.ScenPath, setts.TempPath, version);

	    /* Handle recent file stuff */
	    if (!file)
	    {
		    file = setts.recent_getnext();
		    strcpy(file->path, setts.ScenPath);
		    strcpy(file->display, PathFindFileName(setts.ScenPath));
		    file->game = (int)version;
	    }
	    setts.recent_push(file);
	    UpdateRecentMenu(propdata.menu);

		SetCursor(previous);
		// for some reason this is always read only. on Wine at least
		//SetSaveState(sheet, ofn.Flags & OFN_READONLY ? MF_GRAYED : MF_ENABLED);
		SetSaveState(sheet, ofn.Flags & OFN_READONLY ? MF_ENABLED : MF_ENABLED);

		// set status bar text
		SetWindowTextW(propdata.statusbar, L"载入场景成功。");

	    SendMessage(page, AOKTS_Loading, 0, 0);
        SendMessageW(propdata.mapview, MAP_Recreate, 0, 0);
	    MapView_Reset(propdata.mapview, true);

	    filename = getFilenameFromPath(setts.ScenPath);

	    _snprintf(titleBuffer, sizeof(titleBuffer),
		        "%s - %s", szTitle, filename);

	    SetWindowText(sheet, titleBuffer);
	}
	catch (std::exception &ex)
	{
		// TODO: better atomic cursor handling?
		SetCursor(previous);

		// set status bar text
		SetWindowText(propdata.statusbar, ex.what());

		// report error to user
		std::string desc = "无法打开";

        desc.append(gameName(scen.game));
		desc.append("场景文件。\n");

		switch (scen.game) {
		case AOC:
		    desc.append("尝试作为星战 scx 场景文件打开\n");
		    break;
		case SWGB:
		    desc.append("尝试作为征服者 scx 场景文件打开\n");
		    break;
		}

		desc.append("\n问题：");
		desc.append(ex.what());

		desc.append("\n\n如果游戏能打开这个场景，请汇报这个问题。");
		printf_log("User message: %s\n", desc.c_str());
		MessageBox(sheet, desc.c_str(), "场景载入错误", MB_ICONWARNING);

		// unless it's a debug build, clear the bad data
	#ifndef _DEBUG
		scen.reset();
		SendMessage(page, AOKTS_Closing, 0, 0);
	#endif

	    /* Updates*/
	    SendMessage(page, AOKTS_Loading, 0, 0);

	    _snprintf(titleBuffer, sizeof(titleBuffer),
		        "%s", szTitle);

	    SetWindowText(sheet, titleBuffer);
	}

	//report errors to logfile
	fflush(stdout);
}

/*
	FileClose: Handles user close request.
*/
void FileClose(HWND sheet, HWND control)
{
	HWND page = (HWND)SendMessage(sheet, PSM_GETCURRENTPAGEHWND, 0, 0);
	int sel = IDYES;

	if (!control)
		SendMessage(page, AOKTS_Closing, 0, 0);

	if (scen.needsave())
	{
		sel = MessageBox(sheet, "是否保存更改？", "保存", MB_YESNOCANCEL);
		if (sel == IDYES)
			FileSave(sheet, false, true);
		else if (sel == IDCANCEL)
			return;	//stop closing
	}
	scen.reset();
	*setts.ScenPath = '\0';

	if (!control)
		SendMessage(page, AOKTS_Loading, 0, 0);

	SetSaveState(sheet, MF_ENABLED);
	SetWindowText(propdata.statusbar, "场景重置。");
	SendMessage(propdata.mapview, MAP_Reset, 0, 0);

	SetWindowText(sheet, szTitle);
}

/**
 * Handles a user request to dump triggers to textual format.
 */
void OnFileTrigWrite(HWND dialog)
{
	char path[MAX_PATH] = "trigs.xml";

	// TODO: set the path to aokts directory.
	if (!GetSaveFileNameA(dialog, path, MAX_PATH))
		return;

	AutoFile textout(path, "w");
    std::ostringstream ss;
	scen.accept(TrigXmlVisitor(ss));
	fputs(ss.str().c_str(), textout.get());
}

/**
 * Handles a user request to read triggers from above textual format.
 */
void OnFileTrigRead(HWND dialog)
{
	char path[MAX_PATH] = "";

	if (!GetOpenFileNameA(dialog, path, MAX_PATH))
		return;

	std::ifstream textin(path, std::ios_base::in);
	TrigXmlReader reader;
	reader.read(textin);
}

/*
	PropSheetProc: Handles special messages pertaining to the property sheet.

	Note: See PropSheetProc in the Platform SDK docs for parameters and notes.
*/
int CALLBACK PropSheetProc(HWND sheet, UINT msgid, LPARAM lParam)
{
	switch (msgid)
	{
	case PSCB_PRECREATE:
		{
			DLGTEMPLATE *templ = (DLGTEMPLATE*)lParam;

			templ->cy += 5;

			//add a minimize box
			templ->style |= WS_MINIMIZEBOX;
		}
		break;

	case PSCB_INITIALIZED:
		{
			HWND tooltip;
			HICON icon;

			/* Add Menu. */
			propdata.menu = LoadMenu(aokts, (LPCSTR)IDM_MAIN);
			SetMenu(sheet, propdata.menu);
			//SetSaveState(sheet, MF_GRAYED);
	        scen.reset();
	        SendMessage(PropSheet_GetCurrentPageHwnd(sheet), AOKTS_Loading, 0, 0);
	        MapView_Reset(propdata.mapview, true);

			/* Enable appropriate recent file items. */
			UpdateRecentMenu(propdata.menu);

			/* Remove unused buttons. */
			for (int i = 0; i < sizeof(PropSheetButtons) / sizeof(WORD); i++)
			{
				HWND hWnd = GetDlgItem(sheet, PropSheetButtons[i]);
				if (hWnd != NULL)
				{
					ShowWindow(hWnd, SW_HIDE);
					EnableWindow(hWnd, FALSE);
				}
			}

			/* Add a tooltip window */
			tooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, "AOKTS 提示", WS_POPUP,
				CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
				sheet, NULL, aokts, NULL);
			TooltipInit(tooltip);

			/* Set the big icon */
			icon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_LOGO));
			Window_SetIcon(sheet, ICON_BIG, icon);

            if (setts.editall) {
                CheckMenuItem(GetMenu(sheet), ID_EDIT_ALL, MF_BYCOMMAND | MF_CHECKED);
            } else {
                CheckMenuItem(GetMenu(sheet), ID_EDIT_ALL, MF_BYCOMMAND);
            }
		}
		break;
	}
	return 0;
}

/*
	MakeSheet: Creates the main property sheet window.

	Parameters:
	HINSTANCE app: Handle to the application loading the sheet.

	Note: Called once and only once by WinMain().
*/
HWND MakeSheet(HINSTANCE app)
{
	PROPSHEETHEADER header;
	HPROPSHEETPAGE pages[NUM_PAGES];
	PROPSHEETPAGE pg;	//used to create each page
	HWND sheet;

	//create pages

	pg.dwSize = sizeof(PROPSHEETPAGE);
	pg.dwFlags = PSP_DEFAULT;
	pg.hInstance = app;

	for (int i = 0; i < NUM_PAGES; i++)
	{
		pg.pszTemplate = MAKEINTRESOURCE(IDD_PLAYERS + i);	//template IDs are in display order
		pg.pfnDlgProc = procs[i];
		pg.lParam = 0;
		pages[i] = CreatePropertySheetPage(&pg);
	}

	//create sheet

	header.dwSize = sizeof(header);
	header.dwFlags = PSH_MODELESS | PSH_USECALLBACK |
		PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP | PSH_USEICONID;
	header.hwndParent = NULL;
	header.hInstance = app;
	header.pszIcon = MAKEINTRESOURCE(IDI_LOGO);
	header.pszCaption = szTitle;
	header.nPages = NUM_PAGES;
	header.nStartPage = 0;
	header.phpage = pages;

	header.pfnCallback = &PropSheetProc;

	sheet = (HWND)PropertySheet(&header);

	//add status bar here (can't be done in PropertySheetProc)

	propdata.statusbar = CreateWindow(STATUSCLASSNAME, welcome,
		WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
		sheet, (HMENU)IDS_MAIN, aokts, NULL);

	return sheet;
}

/*
	DefaultDialogProc: A simple DialogProc currently used for the About box.

	Note: See DialogProc in the Platform SDK docs for parameters and notes.
*/
INT_PTR CALLBACK DefaultDialogProc(HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		if (wParam == IDOK)
		{
			EndDialog(dialog, TRUE);
			return 0;
		}
		if (wParam == IDCANCEL)
		{
			EndDialog(dialog, FALSE);
			return 0;
		}
	}
	return 0;
}

/*
	DisplayStats: Fills out controls for statistics dialog.
*/
BOOL DisplayStats(HWND dialog)
{
	// TODO: split this into model/view

	UINT total = 0, i;
	UINT ne = 0, nc = 0;
	Trigger *t_parse;
	Player *p_parse;

	/* total enabled players */
	SetDlgItemInt(dialog, IDC_S_PLAYERS, scen.getPlayerCount(), FALSE);

	/* Units (including buildings, GAIA stuff, etc.) */
	total = 0;
	p_parse = scen.players;
	for (i = 0; i < NUM_PLAYERS; i++, p_parse++)
	{
		int count = p_parse->units.size();
		if (i <= GAIA_INDEX)
			SetDlgItemInt(dialog, IDC_S_UNITS1 + i, count, FALSE);
		total += count;
	}
	SetDlgItemInt(dialog, IDC_S_UNITS, total, FALSE);

	/* Disabled (techs only) */
	total = 0;
	p_parse = scen.players;
	for (i = 0; i < NUM_PLAYERS; i++, p_parse++)
	{
		int count = p_parse->ndis_t;
		if (i <= GAIA_INDEX)
			SetDlgItemInt(dialog, IDC_S_DISABLE1 + i, count, FALSE);
		total += count;
	}
	SetDlgItemInt(dialog, IDC_S_DISABLE, total, FALSE);

	/* total triggers */
	SetDlgItemInt(dialog, IDC_S_TRIGGERS, scen.triggers.size(), FALSE);

	/* total effects & conditions */
	total = scen.triggers.size();
	if (total > 0) {
	    t_parse = &(*scen.triggers.begin());
	    while (total--)
	    {
		    ne += t_parse->effects.size();
		    nc += t_parse->conds.size();
		    t_parse++;
	    }
	}
	SetDlgItemInt(dialog, IDC_S_CONDITIONS, nc, FALSE);
	SetDlgItemInt(dialog, IDC_S_EFFECTS, ne, FALSE);

	/* map size (why here?) */
	SetDlgItemInt(dialog, IDC_S_MAPSIZE, scen.map.x, FALSE);

	return TRUE;
}

/*
	StatsDialogProc

	Note: See DialogProc in the Platform SDK docs for parameters and notes.
*/
INT_PTR CALLBACK StatsDialogProc(HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		return DisplayStats(dialog);

	case WM_CLOSE:
		EndDialog(dialog, FALSE);
		break;

	case WM_COMMAND:
		if (wParam == IDOK)
		{
			EndDialog(dialog, TRUE);
			return 0;
		}
	}
	return 0;
}

BOOL DisplayPreference(HWND dialog,char * path)
{
	SendDlgItemMessage(dialog, IDC_P_ASKCONVERTEFFECTS, BM_SETCHECK, GetPrivateProfileInt("Advanced", "AskConvertEffects", 1, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_DISPLAYHINTS, BM_SETCHECK, GetPrivateProfileInt("Advanced", "DisplayHints", 1, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_DISABLETIPS, BM_SETCHECK, GetPrivateProfileInt("Advanced", "DisableTips", 0, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_FORCEENABLETIPS, BM_SETCHECK, GetPrivateProfileInt("Advanced", "ForceEnableTips", 0, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_SHOWTRIGGERNAMES, BM_SETCHECK, GetPrivateProfileInt("Advanced", "ShowTriggerNames", 0, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_SHOWTRIGGERFUNCTION, BM_SETCHECK, GetPrivateProfileInt("Advanced", "ShowTriggerFunction", 0, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_SHOWDISPLAYORDER, BM_SETCHECK, GetPrivateProfileInt("Advanced", "ShowDisplayOrder", 1, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_SHOWTRIGGERIDS, BM_SETCHECK, GetPrivateProfileInt("Advanced", "ShowTriggerIDs", 1, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_WKMODE, BM_SETCHECK, GetPrivateProfileInt("Advanced", "WololokingdomsMode", 0, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_EDITALL, BM_SETCHECK, GetPrivateProfileInt("Advanced", "EditAll", 0, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_NOWARNINGS, BM_SETCHECK, GetPrivateProfileInt("Advanced", "NoWarnings", 1, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_DRAWCONDS, BM_SETCHECK, GetPrivateProfileInt("Advanced", "DrawConds", 1, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_DRAWEFFECTS, BM_SETCHECK, GetPrivateProfileInt("Advanced", "DrawEffects", 1, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_DRAWLOCATIONS, BM_SETCHECK, GetPrivateProfileInt("Advanced", "DrawLocations", 1, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_DRAWTERRAIN, BM_SETCHECK, GetPrivateProfileInt("Advanced", "DrawTerrain", 1, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_DRAWELEVATION, BM_SETCHECK, GetPrivateProfileInt("Advanced", "DrawElevation", 1, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_DRAWPLAYER1, BM_SETCHECK, GetPrivateProfileInt("Advanced", "DrawPlayer1", 1, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_DRAWPLAYER2, BM_SETCHECK, GetPrivateProfileInt("Advanced", "DrawPlayer2", 1, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_DRAWPLAYER3, BM_SETCHECK, GetPrivateProfileInt("Advanced", "DrawPlayer3", 1, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_DRAWPLAYER4, BM_SETCHECK, GetPrivateProfileInt("Advanced", "DrawPlayer4", 1, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_DRAWPLAYER5, BM_SETCHECK, GetPrivateProfileInt("Advanced", "DrawPlayer5", 1, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_DRAWPLAYER6, BM_SETCHECK, GetPrivateProfileInt("Advanced", "DrawPlayer6", 1, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_DRAWPLAYER7, BM_SETCHECK, GetPrivateProfileInt("Advanced", "DrawPlayer7", 1, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_DRAWPLAYER8, BM_SETCHECK, GetPrivateProfileInt("Advanced", "DrawPlayer8", 1, path) != 0, 0);
	SendDlgItemMessage(dialog, IDC_P_DRAWGAIA, BM_SETCHECK, GetPrivateProfileInt("Advanced", "DrawGaia", 1, path) != 0, 0);

	return TRUE;
}

/* Preference Setting */
INT_PTR CALLBACK Preference(HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char path[_MAX_PATH];
    strcpy(path, global::exedir);
	strcat(path, "\\ts.ini");
	switch (msg)
	{
	case WM_INITDIALOG:
		return DisplayPreference(dialog,path);

	case WM_CLOSE:
		EndDialog(dialog, FALSE);
		break;

	case WM_COMMAND:
		switch (wParam) {
		case IDOK:
			if (SendMessage(GetDlgItem(dialog, IDC_P_ASKCONVERTEFFECTS),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "AskConvertEffects", "1", path); else WritePrivateProfileString("Advanced", "AskConvertEffects", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_DISPLAYHINTS),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "DisplayHints", "1", path); else WritePrivateProfileString("Advanced", "DisplayHints", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_DISABLETIPS),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "DisableTips", "1", path); else WritePrivateProfileString("Advanced", "DisableTips", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_FORCEENABLETIPS),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "ForceEnableTips", "1", path); else WritePrivateProfileString("Advanced", "ForceEnableTips", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_SHOWTRIGGERNAMES),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "ShowTriggerNames", "1", path); else WritePrivateProfileString("Advanced", "ShowTriggerNames", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_SHOWTRIGGERFUNCTION),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "ShowTriggerFunction", "1", path); else WritePrivateProfileString("Advanced", "ShowTriggerFunction", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_SHOWDISPLAYORDER),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "ShowDisplayOrder", "1", path); else WritePrivateProfileString("Advanced", "ShowDisplayOrder", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_SHOWTRIGGERIDS),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "ShowTriggerIDs", "1", path); else WritePrivateProfileString("Advanced", "ShowTriggerIDs", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_WKMODE),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "WololokingdomsMode", "1", path); else WritePrivateProfileString("Advanced", "WololokingdomsMode", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_EDITALL),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "EditAll", "1", path); else WritePrivateProfileString("Advanced", "EditAll", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_NOWARNINGS),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "NoWarnings", "1", path); else WritePrivateProfileString("Advanced", "NoWarnings", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_DRAWCONDS),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "DrawConds", "1", path); else WritePrivateProfileString("Advanced", "DrawConds", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_DRAWEFFECTS),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "DrawEffects", "1", path); else WritePrivateProfileString("Advanced", "DrawEffects", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_DRAWLOCATIONS),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "DrawLocations", "1", path); else WritePrivateProfileString("Advanced", "DrawLocations", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_DRAWTERRAIN),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "DrawTerrain", "1", path); else WritePrivateProfileString("Advanced", "DrawTerrain", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_DRAWELEVATION),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "DrawElevation", "1", path); else WritePrivateProfileString("Advanced", "DrawElevation", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_DRAWPLAYER1),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "DrawPlayer1", "1", path); else WritePrivateProfileString("Advanced", "DrawPlayer1", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_DRAWPLAYER2),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "DrawPlayer2", "1", path); else WritePrivateProfileString("Advanced", "DrawPlayer2", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_DRAWPLAYER3),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "DrawPlayer3", "1", path); else WritePrivateProfileString("Advanced", "DrawPlayer3", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_DRAWPLAYER4),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "DrawPlayer4", "1", path); else WritePrivateProfileString("Advanced", "DrawPlayer4", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_DRAWPLAYER5),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "DrawPlayer5", "1", path); else WritePrivateProfileString("Advanced", "DrawPlayer5", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_DRAWPLAYER6),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "DrawPlayer6", "1", path); else WritePrivateProfileString("Advanced", "DrawPlayer6", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_DRAWPLAYER7),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "DrawPlayer7", "1", path); else WritePrivateProfileString("Advanced", "DrawPlayer7", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_DRAWPLAYER8),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "DrawPlayer8", "1", path); else WritePrivateProfileString("Advanced", "DrawPlayer8", "0", path);
			if (SendMessage(GetDlgItem(dialog, IDC_P_DRAWGAIA),BM_GETCHECK,0,0) != 0) WritePrivateProfileString("Advanced", "DrawGaia", "1", path); else WritePrivateProfileString("Advanced", "DrawGaia", "0", path);
			EndDialog(dialog, TRUE);
			break;
		case IDCANCEL:
			EndDialog(dialog, FALSE);
			break;
		}
		return 0;
	}
	return 0;
}

/* OnMenuSelect */
void OnMenuSelect(WORD id, WORD flags, HMENU handle)
{
	HINSTANCE res = GetModuleHandle(NULL);
	char buffer[0x50] = "";

	LoadString(res, id, buffer, sizeof(buffer));
	SetWindowText(propdata.statusbar, buffer);
}

/* OnCompressOrDecompress */
// TODO: fix alternate cohesion crap
void OnCompressOrDecompress(HWND sheet, bool compress)
{
	int size, ret;
	char path[_MAX_PATH];

	if (!GetOpenFileNameA(sheet, path, sizeof(path)))
		return;

	size = fsize(path);
	std::vector<unsigned char> buffer(size);

	AutoFile fIn(path, "rb");
	fread(&buffer[0], sizeof(char), size, fIn.get()); // contiguous
	fIn.close();

	path[0] = '\0';   // don't pre-fill path
	if (!GetSaveFileNameA(sheet, path, sizeof(path)))
		return;

	AutoFile fOut(path, "wb");

	if (compress)
		ret = deflate_file(&buffer[0], size, fOut.get());
	else
		ret = inflate_file(&buffer[0], size, fOut.get());

	fOut.close();

	if (ret >= 0)
		MessageBox(sheet, "操作成功。",
		"无损压缩/解压缩", MB_OK);
	else
		MessageBox(sheet, "操作失败。",
		"无损压缩/解压缩", MB_ICONWARNING);
}

void SetDrawTriggerCheckboxes(HWND sheet)
{
    if (setts.drawconds && setts.draweffects && setts.drawlocations) {
		CheckMenuItem(GetMenu(sheet), ID_DRAW_TRIGGERS, MF_BYCOMMAND | MF_CHECKED);
    } else {
		CheckMenuItem(GetMenu(sheet), ID_DRAW_TRIGGERS, MF_BYCOMMAND);
	}

    if (setts.drawconds) {
		CheckMenuItem(GetMenu(sheet), ID_DRAW_CONDITIONS, MF_BYCOMMAND | MF_CHECKED);
    } else {
		CheckMenuItem(GetMenu(sheet), ID_DRAW_CONDITIONS, MF_BYCOMMAND);
	}

    if (setts.draweffects) {
		CheckMenuItem(GetMenu(sheet), ID_DRAW_EFFECTS, MF_BYCOMMAND | MF_CHECKED);
    } else {
		CheckMenuItem(GetMenu(sheet), ID_DRAW_EFFECTS, MF_BYCOMMAND);
	}

    if (setts.drawlocations) {
		CheckMenuItem(GetMenu(sheet), ID_DRAW_LOCATIONS, MF_BYCOMMAND | MF_CHECKED);
    } else {
		CheckMenuItem(GetMenu(sheet), ID_DRAW_LOCATIONS, MF_BYCOMMAND);
	}
}

/*
	Sheet_HandleCommand: Handles all commands routed to property sheet (mostly menuitem stuff).
*/
bool Sheet_HandleCommand(HWND sheet, WORD code, WORD id, HWND control)
{
	bool ret = true;
	Player *p = propdata.p;

	switch (id)
	{
	case ID_FILE_REOPEN:
		FileOpen(sheet, false, 0);
		break;

	case ID_TS_FILE_OPEN:
		FileOpen(sheet, true, -1);
		break;

	case ID_TS_FILE_NEW:
	case ID_TS_FILE_CLOSE:
		FileClose(sheet, control);
		break;

	case ID_TS_APP_EXIT:
		if (scen.needsave())
		{
			int sel = MessageBox(sheet, "是否保存更改？", "保存", MB_YESNOCANCEL);
			if (sel == IDYES)
				FileSave(sheet, false, true);
			else if (sel == IDCANCEL)
				break;	//stop closing
		}
		DestroyWindow(sheet);
		break;

	case ID_TS_FILE_SAVE:
		FileSave(sheet, false, true);
		break;

	case ID_TS_FILE_SAVE_AS:
		FileSave(sheet, true, true);
		break;

	case ID_TS_FILE_SAVE_AS2:
		FileSave(sheet, true, false);

	case ID_FILE_DUMP:
		if (!scen.exFile("dump", -1))
		{
			MessageBox(sheet, "转储失败。", "场景转储", MB_ICONWARNING);
		} else {
		    SetWindowText(propdata.statusbar, "Per 文件已转储到dump\\");
		}
		break;

	case IDC_U_CLEARAICPVC:
	    scen.clearaicpvc();
		SetWindowText(propdata.statusbar, "已移除所有 AI，城市计划和胜利条件文件");
	    SendMessage(PropSheet_GetCurrentPageHwnd(sheet), AOKTS_Loading, 0, 0);
		break;

	case IDC_U_RANDOMIZE_ROT:
	    scen.randomize_unit_frames();
		SetWindowText(propdata.statusbar, "已随机化外观 (相位和帧)");
		break;

	case ID_UNITS_TERRAIN_ELEV:
	    scen.set_unit_z_to_map_elev();
		SetWindowText(propdata.statusbar, "已设定单位 z 为地形海拔");
		break;

	case ID_UNITS_DELETE_ALL:
		scen.delete_player_units(propdata.pindex);
		SetWindowText(propdata.statusbar, "已删除所有玩家单位");
	    SendMessage(propdata.mapview, MAP_Reset, 0, 0);
		break;

	case ID_MAP_WATER_CLIFF_INVISIBLE:
		scen.water_cliffs_visibility(FALSE);
		SetWindowText(propdata.statusbar, "水域悬崖现不可视");
		break;

	case ID_MAP_WATER_CLIFF_VISIBLE:
		scen.water_cliffs_visibility(TRUE);
		SetWindowText(propdata.statusbar, "水域悬崖已可视");
		break;

	case ID_TRIGGERS_SORT_CONDS_EFFECTS:
		scen.sort_conds_effects();
		SetWindowText(propdata.statusbar, "已分类条件和效果");
		break;

	case ID_TRIGGERS_NOINSTRUCTIONSSOUND:
		scen.instructions_sound_text_set();
		SetWindowText(propdata.statusbar, "所有显示信息的声音文件已设置为 空");
		break;

	case ID_TRIGGERS_NOINSTRUCTIONSSOUNDID:
		scen.instructions_sound_id_set(-1);
		SetWindowText(propdata.statusbar, "所有显示信息的声音 ID 已设置为 -1 (无)");
		break;

	case ID_TRIGGERS_ZEROINSTRUCTIONSSOUNDID:
		scen.instructions_sound_id_set(0);
		SetWindowText(propdata.statusbar, "将所有显示信息的声音 ID 已设置为 0 (无)");
		break;

	case ID_TRIGGERS_NOPANEL:
		scen.instructions_panel_set(-1);
		SetWindowText(propdata.statusbar, "将所有显示信息的面板 ID 已设置为 -1 (无)");
		break;

	case ID_TRIGGERS_ZEROPANEL:
		scen.instructions_panel_set(0);
		SetWindowText(propdata.statusbar, "将所有显示信息的面板 ID 已设置为 0 (任务目标)");
		break;

	case ID_TRIGGERS_ZERODI:
		scen.instructions_string_zero();
		SetWindowText(propdata.statusbar, "将所有显示信息的字符串 ID 已设置为 0");
		break;

	case ID_TRIGGERS_RESETDI:
		scen.instructions_string_reset();
		SetWindowText(propdata.statusbar, "将所有显示信息的字符串 ID 已设置为 -1");
		break;

	case ID_TRIGGERS_HIDENAMES:
		scen.remove_trigger_names();
		SetWindowText(propdata.statusbar, "已清除所有触发名");
		break;

	case ID_TRIGGERS_COPY_SCRAWL:
	    {
            std::ostringstream ss;
	        scen.accept(TrigScrawlVisitor(ss));
            std::string scrawl = std::string("");
	        scrawl.append(ss.str());

	        const char* output = scrawl.c_str();
            const size_t len = strlen(output) + 1;
            HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE, len);
            memcpy(GlobalLock(hMem), output, len);
            GlobalUnlock(hMem);
            OpenClipboard(0);
            EmptyClipboard();
            SetClipboardData(CF_TEXT, hMem);
            CloseClipboard();
		    SetWindowText(propdata.statusbar, "已复制概略到剪贴板");
		}
		break;

	case ID_TRIGGERS_SAVE_PSEUDONYMS:
		scen.save_pseudonyms();
		SetWindowText(propdata.statusbar, "已覆盖触发名称");
		break;

	case ID_TRIGGERS_PREFIX_DISPLAY_ORDER:
		scen.prefix_display_order();
		SetWindowText(propdata.statusbar, "触发名称已冠以显示顺序的前缀");
		break;

	case ID_TRIGGERS_REMOVE_DISPLAY_ORDER_PREFIX:
		scen.remove_display_order_prefix();
		SetWindowText(propdata.statusbar, "显示顺序前缀已从触发名称移除");
		break;

	case ID_TRIGGERS_HIDE_DESCRIPTIONS:
		scen.remove_trigger_descriptions();
		SetWindowText(propdata.statusbar, "触发描述已清除");
		break;

	case ID_TRIGGERS_SWAP_NAMES_DESCRIPTIONS:
		scen.swap_trigger_names_descriptions();
		SetWindowText(propdata.statusbar, "已交换名称和描述");
		break;

	case ID_TRIGGERS_FIXTRIGGEROUTLIERS:
		scen.fix_trigger_outliers();
		SetWindowText(propdata.statusbar, "地图外的触发已限制到地图边界");
		break;

	case ID_FILE_TRIGWRITE:
		OnFileTrigWrite(sheet);
		break;

	case ID_FILE_TRIGREAD:
		OnFileTrigRead(sheet);
		break;

	case IDC_P_TOUP:
        if (MessageBox(sheet, "你可能需要将场景保存为对应格式之后，再执行此操作。\n该选项用于修复损坏的场景。你确定要这样做吗？", "转换", MB_YESNOCANCEL) == IDYES) {
		    scen.hd_to_up();
		    SetWindowText(propdata.statusbar, "从 AoHD 到 UserPatch 触发效果转换");
		}
		break;

	case IDC_P_TOHD:
        if (MessageBox(sheet, "你可能需要将场景保存为对应格式之后，再执行此操作。\n该选项用于修复损坏的场景。你确定要这样做吗？", "转换", MB_YESNOCANCEL) == IDYES) {
		    scen.up_to_hd();
		    SetWindowText(propdata.statusbar, "从 UserPatch 到 AoHD 触发效果转换");
		}
		break;

	case IDC_P_TOAOFE:
        if (MessageBox(sheet, "你可能需要将场景保存为对应格式之后，再执行此操作。\n该选项用于修复损坏的场景。你确定要这样做吗？", "转换", MB_YESNOCANCEL) == IDYES) {
		    scen.up_to_aofe();
		    SetWindowText(propdata.statusbar, "从 UserPatch 到 AoFE 触发效果转换");
		}
		break;

	case IDC_P_TO1C:
        if (MessageBox(sheet, "你可能需要将场景保存为对应格式之后，再执行此操作。\n该选项用于修复损坏的场景。你确定要这样做吗？", "Convert", MB_YESNOCANCEL) == IDYES) {
		    scen.up_to_10c();
		    SetWindowText(propdata.statusbar, "从 UserPatch 到 1.0c 触发效果转换");
		}
		break;

	case ID_FILE_RECENT1:
	case ID_FILE_RECENT2:
	case ID_FILE_RECENT3:
	case ID_FILE_RECENT4:
		FileOpen(sheet, false, id - ID_FILE_RECENT1);
		break;

	case IDCANCEL:
	case IDOK:
		assert(true);
		break;

	case ID_VIEW_STATISTICS:
		DialogBoxParam(aokts, MAKEINTRESOURCE(IDD_STATS), sheet, StatsDialogProc, 0);
		break;

	case ID_VIEW_STAT_BAR:
		if (GetMenuState(GetMenu(sheet), ID_VIEW_STAT_BAR, MF_BYCOMMAND) & MF_CHECKED)
		{
			ShowWindow(propdata.statusbar, SW_HIDE);
			CheckMenuItem(GetMenu(sheet), ID_VIEW_STAT_BAR, MF_BYCOMMAND);
		}
		else
		{
			ShowWindow(propdata.statusbar, SW_SHOW);
			CheckMenuItem(GetMenu(sheet), ID_VIEW_STAT_BAR, MF_BYCOMMAND | MF_CHECKED);
		}
		break;

	case ID_VIEW_MAP:
		if (GetMenuState(GetMenu(sheet), ID_VIEW_MAP, MF_BYCOMMAND) & MF_CHECKED)
		{
			// hide window
			ShowWindow(propdata.mapview, SW_HIDE);
			// clear check
			CheckMenuItem(GetMenu(sheet), ID_VIEW_MAP, MF_BYCOMMAND);
		}
		else
		{
			ShowWindow(propdata.mapview, SW_SHOW);
			CheckMenuItem(GetMenu(sheet), ID_VIEW_MAP, MF_BYCOMMAND | MF_CHECKED);
		}
		break;

	case ID_DRAW_TERRAIN:
		if (GetMenuState(GetMenu(sheet), ID_DRAW_TERRAIN, MF_BYCOMMAND) & MF_CHECKED)
		{
		    setts.drawterrain = false;
			// clear check
			CheckMenuItem(GetMenu(sheet), ID_DRAW_TERRAIN, MF_BYCOMMAND);
		}
		else
		{
		    setts.drawterrain = true;
			// clear check
			CheckMenuItem(GetMenu(sheet), ID_DRAW_TERRAIN, MF_BYCOMMAND | MF_CHECKED);
		}
		SendMessage(propdata.mapview, MAP_Reset, 0, 0);
		break;

	case ID_DRAW_ELEVATION:
		if (GetMenuState(GetMenu(sheet), ID_DRAW_ELEVATION, MF_BYCOMMAND) & MF_CHECKED)
		{
		    setts.drawelevation = false;
			// clear check
			CheckMenuItem(GetMenu(sheet), ID_DRAW_ELEVATION, MF_BYCOMMAND);
		}
		else
		{
		    setts.drawelevation = true;
			// clear check
			CheckMenuItem(GetMenu(sheet), ID_DRAW_ELEVATION, MF_BYCOMMAND | MF_CHECKED);
		}
		SendMessage(propdata.mapview, MAP_Reset, 0, 0);
		break;

	case ID_DRAW_TRIGGERS:
		if (GetMenuState(GetMenu(sheet), ID_DRAW_TRIGGERS, MF_BYCOMMAND) & MF_CHECKED)
		{
		    setts.drawconds = false;
		    setts.draweffects = false;
		    setts.drawlocations = false;
		}
		else
		{
		    setts.drawconds = true;
		    setts.draweffects = true;
		    setts.drawlocations = true;
		}
		SetDrawTriggerCheckboxes(sheet);
		SendMessage(propdata.mapview, MAP_Reset, 0, 0);
		break;

	case ID_DRAW_CONDITIONS:
		if (GetMenuState(GetMenu(sheet), ID_DRAW_CONDITIONS, MF_BYCOMMAND) & MF_CHECKED)
		{
		    setts.drawconds = false;
		}
		else
		{
		    setts.drawconds = true;
		}
		SetDrawTriggerCheckboxes(sheet);
		SendMessage(propdata.mapview, MAP_Reset, 0, 0);
		break;

	case ID_DRAW_EFFECTS:
		if (GetMenuState(GetMenu(sheet), ID_DRAW_EFFECTS, MF_BYCOMMAND) & MF_CHECKED)
		{
		    setts.draweffects = false;
		}
		else
		{
		    setts.draweffects = true;
		}
		SetDrawTriggerCheckboxes(sheet);
		SendMessage(propdata.mapview, MAP_Reset, 0, 0);
		break;

	case ID_DRAW_LOCATIONS:
		if (GetMenuState(GetMenu(sheet), ID_DRAW_LOCATIONS, MF_BYCOMMAND) & MF_CHECKED)
		{
		    setts.drawlocations = false;
		}
		else
		{
		    setts.drawlocations = true;
		}
		SetDrawTriggerCheckboxes(sheet);
		SendMessage(propdata.mapview, MAP_Reset, 0, 0);
		break;

	case ID_EDIT_ALL:
		if (GetMenuState(GetMenu(sheet), ID_EDIT_ALL, MF_BYCOMMAND) & MF_CHECKED)
		{
		    setts.editall = false;
			// clear check
			CheckMenuItem(GetMenu(sheet), ID_EDIT_ALL, MF_BYCOMMAND);
		}
		else
		{
		    setts.editall = true;
			CheckMenuItem(GetMenu(sheet), ID_EDIT_ALL, MF_BYCOMMAND | MF_CHECKED);
		}
		break;
		
	case ID_PREFERENCE:
			DialogBoxParam(aokts, MAKEINTRESOURCE(IDD_PREFERENCE), sheet, Preference, 0);//StatsDialogProc
		break;

	case ID_TOOLS_COMPRESS:
		OnCompressOrDecompress(sheet, true);
		break;

	case ID_TOOLS_DECOMPRESS:
		OnCompressOrDecompress(sheet, false);
		break;

	case ID_TS_HELP:
		WinHelp(sheet, "ts.hlp", HELP_CONTENTS, 0);
		break;

	case ID_TS_APP_ABOUT:
		DialogBoxParam(aokts, (LPCSTR)IDD_ABOUT, sheet, DefaultDialogProc, 0L);
		break;

	default:
		ret = false;
	}

	return ret;
}

/*
	MainDlgProc: The DialogProc for the main property sheet window.

	Note: See DialogProc in Platform SDK docs for parameters and notes.
*/
#define CALLPROC()	CallWindowProc((WNDPROC)pproc, sheet, msg, wParam, lParam)

INT_PTR CALLBACK MainDlgProc(HWND sheet, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR ret = FALSE;

	switch (msg)
	{
	case WM_CLOSE:
		DestroyWindow(sheet);
		return CALLPROC();

	case WM_SYSCOMMAND:
		//the overloaded DialogProc screwes up the automatic SC_CLOSE translation to WM_CLOSE
		if (wParam == SC_CLOSE)
			DestroyWindow(sheet);
		else
			return CALLPROC();
		break;

	case WM_COMMAND:
		ret = 0;	//processing message
		Sheet_HandleCommand(sheet, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
		CALLPROC();
		break;

	case WM_DESTROY:
		{
		    SendMessage(
			    PropSheet_GetCurrentPageHwnd(sheet),
			    AOKTS_Closing, 0, 0);
			WinHelp(sheet, "ts.hlp", HELP_QUIT, 0);
			PostQuitMessage(0);
		}
		return CALLPROC();

	case WM_HELP:
		WinHelp(sheet, "ts.hlp", HELP_CONTENTS, 0);
		break;

	case WM_MENUSELECT:
		OnMenuSelect(LOWORD(wParam), HIWORD(wParam), (HMENU)lParam);
		break;

	case MAP_Close:
		CheckMenuItem(GetMenu(sheet), ID_VIEW_MAP, MF_BYCOMMAND | MF_UNCHECKED);
		propdata.mapview = NULL;
		break;

	case MAP_Click:
		SendMessage(
			PropSheet_GetCurrentPageHwnd(sheet),
			MAP_Click, wParam, lParam);
		break;

	case MAP_ChangeDrawOptions:
	    SetDrawTriggerCheckboxes(sheet);
		break;

	default:
		return CALLPROC();
	}

	return ret;
}

char *getCmdlinePath(char *cmdline, char *buffer)
{
	while (*cmdline == ' ')
		cmdline++;

	if (!*cmdline)
	{
		printf_log("Too few arguments!\n");
		return NULL;
	}

	if (*cmdline == '\"')	//surrounding doublequotes
	{
		while (*cmdline && *cmdline != '\"' )
			*buffer++ = *cmdline++;
		if (*cmdline == '\"')
			cmdline++;
	}
	else
	{
		while (*cmdline && *cmdline != ' ')
			*buffer++ = *cmdline++;
	}

	*buffer = '\0';

	return cmdline;
}

/*
	ProcessCmdline: processes the command-line and returns whether to exit
*/
bool ProcessCmdline(char *cmdline)
{
	bool ret = true;
	char pathIn[_MAX_PATH], pathOut[_MAX_PATH];
	FILE *fileIn, *fileOut;
	int size, code;
	unsigned char *buffer;
	int c;

	switch (c = tolower(cmdline[1]))
	{
	case 'c':
	case 'u':
		cmdline += 2;
		cmdline = getCmdlinePath(cmdline, pathIn);
		if (!cmdline)
			break;
		cmdline = getCmdlinePath(cmdline, pathOut);
		if (!cmdline)
			break;

		if (c == 'c')
			printf("压缩 %s 到 %s... ", pathIn, pathOut);
		else
			printf("解压缩 %s 到 %s... ", pathIn, pathOut);

		size = fsize(pathIn);
		buffer = new unsigned char[size];
		if (!buffer)
		{
			printf_log("not enough memory.\n");
			break;
		}

		fileIn = fopen(pathIn, "rb");
		if (!fileIn)
		{
			printf_log("couldn\'t open input file\n");
			delete [] buffer;
			break;
		}
		fread(buffer, sizeof(char), size, fileIn);
		fclose(fileIn);

		fileOut = fopen(pathOut, "wb");
		if (!fileOut)
		{
			printf_log("couldn\'t open output file\n");
			delete [] buffer;
			break;
		}

		if (c == 'c')
			code = deflate_file(buffer, size, fileOut);
		else
			code = inflate_file(buffer, size, fileOut);

		fclose(fileOut);

		if (code >= 0)
			printf_log("done!\n");
		else
			printf_log("failed.\n");

		break;

	default:
		ret = false;
	}

	return ret;
}

/*
	WinMain: The entry-point function.

	Note: See WinMain in Platform SDK docs for parameters and notes.
*/

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPTSTR cmdline, int cmdshow)
{
	MSG msg;
	BOOL ret;
	HWND sheet, active;
	HACCEL accelerators;

#ifdef MSVC_MEMLEAK_CHECK
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF);	//check for memory leaks
#endif

    // These two methods are not the same
    GetModuleFileName(NULL, global::exedir, MAX_PATH); // works
    PathRemoveFileSpec(global::exedir);
	//GetCurrentDirectory(_MAX_PATH, global::exedir); // doesn't work

	//basic initializations
	aokts = inst;
	propdata.p = scen.players;	//start pointing to first member
	ret = setts.load();

	if (*setts.logname) {
	    char logpath[_MAX_PATH];

	    //GetCurrentDirectory(_MAX_PATH, logpath);
        strcpy(logpath, global::exedir);
	    strcat(logpath, "\\");
	    strcat(logpath, setts.logname);
		freopen(logpath, "w", stdout);
	    printf_log("Opened log file %s.\n", logpath);
	}
	printf_log("TS Path: %s\n", global::exedir);

    // Hint about whether to open as AOC or SGWB
	if (setts.recent_first) {
	     scen.game = (Game)setts.recent_first->game;
	     printf_log("Last game was %s.\n", gameName(scen.game));
	}

	//process any compress/decompress requests
	if ((*cmdline == '/' || *cmdline == '-') && ProcessCmdline(cmdline))
			return 0;

	//read genie data
	try
	{
		switch (scen.game) {
		case AOK:
		case AOC:
		case AOHD:
		case AOF:
			if ( (scen.game == AOC || scen.game == UP || scen.game == ETP) && setts.wkmode )
				esdata.load(datapath_wk);
			else
				esdata.load(datapath_aok);
		    break;
		case SWGB:
		case SWGBCC:
		    esdata.load(datapath_swgb);
		    break;
		default:
			if ( (scen.game == AOC || scen.game == UP || scen.game == ETP) && setts.wkmode )
				esdata.load(datapath_wk);
			else
				esdata.load(datapath_aok);
		}
	}
	catch (std::exception& ex)
	{
		printf_log("Could not load data: %s\n", ex.what());
		MessageBox(NULL,
			"无法从data.xml中读取Genie数据。正在终止...",
			"错误", MB_ICONERROR);
		return 0;
	}

	//create the property sheet & init misc data
	InitCommonControls();
	sheet = MakeSheet(inst);
	propdata.tformat = RegisterClipboardFormat("AOKTS Trigger");
	propdata.ecformat = RegisterClipboardFormat("AOKTS EC");
	propdata.mcformat = RegisterClipboardFormat("AOKTS Mapcopy");
	accelerators = LoadAccelerators(inst, (LPCTSTR)IDA_MAIN);	//checked for err later

	//give the sheet its own DialogProc
	pproc = (DLGPROC)SetWindowLong(sheet, DWL_DLGPROC, (LONG)&MainDlgProc);

	//check for errors down here, after we create the sheet
	if (!accelerators)
	{
		MessageBox(sheet,
			"快捷键加载失败。键盘快捷键将不可用。",
			"警告", MB_ICONWARNING);
	}
	if (!propdata.tformat | !propdata.ecformat)
	{
		MessageBox(sheet,
			"无法注册剪贴板格式。剪贴板操作将不可用。",
			"警告", MB_ICONWARNING);
	}
	//if (!ret)
	//	MessageBox(sheet, warnNoAOEII, "Warning", MB_ICONWARNING);

	//open mapview window
	propdata.mapview = MakeMapView(sheet, cmdshow || SW_MAXIMIZE);

	//check for, then open the scenario specified in command string
	if (*cmdline != '\0')
	{
		if (*cmdline == '"')
		{
			cmdline++;	//increment past first doublequote
			*strrchr(cmdline, '"') = '\0';	//find last " and replace it
		}

		strcpy(setts.ScenPath, cmdline);
		printf_log("cmdline scenpath: %s\n", setts.ScenPath);
		FileOpen(sheet, false, -1);
	}

	//the message loop
	while (ret = GetMessage(&msg, NULL, 0, 0))
	{
		if (ret < 0)	//did GetMessage() fail?
		{
			MessageBox(sheet,
				"无法从队列中检索消息。单击「确定」终止。",
				"AOKTS 致命错误", MB_ICONERROR);
			break;
		}

		// Give first dibs to keyboard accelerators and the propsheet.
		if (TranslateAccelerator(sheet, accelerators, &msg) ||
			PropSheet_IsDialogMessage(sheet, &msg))
			continue;

		// Usually active is the sheet. If it's not, it's a modeless dialog and
		// it should get a crack, too.
		if ((active = GetActiveWindow()) != sheet &&
			IsDialogMessage(active, &msg))
			continue;

		// If we get here, it's just a normal message, so Translate and
		// Dispatch.
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	//cleanup
	if (setts.DelTempOnExit)
		DeleteFile(setts.TempPath);

	fclose(stdout);

	return msg.wParam;
}
