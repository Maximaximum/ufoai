/**
 * @file cl_map.c
 * @brief Geoscape/Map management
 */

/*
Copyright (C) 2002-2007 UFO: Alien Invasion team.

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
#include "cl_mapfightequip.h"
#include "cl_popup.h"
#include "cl_airfightmap.h"
#include "cl_ufo.h"
#include "renderer/r_draw.h"
#include "cl_map.h"
#include "menu/m_popup.h"
#include "menu/m_font.h"

aircraft_t *aircraft[MAX_AIRCRAFT];
aircraft_t *ufos[MAX_AIRCRAFT];
int numAircraft, numUfos;
vec2_t airFightMapCenter;
vec2_t smoothFinalAirFightCenter = {0.5, 0.5};		/**< value of ccs.center for a smooth change of position (see MAP_CenterOnPoint) */
float smoothFinalZoom = 0.0f;		/**< value of finale ccs.zoom for a smooth change of angle (see MAP_CenterOnPoint)*/
qboolean airFightSmoothMovement = qfalse;

static void AFM_GetAircraftInCombatRange(aircraft_t* aircraftList, int numAircraft)
{
	aircraft_t *aircraft;
	int baseIdx;

	for (baseIdx = 0; baseIdx < MAX_BASES; baseIdx++) {
		base_t *base = B_GetFoundedBaseByIDX(baseIdx);
		if (!base)
			continue;
		for (aircraft = base->aircraft + base->numAircraftInBase - 1; aircraft >= base->aircraft; aircraft--) {
			if (AIR_IsAircraftOnGeoscape(aircraft)) {
				float maxRange = AIR_GetMaxAircraftWeaponRange(aircraft->weapons, aircraft->maxWeapons);

				if (aircraft->aircraftTarget == gd.combatZoomedUfo || aircraft == gd.combatZoomedUfo->aircraftTarget) {
					float distance = MAP_GetDistance(aircraft->pos, gd.combatZoomedUfo->pos);
					if (distance < 2.0f) {
						aircraftList = aircraft;
						numAircraft++;
					}
				}
			}
		}
	}

}

void AFM_Init_f (void)
{
	int idx;
	aircraft_t *thisAircraft;
	int baseIdx;


	Vector2Copy(gd.combatZoomedUfo->pos, airFightMapCenter);

	ccs.zoom = 1.2f;
	Vector2Set(ccs.center, 0.5f, 0.5f);

	Vector2Set(smoothFinalAirFightCenter, 0.5f, 0.5f);
	smoothFinalZoom = 8.0f;
	airFightSmoothMovement = qtrue;


	for (idx = 0; idx < MAX_AIRCRAFT; idx++) {
		aircraft[idx] = NULL;
		ufos[idx] = NULL;
	}

	numAircraft = 0;
	numUfos = 0;

	for (baseIdx = 0; baseIdx < MAX_BASES; baseIdx++) {
		base_t *base = B_GetFoundedBaseByIDX(baseIdx);
		if (!base)
			continue;
		for (thisAircraft = base->aircraft + base->numAircraftInBase - 1; thisAircraft >= base->aircraft; thisAircraft--) {
			if (AIR_IsAircraftOnGeoscape(thisAircraft)) {
				float maxRange = AIR_GetMaxAircraftWeaponRange(thisAircraft->weapons, thisAircraft->maxWeapons);

				if (thisAircraft->aircraftTarget == gd.combatZoomedUfo || thisAircraft == gd.combatZoomedUfo->aircraftTarget) {
					float distance = MAP_GetDistance(thisAircraft->pos, gd.combatZoomedUfo->pos);
					if (distance < 4.0f) {
						aircraft[numAircraft] = thisAircraft;
						numAircraft++;
					}
				}
			}
		}
	}

	/* draws ufos */
	for (thisAircraft = gd.ufos + gd.numUFOs - 1; thisAircraft >= gd.ufos; thisAircraft--) {
		float distance = MAP_GetDistance(thisAircraft->pos, gd.combatZoomedUfo->pos);
		if (distance < 4.0f || gd.combatZoomedUfo == thisAircraft) {
			ufos[numUfos] = thisAircraft;
			numUfos++;
		}
	}

		/*	AFM_GetAircraftInCombatRange(aircraft, numAircraft); */
/*	AFM_GetUfosInCombatRange(ufos); */

}

void AFM_Exit_f (void)
{
	if (gd.combatZoomedUfo) {
		gd.combatZoomOn = qfalse;
		gd.combatZoomedUfo = NULL;
		CL_EnsureValidGameLapseForGeoscape();

	}
}

