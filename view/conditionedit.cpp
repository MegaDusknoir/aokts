/**
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION.
	conditionedit.cpp -- Editors for conditions.

	VIEW/CONTROLLER
**/

#include "../model/scen.h"

#include "../util/settings.h"
#include "../res/resource.h"
//#include "../model/Condition.h"
#include "ecedit.h"
#include "utilui.h"
#include "LCombo.h"
#include "utilunit.h"
#include "mapview.h"
#include "../util/winugly.h"
#include <stdio.h>

extern Scenario scen;

/* Editor classes */

void EditCondition::update(Trigger *t)
{
	t->conds[index] = c;
}

// AoC v1.0c
const char ctable_aok[Condition::NUM_CONDITIONS_AOHD4][EditCondition::N_CONTROLS] = // Using 0 instead of -1 to waste less space
{	//0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// None
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0 },	// Bring Object to Area
	{ 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Bring Object to Object
	{ 1, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0 },	// Own Objects
	{ 1, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0 },	// Own Fewer Objects
	{ 1, 0, 0, 0, 2, 1, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 0 },	// Objects in Area
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Destroy Object
	{ 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Capture Object
	{ 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Accumulate Attribute
	{ 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Research Technology
	{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Timer
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Object Selected
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },	// AI Signal
	{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Player Defeated
	{ 0, 0, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0 },	// Object Has Target
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Object Visible
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Object Not Visible
	{ 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Researching Tech
	{ 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Units Garrisoned
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  // Difficulty Level
	{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Chance
};

// Userpatch v1.5
const char ctable_up[Condition::NUM_CONDITIONS_UP][EditCondition::N_CONTROLS] = // Using 0 instead of -1 to waste less space
{	//0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// None
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0 },	// Bring Object to Area
	{ 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Bring Object to Object
	{ 1, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0 },	// Own Objects
	{ 1, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0 },	// Own Fewer Objects
	{ 1, 0, 0, 0, 2, 1, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 0 },	// Objects in Area
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Destroy Object
	{ 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Capture Object
	{ 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Accumulate Attribute
	{ 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Research Technology
	{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Timer
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Object Selected
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },	// AI Signal
	{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Player Defeated
	{ 0, 0, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0 },	// Object Has Target
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Object Visible
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Object Not Visible
	{ 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Researching Tech
	{ 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Units Garrisoned
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  // Difficulty Level
	{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Civilization
	{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Chance
};

// ETP v1.7
const char ctable_etp[Condition::NUM_CONDITIONS_ETP][EditCondition::N_CONTROLS] = // Using 0 instead of -1 to waste less space
{	//0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// None
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0 },	// Bring Object to Area
	{ 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Bring Object to Object
	{ 1, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0 },	// Own Objects
	{ 1, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0 },	// Own Fewer Objects
	{ 1, 0, 0, 0, 2, 1, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 0 },	// Objects in Area
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Destroy Object
	{ 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Capture Object
	{ 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Accumulate Attribute
	{ 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Research Technology
	{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Timer
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Object Selected
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },	// AI Signal
	{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Player Defeated
	{ 0, 0, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0 },	// Object Has Target
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Object Visible
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Object Not Visible
	{ 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Researching Tech
	{ 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Units Garrisoned
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  // Difficulty Level
	{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Civilization
	{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Chance

	{ 1, 1, 1, 2, 2, 1, 1, 1, 0, 1, 1, 1, 1, 1, 2, 2, 1 },	// �ػ�����С�ڵ���
	{ 1, 1, 1, 2, 2, 1, 1, 1, 0, 1, 1, 1, 1, 1, 2, 2, 1 },	// ��������С�ڵ���
	{ 1, 1, 1, 2, 2, 1, 1, 1, 0, 1, 1, 1, 1, 1, 2, 2, 1 },	// ��ս�趨
	{ 1, 1, 1, 2, 2, 1, 1, 1, 0, 1, 1, 1, 1, 1, 2, 2, 1 },	// ��������
	{ 1, 1, 1, 2, 2, 1, 1, 1, 0, 1, 1, 1, 1, 1, 2, 2, 1 },	// ��������
	{ 1, 1, 1, 2, 2, 1, 1, 1, 0, 1, 1, 1, 1, 1, 2, 2, 1 },	// ����ֵ����
	{ 1, 1, 1, 2, 2, 1, 1, 1, 0, 1, 1, 1, 1, 1, 2, 2, 1 },	// ����ֵ����
	{ 1, 1, 1, 2, 2, 1, 1, 1, 0, 1, 1, 1, 1, 1, 2, 2, 1 },	// �Ƽ�״̬
};

// ../res/resource.h
// go to IDC_C_AMOUNT

// SWGB
const char ctable_swgb[Condition::NUM_CONDITIONS_SWGB][EditCondition::N_CONTROLS] = // Using 0 instead of -1 to waste less space
{	//0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 15
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// None
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0 },	// Bring Object to Area
	{ 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Bring Object to Object
	{ 1, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0 },	// Own Objects
	{ 1, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0 },	// Own Fewer Objects
	{ 1, 0, 0, 0, 2, 1, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 0 },	// Objects in Area
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Destroy Object
	{ 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Capture Object
	{ 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Accumulate Attribute
	{ 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Research Technology
	{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Timer
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Object Selected
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },	// AI Signal
	{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Player Defeated
	{ 0, 0, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0 },	// Object Has Target
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Object Visible
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Object Not Visible
	{ 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Researching Tech
	{ 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	// Units Garrisoned
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  // Difficulty Level
	{ 1, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0 },  // OwnFewerFoundations
	{ 1, 0, 0, 0, 2, 1, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 0 },  // SelectedObjectsInArea
};

// SWGB:CC
const char ctable_cc[Condition::NUM_CONDITIONS_CC][EditCondition::N_CONTROLS] = // Using 0 instead of -1 to waste less space
{	//0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  // None
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0 },  // Bring Object to Area
	{ 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  // Bring Object to Object
	{ 1, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0 },  // Own Objects
	{ 1, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0 },  // Own Fewer Objects
	{ 1, 0, 0, 0, 2, 1, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 0 },  // Objects in Area
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  // Destroy Object
	{ 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  // Capture Object
	{ 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  // Accumulate Attribute
	{ 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  // Research Technology
	{ 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  // Timer
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  // Object Selected
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },  // AI Signal
	{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  // Player Defeated
	{ 0, 0, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0 },  // Object Has Target
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  // Object Visible
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  // Object Not Visible
	{ 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  // Researching Tech
	{ 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  // Units Garrisoned
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  // Difficulty Level
	{ 1, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0 },  // OwnFewerFoundations
	{ 1, 0, 0, 0, 2, 1, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 0 },  // SelectedObjectsInArea
	{ 1, 0, 0, 0, 2, 1, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 0 },  // PoweredObjectsInArea
	{ 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }   // UnitsQueuedPastPopCap  (works in AOK??)
};

void ConditionControls(HWND dialog, int type)
{
	Combo_Clear(dialog, IDC_C_TAUNT_PLAYER);
	Combo_Clear(dialog, IDC_C_TAUNT_SET);
	ENABLE_WND(IDC_C_TAUNT_PLAYER, false);
	ENABLE_WND(IDC_C_TAUNT_SET, false);
	ENABLE_WND(IDC_C_AIGOAL, false);

	int i;

	if (type >= scen.pergame->max_condition_types)
	{
		for (i = IDC_C_AMOUNT; i <= IDC_C_UNKNOWNHD; i++)
			ENABLE_WND(i, true);
		ENABLE_WND(IDC_C_USEL1, true);
		ENABLE_WND(IDC_C_USEL2, true);
		return;
	}

	for (i = IDC_C_AMOUNT; i <= IDC_C_UNKNOWNHD; i++)
		ENABLE_WND(i, false || setts.editall);
	ENABLE_WND(IDC_C_USEL1, setts.editall);
	ENABLE_WND(IDC_C_USEL2, setts.editall);

	const char *table;
	switch (scen.game) {
	case ETP:
		table = ctable_etp[type];
	    break;
	case UP:
		table = ctable_up[type];
	    break;
	case AOK:
	case AOC:
	case AOHD:
	case AOF:
	case AOHD4:
	case AOF4:
	case AOHD6:
	case AOF6:
	    table = ctable_aok[type];
	    break;
	case SWGB:
	    table = ctable_swgb[type];
	    break;
	case SWGBCC:
	    table = ctable_cc[type];
	    break;
	default:
	    table = ctable_cc[type];
    }

	for (i = 0; i < EditCondition::N_CONTROLS; i++)
	{
		if (table[i])
		{
			ENABLE_WND(IDC_C_AMOUNT + i, true);
			if (i == 2)
				ENABLE_WND(IDC_C_USEL1, true);
			if (i == 3)
				ENABLE_WND(IDC_C_USEL2, true);
		}
	}
}

const wchar_t *noselectc = L"<none>";

void C_Init(HWND dialog)
{
    ENABLE_WND(IDC_C_REVERSE, scen.game == AOHD4 || scen.game == AOF4 || scen.game == AOHD6 || scen.game == AOF6 || scen.game == UP || scen.game == ETP || setts.editall);
    ENABLE_WND(IDC_C_RESERVED, scen.game == AOHD4 || scen.game == AOF4 || scen.game == AOHD6 || scen.game == AOF6 || scen.game == UP || scen.game == ETP || setts.editall);
	Combo_Fill(dialog, IDC_C_TYPE, Condition::types, scen.pergame->max_condition_types);
	Combo_Fill(dialog, IDC_C_VTYPE, Condition::virtual_types, scen.pergame->max_virtual_condition_types + 1); // +1 for None option
	Combo_Fill(dialog, IDC_C_PLAYER, players_ec, EC_NUM_PLAYERS + 1);
	LCombo_Fill(dialog, IDC_C_RESEARCH, esdata.techs.head());
	LCombo_Fill(dialog, IDC_C_RESTYPE, esdata.resources.head());
	Combo_PairFill(GetDlgItem(dialog, IDC_C_GROUP), NUM_GROUPS, groups);
	Combo_PairFill(GetDlgItem(dialog, IDC_C_UTYPE), NUM_UTYPES, utypes);
	LCombo_Fill(dialog, IDC_C_UCNST, esdata.units.head(), noselectc);
}

void LoadVirtualTypeConditions(HWND dialog, EditCondition *data) {
	Condition *c = &data->c;

    switch (scen.game) {
    case AOC:
    case ETP:
    case UP:
        switch (c->type) {
        case ConditionType::DifficultyLevel:
            if (c->amount >= DifficultyLevel::Hardest && c->amount <= DifficultyLevel::Easiest) {
	            SendDlgItemMessage(dialog, IDC_C_VTYPE, CB_SETCURSEL, ConditionVirtualTypeAOC::DifficultyLevelHardest + c->amount, 0);
            }
            break;
        case ConditionType::AISignal:
            switch (c->ai_signal) {
            case -1034:
	            SendDlgItemMessage(dialog, IDC_C_VTYPE, CB_SETCURSEL, ConditionVirtualTypeAOC::SinglePlayer, 0);
		        ENABLE_WND(IDC_C_AISIG, false);
                break;
            case -1035:
	            SendDlgItemMessage(dialog, IDC_C_VTYPE, CB_SETCURSEL, ConditionVirtualTypeAOC::StartingAgeStandard, 0);
		        ENABLE_WND(IDC_C_AISIG, false);
                break;
            case -1036:
	            SendDlgItemMessage(dialog, IDC_C_VTYPE, CB_SETCURSEL, ConditionVirtualTypeAOC::StartingResourcesStandard, 0);
		        ENABLE_WND(IDC_C_AISIG, false);
                break;
            case -1039:
	            SendDlgItemMessage(dialog, IDC_C_VTYPE, CB_SETCURSEL, ConditionVirtualTypeAOC::Regicide, 0);
		        ENABLE_WND(IDC_C_AISIG, false);
                break;
            case -1040:
	            SendDlgItemMessage(dialog, IDC_C_VTYPE, CB_SETCURSEL, ConditionVirtualTypeAOC::Deathmatch, 0);
		        ENABLE_WND(IDC_C_AISIG, false);
                break;
            case -70850:
	            SendDlgItemMessage(dialog, IDC_C_VTYPE, CB_SETCURSEL, ConditionVirtualTypeAOC::OneClickGarrison, 0);
		        ENABLE_WND(IDC_C_AISIG, false);
                break;
            default:
                {
                    if (c->ai_signal >= -518 && c->ai_signal <= -7) {
	                    Combo_Fill(dialog, IDC_C_TAUNT_PLAYER, players_ec + 1, EC_NUM_PLAYERS - 1);
	                    Combo_Fill(dialog, IDC_C_TAUNT_SET, Condition::taunt_set, Condition::NUM_TAUNT_SETS);
	                    ENABLE_WND(IDC_C_TAUNT_PLAYER, true);
	                    ENABLE_WND(IDC_C_TAUNT_SET, true);
                        int signal = (c->ai_signal + 518);
                        int taunt_player = signal / 64;
                        int taunt_set = signal % 64;
	                    SendDlgItemMessage(dialog, IDC_C_TAUNT_PLAYER, CB_SETCURSEL, taunt_player, 0);
	                    SendDlgItemMessage(dialog, IDC_C_TAUNT_SET, CB_SETCURSEL, taunt_set, 0);
	                    SendDlgItemMessage(dialog, IDC_C_VTYPE, CB_SETCURSEL, ConditionVirtualTypeAOC::Taunt, 0);
		                ENABLE_WND(IDC_C_AISIG, false);
	                    return;
	                } else if (c->ai_signal >= -774 && c->ai_signal <= -519) {
	                    SendDlgItemMessage(dialog, IDC_C_VTYPE, CB_SETCURSEL, ConditionVirtualTypeAOC::AIScriptGoal, 0);
	                    ENABLE_WND(IDC_C_AIGOAL, true);
	                    SetDlgItemInt(dialog, IDC_C_AIGOAL, c->ai_signal + 774, TRUE);
		                ENABLE_WND(IDC_C_AISIG, false);
                    } else {
	                    SendDlgItemMessage(dialog, IDC_C_VTYPE, CB_SETCURSEL, 0, 0);
		                ENABLE_WND(IDC_C_AISIG, true);
	                }
	            }
            }
            break;
        default:
	        SendDlgItemMessage(dialog, IDC_C_VTYPE, CB_SETCURSEL, 0, 0);
        }
        break;
    default:
        switch (c->type) {
        case ConditionType::DifficultyLevel:
            if (c->amount >= DifficultyLevel::Hardest && c->amount <= DifficultyLevel::Easiest) {
	            SendDlgItemMessage(dialog, IDC_C_VTYPE, CB_SETCURSEL, ConditionVirtualTypeDefault::DifficultyLevelHardest + c->amount, 0);
            }
            break;
        default:
	        SendDlgItemMessage(dialog, IDC_C_VTYPE, CB_SETCURSEL, 0, 0);
        }
        break;
    }
}

void LoadCond(HWND dialog, EditCondition *data)
{
	Condition *c = &data->c;

	SendDlgItemMessage(dialog, IDC_C_TYPE, CB_SETCURSEL, c->type, 0);
	SetDlgItemInt(dialog, IDC_C_TYPEVAL, c->type, TRUE);
	SendDlgItemMessage(dialog, IDC_C_PLAYER, CB_SETCURSEL, c->player + 1, 0);
	SetDlgItemInt(dialog, IDC_C_UIDOBJ, c->object, TRUE);
	SetDlgItemInt(dialog, IDC_C_UIDLOC, c->u_loc, TRUE);
	LCombo_Select(dialog, IDC_C_UCNST, c->pUnit);
	Combo_SelectByData(GetDlgItem(dialog, IDC_C_GROUP), c->group);
	Combo_SelectByData(GetDlgItem(dialog, IDC_C_UTYPE), c->utype);
	SetDlgItemInt(dialog, IDC_C_AREAX1, c->area.left, TRUE);
	SetDlgItemInt(dialog, IDC_C_AREAY1, c->area.bottom, TRUE);
	SetDlgItemInt(dialog, IDC_C_AREAX2, c->area.right, TRUE);
	SetDlgItemInt(dialog, IDC_C_AREAY2, c->area.top, TRUE);
	SetDlgItemInt(dialog, IDC_C_TIMER, c->timer, TRUE);
	SetDlgItemInt(dialog, IDC_C_AISIG, c->ai_signal, TRUE);
	LCombo_Select(dialog, IDC_C_RESEARCH, c->pTech);
	SetDlgItemInt(dialog, IDC_C_RESERVED, c->reserved, TRUE);
	SetDlgItemInt(dialog, IDC_C_AMOUNT, c->amount, TRUE);
	LCombo_SelById(dialog, IDC_C_RESTYPE, c->res_type);
	SetDlgItemInt(dialog, IDC_C_REVERSEHD, c->reverse_hd, TRUE);
	SetDlgItemInt(dialog, IDC_C_UNKNOWNHD, c->unknown2, TRUE);
	if (scen.game == UP || scen.game == ETP) {
	    if (c->reserved == -1) {
	        SendMessage(GetDlgItem(dialog, IDC_C_REVERSE), WM_SETTEXT, 0, (LPARAM) _T("��������"));
	    } else if (c->reserved == -256) {
	        SendMessage(GetDlgItem(dialog, IDC_C_REVERSE), WM_SETTEXT, 0, (LPARAM) _T("ȡ����������"));
	    } else {
	        SendMessage(GetDlgItem(dialog, IDC_C_REVERSE), WM_SETTEXT, 0, (LPARAM) _T("����"));
	    }
	} else if (scen.game == AOHD4 || scen.game == AOF4 || scen.game == AOHD6 || scen.game == AOF6) {
	    if (c->reverse_hd == 0) {
	        SendMessage(GetDlgItem(dialog, IDC_C_REVERSE), WM_SETTEXT, 0, (LPARAM) _T("��������"));
	    } else if (c->reverse_hd == 1) {
	        SendMessage(GetDlgItem(dialog, IDC_C_REVERSE), WM_SETTEXT, 0, (LPARAM) _T("ȡ����������"));
	    } else {
	        SendMessage(GetDlgItem(dialog, IDC_C_REVERSE), WM_SETTEXT, 0, (LPARAM) _T("����"));
	    }
	}
	LoadVirtualTypeConditions(dialog, data);
}

void SaveCond(HWND dialog, EditCondition *data)
{
	Condition *c = &data->c;

	int newtype = SendDlgItemMessage(dialog, IDC_C_TYPE, CB_GETCURSEL, 0, 0);
	if (newtype != CB_ERR)
	{
		c->type = newtype;
		c->pUnit = (UnitLink*)LCombo_GetSelPtr(dialog, IDC_C_UCNST);
		c->player = SendDlgItemMessage(dialog, IDC_C_PLAYER, CB_GETCURSEL, 0, 0) - 1;
		c->object = GetDlgItemInt(dialog, IDC_C_UIDOBJ, NULL, TRUE);
		c->u_loc = GetDlgItemInt(dialog, IDC_C_UIDLOC, NULL, TRUE);
		c->group = SendDlgItemMessage(dialog, IDC_C_GROUP, CB_GETCURSEL, 0, 0);
		c->area.left = GetDlgItemInt(dialog, IDC_C_AREAX1, NULL, TRUE);
		c->area.bottom = GetDlgItemInt(dialog, IDC_C_AREAY1, NULL, TRUE);
		c->area.right = GetDlgItemInt(dialog, IDC_C_AREAX2, NULL, TRUE);
		c->area.top = GetDlgItemInt(dialog, IDC_C_AREAY2, NULL, TRUE);
		c->timer = GetDlgItemInt(dialog, IDC_C_TIMER, NULL, TRUE);
		c->ai_signal = GetDlgItemInt(dialog, IDC_C_AISIG, NULL, TRUE);
		c->pTech = (TechLink*)LCombo_GetSelPtr(dialog, IDC_C_RESEARCH);
		c->amount = GetDlgItemInt(dialog, IDC_C_AMOUNT, NULL, TRUE);
		c->res_type = LCombo_GetSelId(dialog, IDC_C_RESTYPE);
		c->reserved = GetDlgItemInt(dialog, IDC_C_RESERVED, NULL, TRUE);
		c->reverse_hd = GetDlgItemInt(dialog, IDC_C_REVERSEHD, NULL, TRUE);
		c->unknown2 = GetDlgItemInt(dialog, IDC_C_UNKNOWNHD, NULL, TRUE);

		c->group = Combo_GetSelData(GetDlgItem(dialog, IDC_C_GROUP));
		c->utype = Combo_GetSelData(GetDlgItem(dialog, IDC_C_UTYPE));
	}
}

void C_HandleSetFocus(HWND dialog, WORD id)
{
	EditEC * data =
		static_cast<EditEC*>(GetDialogUserData_ptr(dialog));

	switch (id)
	{
	case IDC_C_AREAX1:
	case IDC_C_AREAY1:
	case IDC_C_AREAX2:
	case IDC_C_AREAY2:
		SendMessage(data->mapview, MAP_HighlightPoint,
			GetDlgItemInt(dialog, IDC_C_AREAX1, NULL, TRUE),
			GetDlgItemInt(dialog, IDC_C_AREAY1, NULL, TRUE));
		SendMessage(data->mapview, MAP_HighlightPoint,
			GetDlgItemInt(dialog, IDC_C_AREAX2, NULL, TRUE),
			GetDlgItemInt(dialog, IDC_C_AREAY2, NULL, TRUE));
		break;
	}
}

void C_HandleKillFocus(HWND dialog, WORD)
{
	EditEC * data =
		static_cast<EditEC*>(GetDialogUserData_ptr(dialog));

	if (data->mapview)
	{
		SendMessage(data->mapview, MAP_UnhighlightPoint,
			MAP_UNHIGHLIGHT_ALL, 0);
	}
}

void C_HandleChangeType(HWND dialog, EditCondition *data)
{
/*
	static const char *promptClearCondition =
		"Would you like to clear the condition? (Recommended)";

	if (data->c.type != ConditionType::None &&
		MessageBox(dialog, promptClearCondition, "Condition Editor", MB_YESNO) == IDYES)
	{
		// reset to default-constructor state
		data->c = Condition();
	}
*/
    // Assume the user wants to change to similar condition. Otherwise
    // would make new
	int newtype = SendDlgItemMessage(dialog, IDC_C_TYPE, CB_GETCURSEL, 0, 0);
	//if (data->c.type != newtype)
	//	data->c = Condition();
	data->c.type = newtype;
	ConditionControls(dialog, newtype);
	LoadCond(dialog, data);
}

void C_HandleChangeAIGoal(HWND dialog, EditCondition *data)
{
	int goal = GetDlgItemInt(dialog, IDC_C_AIGOAL, NULL, TRUE);

    if (goal != CB_ERR && goal >= 0) {
        data->c.ai_signal = -774 + goal;
	    SetDlgItemInt(dialog, IDC_C_AISIG, data->c.ai_signal, TRUE);
	} else {
	    SetDlgItemInt(dialog, IDC_C_AISIG, -744, TRUE);
    }
}

void C_HandleChangeVTypeTaunt(HWND dialog, EditCondition *data)
{
	int taunt_player = SendDlgItemMessage(dialog, IDC_C_TAUNT_PLAYER, CB_GETCURSEL, 0, 0);
	int taunt_set = SendDlgItemMessage(dialog, IDC_C_TAUNT_SET, CB_GETCURSEL, 0, 0);

    if (taunt_player != CB_ERR && taunt_set != CB_ERR) {
        data->c.ai_signal = -518 + taunt_player * 64 + taunt_set;

	    LoadCond(dialog, data);
    }
}

void C_HandleChangeVType(HWND dialog, EditCondition *data)
{
	int newtype = SendDlgItemMessage(dialog, IDC_C_VTYPE, CB_GETCURSEL, 0, 0);
	if (newtype == 0) {
	    ConditionControls(dialog, data->c.type);
	    return;
	}

	data->c = Condition();

    switch (scen.game) {
    case AOC:
    case ETP:
    case UP:
        switch (newtype) {
        case ConditionVirtualTypeAOC::SinglePlayer:
            data->c.ai_signal = -1034;
            data->c.type = ConditionType::AISignal;
	        ConditionControls(dialog, data->c.type);
		    ENABLE_WND(IDC_C_AISIG, false);
            break;
        case ConditionVirtualTypeAOC::Taunt:
            data->c.ai_signal = -518;
            data->c.type = ConditionType::AISignal;
	        ConditionControls(dialog, data->c.type);
		    ENABLE_WND(IDC_C_AISIG, false);
            break;
        case ConditionVirtualTypeAOC::AIScriptGoal:
            data->c.ai_signal = -774;
            data->c.type = ConditionType::AISignal;
	        ConditionControls(dialog, data->c.type);
		    ENABLE_WND(IDC_C_AISIG, false);
            break;
        case ConditionVirtualTypeAOC::StartingAgeStandard:
            data->c.ai_signal = -1035;
            data->c.type = ConditionType::AISignal;
	        ConditionControls(dialog, data->c.type);
		    ENABLE_WND(IDC_C_AISIG, false);
            break;
        case ConditionVirtualTypeAOC::StartingResourcesStandard:
            data->c.ai_signal = -1036;
            data->c.type = ConditionType::AISignal;
	        ConditionControls(dialog, data->c.type);
		    ENABLE_WND(IDC_C_AISIG, false);
            break;
        case ConditionVirtualTypeAOC::Regicide:
            data->c.ai_signal = -1039;
            data->c.type = ConditionType::AISignal;
	        ConditionControls(dialog, data->c.type);
		    ENABLE_WND(IDC_C_AISIG, false);
            break;
        case ConditionVirtualTypeAOC::Deathmatch:
            data->c.ai_signal = -1040;
            data->c.type = ConditionType::AISignal;
	        ConditionControls(dialog, data->c.type);
		    ENABLE_WND(IDC_C_AISIG, false);
            break;
        case ConditionVirtualTypeAOC::OneClickGarrison:
            data->c.ai_signal = -70850;
            data->c.type = ConditionType::AISignal;
	        ConditionControls(dialog, data->c.type);
		    ENABLE_WND(IDC_C_AISIG, false);
            break;
        case ConditionVirtualTypeAOC::DifficultyLevelHardest:
            data->c.type = ConditionType::DifficultyLevel;
            data->c.amount = DifficultyLevel::Hardest;
	        ConditionControls(dialog, data->c.type);
            break;
        case ConditionVirtualTypeAOC::DifficultyLevelHard:
            data->c.type = ConditionType::DifficultyLevel;
            data->c.amount = DifficultyLevel::Hard;
	        ConditionControls(dialog, data->c.type);
            break;
        case ConditionVirtualTypeAOC::DifficultyLevelModerate:
            data->c.type = ConditionType::DifficultyLevel;
            data->c.amount = DifficultyLevel::Moderate;
	        ConditionControls(dialog, data->c.type);
            break;
        case ConditionVirtualTypeAOC::DifficultyLevelStandard:
            data->c.type = ConditionType::DifficultyLevel;
            data->c.amount = DifficultyLevel::Standard;
	        ConditionControls(dialog, data->c.type);
            break;
        case ConditionVirtualTypeAOC::DifficultyLevelEasiest:
            data->c.type = ConditionType::DifficultyLevel;
            data->c.amount = DifficultyLevel::Easiest;
	        ConditionControls(dialog, data->c.type);
            break;
        }
        break;
    default:
        switch (newtype) {
        case ConditionVirtualTypeDefault::DifficultyLevelHardest:
            data->c.type = ConditionType::DifficultyLevel;
            data->c.amount = DifficultyLevel::Hardest;
	        ConditionControls(dialog, data->c.type);
            break;
        case ConditionVirtualTypeDefault::DifficultyLevelHard:
            data->c.type = ConditionType::DifficultyLevel;
            data->c.amount = DifficultyLevel::Hard;
	        ConditionControls(dialog, data->c.type);
            break;
        case ConditionVirtualTypeDefault::DifficultyLevelModerate:
            data->c.type = ConditionType::DifficultyLevel;
            data->c.amount = DifficultyLevel::Moderate;
	        ConditionControls(dialog, data->c.type);
            break;
        case ConditionVirtualTypeDefault::DifficultyLevelStandard:
            data->c.type = ConditionType::DifficultyLevel;
            data->c.amount = DifficultyLevel::Standard;
	        ConditionControls(dialog, data->c.type);
            break;
        case ConditionVirtualTypeDefault::DifficultyLevelEasiest:
            data->c.type = ConditionType::DifficultyLevel;
            data->c.amount = DifficultyLevel::Easiest;
	        ConditionControls(dialog, data->c.type);
            break;
        default:
	        ConditionControls(dialog, data->c.type);
	        // change to:
	        //VirtualConditionControls(dialog, data->c.type);
        }
    }

	LoadCond(dialog, data);
}

const char warnInvalidC[] =
"ע�⣬�������ƺ�����Ч�ġ�";

void C_HandleCommand(HWND dialog, WORD id, WORD code, HWND)
{
	EditCondition *data = (EditCondition*)GetWindowLongPtr(dialog, DWLP_USER);

	switch (code)
	{
	case BN_CLICKED:
		switch (id)
		{
		case IDC_C_REVERSE:
		    if (scen.game == UP || scen.game == ETP) {
	            if (GetDlgItemInt(dialog, IDC_C_RESERVED, NULL, TRUE) == -1) {
	                SetDlgItemInt(dialog, IDC_C_RESERVED, -256, TRUE);
	                SendMessage(GetDlgItem(dialog, IDC_C_REVERSE), WM_SETTEXT, 0, (LPARAM) _T("ȡ����������"));
	            } else {
	                SetDlgItemInt(dialog, IDC_C_RESERVED, -1, TRUE);
	                SendMessage(GetDlgItem(dialog, IDC_C_REVERSE), WM_SETTEXT, 0, (LPARAM) _T("��������"));
	            }
	        } else if (scen.game == AOHD4 || scen.game == AOF4 || scen.game == AOHD6 || scen.game == AOF6) {
	            if (GetDlgItemInt(dialog, IDC_C_REVERSEHD, NULL, TRUE) == 0) {
	                SetDlgItemInt(dialog, IDC_C_REVERSEHD, 1, TRUE);
	                SendMessage(GetDlgItem(dialog, IDC_C_REVERSE), WM_SETTEXT, 0, (LPARAM) _T("ȡ����������"));
	            } else {
	                SetDlgItemInt(dialog, IDC_C_REVERSEHD, 0, TRUE);
	                SendMessage(GetDlgItem(dialog, IDC_C_REVERSE), WM_SETTEXT, 0, (LPARAM) _T("��������"));
	            }
	        }
	        break;
		case IDC_C_AREA_ALL:
			{
				SetDlgItemInt(dialog, IDC_C_AREAX1, -1, TRUE);
				SetDlgItemInt(dialog, IDC_C_AREAY1, -1, TRUE);
				SetDlgItemInt(dialog, IDC_C_AREAX2, -1, TRUE);
				SetDlgItemInt(dialog, IDC_C_AREAY2, -1, TRUE);
			}
			break;
		case IDC_C_CLEAR:
		    {
			    data->c = Condition();
			    LoadCond(dialog, data);
			}
			break;
	    case IDOK:
	        {
		        bool valid;
		        int ret = IDOK;

		        SaveCond(dialog, data);	//update type
		        valid = data->c.check_and_save();

		        if (!valid && !setts.editall)
			        ret = MessageBox(dialog, warnInvalidC, "�����༭��", MB_OKCANCEL);

		        if (ret == IDOK)
		        {
			        SendMessage(data->parent, EC_Closing,
				        MAKELONG(1, valid), reinterpret_cast<LPARAM>(data));
			        DestroyWindow(dialog);
		        }
	        }
	        break;

	    case IDCANCEL:
	        {
		        SendMessage(data->parent, EC_Closing,
			        0, reinterpret_cast<LPARAM>(data));
		        DestroyWindow(dialog);
	        }
	        break;

	    case IDC_C_USEL1:
	        {
		        if (SingleUnitSelDialogBox(dialog, data->players,
			        data->c.object, data->c.object != -1))
		        {
			        SetDlgItemInt(dialog, IDC_C_UIDOBJ, data->c.object, FALSE);
		        }
	        }
		    break;

	    case IDC_C_USEL2:
	        {
		        if (SingleUnitSelDialogBox(dialog, data->players,
			        data->c.u_loc, data->c.u_loc != -1))
		        {
			        SetDlgItemInt(dialog, IDC_C_UIDLOC, data->c.u_loc, FALSE);
		        }
	        }
		    break;

		}
		break;

	case EN_CHANGE:
		switch (id)
		{
		case IDC_C_AIGOAL:
		    C_HandleChangeAIGoal(dialog, data);
		    break;
		}
		break;

    case CBN_SELCHANGE:
	    switch (id)
	    {
	    case IDC_C_TYPE:
		    C_HandleChangeType(dialog, data);
		    break;

		case IDC_C_VTYPE:
			C_HandleChangeVType(dialog, data);
			break;

		case IDC_C_TAUNT_PLAYER:
		case IDC_C_TAUNT_SET:
			C_HandleChangeVTypeTaunt(dialog, data);
			break;
	    }
		break;

	case EN_SETFOCUS:
		C_HandleSetFocus(dialog, id);
		break;

	case EN_KILLFOCUS:
		C_HandleKillFocus(dialog, id);
		break;

	}
}

const char errorNoDataC[] =
"û�����ݣ����ܽ����𻵡�";

INT_PTR CALLBACK CondWndProc(HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR ret = FALSE;

	switch (msg)
	{

	case WM_INITDIALOG:
		{
			class EditCondition *data = (EditCondition*)lParam;

			C_Init(dialog);

			if (data)
			{
				SetWindowLongPtr(dialog, DWLP_USER, lParam);
				ConditionControls(dialog, data->c.type);
				LoadCond(dialog, data);

				ret = TRUE;
			}
			else
			{
				MessageBox(dialog, errorNoDataC, "����", MB_ICONWARNING);
				DestroyWindow(dialog);

				ret = FALSE;
			}
		}
		break;

	case WM_COMMAND:
		ret = 0;
		C_HandleCommand(dialog, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
		break;

	case WM_DESTROY:
		{
			class EditCondition *data = (EditCondition*)GetWindowLongPtr(dialog, DWLP_USER);
			delete data;
		}
		break;

	case EC_Update:
		{
			class EditCondition *data = (EditCondition*)GetWindowLongPtr(dialog, DWLP_USER);
			data->index += wParam;
		}
	}

	return ret;
}
