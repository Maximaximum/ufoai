/**
 * @file client.h
 * @brief Primary header for client.
 */

/*
All original materal Copyright (C) 2002-2007 UFO: Alien Invasion team.

Original file from Quake 2 v3.21: quake2-2.31/client/client.h
Copyright (C) 1997-2001 Id Software, Inc.

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

#ifndef CLIENT_CLIENT_H
#define CLIENT_CLIENT_H

#include "cl_renderer.h"
#include "cl_video.h"
#include "../game/inv_shared.h"
#include "cl_sound.h"
#include "cl_screen.h"
#include "cl_input.h"
#include "cl_keys.h"
#include "cl_cinematic.h"
#include "cl_console.h"
#include "cl_market.h"
#include "cl_event.h"
#include "cl_save.h"
#include "cl_http.h"
#include "../renderer/r_entity.h"

/*============================================================================= */

#define MAX_TEAMLIST	8

/* Macros for faster access to the inventory-container. */
#define RIGHT(e) ((e)->i.c[csi.idRight])
#define LEFT(e)  ((e)->i.c[csi.idLeft])
#define FLOOR(e) ((e)->i.c[csi.idFloor])
#define HEADGEAR(e) ((e)->i.c[csi.idHeadgear])
#define EXTENSION(e) ((e)->i.c[csi.idExtension])

typedef struct {
	char name[MAX_QPATH];
	char cinfo[MAX_QPATH];
} clientinfo_t;

typedef enum {
	M_MOVE,		/**< We are currently in move-mode (destination selection). */
	M_FIRE_R,	/**< We are currently in fire-mode for the right weapon (target selection). */
	M_FIRE_L,	/**< We are currently in fire-mode for the left weapon (target selection). */
	M_FIRE_HEADGEAR, /**< We'll fire headgear item shortly. */
	M_PEND_MOVE,	/**< A move target has been selected, we are waiting for player-confirmation. */
	M_PEND_FIRE_R,	/**< A fire target (right weapon) has been selected, we are waiting for player-confirmation. */
	M_PEND_FIRE_L	/**< A fire target (left weapon) has been selected, we are waiting for player-confirmation. */
} cmodes_t;

#define IS_MODE_FIRE_RIGHT(x)	((x) == M_FIRE_R || (x) == M_PEND_FIRE_R)
#define IS_MODE_FIRE_LEFT(x)		((x) == M_FIRE_L || (x) == M_PEND_FIRE_L)
#define IS_MODE_FIRE_HEADGEAR(x)		((x) == M_FIRE_HEADGEAR)

/**
 * @brief the client_state_t structure is wiped completely at every server map change
 * @note the client_static_t structure is persistant through an arbitrary
 * number of server connections
 * @sa client_static_t
 */
typedef struct client_state_s {
	qboolean refresh_prepped;	/**< false if on new level or vid restart */
	qboolean force_refdef;		/**< vid has changed, so we can't use a paused refdef */

	int time;					/**< this is the time value that the client
								 * is rendering at.  always <= cls.realtime */
	int eventTime;				/**< similar to time, but not counting if blockEvents is set */

	camera_t cam;

	cmodes_t cmode;
	int cfiremode;
	cmodes_t oldcmode;
	int oldstate;

	int msgTime;
	char msgText[256];

	struct le_s *teamList[MAX_TEAMLIST];
	int numTeamList;
	int numAliensSpotted;

	/** server state information */
	int servercount;			/**< server identification for prespawns */
	char gamedir[MAX_QPATH];
	int pnum;
	int actTeam;

	char configstrings[MAX_CONFIGSTRINGS][MAX_TOKEN_CHARS];

	/** locally derived information from server state */
	struct model_s *model_draw[MAX_MODELS];
	struct cBspModel_s *model_clip[MAX_MODELS];

	clientinfo_t clientinfo[MAX_CLIENTS]; /* client info of all connected clients */
} client_state_t;

extern client_state_t cl;

typedef enum {
	ca_uninitialized,
	ca_disconnected,			/**< not talking to a server */
	ca_sequence,				/**< rendering a sequence */
	ca_connecting,				/**< sending request packets to the server */
	ca_connected,				/**< netchan_t established, waiting for svc_serverdata */
	ca_ptledit,					/**< particles should be rendered */
	ca_active					/**< game views should be displayed */
} connstate_t;

typedef enum {
	key_game,
	key_input,
	key_console,
	key_message
} keydest_t;