#define CIRCLE_DRAW_POINTS	60
/**
 * @brief Draw equidistant points from a given point on a menu node
 * @param[in] node The menu node which will be used for drawing dimensions.
 * This is usually the geoscape menu node.
 * @param[in] center The latitude and longitude of center point
 * @param[in] angle The angle defining the distance of the equidistant points to center
 * @param[in] color The color for drawing
 * @sa RADAR_DrawCoverage
 */
static void AFM_MapDrawEquidistantPoints (const menuNode_t* node, const vec2_t center, const float angle, const vec4_t color)
{
	int i, xCircle, yCircle;
	screenPoint_t pts[CIRCLE_DRAW_POINTS + 1];
	vec2_t posCircle;
	qboolean oldDraw = qfalse;
	int numPoints = 0;
	vec3_t initialVector, rotationAxis, currentPoint, centerPos;

	R_ColorBlend(color);

	/* Set centerPos corresponding to cartesian coordinates of the center point */
	PolarToVec(center, centerPos);

	/* Find a perpendicular vector to centerPos, and rotate centerPos around him to obtain one point distant of angle from centerPos */
	PerpendicularVector(rotationAxis, centerPos);
	RotatePointAroundVector(initialVector, rotationAxis, centerPos, angle);

	/* Now, each equidistant point is given by a rotation around centerPos */
	for (i = 0; i <= CIRCLE_DRAW_POINTS; i++) {
		qboolean draw = qfalse;
		RotatePointAroundVector(currentPoint, centerPos, initialVector, i * 360 / CIRCLE_DRAW_POINTS);
		VecToPolar(currentPoint, posCircle);
		if (AFM_MapToScreen(node, posCircle, &xCircle, &yCircle)) {
			draw = qtrue;
		}

		/* if moving from a point of the screen to a distant one, draw the path we already calculated, and begin a new path
		 * (to avoid unwanted lines) */
		if (draw != oldDraw && i != 0) {
			R_DrawLineStrip(numPoints, (int*)(&pts));
			numPoints = 0;
		}
		/* if the current point is to be drawn, add it to the path */
		if (draw) {
			pts[numPoints].x = xCircle;
			pts[numPoints].y = yCircle;
			numPoints++;
		}
		/* update value of oldDraw */
		oldDraw = draw;
	}

	/* Draw the last path */
	R_DrawLineStrip(numPoints, (int*)(&pts));
	R_ColorBlend(NULL);
}

#define BULLET_SIZE	1
/**
 * @brief Draws on bunch of bullets on the geoscape map
 * @param[in] node Pointer to the node in which you want to draw the bullets.
 * @param[in] pos
 * @sa MAP_DrawMap
 */
static void AFM_DrawBullets (const menuNode_t* node, const vec3_t pos)
{
	int x, y;
	const vec4_t yellow = {1.0f, 0.874f, 0.294f, 1.0f};

	if (AFM_MapToScreen(node, pos, &x, &y))
		R_DrawFill(x, y, BULLET_SIZE, BULLET_SIZE, ALIGN_CC, yellow);
}

/**
 * @brief Transform a 2D position on the map to screen coordinates.
 * @param[in] node Menu node
 * @param[in] pos Position on the map described by longitude and latitude
 * @param[out] x X coordinate on the screen
 * @param[out] y Y coordinate on the screen
 * @returns qtrue if the screen position is within the boundaries of the menu
 * node. Otherwise returns qfalse.
 * @sa MAP_3DMapToScreen
 */
qboolean AFM_MapToScreen (const menuNode_t* node, const vec2_t pos,
		int *x, int *y)
{
	float sx;

	/* get "raw" position */
	sx = ((pos[0] - airFightMapCenter[0]) / 360) * 12 + ccs.center[0] - 0.5;

	/* shift it on screen */
	if (sx < -0.5)
		sx += 1.0;
	else if (sx > +0.5)
		sx -= 1.0;

	*x = node->pos[0] + 0.5 * node->size[0] - sx * node->size[0] * ccs.zoom; /* (ccs.zoom * 0.0379); */
	*y = node->pos[1] + 0.5 * node->size[1] -
		(((pos[1] - airFightMapCenter[1]) / 180) * 12 + ccs.center[1] - 0.5) * node->size[1] * ccs.zoom; /* (ccs.zoom * 0.0379); */

	if (*x < node->pos[0] && *y < node->pos[1] &&
		*x > node->pos[0] + node->size[0] && *y > node->pos[1] + node->size[1])
		return qfalse;
	return qtrue;
}


