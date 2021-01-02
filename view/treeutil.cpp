#include "treeutil.h"

#include <cstdio>

WNDPROC EditWndProc = NULL;

/* Special subclassed edit control for Tree-Views */
LRESULT CALLBACK TVEditWndProc(HWND control, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (!EditWndProc)
		return 0; // unreasonable default?

	switch (msg)
	{
	case WM_GETDLGCODE:
		/* Get ENTER and ESC keys to accept or cancel edits. */
		return (wParam == VK_RETURN || wParam == VK_ESCAPE) ?
			DLGC_WANTALLKEYS :
			CallWindowProc(EditWndProc, control, msg, wParam, lParam);

	default:
		return CallWindowProc(EditWndProc, control, msg, wParam, lParam);
	}
}


void SubclassTreeEditControl(HWND control)
{
	EditWndProc = SetWindowWndProc(control, TVEditWndProc);
}

LPARAM GetItemParam(HWND treeview, HTREEITEM which)
{
	TVITEM item;

	item.mask = TVIF_HANDLE | TVIF_PARAM;
	item.hItem = which ? which : TreeView_GetNextItem(treeview, NULL, TVGN_CARET);
	item.lParam = 0; //why was -1?

	if (item.hItem)	//TVM_GETITIEM does NOT have NULL-protection
		TreeView_GetItem(treeview, &item);

	return item.lParam;
}

void TreeView_AddChild(HWND treeview, LPARAM param, HTREEITEM parent, HTREEITEM after, int type)
{
	//variables
	HTREEITEM newitem;
	TVINSERTSTRUCT tvis;

	if (TreeView_GetNextItem(treeview, parent, TVGN_PARENT))
	{
		printf("���棺û��ΪTreeView_AddChild�ṩ��������\n");
		parent = TreeView_GetParent(treeview, parent);
	}

	//common settings
	tvis.hParent = parent;
	tvis.hInsertAfter = after;
	tvis.item.mask = TVIF_IMAGE | TVIF_TEXT | TVIF_PARAM | TVIF_SELECTEDIMAGE;
	tvis.item.pszText = (LPSTR)LPSTR_TEXTCALLBACK;
	tvis.item.lParam = param;
	switch (type) {
	case 0: // condition
		tvis.item.iImage = BitmapIcons::COND_BAD;
		break;
	case 1: // effect
		tvis.item.iImage = BitmapIcons::EFFECT_BAD;
		break;
	}
	tvis.item.iSelectedImage = tvis.item.iImage;

	//insert the item
	newitem = TreeView_InsertItem(treeview, &tvis);
	TreeView_Expand(treeview, parent, TVE_EXPAND);
	TreeView_SelectItem(treeview, newitem);
	SetFocus(treeview);
}
