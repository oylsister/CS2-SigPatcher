#include <stdio.h>
#include "cs2sigpatcher.h"
#include "iserver.h"

SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);
SH_DECL_HOOK4_void(IServerGameClients, ClientActive, SH_NOATTRIB, 0, CPlayerSlot, bool, const char *, uint64);
SH_DECL_HOOK5_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, CPlayerSlot, ENetworkDisconnectionReason, const char *, uint64, const char *);
SH_DECL_HOOK4_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, CPlayerSlot, char const *, int, uint64);
SH_DECL_HOOK1_void(IServerGameClients, ClientSettingsChanged, SH_NOATTRIB, 0, CPlayerSlot );
SH_DECL_HOOK6_void(IServerGameClients, OnClientConnected, SH_NOATTRIB, 0, CPlayerSlot, const char*, uint64, const char *, const char *, bool);
SH_DECL_HOOK6(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, CPlayerSlot, const char*, uint64, const char *, bool, CBufferString *);
SH_DECL_HOOK2(IGameEventManager2, FireEvent, SH_NOATTRIB, 0, bool, IGameEvent *, bool);

SH_DECL_HOOK2_void( IServerGameClients, ClientCommand, SH_NOATTRIB, 0, CPlayerSlot, const CCommand & );

CS2SigPatcher g_CS2SigPatcher;
IServerGameDLL *server = NULL;
IServerGameClients *gameclients = NULL;
IVEngineServer *engine = NULL;
IVEngineServer2* g_pEngineServer2 = nullptr;
IGameEventManager2 *gameevents = NULL;
CGameConfig* g_GameConfig = nullptr;

CMemPatch g_CommonPatches[] =
{
	CMemPatch("ServerMovementUnlock", "ServerMovementUnlock"),
	CMemPatch("CheckJumpButtonWater", "FixWaterFloorJump"),
	CMemPatch("WaterLevelGravity", "WaterLevelGravity"),
	CMemPatch("CPhysBox_Use", "CPhysBox_Use"),
	CMemPatch("BotNavIgnore", "BotNavIgnore"),
};

// Should only be called within the active game loop (i e map should be loaded and active)
// otherwise that'll be nullptr!
CGlobalVars *GetGameGlobals()
{
	INetworkGameServer *server = g_pNetworkServerService->GetIGameServer();

	if(!server)
		return nullptr;

	return g_pNetworkServerService->GetIGameServer()->GetGlobals();
}

bool CS2SigPatcher::InitPatches(CGameConfig * g_GameConfig)
{
	bool success = true;
	for (int i = 0; i < sizeof(g_CommonPatches) / sizeof(*g_CommonPatches); i++)
	{
		if (!g_CommonPatches[i].PerformPatch(g_GameConfig))
			success = false;
	}

	return success;
}