/**
 * @brief Draws a 3D marker on geoscape if the player can see it.
 * @param[in] node Menu node.
 * @param[in] pos Longitude and latitude of the marker to draw.
 * @param[in] theta Angle (degree) of the model to the horizontal.
 * @param[in] model The name of the model of the marker.
 */
qboolean AFM_Draw3DMarkerIfVisible (const menuNode_t* node, const vec2_t pos, float theta, const char *model, int skin)
{
	int x, y, z;
	vec3_t screenPos, angles, v;
	float zoom;
	qboolean test;

/*	test = MAP_AllMapToScreen(node, pos, &x, &y, &z); */
	test = AFM_MapToScreen(node, pos, &x, &y);
	z = -10;
	if (test) {
		/* Set position of the model on the screen */
		VectorSet(screenPos, x, y, z);
		VectorSet(angles, theta, 180, 0);

		zoom = ccs.zoom/37;
/*		zoom = 0.4f; */

		/* Draw */
		R_Draw3DMapMarkers(angles, zoom, screenPos, model, skin);
		return qtrue;
	}
	return qfalse;
}

static void AFM_CenterMapPosition(vec3_t pos)
{
	Vector2Set(ccs.center, 0.5f - ((pos[0] - airFightMapCenter[0]) / 360)*12, 0.5f - ((pos[1] - airFightMapCenter[1]) / 180)*12);
}


static void GetNextProjectedStepPosition(const int maxInterpolationPoints, const vec3_t pos, const vec3_t projectedPos, vec3_t drawPos, int numInterpolationPoints)
{
	if (maxInterpolationPoints > 2 && numInterpolationPoints < maxInterpolationPoints) {
		/* If a new point hasn't been given and there is at least 3 points need to be filled in then
		 * use linear interpolation to draw the points until a new projectile point is provided.
		 * The reason you need at least 3 points is that acceptable results can be achieved with 2 or less
		 * gaps in points so dont add the overhead of interpolation. */
		const float xInterpolStep = (projectedPos[0] - pos[0]) / (float)maxInterpolationPoints;
		numInterpolationPoints += 1;
		drawPos[0] = pos[0] + (xInterpolStep * numInterpolationPoints);
		LinearInterpolation(pos, projectedPos, drawPos[0], drawPos[1]);
	} else {
		VectorCopy(pos, drawPos);
	}
}

/**
 * @brief Draws all ufos, aircraft, bases and so on to the geoscape map (2D and 3D)
 * @param[in] node The menu node which will be used for drawing markers.
 * @sa MAP_DrawMap
 */
