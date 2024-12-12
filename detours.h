#pragma once

#include "cdetour.h"

class CCSPlayerController;
class CBaseEntity;
class CTriggerPush;
class CUserCmd;

bool InitDetours(CGameConfig* gameConfig);
void FlushAllDetours();

void* FASTCALL Detour_ProcessUsercmds(CCSPlayerController* pController, CUserCmd* cmds, int numcmds, bool paused, float margin);
void FASTCALL Detour_TriggerPush_Touch(CTriggerPush* pPush, CBaseEntity* pOther);