/**
 * @brief Not cleared on levelchange (static data)
 * @sa client_state_t
 */
typedef struct client_static_s {
	connstate_t state;
	keydest_t key_dest;
	keydest_t key_dest_old;

	int realtime;				/**< always increasing, no clamping, etc */
	float frametime;			/**< seconds since last frame */

	float framerate;

	/* screen rendering information */
	float disable_screen;		/* showing loading plaque between levels */
	/* or changing rendering dlls */
	/* if time gets > 30 seconds ahead, break it */

	/* connection information */
	char servername[MAX_OSPATH];	/**< name of server from original connect */
	int connectTime;				/**< for connection retransmits */
	int waitingForStart;			/**< waiting for EV_START or timeout */

	struct datagram_socket *datagram_socket;
	struct net_stream *stream;

	int challenge;				/**< from the server to use for connecting */

	/** needs to be here, because server can be shutdown, before we see the ending screen */
	int team;
	struct aircraft_s *missionaircraft;	/**< aircraft pointer for mission handling */

	float loadingPercent;
	char loadingMessages[96];

	int playingCinematic;	/**< Set to cinematic playing flags or 0 when not playing */

	char downloadName[MAX_OSPATH];
	size_t downloadPosition;
	int downloadPercent;

	dlqueue_t downloadQueue;	/**< queue of paths we need */
	dlhandle_t HTTPHandles[4];	/**< actual download handles */
	char downloadServer[512];	/**< base url prefix to download from */
	char downloadReferer[32];	/**< libcurl requires a static string :( */
	CURL *curl;

	/* these models must only be loaded once */
	struct model_s *model_weapons[MAX_OBJDEFS];
} client_static_t;

extern client_static_t cls;

extern struct memPool_s *cl_localPool;
extern struct memPool_s *cl_genericPool;
extern struct memPool_s *cl_ircSysPool;
extern struct memPool_s *cl_menuSysPool;
extern struct memPool_s *cl_soundSysPool;

/* TODO: Made use of the tags */
typedef enum {
	CL_TAG_NONE,				/**< will be wiped on every new game */
	CL_TAG_PARSE_ONCE,			/**< will not be wiped on a new game (shaders, fonts) */
	CL_TAG_REPARSE_ON_NEW_GAME,	/**< reparse this stuff on a new game (techs, ...) */
	CL_TAG_MENU					/**< never delete it */
} clientMemoryTags_t;

/*============================================================================= */

/* i18n support via gettext */
#include <libintl.h>

/* the used textdomain for gettext */
#define TEXT_DOMAIN "ufoai"
#include <locale.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

/* cvars */
extern cvar_t *cl_http_downloads;
extern cvar_t *cl_http_filelists;
extern cvar_t *cl_http_max_connections;
extern cvar_t *cl_isometric;
extern cvar_t *cl_particleweather;
extern cvar_t *cl_fps;
extern cvar_t *cl_logevents;
extern cvar_t *cl_centerview;
extern cvar_t *cl_worldlevel;
extern cvar_t *cl_selected;
extern cvar_t *cl_3dmap;
extern cvar_t *cl_numnames;
extern cvar_t *cl_start_employees;
extern cvar_t *cl_initial_equipment;
extern cvar_t *cl_start_buildings;
extern cvar_t *cl_team;
extern cvar_t *cl_teamnum;
extern cvar_t *cl_camzoommin;

extern cvar_t *cl_mapzoommax;
extern cvar_t *cl_mapzoommin;

extern cvar_t *mn_active;
extern cvar_t *mn_afterdrop;
extern cvar_t *mn_main_afterdrop;
extern cvar_t *mn_main;
extern cvar_t *mn_sequence;
extern cvar_t *mn_hud;
extern cvar_t *mn_inputlength;

extern cvar_t *s_language;
extern cvar_t *difficulty;
extern cvar_t *confirm_actions;

/** limit the input for cvar editing (base name, save slots and so on) */
#define MAX_CVAR_EDITING_LENGTH 256 /* MAXCMDLINE */

void CL_LoadMedia(void);
void CL_RegisterSounds(void);
void CL_RegisterLocalModels(void);

/* cl_language.c */
void CL_ParseLanguages(const char *name, const char **text);
void CL_LanguageInit(void);
qboolean CL_LanguageTryToSet(const char *localeID);