static void AFM_DrawMapMarkers (const menuNode_t* node)
{
	aircraft_t *thisAircraft;
	aircraftProjectile_t *projectile;
	int idx;
	int maxInterpolationPoints;
	const vec4_t darkBrown = {0.3608f, 0.251f, 0.2f, 1.0f};
	const vec2_t northPole = {0.0f, 90.0f};
	vec3_t centroid = {0,0,0};
	int combatAirIdx;
	vec3_t combatZoomAircraftInCombatPos[MAX_AIRCRAFT];
	int combatZoomNumCombatAircraft = 0;


	for (idx = 0; idx < numAircraft; idx++) {
		thisAircraft = aircraft[idx];
		VectorCopy(thisAircraft->pos, combatZoomAircraftInCombatPos[combatZoomNumCombatAircraft]);
		combatZoomNumCombatAircraft++;
	}
	for (idx = 0; idx < numUfos; idx++) {
		VectorCopy(ufos[idx]->pos, combatZoomAircraftInCombatPos[combatZoomNumCombatAircraft]);
		combatZoomNumCombatAircraft++;
	}

	/* finds the centroid of all aircraft in combat range who are targeting the zoomed ufo
	 * and uses this as the point on which to center. */
	for (combatAirIdx = 0; combatAirIdx < combatZoomNumCombatAircraft; combatAirIdx++) {
		VectorAdd(combatZoomAircraftInCombatPos[combatAirIdx], centroid, centroid);
	}
	if (combatAirIdx > 1) {
		VectorScale(centroid, 1.0/(float)combatAirIdx, centroid);
		AFM_CenterMapPosition(centroid);
	} else {
		AFM_CenterMapPosition(gd.combatZoomedUfo->pos);
	}

	if (gd.gameTimeScale > 0)
		maxInterpolationPoints = floor(1.0f/(cls.frametime * (float)gd.gameTimeScale));
	else
		maxInterpolationPoints = 0;

	/* draw all aircraft in combat range */
	for (idx = 0; idx < numAircraft; idx++) {
		vec3_t drawPos = {0, 0, 0};
		thisAircraft = aircraft[idx];
		float weaponRanges[MAX_AIRCRAFTSLOT];
		float angle;
		int numWeaponRanges = AIR_GetAircraftWeaponRanges(thisAircraft->weapons, thisAircraft->maxWeapons, weaponRanges);
#if 0
		Com_Printf("pos=%f,%f projPos=%f,%f \n",thisAircraft->pos[0],thisAircraft->pos[1],thisAircraft->projectedPos[0], thisAircraft->projectedPos[1]);
#endif
		if (maxInterpolationPoints > 2 && thisAircraft->numInterpolationPoints < maxInterpolationPoints && 1 == 2) {
			/* If a new point hasn't been given and there is at least 3 points need to be filled in then
			 * use linear interpolation to draw the points until a new projectile point is provided.
			 * The reason you need at least 3 points is that acceptable results can be achieved with 2 or less
			 * gaps in points so dont add the overhead of interpolation. */
			const float xInterpolStep = (thisAircraft->projectedPos[0] - thisAircraft->pos[0]) / (float)maxInterpolationPoints;
			thisAircraft->numInterpolationPoints += 1;
			drawPos[0] = thisAircraft->pos[0] + (xInterpolStep * thisAircraft->numInterpolationPoints);
			LinearInterpolation(thisAircraft->pos, thisAircraft->projectedPos, drawPos[0], drawPos[1]);
		} else {
			VectorCopy(thisAircraft->pos, drawPos);
		}

		/* Draw all weapon ranges for this aircraft if at least one UFO is visible */
		if (numWeaponRanges > 0) {
			int idxWeaponRanges;
			for (idxWeaponRanges = 0; idxWeaponRanges < numWeaponRanges; idxWeaponRanges++) {
				AFM_MapDrawEquidistantPoints(node, drawPos, weaponRanges[idxWeaponRanges], darkBrown);
			}
		}

		if (thisAircraft->status >= AIR_TRANSIT) {
			angle = MAP_AngleOfPath(thisAircraft->pos, thisAircraft->route.point[thisAircraft->route.numPoints - 1], thisAircraft->direction, NULL);
		} else {
			angle = MAP_AngleOfPath(thisAircraft->pos, northPole, thisAircraft->direction, NULL);
		}

		AFM_Draw3DMarkerIfVisible(node, drawPos, angle, thisAircraft->model, 0);
	}

	/* draw all ufos in combat range */
	for (idx = 0; idx < numUfos; idx++) {
		vec3_t drawPos = {0, 0, 0};
		thisAircraft = ufos[idx];
		float weaponRanges[MAX_AIRCRAFTSLOT];
		float angle;
		int numWeaponRanges = AIR_GetAircraftWeaponRanges(thisAircraft->weapons, thisAircraft->maxWeapons, weaponRanges);

		if (maxInterpolationPoints > 2 && thisAircraft->numInterpolationPoints < maxInterpolationPoints) {
			/* If a new point hasn't been given and there is at least 3 points need to be filled in then
			 * use linear interpolation to draw the points until a new projectile point is provided.
			 * The reason you need at least 3 points is that acceptable results can be achieved with 2 or less
			 * gaps in points so dont add the overhead of interpolation. */
			const float xInterpolStep = (thisAircraft->projectedPos[0] - thisAircraft->pos[0]) / (float)maxInterpolationPoints;
			thisAircraft->numInterpolationPoints += 1;
			drawPos[0] = thisAircraft->pos[0] + (xInterpolStep * thisAircraft->numInterpolationPoints);
			LinearInterpolation(thisAircraft->pos, thisAircraft->projectedPos, drawPos[0], drawPos[1]);
		} else {
			VectorCopy(thisAircraft->pos, drawPos);
		}

		/* Draw all weapon ranges for this aircraft if at least one UFO is visible */
		if (numWeaponRanges > 0) {
			int idxWeaponRanges;
			for (idxWeaponRanges = 0; idxWeaponRanges < numWeaponRanges; idxWeaponRanges++) {
				AFM_MapDrawEquidistantPoints(node, drawPos, weaponRanges[idxWeaponRanges], darkBrown);
			}
		}

		if (thisAircraft->status >= AIR_TRANSIT) {
			angle = MAP_AngleOfPath(thisAircraft->pos, thisAircraft->route.point[thisAircraft->route.numPoints - 1], thisAircraft->direction, NULL);
		} else {
			angle = MAP_AngleOfPath(thisAircraft->pos, northPole, thisAircraft->direction, NULL);
		}

		AFM_Draw3DMarkerIfVisible(node, drawPos, angle, thisAircraft->model, 0);
	}


	/* draws projectiles */
	for (projectile = gd.projectiles + gd.numProjectiles - 1; projectile >= gd.projectiles; projectile --) {
		vec3_t drawPos = {0, 0, 0};

		if (!projectile->aimedAircraft)
			continue;

		if (projectile->hasMoved) {
			projectile->hasMoved = qfalse;
			VectorCopy(projectile->pos[0], drawPos);
		} else {
#if 0
			GetNextProjectedStepPosition(projectile->numInterpolationPoints, maxInterpolationPoints, projectile->pos[0], projectile->projectedPos[0], drawPos);
#endif
			if (maxInterpolationPoints > 2 && projectile->numInterpolationPoints < maxInterpolationPoints) {
				/* If a new point hasn't been given and there is at least 3 points need to be filled in then
				 * use linear interpolation to draw the points until a new projectile point is provided.
				 * The reason you need at least 3 points is that acceptable results can be achieved with 2 or less
				 * gaps in points so dont add the overhead of interpolation. */
				const float xInterpolStep = (projectile->projectedPos[0][0] - projectile->pos[0][0]) / (float)maxInterpolationPoints;
				projectile->numInterpolationPoints += 1;
				drawPos[0] = projectile->pos[0][0] + (xInterpolStep * projectile->numInterpolationPoints);
				LinearInterpolation(projectile->pos[0], projectile->projectedPos[0], drawPos[0], drawPos[1]);
			} else {
				VectorCopy(projectile->pos[0], drawPos);
			}
		}

		if (projectile->bullets)
			AFM_DrawBullets(node, drawPos);
/*		else if (projectile->laser)
			** @todo Implement rendering of laser shot
			 MAP_DrawLaser(node, vec3_origin, vec3_origin); */
		else
			AFM_Draw3DMarkerIfVisible(node, drawPos, projectile->angle, projectile->aircraftItem->model, 0);
	}

}

