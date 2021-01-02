/**
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION.

	utilunit.cpp: Various utility functions for units management.

	VIEW/CONTROLLER
**/

#include "utilunit.h"

#include "../util/settings.h"

#include "LinkListBox.h"
#include "utilui.h"
#include "../res/resource.h"
#include <stdio.h>
#include <algorithm>

extern Scenario scen;

using std::vector;

/** Options **/

const size_t UNITSTR_MAX = 48; // buffer size for unit "id: name" strings

/** Internal Use **/

// size will probably always be UNITSTR_MAX, but I make each caller supply it.
bool print_name(wchar_t * buffer, size_t size, const Unit& u)
{
	return
		swprintf(buffer, size, L"%d: %s", u.ident, u.getType()->name()) != -1;
}

const wchar_t * warningSelchange =
L"�����ҡ������ǰѡ����������Ŀ��";

/** Sorting **/

const char *sorts[] = { "ID", "����" };

struct UnitSortData
{
	const Unit * u;
	vector<Unit>::size_type index;
};

bool compare_id(const UnitSortData& first,
				const UnitSortData& second)
{
	return (first.u->ident < second.u->ident);
}

bool compare_name(const UnitSortData& first,
				  const UnitSortData& second)
{
	// Windows doesn't support POSIX strcasecmp().
	int sort = _wcsicmp(first.u->getType()->name(),
		second.u ->getType()->name());

	if (sort != 0) // not equal
	{
		return (sort < 0);
	}
	else
	{
		// resort to ID comparison if names are equal
		return compare_id(first, second);
	}
}

vector<UnitSortData> Sort(const vector<Unit>& from, enum Sorts sorttype)
{
	vector<UnitSortData> data;
	data.reserve(from.size());

	for (vector<Unit>::size_type i = 0; i < from.size(); ++i)
	{
		UnitSortData d;
		d.u = &from[i];
		d.index = i;
		data.push_back(d);
	}

	switch (sorttype)
	{
	case SORT_ID:
		std::sort(data.begin(), data.end(), compare_id);
		break;
	case SORT_Name:
		std::sort(data.begin(), data.end(), compare_name);
		break;
	}

	return data;
}

/*
	UnitList_Fill: Fill a listbox from an array of struct Unit's.

	Note: Will SETITEMDATA to the index in the vector.
*/
void UnitList_Fill(HWND listbox, enum Sorts sorttype,
				   const vector<Unit>& list)
{
	SendMessage(listbox, LB_RESETCONTENT, 0, 0);

	vector<UnitSortData> sorted = Sort(list, sorttype);

	for (vector<UnitSortData>::const_iterator iter = sorted.begin();
		iter != sorted.end(); ++iter)
		UnitList_Append(listbox, iter->u, iter->index);
}

int UnitList_Append(HWND listbox, const Unit * unit, size_t u_index)
{
	int index;

	wchar_t string[UNITSTR_MAX];
	print_name(string, UNITSTR_MAX, *unit);

	index = List_AddStringW(listbox, string);
	SendMessage(listbox, LB_SETITEMDATA, index, u_index);

	return index;
}

/*
	UnitList_InsertItem: Inserts a new item in the listbox (in sort order)
		and returns its position.
*/
int UnitList_InsertItem(HWND listbox, enum Sorts sorttype,
						const vector<Unit>& list, const Unit *which)
{
	int ret = -1;

	vector<UnitSortData> sorted = Sort(list, sorttype);

	for (vector<UnitSortData>::const_iterator iter = sorted.begin();
		iter != sorted.end(); ++iter)
	{
		if (iter->u == which)
		{
			wchar_t string[UNITSTR_MAX];
			print_name(string, UNITSTR_MAX, *which);

			ret = iter - sorted.begin();
			List_InsertStringW(listbox, ret, string);
			SendMessage(listbox, LB_SETITEMDATA, ret, iter->index);
		}
	}

	return ret;
}

// This is not really like the other UnitList_ functions.
void UnitList_FillGroup(HWND typebox, const UnitGroupLink *group)
{
	SendMessage(typebox, LB_RESETCONTENT, 0, 0);

	if (group)
	{
		unsigned count = group->count;
		const UnitLink **c_parse = group->list;

		if (!c_parse)
			return;

		while (--count)
			LinkListBox_Add(typebox, *c_parse++);
	}
	else
	{
		LinkListBox_Fill(typebox, esdata.units.head());
	}
}