/* cl_main.c */
void CL_SetClientState(int state);
void CL_Disconnect(void);
void CL_Init(void);
void CL_ReadSinglePlayerData(void);
void CL_RequestNextDownload(void);
void CL_StartSingleplayer(qboolean singleplayer);
void CL_GetChallengePacket(void);
void CL_ParseMedalsAndRanks(const char *name, const char **text, byte parserank);
void CL_ParseUGVs(const char *name, const char **text);
void CL_UpdateCharacterSkills(character_t *chr);	/* cl_team.c */
const char* CL_ToDifficultyName(int difficulty);
void CL_ScriptSanityCheck(void);

void CL_ClearState(void);

/* cl_le.c */
#define MAX_LE_PATHLENGTH 32

/** @brief a local entity */
typedef struct le_s {
	qboolean inuse;
	qboolean invis;
	qboolean autohide;
	qboolean selected;
	int hearTime;		/**< draw a marker over the entity if its an actor and he heard something */
	int type;				/**< the local entity type */
	int entnum;				/**< the server side edict num this le belongs to */

	vec3_t origin, oldOrigin;	/**< position given via world coordinates */
	pos3_t pos, oldPos;		/**< position on the grid */
	int dir;				/**< the current dir the le is facing into */

	int TU, maxTU;				/**< time units */
	int morale, maxMorale;		/**< morale value - used for soldier panic and the like */
	int HP, maxHP;				/**< health points */
	int STUN;					/**< if stunned - state STATE_STUN */
	int state;					/**< rf states, dead, crouched and so on */
	int reaction_minhit;

	float angles[3];
	float alpha;

	int team;		/**< the team number this local entity belongs to */
	int pnum;		/**< the player number this local entity belongs to */

	int contents;			/**< content flags for this LE - used for tracing */
	vec3_t mins, maxs;

	int modelnum1;	/**< the number of the body model in the cl.model_draw array */
	int modelnum2;	/**< the number of the head model in the cl.model_draw array */
	int skinnum;	/**< the skin number of the body and head model */
	struct model_s *model1, *model2;	/**< pointers to the cl.model_draw array
					 * that holds the models for body and head - model1 is body,
					 * model2 is head */

/* 	character_t	*chr; */

	/** is called every frame */
	void (*think) (struct le_s * le);
	/** number of frames to skip the think function for */
	int thinkDelay;

	/** various think function vars */
	byte path[MAX_LE_PATHLENGTH];
	int pathContents[MAX_LE_PATHLENGTH];	/**< content flags of the brushes the actor is walking in */
	int positionContents;					/**< content flags for the current brush the actor is standing in */
	int pathLength, pathPos;
	int startTime, endTime;
	int speed;			/**< the speed the le is moving with */

	/** sound effects */
	struct sfx_s* sfx;
	float attenuation;
	float volume;

	/** gfx */
	animState_t as;	/**< holds things like the current active frame and so on */
	const char *particleID;
	int particleLevelFlags;	/**< the levels this particle should be visible at */
	ptl_t *ptl;				/**< particle pointer to display */
	char *ref1, *ref2;
	inventory_t i;
	int left, right, extension;
	int fieldSize;				/**< ACTOR_SIZE_* */
	teamDef_t* teamDef;
	int gender;
	fireDef_t *fd;	/**< in case this is a projectile */

	/** is called before adding a le to scene */
	qboolean(*addFunc) (struct le_s * le, entity_t * ent);
} le_t;							/* local entity */

#define MAX_LOCALMODELS		512
#define MAX_MAPPARTICLES	1024

/** @brief local models */
typedef struct lm_s {
	char name[MAX_VAR];
	char particle[MAX_VAR];

	vec3_t origin;
	vec3_t angles;

	int num;
	int skin;
	int flags;
	int frame;	/**< which frame to show */
	char animname[MAX_QPATH];	/**< is this an animated model */
	int levelflags;
	animState_t as;

	struct model_s *model;
} localModel_t;							/* local models */

/** @brief map particles */
typedef struct mp_s {
	char ptl[MAX_QPATH];
	const char *info;
	vec3_t origin;
	vec2_t wait;
	int nextTime;
	int levelflags;
} mapParticle_t;							/* mapparticles */

extern localModel_t LMs[MAX_LOCALMODELS];

extern int numLMs;
extern int numMPs;

extern le_t LEs[MAX_EDICTS];
extern int numLEs;

