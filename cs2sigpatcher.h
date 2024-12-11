#ifndef _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
#define _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_

#include <ISmmPlugin.h>
#include <igameevents.h>
#include <sh_vector.h>
#include "network_connection.pb.h"
#include "mempatch.h"

class CS2SigPatcher : public ISmmPlugin, public IMetamodListener
{
public:
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);
	bool Unload(char *error, size_t maxlen);
	bool Pause(char *error, size_t maxlen);
	bool Unpause(char *error, size_t maxlen);
	void AllPluginsLoaded();
private:
	bool InitPatches(CGameConfig* g_GameConfig);
	void UndoPatches();
public:
	void Hook_StartupServer(const GameSessionConfiguration_t& config, ISource2WorldSession* pSession, const char* pszMapName);
public:
	const char *GetAuthor();
	const char *GetName();
	const char *GetDescription();
	const char *GetURL();
	const char *GetLicense();
	const char *GetVersion();
	const char *GetDate();
	const char *GetLogTag();
};

extern CS2SigPatcher g_CS2SigPatcher;

PLUGIN_GLOBALVARS();

#endif //_INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
