/**
 * @file cp_uforecovery_callbacks.c
 * @brief UFO recovery and storing callback functions
 * @note UFO recovery functions with UR_*
 * @note UFO storing functions with US_*
 */

/*
Copyright (C) 2002-2010 UFO: Alien Invasion.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "../cl_shared.h"
#include "cp_campaign.h"
#include "cp_ufo.h"
#include "cp_uforecovery.h"
#include "cp_uforecovery_callbacks.h"

#include "../ui/ui_main.h"

#ifdef DEBUG
#include "cp_time.h"
#endif

/**
 * @brief Entries on Sell UFO dialog
 */
typedef struct ufoRecoveryNation_s {
	nation_t *nation;
	int price;										/**< price proposed by nation. */
} ufoRecoveryNation_t;

/**
 * @brief Pointer to compare function
 * @note This function is used by sorting algorithm.
 */
typedef int (*COMP_FUNCTION)(ufoRecoveryNation_t *a, ufoRecoveryNation_t *b);

/**
 * @brief Constants for Sell UFO menu order
 */
typedef enum {
	ORDER_NATION,
	ORDER_PRICE,
	ORDER_HAPPINESS,

	MAX_ORDER
} ufoRecoveryNationOrder_t;

/** @sa ufoRecoveries_t */
typedef struct ufoRecovery_s {
	installation_t *installation;					/**< selected ufoyard for current selected ufo recovery */
	const aircraft_t *ufoTemplate;					/**< the ufo type of the current ufo recovery */
	nation_t *nation;								/**< selected nation to sell to for current ufo recovery */
	qboolean recoveryDone;							/**< recoveryDone? Then the buttons are disabled */
	float condition;								/**< How much the UFO is damaged - used for disassembies */
	ufoRecoveryNation_t UFONations[MAX_NATIONS];	/**< Sorted array of nations. */
	ufoRecoveryNationOrder_t sortedColumn;			/**< Column sell sorted by */
	qboolean sortDescending;						/**< ascending (qfalse) / descending (qtrue) */
} ufoRecovery_t;

static ufoRecovery_t ufoRecovery;

/**
 * @brief Updates UFO recovery process.
 */
static void UR_DialogRecoveryDone (void)
{
	ufoRecovery.recoveryDone = qtrue;
}

/**
 * @brief Function to trigger UFO Recovered event.
 * @note This function prepares related cvars for the recovery dialog.
 * @note Command to call this: cp_uforecovery_init.
 */
static void UR_DialogInit_f (void)
{
	char ufoID[MAX_VAR];
	const aircraft_t *ufoCraft;
	float cond = 1.0f;

	if (Cmd_Argc() < 2) {
		Com_Printf("Usage: %s <ufoID> [UFO-Condition]\n", Cmd_Argv(0));
		return;
	}

	Q_strncpyz(ufoID, Cmd_Argv(1), sizeof(ufoID));

	if (Cmd_Argc() >= 3) {
		cond = atof(Cmd_Argv(2));
	}

	ufoCraft = AIR_GetAircraft(ufoID);

	/* Put relevant info into missionResults array. */
	ccs.missionResults.recovery = qtrue;
	ccs.missionResults.ufoCondition = cond;
	ccs.missionResults.crashsite = (cond < 1);
	ccs.missionResults.ufotype = ufoCraft->ufotype;
	/* Prepare related cvars. */
	Cvar_SetValue("mission_uforecovered", 1);	/* This is used in menus to enable UFO Recovery nodes. */
	/* Fill ufoRecovery structure */
	memset(&ufoRecovery, 0, sizeof(ufoRecovery));
	ufoRecovery.ufoTemplate = ufoCraft;
	ufoRecovery.condition = cond;
	ufoRecovery.sortedColumn = ORDER_NATION;

	Cvar_Set("mn_uforecovery_actualufo", UFO_MissionResultToString());
}

/**
 * @brief Function to initialize list of storage locations for recovered UFO.
 * @note Command to call this: cp_uforecovery_store_init.
 * @sa UR_ConditionsForStoring
 */