#define AF_SMOOTHING_STEP_2D	0.02f
/**
 * @brief smooth translation of the 2D geoscape
 * @note updates slowly values of ccs.center so that its value goes to smoothFinal2DGeoscapeCenter
 * @note updates slowly values of ccs.zoom so that its value goes to ZOOM_LIMIT
 * @sa MAP_DrawMap
 * @sa MAP_CenterOnPoint
 */
void AFM_SmoothTranslate (void)
{
	const float dist1 = smoothFinalAirFightCenter[0] - ccs.center[0];
	const float dist2 = smoothFinalAirFightCenter[1] - ccs.center[1];
	const float length = sqrt(dist1 * dist1 + dist2 * dist2);
	const float diff_zoom = smoothFinalZoom - ccs.zoom;
	const float speed = 1.0f;

	if (/*length < AF_SMOOTHING_STEP_2D  && */ fabs(diff_zoom) < AF_SMOOTHING_STEP_2D) {
/*		ccs.center[0] = smoothFinalAirFightCenter[0];
		ccs.center[1] = smoothFinalAirFightCenter[1]; */
		ccs.zoom = smoothFinalZoom;
		airFightSmoothMovement = qfalse;
	} else {
/*		if (length > 0)
			ccs.center[0] = ccs.center[0] + AF_SMOOTHING_STEP_2D * (dist1) / length;
			ccs.center[1] = ccs.center[1] + AF_SMOOTHING_STEP_2D * (dist2) / length;
			ccs.zoom = ccs.zoom + AF_SMOOTHING_STEP_2D * (diff_zoom * speed) / length;
		else */
			ccs.zoom = ccs.zoom + AF_SMOOTHING_STEP_2D * (diff_zoom * speed);
	}
}

void AFM_StopSmoothMovement (void)
{
	airFightSmoothMovement = qfalse;
}

/**
 * @brief Draw the geoscape
 * @param[in] node The map menu node
 * @sa MAP_DrawMapMarkers
 */
void AFM_DrawMap (const menuNode_t* node)
{
	float q = 0.0f;

	/* store these values in ccs struct to be able to handle this even in the input code */
	Vector2Copy(node->pos, ccs.mapPos);
	Vector2Copy(node->size, ccs.mapSize);

	if (airFightSmoothMovement)
		AFM_SmoothTranslate();
	R_DrawAirFightBackground(node->pos[0], node->pos[1], node->size[0], node->size[1], ccs.center[0], ccs.center[1], (0.5 / ccs.zoom));
	AFM_DrawMapMarkers (node);
}
