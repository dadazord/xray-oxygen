#include "stdafx.h"
#include "xrserver.h"
#include "game_sv_single.h"
#include "xrMessages.h"
#include "game_cl_single.h"
#include "MainMenu.h"
#include "../xrEngine/x_ray.h"

#pragma warning(push)
#pragma warning(disable:4995)
#include <malloc.h>
#pragma warning(pop)

xrServer::EConnect xrServer::Connect(shared_str &session_name)
{
#ifdef DEBUG
	Msg						("* sv_Connect: %s",	*session_name);
#endif

	// Parse options and create game
	if (!strchr(*session_name,'/'))
		return				ErrConnect;

	string1024				options;
	R_ASSERT2(xr_strlen(session_name) <= sizeof(options), "session_name too BIIIGGG!!!");
	xr_strcpy					(options,strchr(*session_name,'/')+1);
	
	// Parse game type
	string1024				type;
	R_ASSERT2(xr_strlen(options) <= sizeof(type), "session_name too BIIIGGG!!!");
	xr_strcpy					(type,options);
	if (strchr(type,'/'))	*strchr(type,'/') = 0;
	game					= NULL;

	CLASS_ID clsid			= game_GameState::getCLASS_ID(type,true);
	game					= smart_cast<game_sv_GameState*> (NEW_INSTANCE(clsid));
	
	// Options
	if (0==game)			return ErrConnect;
	
	game->Create			(session_name);

	return IPureServer::Connect(*session_name);
}

void xrServer::AttachNewClient			(IClient* CL)
{
	MSYS_CONFIG	msgConfig;
	msgConfig.sign1 = 0x12071980;
	msgConfig.sign2 = 0x26111975;

    SV_Client			= CL;
	CL->flags.bLocal	= 1;
	SendTo_LL(&msgConfig, sizeof(msgConfig));

	// gen message
	RequestClientDigest(CL);
}

void xrServer::RequestClientDigest(IClient* CL)
{
	Check_BuildVersion_Success(CL);
	return;
}