inline std::string UnicodeToANSI(const std::wstring& str)
{
	char*  pElementText;
	int    iTextLen;
	iTextLen = WideCharToMultiByte(CP_ACP, 0,
		str.c_str(),
		-1,
		nullptr,
		0,
		nullptr,
		nullptr);
 
	pElementText = new char[iTextLen + 1];
	memset((void*)pElementText, 0, sizeof(char) * (iTextLen + 1));
	::WideCharToMultiByte(CP_ACP,
		0,
		str.c_str(),
		-1,
		pElementText,
		iTextLen,
		nullptr,
		nullptr);
 
	std::string strText;
	strText = pElementText;
	delete[] pElementText;
	return strText;
}

std::string get_unit_full_name(UID id)
{
    std::ostringstream ss;
    std::string name = "";
    PlayersUnit fu = find_map_unit(id);
    if (fu.u) {
        std::string unitname(UnicodeToANSI(fu.u->getType()->name()));
        switch (fu.player) {
            case 8:
                ss << "���ǵ�";
                break;
            default:
                ss << "���" << fu.player + 1 << " ��";
        }
        ss << std::string(unitname.begin(), unitname.end());
        return ss.str();
    }
    return "<��Ч��λ ID >";
}

bool valid_unit_id(UID id)
{
    PlayersUnit fu = find_map_unit(id);
    if (fu.u) {
        return true;
    }
    return false;
}

PlayersUnit find_map_unit(UID id)
{
    PlayersUnit ret;
	/* Find the owner player from first UID. */
	for (int i = 0; i < NUM_PLAYERS; i++)
	{
	    int pos = scen.players[i].find_unit(id);
		if (pos != scen.players[i].units.size())
		{
		    //return &scen.players[i].units.at(pos);
            ret.player = i;
            ret.u = &scen.players[i].units.at(pos);
		    return ret;
		}
	}
	return ret;
}

/*
	UnitList_FillSelected: Fill a listbox with an array of pointers to Unit structs.

	Note: Will SETITEMDATA to the index of the unit in the array.
*/
void UnitList_FillSelected(HWND listbox, const vector<Unit>& units,
						   UID *selected, size_t count, enum Sorts sorttype)
{
	// Multi-selection list box?
	bool multi = (GetWindowLongPtr(listbox, GWL_STYLE) & LBS_MULTIPLESEL) != 0;

	// Sort full list
	vector<UnitSortData> sorted = Sort(units, sorttype);

	SendMessage(listbox, LB_RESETCONTENT, 0, 0);

	for (vector<UnitSortData>::const_iterator iter = sorted.begin();
		iter != sorted.end(); ++iter)
	{
		wchar_t string[UNITSTR_MAX];
		print_name(string, UNITSTR_MAX, *iter->u);
		unsigned int index = List_AddStringW(listbox, string);

		/* Set the item's data to its index in the array. */
		SendMessage(listbox, LB_SETITEMDATA, index, iter->index);

		/* if the unit is one of the selected, select it */
		if (multi)
		{
			// If unit is in selected list ...
			if (std::find(selected, selected + count, iter->u->ident)
				!= selected + count)
			{
				// ... select it!
				SendMessage(listbox, LB_SETSEL, TRUE, index);
			}
		}
		else if (selected && iter->u->ident == *selected)
			SendMessage(listbox, LB_SETCURSEL, index, 0);
	}
}

/* The Unit Selector */

void Init(HWND dialog, UnitEdit * ue)
{
	SetWindowLongPtr(dialog, DWLP_USER, reinterpret_cast<LPARAM>(ue));

	/* Initialize controls. */
	HWND control = GetDlgItem(dialog, IDC_US_SORT);
	SendMessage(control, CB_ADDSTRING, 0, (LPARAM)sorts[0]);
  	SendMessage(control, CB_ADDSTRING, 0, (LPARAM)sorts[1]);
	SendMessage(control, CB_SETCURSEL, 0, 0);
	Combo_Fill(dialog, IDC_US_PLAYER, Player::names, NUM_PLAYERS);

	/* Temporarily convert the player id to standard value. (See trigger.h.) */
	if (ue->player >= 0)
	{
		P_ECToStd(ue->player);
	}

	if (ue->count > 0)
	{
		/* Find the owner player from first UID. */
		for (int i = 0; i < NUM_PLAYERS && ue->player == -1; i++)
		{
			if (ue->players[i].find_unit(*ue->ids) !=
				ue->players[i].units.size())
			{
				ue->player = i;
				break;
			}
		}

		// select appropriate player
		SendDlgItemMessage(dialog, IDC_US_PLAYER, CB_SETCURSEL, ue->player, 0);

		UnitList_FillSelected(GetDlgItem(dialog, IDC_US_UNITS),
			ue->players[ue->player].units, ue->ids, ue->count, SORT_ID);
	}
}