PLUGIN_EXPOSE(CS2SigPatcher, g_CS2SigPatcher);
bool CS2SigPatcher::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, g_pEngineServer2, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_ANY(GetServerFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_ANY(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_ANY(GetEngineFactory, g_pNetworkServerService, INetworkServerService, NETWORKSERVERSERVICE_INTERFACE_VERSION);

	CBufferStringGrowable<256> gamedirpath;
	g_pEngineServer2->GetGameDir(gamedirpath);

	std::string gamedirname = CGameConfig::GetDirectoryName(gamedirpath.Get());

	const char* gamedataPath = "addons/cs2sigpatcher/gamedata/cs2sigpatcher.games.txt";
	Message("Loading %s for game: %s\n", gamedataPath, gamedirname.c_str());

	g_GameConfig = new CGameConfig(gamedirname, gamedataPath);
	char conf_error[255] = "";
	if (!g_GameConfig->Init(g_pFullFileSystem, conf_error, sizeof(conf_error)))
	{
		snprintf(error, maxlen, "Could not read %s: %s", g_GameConfig->GetPath().c_str(), conf_error);
		Panic("%s\n", error);
		return false;
	}

	// Required to get the IMetamodListener events
	g_SMAPI->AddListener( this, this );

	META_CONPRINTF( "Starting plugin.\n" );

	SH_ADD_HOOK(IServerGameDLL, GameFrame, server, SH_MEMBER(this, &CS2SigPatcher::Hook_GameFrame), true);
	SH_ADD_HOOK(IServerGameClients, ClientActive, gameclients, SH_MEMBER(this, &CS2SigPatcher::Hook_ClientActive), true);
	SH_ADD_HOOK(IServerGameClients, ClientDisconnect, gameclients, SH_MEMBER(this, &CS2SigPatcher::Hook_ClientDisconnect), true);
	SH_ADD_HOOK(IServerGameClients, ClientPutInServer, gameclients, SH_MEMBER(this, &CS2SigPatcher::Hook_ClientPutInServer), true);
	SH_ADD_HOOK(IServerGameClients, ClientSettingsChanged, gameclients, SH_MEMBER(this, &CS2SigPatcher::Hook_ClientSettingsChanged), false);
	SH_ADD_HOOK(IServerGameClients, OnClientConnected, gameclients, SH_MEMBER(this, &CS2SigPatcher::Hook_OnClientConnected), false);
	SH_ADD_HOOK(IServerGameClients, ClientConnect, gameclients, SH_MEMBER(this, &CS2SigPatcher::Hook_ClientConnect), false);
	SH_ADD_HOOK(IServerGameClients, ClientCommand, gameclients, SH_MEMBER(this, &CS2SigPatcher::Hook_ClientCommand), false);

	META_CONPRINTF( "All hooks started!\n" );

	InitPatches(g_GameConfig);

	if (g_GameConfig)
		delete g_GameConfig;

	return true;
}

bool CS2SigPatcher::Unload(char *error, size_t maxlen)
{
	SH_REMOVE_HOOK(IServerGameDLL, GameFrame, server, SH_MEMBER(this, &CS2SigPatcher::Hook_GameFrame), true);
	SH_REMOVE_HOOK(IServerGameClients, ClientActive, gameclients, SH_MEMBER(this, &CS2SigPatcher::Hook_ClientActive), true);
	SH_REMOVE_HOOK(IServerGameClients, ClientDisconnect, gameclients, SH_MEMBER(this, &CS2SigPatcher::Hook_ClientDisconnect), true);
	SH_REMOVE_HOOK(IServerGameClients, ClientPutInServer, gameclients, SH_MEMBER(this, &CS2SigPatcher::Hook_ClientPutInServer), true);
	SH_REMOVE_HOOK(IServerGameClients, ClientSettingsChanged, gameclients, SH_MEMBER(this, &CS2SigPatcher::Hook_ClientSettingsChanged), false);
	SH_REMOVE_HOOK(IServerGameClients, OnClientConnected, gameclients, SH_MEMBER(this, &CS2SigPatcher::Hook_OnClientConnected), false);
	SH_REMOVE_HOOK(IServerGameClients, ClientConnect, gameclients, SH_MEMBER(this, &CS2SigPatcher::Hook_ClientConnect), false);
	SH_REMOVE_HOOK(IServerGameClients, ClientCommand, gameclients, SH_MEMBER(this, &CS2SigPatcher::Hook_ClientCommand), false);

	return true;
}

void Message(const char* msg, ...)
{
	va_list args;
	va_start(args, msg);

	char buf[1024] = {};
	V_vsnprintf(buf, sizeof(buf) - 1, msg, args);

	ConColorMsg(Color(255, 0, 255, 255), "[CS2SigPatcher] %s", buf);

	va_end(args);
}

void Panic(const char* msg, ...)
{
	va_list args;
	va_start(args, msg);

	char buf[1024] = {};
	V_vsnprintf(buf, sizeof(buf) - 1, msg, args);

	Warning("[CS2SigPatcher] %s", buf);

	va_end(args);
}

void CS2SigPatcher::AllPluginsLoaded()
{
	/* This is where we'd do stuff that relies on the mod or other plugins 
	 * being initialized (for example, cvars added and events registered).
	 */
}

void CS2SigPatcher::Hook_ClientActive( CPlayerSlot slot, bool bLoadGame, const char *pszName, uint64 xuid )
{
	META_CONPRINTF( "Hook_ClientActive(%d, %d, \"%s\", %d)\n", slot, bLoadGame, pszName, xuid );
}

void CS2SigPatcher::Hook_ClientCommand( CPlayerSlot slot, const CCommand &args )
{
	META_CONPRINTF( "Hook_ClientCommand(%d, \"%s\")\n", slot, args.GetCommandString() );
}

void CS2SigPatcher::Hook_ClientSettingsChanged( CPlayerSlot slot )
{
	META_CONPRINTF( "Hook_ClientSettingsChanged(%d)\n", slot );
}

void CS2SigPatcher::Hook_OnClientConnected( CPlayerSlot slot, const char *pszName, uint64 xuid, const char *pszNetworkID, const char *pszAddress, bool bFakePlayer )
{
	META_CONPRINTF( "Hook_OnClientConnected(%d, \"%s\", %d, \"%s\", \"%s\", %d)\n", slot, pszName, xuid, pszNetworkID, pszAddress, bFakePlayer );
}

bool CS2SigPatcher::Hook_ClientConnect( CPlayerSlot slot, const char *pszName, uint64 xuid, const char *pszNetworkID, bool unk1, CBufferString *pRejectReason )
{
	META_CONPRINTF( "Hook_ClientConnect(%d, \"%s\", %d, \"%s\", %d, \"%s\")\n", slot, pszName, xuid, pszNetworkID, unk1, pRejectReason->ToGrowable()->Get() );

	RETURN_META_VALUE(MRES_IGNORED, true);
}

void CS2SigPatcher::Hook_ClientPutInServer( CPlayerSlot slot, char const *pszName, int type, uint64 xuid )
{
	META_CONPRINTF( "Hook_ClientPutInServer(%d, \"%s\", %d, %d)\n", slot, pszName, type, xuid );
}

void CS2SigPatcher::Hook_ClientDisconnect( CPlayerSlot slot, ENetworkDisconnectionReason reason, const char *pszName, uint64 xuid, const char *pszNetworkID )
{
	META_CONPRINTF( "Hook_ClientDisconnect(%d, %d, \"%s\", %d, \"%s\")\n", slot, reason, pszName, xuid, pszNetworkID );
}

void CS2SigPatcher::Hook_GameFrame( bool simulating, bool bFirstTick, bool bLastTick )
{
	/**
	 * simulating:
	 * ***********
	 * true  | game is ticking
	 * false | game is not ticking
	 */
}

void CS2SigPatcher::OnLevelInit( char const *pMapName,
									 char const *pMapEntities,
									 char const *pOldLevel,
									 char const *pLandmarkName,
									 bool loadGame,
									 bool background )
{
	META_CONPRINTF("OnLevelInit(%s)\n", pMapName);
}

void CS2SigPatcher::OnLevelShutdown()
{
	META_CONPRINTF("OnLevelShutdown()\n");
}

bool CS2SigPatcher::Pause(char *error, size_t maxlen)
{
	return true;
}

bool CS2SigPatcher::Unpause(char *error, size_t maxlen)
{
	return true;
}

const char *CS2SigPatcher::GetLicense()
{
	return "Public Domain";
}

const char *CS2SigPatcher::GetVersion()
{
	return "1.0.0.0";
}

const char *CS2SigPatcher::GetDate()
{
	return __DATE__;
}

const char *CS2SigPatcher::GetLogTag()
{
	return "CS2SIGPATCH";
}

const char *CS2SigPatcher::GetAuthor()
{
	return "Oylsister";
}

const char *CS2SigPatcher::GetDescription()
{
	return "Patching Signature for CS2";
}

const char *CS2SigPatcher::GetName()
{
	return "CS2 Signature Patcher";
}

const char *CS2SigPatcher::GetURL()
{
	return "http://www.sourcemm.net/";
}
