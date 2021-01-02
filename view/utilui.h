#ifndef INC_UTILUI_H
#define INC_UTILUI_H

/**
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION.
	utilui.h -- Custom controls and utility UI functions & macros.

	VIEW/CONTROLLER
**/

#include "../model/scen_const.h"
#include "../util/winugly.h"
#include <climits>
#include <string>
#include <vector>
#include <sstream>

/* Shared msgbox strings */

const char errorOpenClipboard[] =
"�޷��򿪼����塣���ܱ���һ������ռ�á�";
const char errorSetClipboard[] =
"�޷����ü��������ݡ�";
const char errorAllocFailed[] =
"�޷�Ϊ�˲��������ڴ档����ܱ�ʾ�ڴ治�㣬��ͨ����ʾ���ڴ����йش˴�������ϵ DiGiT ��";
const char warningNoFormat[] =
"ָ���ļ��������ݲ����ڡ�\n�뽫�˴��󱨸�� cyan.spam@gmail.com ��";


/** Common functions **/

#define ENABLE_WND(id, en) EnableWindow(::GetDlgItem(dialog, id), en)

/*
	GetWindowText: Retrieves text from a window and stores it in an SString class.
*/
void GetWindowText(HWND wnd, class SString &value);
void GetWindowTextCstr(HWND wnd, char *value);

void SetWindowText(HWND wnd, int value);

template<typename T>
std::string toString(const T& value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

/**
 * Displays an error message box indicating that an unhandled exception
 * occurred while processing /msg/ and that the application will now quit.
 * Intended to be used in a WindowProc or DialogProc try/catch wrapper.
 * @param parent the parent window of the message box
 * @param msg the Windows Message ID that was being processed
 * @param ex the unhandled exception that occurred
 */
void unhandledExceptionAlert(HWND parent, UINT msg, std::exception& ex);

/** Combo Box Utility Functions **/

/**
 * Fills a combo box with the specified strings.
 */
void Combo_Fill(HWND dialog, int id, std::vector<std::string> strings);
void Combo_Fill(HWND dialog, int id, char const * * strings, size_t count);

void Combo_Clear(HWND dialog, int id);
void List_Clear(HWND dialog, int id);

/*
	Combo_PairFill: Fills a combobox with strings & data from PAIRs.
*/
void Combo_PairFill(HWND combobox, unsigned count, const struct PAIR *pairs);

/**
 * Selects the item from a Combo Box that has the associated data.
 * @return the index of the selected item
 */
LRESULT Combo_SelectByData(HWND combobox, int data);

/*
 * Retrieves item data from the currently-selected item in the combo box.
 */
inline LRESULT Combo_GetSelData(HWND combobox);

/** List Box Utility Functions **/

/**
 * @return the index of the item with the associated cPtr, or UINT_MAX if none
 * found.
 */
unsigned ListBox_Find(HWND listbox, const void *);

/** Dialog Utility Functions **/

/**
 * Convert an int to a short, since many UI functions just deal with ints.
 */
inline short toshort(int n)
{
	return (n > SHRT_MAX) ? SHRT_MAX : static_cast<short>(n);
}

/*
	GetDlgItemFloat: Return a float from a dialog box edit control.

	dialog: The owner dialog.
	id: The identifier of the control.

  Note: Text longer than 32 characters will be truncated.
*/
float GetDlgItemFloat(HWND dialog, UINT id);

/*
	SetDlgItemFloat: Set a dialog box edit control's text to a floating-point value.

	dialog: The owner dialog.
	id: The identifier of the control.
	value: The value to display.
*/
void SetDlgItemFloat(HWND dialog, UINT id, double value);

/*
	GetCheckedRadio: Returns the ID of the checked radio button from a range of radio buttons.
*/
UINT GetCheckedRadio(HWND dialog, UINT first, UINT last);

/** Common Dialog Utility Functions **/

/**
 * Presents an "Open File" dialog box to the user. The box:
 * - requires the user to select an existing file
 * - has no file type filters
 *
 * The full path is returned in /path/.
 *
 * @return true if the user clicked "OK", false otherwise
 */
bool GetOpenFileNameA(HWND owner, char * path, DWORD maxPath);

/**
 * Presents a "Save File" dialog box to the user. The box:
 * - has no file type filters
 *
 * @param owner desired owner window of the dialog box
 * @param path in/out: contains full path upon return
 * @param maxPath specifies maximum number of characters to store in path
 *
 * @return true if the user clicked "OK", false otherwise
 */
bool GetSaveFileNameA(HWND owner, char * path, DWORD maxPath);
// TODO: allow pre-filled path

/* Tooltips */
void TooltipInit(HWND tt);
BOOL AddTool(HWND dialog, int ctrlId, LPTSTR text);

/* Special Edit control subclass */
LRESULT CALLBACK TVEditWndProc(HWND control, UINT msg, WPARAM wParam, LPARAM lParam);

/** Inline function definitions **/

LRESULT Combo_GetSelData(HWND combobox)
{
	return SendMessage(combobox, CB_GETITEMDATA,
			SendMessage(combobox, CB_GETCURSEL, 0, 0), 0);
}

bool TSSetDlgItemInt(HWND hDlg, int nIDDlgItem, UINT uValue, BOOL bSigned);

const char * gameName(Game version);

#endif // INC_UTILUI_H