void HandleCommand(HWND dialog, WORD code, WORD id, HWND control)
{
	int player;
	HWND unitbox;

	/* Init */
	player = SendMessage(GetDlgItem(dialog, IDC_US_PLAYER), CB_GETCURSEL, 0, 0);
	unitbox = GetDlgItem(dialog, IDC_US_UNITS);

	// Get this here since most commands use it.
	// reinterpret_cast: see SetWindowLongPtr in Init()
	UnitEdit * ue =
		reinterpret_cast<UnitEdit*>(GetWindowLongPtr(dialog, DWLP_USER));

    switch (code) {
    case BN_CLICKED:
        switch (id) {
        case IDOK:
            {
		        HWND listbox = GetDlgItem(dialog, IDC_US_UNITS);
		        int buffer[MAX_UNITSEL];
		        int index;
		        bool multi;	//is this IDD_UNITSEL2 or just IDD_UNITSEL1?
		        multi = (GetWindowLongPtr(listbox, GWL_STYLE) & LBS_MULTIPLESEL) != 0;

		        if (multi)
		        {
			        ue->count = (short)SendMessage(listbox, LB_GETSELCOUNT, 0, 0);
			        SendMessage(listbox, LB_GETSELITEMS, MAX_UNITSEL, (LPARAM)&buffer);
		        }
		        else
		        {
			        *buffer = SendMessage(listbox, LB_GETCURSEL, 0, 0);
			        ue->count = (*buffer != LB_ERR);	//boolean magic!
		        }

		        /* Update the uids. We don't have to worry about clearing the
		           unused ones because only the used ones are written to the scen. */
		        for (int i = 0; i < ue->count; i++)
		        {
			        index = SendMessage(listbox, LB_GETITEMDATA, buffer[i], 0);
			        ue->ids[i] = ue->players[ue->player].units[index].ident;
		        }

		        ue->player = (char)SendDlgItemMessage(dialog, IDC_US_PLAYER, CB_GETCURSEL, 0, 0);

		        P_StdToEC(ue->player);

		        EndDialog(dialog, 1);
		    }
		    break;
		case IDCANCEL:
		    P_StdToEC(ue->player);
		    EndDialog(dialog, 0);
		    break;
		}
		break;
    case CBN_SELCHANGE:
        switch (id) {
        case IDC_US_PLAYER:
	        {
		        int code = IDYES;

		        /* Warn user if there are currently items selected */
		        if (SendMessage(unitbox, LB_GETSELCOUNT, 0, 0) > 0)
			        code = MessageBoxW(dialog, warningSelchange,
								        L"��ұ��", MB_ICONWARNING | MB_YESNO);

		        /* Take appropriate action */
		        if (code == IDYES)
		        {
			        ue->player = player;

	                enum Sorts sort = (enum Sorts)SendDlgItemMessage(dialog, IDC_US_SORT, CB_GETCURSEL, 0, 0);
		            UnitList_FillSelected(unitbox,
				            ue->players[player].units, NULL, 0, sort);
		        }
		        else
			        SendMessage(control, CB_SETCURSEL, ue->player, 0);
	        }
            break;
        case IDC_US_SORT:
            {
	            enum Sorts sort = (enum Sorts)SendDlgItemMessage(dialog, IDC_US_SORT, CB_GETCURSEL, 0, 0);
			    UnitList_FillSelected(unitbox,
				        ue->players[player].units, NULL, 0, sort);
            }
            break;
        }
        break;
	}
}

INT_PTR CALLBACK SelDlgProc(HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR ret = FALSE;

	switch (msg)
	{
	case WM_INITDIALOG:
		// reinterpret_cast: see DialogBoxParam() in UnitSelDialogBox()
		Init(dialog, reinterpret_cast<UnitEdit *>(lParam));
		ret = TRUE;
		break;

	case WM_COMMAND:
		HandleCommand(dialog, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
		break;
	}

	return ret;
}

INT_PTR UnitSelDialogBox(HINSTANCE instance,
						 HWND parent,
						 UnitEdit& ue,
						 bool multisel)
{
	return DialogBoxParam(
		instance,
		MAKEINTRESOURCE(multisel ? IDD_UNITSEL2 : IDD_UNITSEL1),
		parent,
		SelDlgProc,
		reinterpret_cast<LPARAM>(&ue));
}

bool SingleUnitSelDialogBox(
		HWND parent, Player const * players, UID& uid, bool uid_specified)
{
	bool ret;
	struct UnitEdit ue =
	{ players, -1, uid_specified };
	if (uid_specified)
		ue.ids[0] = uid;
	else
		ue.player = 0;  // default to player 1

	ret = (UnitSelDialogBox(GetModuleHandle(NULL), parent, ue, false) != 0);

	if (ret)
	{
		uid = ue.ids[0];
	}

	return ret;
}