static void UR_DialogInitStore_f (void)
{
	int i;
	int count = 0;
	linkedList_t *recoveryYardName = NULL;
	linkedList_t *recoveryYardCapacity = NULL;
	static char cap[MAX_VAR];

	/* Do nothing if recovery process is finished. */
	if (ufoRecovery.recoveryDone)
		return;

	/* Check how many bases can store this UFO. */
	for (i = 0; i < ccs.numInstallations; i++) {
		const installation_t *installation = INS_GetFoundedInstallationByIDX(i);

		if (!installation)
			continue;

		if (installation->ufoCapacity.max > 0
		 && installation->ufoCapacity.max > installation->ufoCapacity.cur) {

			Com_sprintf(cap, lengthof(cap), "%i/%i", (installation->ufoCapacity.max - installation->ufoCapacity.cur), installation->ufoCapacity.max);
			LIST_AddString(&recoveryYardName, installation->name);
			LIST_AddString(&recoveryYardCapacity, cap);
			count++;
		}
	}

	Cvar_Set("mn_uforecovery_actualufo", UFO_MissionResultToString());
	if (count == 0) {
		/* No UFO base with proper conditions, show a hint and disable list. */
		LIST_AddString(&recoveryYardName, _("No free UFO yard available."));
		UI_ExecuteConfunc("uforecovery_tabselect sell");
		UI_ExecuteConfunc("btbasesel disable");
	} else {
		UI_ExecuteConfunc("cp_basesel_select 0");
		UI_ExecuteConfunc("btbasesel enable");
	}
	UI_RegisterLinkedListText(TEXT_UFORECOVERY_UFOYARDS, recoveryYardName);
	UI_RegisterLinkedListText(TEXT_UFORECOVERY_CAPACITIES, recoveryYardCapacity);
}

/**
 * @brief Function to start UFO recovery process.
 * @note Command to call this: cp_uforecovery_store_start.
 */
static void UR_DialogStartStore_f (void)
{
	installation_t *UFOYard = NULL;
	int idx;
	int i;
	int count = 0;
	date_t date;

	if (Cmd_Argc() < 2) {
		Com_Printf("Usage: %s <installationIDX>\n", Cmd_Argv(0));
		return;
	}

	idx = atoi(Cmd_Argv(1));

	for (i = 0; i < ccs.numInstallations; i++) {
		installation_t *installation = INS_GetFoundedInstallationByIDX(i);

		if (!installation)
			continue;

		if (installation->ufoCapacity.max <= 0
		 || installation->ufoCapacity.max <= installation->ufoCapacity.cur)
		 	continue;

		if (count == idx) {
			UFOYard = installation;
			break;
		}
		count++;
	}

	if (!UFOYard)
		return;

	Com_sprintf(cp_messageBuffer, lengthof(cp_messageBuffer), _("Recovered %s from the battlefield. UFO is being transported to %s."),
			UFO_TypeToName(ufoRecovery.ufoTemplate->ufotype), UFOYard->name);
	MS_AddNewMessage(_("UFO Recovery"), cp_messageBuffer, qfalse, MSG_STANDARD, NULL);
	date = ccs.date;
	date.day += (int) RECOVERY_DELAY;

	US_StoreUFO(ufoRecovery.ufoTemplate, UFOYard, date, ufoRecovery.condition);
	UR_DialogRecoveryDone();
}

/**
 * @brief Build the Sell UFO dialog's nationlist
 */
static void UR_DialogFillNations (void)
{
	int i;
	linkedList_t *nationList = NULL;

	for (i = 0; i < ccs.numNations; i++) {
		const nation_t *nation = ufoRecovery.UFONations[i].nation;
		if (nation) {
			char row[512];
			Com_sprintf(row, lengthof(row), "%s\t\t\t%i\t\t%s", _(nation->name),
				ufoRecovery.UFONations[i].price, NAT_GetHappinessString(nation));
			LIST_AddString(&nationList, row);
		}
	}

	UI_RegisterLinkedListText(TEXT_UFORECOVERY_NATIONS, nationList);
}

/**
 * @brief Compare nations by nation name.
 * @param[in] a First item to compare
 * @param[in] b Second item to compare
 * @sa UR_SortNations
 */
static int UR_CompareByName (ufoRecoveryNation_t *a, ufoRecoveryNation_t *b)
{
	return strcmp(_(a->nation->name), _(b->nation->name));
}

/**
 * @brief Compare nations by price.
 * @param[in] a First item to compare
 * @param[in] b Second item to compare
 * @return 1 if a > b
 * @return -1 if b > a
 * @return 0 if a == b
 * @sa UR_SortNations
 */
static int UR_CompareByPrice (ufoRecoveryNation_t *a, ufoRecoveryNation_t *b)
{
	if (a->price > b->price)
		return 1;
	if (a->price < b->price)
		return -1;
	return 0;
}

/**
 * @brief Compare nations by happiness.
 * @param[in] a First item to compare
 * @param[in] b Second item to compare
 * @return 1 if a > b
 * @return -1 if b > a
 * @return 0 if a == b
 * @sa UR_SortNations
 */