static const vec3_t player_mins = { -PLAYER_WIDTH, -PLAYER_WIDTH, PLAYER_MIN };
static const vec3_t player_maxs = { PLAYER_WIDTH, PLAYER_WIDTH, PLAYER_STAND };
static const vec3_t player_dead_maxs = { PLAYER_WIDTH, PLAYER_WIDTH, PLAYER_DEAD };

qboolean CL_OutsideMap(vec3_t impact);
const char *LE_GetAnim(const char *anim, int right, int left, int state);
void LE_AddProjectile(fireDef_t * fd, int flags, vec3_t muzzle, vec3_t impact, int normal, qboolean autohide);
void LE_AddGrenade(fireDef_t * fd, int flags, vec3_t muzzle, vec3_t v0, int dt);
void LE_AddAmbientSound(const char *sound, vec3_t origin, float volume, float attenuation);
le_t *LE_GetClosestActor(const vec3_t origin);

void LE_Think(void);
/* think functions */
void LET_StartIdle(le_t * le);
void LET_Appear(le_t * le);
void LET_StartPathMove(le_t * le);
void LET_ProjectileAutoHide(le_t *le);
void LET_PlayAmbientSound(le_t * le);

/* local model functions */
void LM_Perish(struct dbuffer *msg);
void LM_Explode(struct dbuffer *msg);
void LM_DoorOpen(struct dbuffer *msg);
void LM_DoorClose(struct dbuffer *msg);

void LM_AddToScene(void);
void LE_AddToScene(void);
le_t *LE_Add(int entnum);
le_t *LE_Get(int entnum);
le_t *LE_Find(int type, pos3_t pos);
void LE_Cleanup(void);
trace_t CL_Trace(vec3_t start, vec3_t end, const vec3_t mins, const vec3_t maxs, le_t * passle, le_t * passle2, int contentmask);

localModel_t *CL_AddLocalModel(const char *model, const char *particle, vec3_t origin, vec3_t angles, int num, int levelflags);
void CL_AddMapParticle(const char *particle, vec3_t origin, vec2_t wait, const char *info, int levelflags);
void CL_ParticleCheckRounds(void);
void CL_ParticleSpawnFromSizeBuf (struct dbuffer *msg);
void CL_ParticleFree(ptl_t *p);

/* cl_actor.c */
/* vertical cursor offset */
#define CURSOR_OFFSET UNIT_HEIGHT * 0.4
/* distance from vertical center of grid-point to head when standing */
#define EYE_HT_STAND  UNIT_HEIGHT * 0.25
/* distance from vertical center of grid-point to head when crouched */
#define EYE_HT_CROUCH UNIT_HEIGHT * 0.06

/**
 * @brief Reaction fire toggle state, don't mess with the order!!!
 */
typedef enum {
	R_FIRE_OFF,
	R_FIRE_ONCE,
	R_FIRE_MANY
} reactionmode_t;

typedef enum {
	BT_RIGHT_FIRE,
	BT_REACTION,
	BT_LEFT_FIRE,
	BT_RIGHT_RELOAD,
	BT_LEFT_RELOAD,
	BT_STAND,
	BT_CROUCH,
	BT_HEADGEAR,
	BT_NUM_TYPES
} button_types_t;

extern le_t *selActor;
extern int actorMoveLength;
extern invList_t invList[MAX_INVLIST];

extern pos_t *fb_list[MAX_FORBIDDENLIST];
extern int fb_length;

void MSG_Write_PA(player_action_t player_action, int num, ...);

void CL_CharacterCvars(character_t * chr);
void CL_UGVCvars(character_t * chr);
void CL_ActorUpdateCVars(void);
qboolean CL_CheckMenuAction(int time, invList_t * weapon, int mode);

void CL_ResetWeaponButtons(void);
void CL_SetDefaultReactionFiremode(le_t * actor, const char hand);
void CL_DisplayFiremodes_f(void);
void CL_SwitchFiremodeList_f(void);
void CL_FireWeapon_f(void);
void CL_SelectReactionFiremode_f(void);

character_t *CL_GetActorChr(const le_t * le);
qboolean CL_WorkingReactionFiremode(const le_t * actor);
int CL_ReservedTUs(const le_t * le, reservation_types_t type);
void CL_ReserveTUs(const le_t * le, reservation_types_t type, int tus);

