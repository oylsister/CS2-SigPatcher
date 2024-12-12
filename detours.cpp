#include "usercmd.pb.h"
#include "cs_usercmd.pb.h"
#include "networkbasetypes.pb.h"

#include "entity/ctriggerpush.h"
#include "entity/cbaseentity.h"

#include "detours.h"
#include "tier0/vprof.h"

CUtlVector<CDetourBase*> g_vecDetours;
extern CGlobalVars* gpGlobals;

DECLARE_DETOUR(ProcessUsercmds, Detour_ProcessUsercmds);
DECLARE_DETOUR(TriggerPush_Touch, Detour_TriggerPush_Touch);

class CUserCmd
{
public:
	[[maybe_unused]] char pad0[0x10];
	CSGOUserCmdPB cmd;
	[[maybe_unused]] char pad1[0x38];
#ifdef PLATFORM_WINDOWS
	[[maybe_unused]] char pad2[0x8];
#endif
};

void* FASTCALL Detour_ProcessUsercmds(CCSPlayerController* pController, CUserCmd* cmds, int numcmds, bool paused, float margin)
{
	VPROF_SCOPE_BEGIN("Detour_ProcessUsercmds");

	for (int i = 0; i < numcmds; i++)
	{
		cmds[i].cmd.mutable_base()->mutable_subtick_moves()->Clear();

		// subtick attacker.
		/*
		cmds[i].cmd.set_attack1_start_history_index(-1);
		cmds[i].cmd.set_attack2_start_history_index(-1);
		cmds[i].cmd.set_attack3_start_history_index(-1);
		cmds[i].cmd.mutable_input_history()->Clear();
		*/
	}

	VPROF_SCOPE_END();

	return ProcessUsercmds(pController, cmds, numcmds, paused, margin);
}

void FASTCALL Detour_TriggerPush_Touch(CTriggerPush* pPush, CBaseEntity* pOther)
{
	// This trigger pushes only once (and kills itself) or pushes only on StartTouch, both of which are fine already
	if (pPush->m_spawnflags() & SF_TRIG_PUSH_ONCE || pPush->m_bTriggerOnStartTouch())
	{
		TriggerPush_Touch(pPush, pOther);
		return;
	}

	MoveType_t movetype = pOther->m_nActualMoveType();

	// VPhysics handling doesn't need any changes
	if (movetype == MOVETYPE_VPHYSICS)
	{
		TriggerPush_Touch(pPush, pOther);
		return;
	}

	if (movetype == MOVETYPE_NONE || movetype == MOVETYPE_PUSH || movetype == MOVETYPE_NOCLIP)
		return;

	CCollisionProperty* collisionProp = pOther->m_pCollision();
	if (!IsSolid(collisionProp->m_nSolidType(), collisionProp->m_usSolidFlags()))
		return;

	if (!pPush->PassesTriggerFilters(pOther))
		return;

	if (pOther->m_CBodyComponent()->m_pSceneNode()->m_pParent())
		return;

	Vector vecAbsDir;
	matrix3x4_t matTransform = pPush->m_CBodyComponent()->m_pSceneNode()->EntityToWorldTransform();

	Vector vecPushDir = pPush->m_vecPushDirEntitySpace();
	VectorRotate(vecPushDir, matTransform, vecAbsDir);

	Vector vecPush = vecAbsDir * pPush->m_flSpeed();

	uint32 flags = pOther->m_fFlags();

	if (flags & FL_BASEVELOCITY)
	{
		vecPush = vecPush + pOther->m_vecBaseVelocity();
	}

	if (vecPush.z > 0 && (flags & FL_ONGROUND))
	{
		pOther->SetGroundEntity(nullptr);
		Vector origin = pOther->GetAbsOrigin();
		origin.z += 1.0f;

		pOther->Teleport(&origin, nullptr, nullptr);
	}

	Vector vecEntBaseVelocity = pOther->m_vecBaseVelocity;
	Vector vecOrigPush = vecAbsDir * pPush->m_flSpeed();

	Message("Pushing entity %i | frame = %i | tick = %i | entity basevelocity %s = %.2f %.2f %.2f | original push velocity = %.2f %.2f %.2f | final push velocity = %.2f %.2f %.2f\n",
		pOther->GetEntityIndex(),
		gpGlobals->framecount,
		gpGlobals->tickcount,
		(flags & FL_BASEVELOCITY) ? "WITH FLAG" : "",
		vecEntBaseVelocity.x, vecEntBaseVelocity.y, vecEntBaseVelocity.z,
		vecOrigPush.x, vecOrigPush.y, vecOrigPush.z,
		vecPush.x, vecPush.y, vecPush.z);

	pOther->m_vecBaseVelocity(vecPush);

	flags |= (FL_BASEVELOCITY);
	pOther->m_fFlags(flags);
}

bool InitDetours(CGameConfig* gameConfig)
{
	bool success = true;

	FOR_EACH_VEC(g_vecDetours, i)
	{
		if (!g_vecDetours[i]->CreateDetour(gameConfig))
			success = false;

		g_vecDetours[i]->EnableDetour();
	}

	return success;
}

void FlushAllDetours()
{
	g_vecDetours.Purge();
}