static int UR_CompareByHappiness (ufoRecoveryNation_t *a, ufoRecoveryNation_t *b)
{
	const nationInfo_t *statsA = &a->nation->stats[0];
	const nationInfo_t *statsB = &b->nation->stats[0];

	if (statsA->happiness > statsB->happiness)
		return 1;
	if (statsA->happiness < statsB->happiness)
		return -1;
	return 0;
}

/**
 * @brief Sort nations
 * @note uses Bubble sort algorithm
 * @param[in] comp Compare function
 * @param[in] order Ascending/Descending order
 * @sa UR_CompareByName
 * @sa UR_CompareByPrice
 * @sa UR_CompareByHappiness
 */
static void UR_SortNations (COMP_FUNCTION comp, qboolean order)
{
	int i;

	for (i = 0; i < ccs.numNations; i++) {
		qboolean swapped = qfalse;
		int j;

		for (j = 0; j < ccs.numNations - 1; j++) {
			int value = (*comp)(&ufoRecovery.UFONations[j], &ufoRecovery.UFONations[j + 1]);
			ufoRecoveryNation_t tmp;

			if (order)
				value *= -1;
			if (value > 0) {
				/* swap nations */
				tmp = ufoRecovery.UFONations[j];
				ufoRecovery.UFONations[j] = ufoRecovery.UFONations[j + 1];
				ufoRecovery.UFONations[j + 1] = tmp;
				swapped = qtrue;
			}
		}
		if (!swapped)
			break;
	}
}

/**
 * @brief Returns the sort function for a column
 * @param[in] column Column ordertype.
 */
static COMP_FUNCTION UR_GetSortFunctionByColumn (ufoRecoveryNationOrder_t column)
{
	switch (column) {
	case ORDER_NATION:
		return UR_CompareByName;
	case ORDER_PRICE:
		return UR_CompareByPrice;
	case ORDER_HAPPINESS:
		return UR_CompareByHappiness;
	default:
		Com_Printf("UR_DialogSortByColumn_f: Invalid sort option!\n");
		return NULL;
	}
}

/**
 * @brief Function to initialize list to sell recovered UFO to desired nation.
 * @note Command to call this: cp_uforecovery_sell_init.
 */
static void UR_DialogInitSell_f (void)
{
	int i;

	/* Do nothing if recovery process is finished. */
	if (ufoRecovery.recoveryDone)
		return;
	/* Do nothing without a ufoTemplate set */
	if (!ufoRecovery.ufoTemplate)
		return;

	for (i = 0; i < ccs.numNations; i++) {
		nation_t *nation = NAT_GetNationByIDX(i);
		int price;

		price = (int) (ufoRecovery.ufoTemplate->price * (.85f + frand() * .3f));
		/* Nation will pay less if corrupted */
		price = (int) (price * exp(-nation->stats[0].xviInfection / 20.0f));

		ufoRecovery.UFONations[i].nation = nation;
		ufoRecovery.UFONations[i].price = price;
	}
	UR_SortNations(UR_GetSortFunctionByColumn(ufoRecovery.sortedColumn), ufoRecovery.sortDescending);
	UR_DialogFillNations();
	UI_ExecuteConfunc("btnatsel disable");
}

/**
 * @brief Returns the index of the selected nation in SellUFO list
 */
static int UR_DialogGetCurrentNationIndex (void)
{
	int i;

	for (i = 0; i < ccs.numNations; i++)
		if (ufoRecovery.UFONations[i].nation == ufoRecovery.nation)
			return i;
	return -1;
}

/**
 * @brief Converts script id string to ordercolumn constant
 * @param[in] id Script id for order column
 * @sa ufoRecoveryNationOrder_t
 */
static ufoRecoveryNationOrder_t UR_GetOrderTypeByID (const char *id)
{
	if (!id)
		return MAX_ORDER;
	if (!strcmp(id, "nation"))
		return ORDER_NATION;
	if (!strcmp(id, "price"))
		return ORDER_PRICE;
	if (!strcmp(id, "happiness"))
		return ORDER_HAPPINESS;
	return MAX_ORDER;
}

/**
 * @brief Sort Sell UFO dialog
 */
static void UR_DialogSortByColumn_f (void)
{
	COMP_FUNCTION comp = 0;
	ufoRecoveryNationOrder_t column;

	if (Cmd_Argc() < 2) {
		Com_Printf("Usage: %s <nation|price|happiness>\n", Cmd_Argv(0));
		return;
	}

	column = UR_GetOrderTypeByID(Cmd_Argv(1));
	if (ufoRecovery.sortedColumn != column) {
		ufoRecovery.sortDescending = qfalse;
		ufoRecovery.sortedColumn = column;
	} else {
		ufoRecovery.sortDescending = !ufoRecovery.sortDescending;
	}

	comp = UR_GetSortFunctionByColumn(column);
	if (comp) {
		int index;

		UR_SortNations(comp, ufoRecovery.sortDescending);
		UR_DialogFillNations();

		/* changed line selection corresponding to current nation */
		index = UR_DialogGetCurrentNationIndex();
		if (index != -1)
			UI_ExecuteConfunc("cp_nationsel_select %d", index);
	}
}