#ifdef DEBUG
void CL_ListReactionAndReservations_f (void);
void CL_DisplayBlockedPaths_f(void);
void LE_List_f(void);
#endif
void CL_ConditionalMoveCalc(struct routing_s *map, le_t *le, int distance);
qboolean CL_ActorSelect(le_t * le);
qboolean CL_ActorSelectList(int num);
qboolean CL_ActorSelectNext(void);
void CL_AddActorToTeamList(le_t * le);
void CL_RemoveActorFromTeamList(const le_t * le);
void CL_ActorSelectMouse(void);
void CL_ActorReload(int hand);
void CL_ActorTurnMouse(void);
void CL_ActorDoTurn(struct dbuffer *msg);
void CL_ActorStandCrouch_f(void);
void CL_ActorToggleReaction_f(void);
void CL_ActorUseHeadgear_f(void);
void CL_ActorStartMove(const le_t * le, pos3_t to);
void CL_ActorShoot(const le_t * le, pos3_t at);
void CL_InvCheckHands(struct dbuffer *msg);
void CL_ActorDoMove(struct dbuffer *msg);
void CL_ActorDoShoot(struct dbuffer *msg);
void CL_ActorShootHidden(struct dbuffer *msg);
void CL_ActorDoThrow(struct dbuffer *msg);
void CL_ActorStartShoot(struct dbuffer *msg);
void CL_ActorDie(struct dbuffer *msg);
void CL_PlayActorSound(const le_t * le, actorSound_t soundType);

void CL_ActorActionMouse(void);

void CL_NextRound_f(void);
void CL_DoEndRound(struct dbuffer * msg);

void CL_ResetMouseLastPos(void);
void CL_ResetActorMoveLength(void);
void CL_ActorMouseTrace(void);

qboolean CL_AddActor(le_t * le, entity_t * ent);
qboolean CL_AddUGV(le_t * le, entity_t * ent);

void CL_AddTargeting(void);
void CL_ActorTargetAlign_f(void);
void CL_ActorInventoryOpen_f(void);

/* cl_team.c */
/* if you increase this, you also have to change the aircraft buy/sell menu scripts */
#define MAX_ACTIVETEAM	8
#define MAX_WHOLETEAM	32

#define NUM_TEAMSKINS	6
#define NUM_TEAMSKINS_SINGLEPLAYER 4
struct base_s;

typedef struct chr_list_s {
	character_t* chr[MAX_ACTIVETEAM];
	int num;	/* Number of entries */
} chrList_t;

void CL_SaveInventory(sizebuf_t * buf, inventory_t * i);
void CL_NetReceiveItem(struct dbuffer * buf, item_t * item, int * container, int * x, int * y);
void CL_LoadInventory(sizebuf_t * buf, inventory_t * i);
void CL_ResetTeams(void);
void CL_ParseResults(struct dbuffer *msg);
void CL_SendCurTeamInfo(struct dbuffer * buf, chrList_t *team);
void CL_AddCarriedToEq(struct aircraft_s *aircraft, equipDef_t * equip);
void CL_ParseCharacterData(struct dbuffer *msg, qboolean updateCharacter);
qboolean CL_SoldierInAircraft(int employee_idx, employeeType_t employeeType, int aircraft_idx);
void CL_RemoveSoldierFromAircraft(int employee_idx, employeeType_t employeeType, int aircraft_idx, struct base_s *base);
void CL_RemoveSoldiersFromAircraft(int aircraft_idx, struct base_s *base);

#include "cl_menu.h"

/* cl_radar.c */
#include "cl_radar.h"

/* cl_research.c */
#include "cl_research.h"

/* cl_produce.c */
#include "cl_produce.h"

/* cl_aliencont.c */
#include "cl_aliencont.h"

/* cl_basemanagment.c */
/* we need this in cl_aircraft.h */
#define MAX_EMPLOYEES 512

/* cl_aircraft.c */
#include "cl_aircraft.h"

/* cl_airfight.c */
#include "cl_airfight.h"

/* cl_basemanagment.c */
/* needs the MAX_ACTIVETEAM definition from above. */
#include "cl_basemanagement.h"

/* cl_employee.c */
#include "cl_employee.h"

/* cl_hospital.c */
#include "cl_hospital.h"

/* cl_transfer.c */
#include "cl_transfer.h"

/* cl_basesummary.c */
#include "cl_basesummary.h"

/* cl_mapfightequip.c */
#include "cl_mapfightequip.h"

/* MISC */
/* @todo: needs to be sorted (e.g what file is it defined?) */
#define MAX_TEAMDATASIZE	32768

