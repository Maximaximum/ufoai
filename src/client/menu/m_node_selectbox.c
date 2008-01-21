/**
 * @file m_node_selectbox.c
 */

/*
Copyright (C) 1997-2008 UFO:AI Team

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

#include "../client.h"
#include "m_main.h"
#include "m_parse.h"
#include "m_font.h"

menuNode_t *selectBoxNode = NULL;

/**
 * @brief Adds a new selectbox option to a selectbox node
 * @sa MN_SELECTBOX
 * @return NULL if menuSelectBoxes is 'full' - otherwise pointer to the selectBoxOption
 * @param[in] node The node (must be of type MN_SELECTBOX) where you want to append
 * the option
 * @note You have to add the values manually to the option pointer
 */
selectBoxOptions_t* MN_AddSelectboxOption (menuNode_t *node)
{
	selectBoxOptions_t *selectBoxOption;

	assert(node->type == MN_SELECTBOX);

	if (mn.numSelectBoxes >= MAX_SELECT_BOX_OPTIONS) {
		Com_Printf("MN_AddSelectboxOption: numSelectBoxes exceeded - increase MAX_SELECT_BOX_OPTIONS\n");
		return NULL;
	}
	/* initial options entry */
	if (!node->options)
		node->options = &mn.menuSelectBoxes[mn.numSelectBoxes];
	else {
		/* link it in */
		for (selectBoxOption = node->options; selectBoxOption->next; selectBoxOption = selectBoxOption->next);
		selectBoxOption->next = &mn.menuSelectBoxes[mn.numSelectBoxes];
		selectBoxOption->next->next = NULL;
	}
	selectBoxOption = &mn.menuSelectBoxes[mn.numSelectBoxes];
	node->height++;
	mn.numSelectBoxes++;
	return selectBoxOption;
}

void MN_NodeSelectBoxInit (void)
{
	selectBoxNode = NULL;
}

void MN_DrawSelectBoxNode (const menuNode_t *node, const char *image)
{
	selectBoxOptions_t* selectBoxOption;
	int selBoxX, selBoxY;
	const char *ref;
	const char *font;

	if (!image)
		image = "menu/selectbox";

	ref = MN_GetReferenceString(node->menu, node->data[MN_DATA_MODEL_SKIN_OR_CVAR]);

	font = MN_GetFont(node->menu, node);
	selBoxX = node->pos[0] + SELECTBOX_SIDE_WIDTH;
	selBoxY = node->pos[1] + SELECTBOX_SPACER;

	/* left border */
	R_DrawNormPic(node->pos[0], node->pos[1], SELECTBOX_SIDE_WIDTH, node->size[1],
		SELECTBOX_SIDE_WIDTH, 20.0f, 0.0f, 0.0f, node->align, node->blend, image);
	/* stretched middle bar */
	R_DrawNormPic(node->pos[0] + SELECTBOX_SIDE_WIDTH, node->pos[1], node->size[0], node->size[1],
		12.0f, 20.0f, 7.0f, 0.0f, node->align, node->blend, image);
	/* right border (arrow) */
	R_DrawNormPic(node->pos[0] + SELECTBOX_SIDE_WIDTH + node->size[0], node->pos[1], SELECTBOX_DEFAULT_HEIGHT, node->size[1],
		32.0f, 20.0f, 12.0f, 0.0f, node->align, node->blend, image);
	/* draw the label for the current selected option */
	for (selectBoxOption = node->options; selectBoxOption; selectBoxOption = selectBoxOption->next) {
		if (!Q_strcmp(selectBoxOption->value, ref)) {
			R_FontDrawString(font, node->align, selBoxX, selBoxY,
				selBoxX, selBoxY, node->size[0] - 4, 0,
				node->texh[0], _(selectBoxOption->label), 0, 0, NULL, qfalse);
		}
	}

	/* active? */
	if (node->state) {
		selBoxY += node->size[1];
		selectBoxNode = node;

		/* drop down menu */
		/* left side */
		R_DrawNormPic(node->pos[0], node->pos[1] + node->size[1], SELECTBOX_SIDE_WIDTH, node->size[1] * node->height,
			7.0f, 28.0f, 0.0f, 21.0f, node->align, node->blend, image);

		/* stretched middle bar */
		R_DrawNormPic(node->pos[0] + SELECTBOX_SIDE_WIDTH, node->pos[1] + node->size[1], node->size[0], node->size[1] * node->height,
			16.0f, 28.0f, 7.0f, 21.0f, node->align, node->blend, image);

		/* right side */
		R_DrawNormPic(node->pos[0] + SELECTBOX_SIDE_WIDTH + node->size[0], node->pos[1] + node->size[1], SELECTBOX_SIDE_WIDTH, node->size[1] * node->height,
			23.0f, 28.0f, 16.0f, 21.0f, node->align, node->blend, image);

		/* now draw all available options for this selectbox */
		for (selectBoxOption = node->options; selectBoxOption; selectBoxOption = selectBoxOption->next) {
			/* draw the hover effect */
			if (selectBoxOption->hovered)
				R_DrawFill(selBoxX, selBoxY, node->size[0], SELECTBOX_DEFAULT_HEIGHT, ALIGN_UL, node->color);
			/* print the option label */
			R_FontDrawString(font, node->align, selBoxX, selBoxY,
				selBoxX, node->pos[1] + node->size[1], node->size[0] - 4, 0,
				node->texh[0], _(selectBoxOption->label), 0, 0, NULL, qfalse);
			/* next entries' position */
			selBoxY += node->size[1];
			/* reset the hovering - will be recalculated next frame */
			selectBoxOption->hovered = qfalse;
		}
		/* left side */
		R_DrawNormPic(node->pos[0], selBoxY - SELECTBOX_SPACER, SELECTBOX_SIDE_WIDTH, SELECTBOX_BOTTOM_HEIGHT,
			7.0f, 32.0f, 0.0f, 28.0f, node->align, node->blend, image);

		/* stretched middle bar */
		R_DrawNormPic(node->pos[0] + SELECTBOX_SIDE_WIDTH, selBoxY - SELECTBOX_SPACER, node->size[0], SELECTBOX_BOTTOM_HEIGHT,
			16.0f, 32.0f, 7.0f, 28.0f, node->align, node->blend, image);

		/* right bottom side */
		R_DrawNormPic(node->pos[0] + SELECTBOX_SIDE_WIDTH + node->size[0], selBoxY - SELECTBOX_SPACER,
			SELECTBOX_SIDE_WIDTH, SELECTBOX_BOTTOM_HEIGHT,
			23.0f, 32.0f, 16.0f, 28.0f, node->align, node->blend, image);
	}
}