/**
 * @brief Finds the nation to which recovered UFO will be sold.
 * @note The nation selection is being done here.
 * @note Callback command: cp_uforecovery_nationlist_click.
 */
static void UR_DialogSelectSellNation_f (void)
{
	int num;
	nation_t *nation;

	if (Cmd_Argc() < 2) {
		Com_Printf("Usage: %s <nationid>\n", Cmd_Argv(0));
		return;
	}

	num = atoi(Cmd_Argv(1));

	/* don't do anything if index is higher than visible nations */
	if (0 > num || num >= ccs.numNations)
		return;

	nation = ufoRecovery.UFONations[num].nation;

	ufoRecovery.nation = nation;
	Com_DPrintf(DEBUG_CLIENT, "CP_UFORecoveryNationSelectPopup_f: picked nation: %s\n", nation->name);

	Cvar_Set("mission_recoverynation", _(nation->name));
	UI_ExecuteConfunc("btnatsel enable");
}

/**
 * @brief Function to start UFO selling process.
 * @note Command to call this: cp_uforecovery_sell_start.
 */
static void UR_DialogStartSell_f (void)
{
	int price = -1;
	nation_t *nation;
	int i;

	if (!ufoRecovery.nation)
		return;

	nation = ufoRecovery.nation;
	assert(nation->name);

	i = UR_DialogGetCurrentNationIndex();
	price = ufoRecovery.UFONations[i].price;

	assert(price >= 0);
#if 0
	if (ufoRecovery.selectedStorage) {
		Com_sprintf(cp_messageBuffer, sizeof(cp_messageBuffer), _("Sold previously recovered %s from %s to nation %s, gained %i credits."), UFO_TypeToName(
				ufoRecovery.selectedStorage->ufoTemplate->ufotype), ufoRecovery.selectedStorage->base->name, _(nation->name), price);
	} else
#endif
	{
		Com_sprintf(cp_messageBuffer, sizeof(cp_messageBuffer), _("Recovered %s from the battlefield. UFO sold to nation %s, gained %i credits."), UFO_TypeToName(
				ufoRecovery.ufoTemplate->ufotype), _(nation->name), price);
	}
	MS_AddNewMessage(_("UFO Recovery"), cp_messageBuffer, qfalse, MSG_STANDARD, NULL);
	CL_UpdateCredits(ccs.credits + price);

	/* update nation happiness */
	for (i = 0; i < ccs.numNations; i++) {
		nation_t *nat = NAT_GetNationByIDX(i);

		assert(nat);
		if (nat == nation)
			/* nation is happy because it got the UFO */
			NAT_SetHappiness(nation, nation->stats[0].happiness + HAPPINESS_UFO_SALE_GAIN);
		else
			/* nation is unhappy because it wanted the UFO */
			NAT_SetHappiness(nat, nat->stats[0].happiness + HAPPINESS_UFO_SALE_LOSS);
	}

	/* UFO recovery process is done, disable buttons. */
	UR_DialogRecoveryDone();
}


/* === UFO Storing === */

#ifdef DEBUG
/**
 * @brief Debug callback to list ufostores
 */
static void US_ListStoredUFOs_f (void)
{
	int i;

	Com_Printf("Number of stored UFOs: %i\n", ccs.numStoredUFOs);
	for (i = 0; i < ccs.numStoredUFOs; i++) {
		const storedUFO_t *ufo = US_GetStoredUFOByIDX(i);
		const base_t *prodBase = PR_ProductionBase(ufo->disassembly);
		dateLong_t date;

		Com_Printf("IDX: %i\n", ufo->idx);
		Com_Printf("id: %s\n", ufo->id);
		Com_Printf("stored at %s\n", (ufo->installation) ? ufo->installation->name : "NOWHERE");

		CL_DateConvertLong(&(ufo->arrive), &date);
		Com_Printf("arrived at: %i %s %02i, %02i:%02i\n", date.year,
		Date_GetMonthName(date.month - 1), date.day, date.hour, date.min);

		if (ufo->ufoTemplate->tech->base)
			Com_Printf("tech being researched at %s\n", ufo->ufoTemplate->tech->base->name);
		if (prodBase)
			Com_Printf("being disassembled at %s\n", prodBase->name);
	}
}

