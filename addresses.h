#pragma once
#include "platform.h"
#include "stdint.h"
#include "utils/module.h"
#include "utlstring.h"
#include "variant.h"

namespace modules
{
	inline CModule *engine;
	inline CModule *tier0;
	inline CModule *server;
	inline CModule *schemasystem;
	inline CModule *vscript;
	inline CModule *client;
	inline CModule* networksystem;
	inline CModule* vphysics2;
#ifdef _WIN32
	inline CModule *hammer;
#endif
}