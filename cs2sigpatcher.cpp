#include <stdio.h>
#include "cs2sigpatcher.h"
#include "iserver.h"
#include "addresses.h"

CS2SigPatcher g_CS2SigPatcher;
IVEngineServer2 *g_pEngineServer2 = nullptr;
CGlobalVars* gpGlobals = nullptr;
CGameConfig* g_GameConfig = nullptr;

CMemPatch g_CommonPatches[] =
{
	CMemPatch("ServerMovementUnlock", "ServerMovementUnlock"),
	CMemPatch("CheckJumpButtonWater", "FixWaterFloorJump"),
	CMemPatch("WaterLevelGravity", "WaterLevelGravity"),
	CMemPatch("CPhysBox_Use", "CPhysBox_Use"),
	CMemPatch("BotNavIgnore", "BotNavIgnore"),
};

bool CS2SigPatcher::InitPatches(CGameConfig * g_GameConfig)
{
	bool success = true;
	for (int i = 0; i < sizeof(g_CommonPatches) / sizeof(*g_CommonPatches); i++)
	{
		if (!g_CommonPatches[i].PerformPatch(g_GameConfig))
		{
			Panic("Failed to patch\n");
			success = false;
		}
	}

	return success;
}

void CS2SigPatcher::UndoPatches()
{
	for (int i = 0; i < sizeof(g_CommonPatches) / sizeof(*g_CommonPatches); i++)
		g_CommonPatches[i].UndoPatch();
}

PLUGIN_EXPOSE(CS2SigPatcher, g_CS2SigPatcher);
bool CS2SigPatcher::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, g_pEngineServer2, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	
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

	Message("Starting plugin.\n");

	addresses::Initialize(g_GameConfig);
	InitPatches(g_GameConfig);

	if (g_GameConfig)
		delete g_GameConfig;

	if (late)
	{
		gpGlobals = g_pEngineServer2->GetServerGlobals();
	}

	return true;
}

bool CS2SigPatcher::Unload(char *error, size_t maxlen)
{
	UndoPatches();
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

void CS2SigPatcher::Hook_StartupServer(const GameSessionConfiguration_t& config, ISource2WorldSession* pSession, const char* pszMapName)
{
	gpGlobals = g_pEngineServer2->GetServerGlobals();

	Message("Hook_StartupServer: %s\n", pszMapName);
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
	return "1.0";
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
	return "https://github.com/oylsister/CS2-SigPatcher";
}
