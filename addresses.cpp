#include "addresses.h"
#include "utils/module.h"
#include "gameconfig.h"

#include "tier0/memdbgon.h"

extern CGameConfig* g_GameConfig;

#define RESOLVE_SIG(gameConfig, name, variable) \
	variable = (decltype(variable))gameConfig->ResolveSignature(name);	\
	if (!variable)														\
		return false;													\
	Message("Found %s at 0x%p\n", name, variable);

bool addresses::Initialize(CGameConfig* g_GameConfig)
{
	modules::engine = new CModule(ROOTBIN, "engine2");
	modules::tier0 = new CModule(ROOTBIN, "tier0");
	modules::server = new CModule(GAMEBIN, "server");
	modules::schemasystem = new CModule(ROOTBIN, "schemasystem");
	modules::vscript = new CModule(ROOTBIN, "vscript");
	modules::networksystem = new CModule(ROOTBIN, "networksystem");
	modules::vphysics2 = new CModule(ROOTBIN, "vphysics2");
	modules::client = nullptr;

	if (!CommandLine()->HasParm("-dedicated"))
		modules::client = new CModule(GAMEBIN, "client");

#ifdef _WIN32
	modules::hammer = nullptr;
	if (CommandLine()->HasParm("-tools"))
		modules::hammer = new CModule(ROOTBIN, "tools/hammer");
#endif
	RESOLVE_SIG(g_GameConfig, "SetGroundEntity", addresses::SetGroundEntity);

	return true;
}