/* cl_team.c: CL_UpdateHireVar(), CL_ReloadAndRemoveCarried() */
/* cl_team.c should have own header file afterall 24042007 Zenerka */
qboolean CL_SoldierAwayFromBase(employee_t *soldier);
void CL_UpdateHireVar(aircraft_t *aircraft, employeeType_t employeeType);
void CL_ReloadAndRemoveCarried(aircraft_t *aircraft, equipDef_t * equip);
void CL_CleanTempInventory(base_t* base);

void CL_ResetCharacters(base_t* const base);
void CL_ResetTeamInBase(void);
void CL_GenerateCharacter(employee_t *employee, const char *team, employeeType_t employeeType);
char* CL_GetTeamSkinName(int id);


/* END MISC */

/* stats */

typedef struct stats_s {
	int missionsWon;
	int missionsLost;
	int basesBuild;
	int basesAttacked;
	int interceptions;
	int soldiersLost;
	int soldiersNew;			/**< new recruits */
	int killedAliens;
	int rescuedCivilians;
	int researchedTechnologies;
	int moneyInterceptions;
	int moneyBases;
	int moneyResearch;
	int moneyWeapons;
} stats_t;

extern stats_t stats;

#include "cl_campaign.h"
missionResults_t missionresults;	/**< Mission results pointer used for Menu Won. */

/* cl_map.c */
#include "cl_map.h"

/* cl_inventory.c */
#include "cl_inventory.h"

/**
 * @brief List of currently displayed or equipeable characters.
 * @sa cl_team.c and cl_menu.c for usage.
 */
extern chrList_t chrDisplayList;

/* cl_particle.c */
void CL_ParticleRegisterArt(void);
void CL_ResetParticles(void);
void CL_ParticleRun(void);
void CL_RunMapParticles(void);
int CL_ParseParticle(const char *name, const char **text);
void CL_InitParticles(void);
ptl_t *CL_ParticleSpawn(const char *name, int levelFlags, const vec3_t s, const vec3_t v, const vec3_t a);
void PE_RenderParticles(void);
void CL_ParticleVisible(ptl_t *p, qboolean hide);

/* cl_parse.c */
extern const char *ev_format[128];
extern qboolean blockEvents;
void CL_BlockEvents(void);
void CL_UnblockEvents(void);

void CL_SetLastMoving(le_t *le);
void CL_ParseServerMessage(int cmd, struct dbuffer *msg);
qboolean CL_CheckOrDownloadFile(const char *filename);
void CL_DrawLineOfSight(le_t *watcher, le_t *target);

/* cl_view.c */
extern int map_maxlevel;
extern int map_maxlevel_base;

void V_Init(void);
void V_RenderView(void);
void V_UpdateRefDef(void);
void V_CenterView(pos3_t pos);
void V_CalcFovX(void);

/* cl_sequence.c */
void CL_SequenceRender(void);
void CL_Sequence2D(void);
void CL_SequenceClick_f(void);
void CL_SequenceStart_f(void);
void CL_SequenceEnd_f(void);
void CL_ResetSequences(void);
void CL_ParseSequence(const char *name, const char **text);

/* cl_ufo.c */
enum {
	UFO_IS_NO_TARGET,
	UFO_IS_TARGET_OF_MISSILE,
	UFO_IS_TARGET_OF_LASER
};

ufoType_t UFO_ShortNameToID(const char *token);
const char* UFO_TypeToShortName(ufoType_t type);
const char* UFO_TypeToName(ufoType_t type);
void UFO_FleePhalanxAircraft(aircraft_t *ufo, const vec2_t v);
void UFO_CampaignRunUfos(int dt);
void UFO_CampaignCheckEvents(void);
void UFO_Reset(void);
void UFO_RemoveUfoFromGeoscape(aircraft_t* ufo);
void UFO_PrepareRecovery(base_t *base);
void UFO_Recovery(void);
qboolean UFO_ConditionsForStoring(const base_t *base, const aircraft_t *ufocraft);
qboolean UFO_UFOCanShoot(const aircraft_t *ufo);

/* cl_popup.c */
void CL_PopupInit(void);
void CL_PopupNotifyMissionRemoved(const actMis_t* mission);
void CL_PopupNotifyUfoRemoved(const aircraft_t* ufo);
void CL_PopupNotifyUfoDisappeared(const aircraft_t* ufo);
void CL_DisplayPopupAircraft(const aircraft_t* aircraft);
void CL_DisplayPopupIntercept(struct actMis_s* mission, aircraft_t* ufo);

#endif /* CLIENT_CLIENT_H */