/**
 * @brief Adds an UFO to the stores
 */
static void US_StoreUFO_f (void)
{
	char ufoId[MAX_VAR];
	int installationIDX;
	installation_t *installation;
	aircraft_t *ufoType = NULL;
	int i;

	if (Cmd_Argc() <= 2) {
		Com_Printf("Usage: %s <ufoType> <installationIdx>\n", Cmd_Argv(0));
		return;
	}

	Q_strncpyz(ufoId, Cmd_Argv(1), sizeof(ufoId));
	installationIDX = atoi(Cmd_Argv(2));

	/* Get The UFO Yard */
	if (installationIDX < 0 || installationIDX >= MAX_INSTALLATIONS) {
		Com_Printf("US_StoreUFO_f: Invalid Installation index.\n");
		return;
	}
	installation = INS_GetFoundedInstallationByIDX(installationIDX);
	if (!installation) {
		Com_Printf("US_StoreUFO_f: There is no Installation: idx=%i.\n", installationIDX);
		return;
	}

	/* Get UFO Type */
	for (i = 0; i < ccs.numAircraftTemplates; i++) {
		if (strstr(ccs.aircraftTemplates[i].id, ufoId)) {
			ufoType = &ccs.aircraftTemplates[i];
			break;
		}
	}
	if (ufoType == NULL) {
		Com_Printf("US_StoreUFO_f: In valid UFO Id.\n");
		return;
	}

	US_StoreUFO(ufoType, installation, ccs.date, 1.0f);
}

/**
 * @brief Removes an UFO from the stores
 */
static void US_RemoveStoredUFO_f (void)
{
	if (Cmd_Argc() < 2) {
		Com_Printf("Usage: %s <idx>\n", Cmd_Argv(0));
		return;
	} else {
		const int idx = atoi(Cmd_Argv(1));
		storedUFO_t *storedUFO = US_GetStoredUFOByIDX(idx);
		if (!storedUFO) {
			Com_Printf("US_RemoveStoredUFO_f: No such ufo index.\n");
			return;
		}
		US_RemoveStoredUFO(storedUFO);
	}
}

#endif

void UR_InitCallbacks (void)
{
	Cmd_AddCommand("cp_uforecovery_init", UR_DialogInit_f, "Function to trigger UFO Recovered event");
	Cmd_AddCommand("cp_uforecovery_sell_init", UR_DialogInitSell_f, "Function to initialize sell recovered UFO to desired nation.");
	Cmd_AddCommand("cp_uforecovery_store_init", UR_DialogInitStore_f, "Function to initialize store recovered UFO in desired base.");
	Cmd_AddCommand("cp_uforecovery_nationlist_click", UR_DialogSelectSellNation_f, "Callback for recovery sell to nation list.");
	Cmd_AddCommand("cp_uforecovery_store_start", UR_DialogStartStore_f, "Function to start UFO recovery processing.");
	Cmd_AddCommand("cp_uforecovery_sell_start", UR_DialogStartSell_f, "Function to start UFO selling processing.");
	Cmd_AddCommand("cp_uforecovery_sort", UR_DialogSortByColumn_f, "Sorts nations and update ui state.");
#ifdef DEBUG
	Cmd_AddCommand("debug_liststoredufos", US_ListStoredUFOs_f, "Debug function to list UFOs in Hangars.");
	Cmd_AddCommand("debug_storeufo", US_StoreUFO_f, "Debug function to Add UFO to Hangars.");
	Cmd_AddCommand("debug_removestoredufo", US_RemoveStoredUFO_f, "Debug function to Remove UFO from Hangars.");
#endif

	Cvar_Set("mn_uforecovery_actualufo", "");
}

void UR_ShutdownCallbacks (void)
{
	Cmd_RemoveCommand("cp_uforecovery_init");
	Cmd_RemoveCommand("cp_uforecovery_sell_init");
	Cmd_RemoveCommand("cp_uforecovery_store_init");
	Cmd_RemoveCommand("cp_uforecovery_nationlist_click");
	Cmd_RemoveCommand("cp_uforecovery_store_start");
	Cmd_RemoveCommand("cp_uforecovery_sell_start");
	Cmd_RemoveCommand("cp_uforecovery_sort");
#ifdef DEBUG
	Cmd_RemoveCommand("debug_liststoredufos");
	Cmd_RemoveCommand("debug_storeufo");
	Cmd_RemoveCommand("debug_removestoredufo");
#endif

	Cvar_Delete("mn_uforecovery_actualufo");
}
