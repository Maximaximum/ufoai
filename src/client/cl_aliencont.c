/**
 * @file cl_aliencont.c
 * @brief Deals with the alien containment stuff
 */

/*
Copyright (C) 2002-2006 UFO: Alien Invasion team.

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

#include "client.h"
#include "cl_global.h"
static char aliencontText[1024];
aliensTmp_t cargo[MAX_CARGO];

/*
Collecting aliens functions
*/

/*
 * @brief Collecting stunned aliens and alien bodies after the mission
 */
void CL_CollectingAliens(void)
{
	int i, j;
	int alienTypeNum = 0;

	le_t *le = NULL;
	teamDesc_t *td = NULL;
	aliensTmp_t *cargo = NULL;

	/* uncomment this if you want to test Alien Containment (WIP) stuff
	cargo = &gd.cargo[MAX_CARGO];
	*/
	td = teamDesc;

	for (i = 0, le = LEs; i < numLEs; i++, le++) {
		if (!le->inuse)
			continue;
		if ((le->type == ET_ACTOR || le->type == ET_UGV) && le->team == TEAM_ALIEN) {
			if (le->HP <= 0) {
				/* count alien bodies */

				/* NOTE:
				This function should be called by CL_ParseResults() in cl_team.c in place
				of already called CL_CollectAliens there (just rename CL_CollectAliens to
				CL_CollectingAliens in cl_team.c.
				Exactly at this point i need to get alien type (Ortnok, Antarean, etc)
				as char[] (or enum, does not matter);
				for now i use td->name, but unfortunately that points to team name,
				not alien type, so it is useless for now
				*/
				
				j = 0;
				while(j < alienTypeNum) {
					/* search alien type and increase amount */
					if(Q_strncmp(cargo[j].alientype, td->name, MAX_VAR) == 0) {
					++cargo[j].amount;
					Com_Printf("Counting: %s count: %i\n", cargo[j].alientype, cargo[j].amount);
					break;
					}
					j++;
				}
				if(j == alienTypeNum) {
					/* otherwise add new alien type */
					Q_strncpyz(cargo[j].alientype, td->name, MAX_VAR);
					alienTypeNum++;
					Com_Printf("Adding: %s count: %i\n", cargo[j].alientype, cargo[j].amount);
				}
			}
		}
	}

	/* print all of them */
	j = 0;
	while(j < alienTypeNum) {
		Com_Printf("Collecting alien bodies... type: %s amount: %i\n", cargo[j].alientype, cargo[j].amount+1);
		j++;
	}
}

/*
Menu functions
*/

/**
 * @brief Alien containment menu init function
 * @note Command to call this: aliencont_init
 * @note Should be called whenever the research menu gets active
 */
void AC_Init (void)
{
	menuText[TEXT_STANDARD] = aliencontText;
}

/**
 * @brief Defines commands and cvars for the alien containment menu(s)
 * @sa MN_ResetMenus
 */
void AC_Reset (void)
{
	/* add commands */
	Cmd_AddCommand("aliencont_init", AC_Init, "Init function for alien containment menu");

	/* add cvars */
	/* TODO */

	memset(aliencontText, 0, sizeof(aliencontText));
}
