#include "stdafx.h"
#include "gamepersistent.h"
#include "../xrEngine/fmesh.h"
#include "../xrEngine/xr_ioconsole.h"
#include "../xrEngine/gamemtllib.h"
#include "../Include/xrRender/Kinematics.h"
#include "profiler.h"
#include "MainMenu.h"
#include "UICursor.h"
#include "game_base_space.h"
#include "level.h"
#include "../xrParticles/psystem.h"
#include "../xrParticles/ParticlesObject.h"
#include "game_base_space.h"
#include "stalker_animation_data_storage.h"
#include "stalker_velocity_holder.h"

#include "ActorEffector.h"
#include "actor.h"

#include "UI/UItextureMaster.h"
#include "ai_space.h"
#include "../xrServerEntities/script_engine.h"

#include "holder_custom.h"
#include "game_cl_base.h"
#include "xrserver_objects_alife_monsters.h"
#include "../xrServerEntities/xrServer_Object_Base.h"
#include "UI/UIGameTutorial.h"

#ifndef MASTER_GOLD
#	include "custommonster.h"
#endif // MASTER_GOLD

#ifndef _EDITOR
#	include "ai_debug.h"
#endif // _EDITOR

using MySuper = IGame_Persistent;

CGamePersistent::CGamePersistent(void)
{
	m_bPickableDOF				= false;
	m_game_params.m_e_game_type	= eGameIDNoGame;
	ambient_effect_next_time	= 0;
	ambient_effect_stop_time	= 0;
	ambient_particles			= 0;

	ambient_effect_wind_start	= 0.f;
	ambient_effect_wind_in_time	= 0.f;
	ambient_effect_wind_end		= 0.f;
	ambient_effect_wind_out_time= 0.f;
	ambient_effect_wind_on		= false;

    std::memset(ambient_sound_next_time, 0, sizeof(ambient_sound_next_time));
	
	m_pUI_core					= nullptr;
	m_pMainMenu					= nullptr;
	m_intro						= nullptr;
	m_intro_event.bind			(this, &CGamePersistent::start_logo_intro);
#ifdef DEBUG
	m_frame_counter				= 0;
	m_last_stats_frame			= u32(-2);
#endif

    m_useThirst = (0 != strstr(Core.Params, "-thrist"));

	eQuickLoad				= Engine.Event.Handler_Attach("Game:QuickLoad",this);
	Fvector3* DofValue		= Console->GetFVectorPtr("r2_dof");
	SetBaseDof				(*DofValue);
}

CGamePersistent::~CGamePersistent(void)
{	
	Device.seqFrame.Remove		(this);
	Engine.Event.Handler_Detach	(eQuickLoad,this);
}

void CGamePersistent::RegisterModel(IRenderVisual* V)
{
	// Check types
	switch (V->getType())
	{
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID:
	{
		u16 def_idx		= GMLib.GetMaterialIdx("default_object");
		R_ASSERT2		(GMLib.GetMaterialByIdx(def_idx)->Flags.is(SGameMtl::flDynamic),"'default_object' - must be dynamic");
		IKinematics* K	= smart_cast<IKinematics*>(V); VERIFY(K);
		int cnt = K->LL_BoneCount();
		for (u16 k=0; k<cnt; k++){
			CBoneData& bd	= K->LL_GetData(k); 
			if (*(bd.game_mtl_name)){
				bd.game_mtl_idx	= GMLib.GetMaterialIdx(*bd.game_mtl_name);
				R_ASSERT2(GMLib.GetMaterialByIdx(bd.game_mtl_idx)->Flags.is(SGameMtl::flDynamic),"Required dynamic game material");
			}else{
				bd.game_mtl_idx	= def_idx;
			}
		}
	}break;
	}
}

extern void clean_game_globals	();
extern void init_game_globals	();

void CGamePersistent::OnAppStart()
{
	// load game materials
	GMLib.Load					();
	init_game_globals			();
	MySuper::OnAppStart			();
	m_pUI_core					= xr_new<ui_core>();
	m_pMainMenu					= xr_new<CMainMenu>();
}


void CGamePersistent::OnAppEnd	()
{
	if(m_pMainMenu->IsActive())
		m_pMainMenu->Activate(false);

	xr_delete					(m_pMainMenu);
	xr_delete					(m_pUI_core);

	MySuper::OnAppEnd			();

	clean_game_globals			();

	GMLib.Unload				();

}

void CGamePersistent::Start		(LPCSTR op)
{
	MySuper::Start				(op);
}

void CGamePersistent::Disconnect()
{
	// destroy ambient particles
	CParticlesObject::Destroy(ambient_particles);

	MySuper::Disconnect			();
	// stop all played emitters
	::Sound->stop_emitters		();
	m_game_params.m_e_game_type	= eGameIDNoGame;
}

#include "xr_level_controller.h"

void CGamePersistent::OnGameStart()
{
	MySuper::OnGameStart		();
	UpdateGameType				();
}

const char* GameTypeToString(EGameIDs gt, bool bShort)
{
	return (gt == eGameIDSingle) ? "single" : "---";
}

EGameIDs ParseStringToGameType(LPCSTR str)
{
	return (!xr_strcmp(str, "single")) ? eGameIDSingle : eGameIDNoGame;
}

void CGamePersistent::UpdateGameType			()
{
	MySuper::UpdateGameType();

	m_game_params.m_e_game_type = eGameIDSingle;
}

void CGamePersistent::OnGameEnd()
{
	MySuper::OnGameEnd();

	xr_delete(g_stalker_animation_data_storage);
	xr_delete(g_stalker_velocity_holder);
}

void CGamePersistent::WeathersUpdate()
{
	if (g_pGameLevel)
	{
		CActor* actor				= smart_cast<CActor*>(Level().CurrentViewEntity());
		BOOL bIndoor				= true;
		if (actor) bIndoor			= actor->renderable_ROS()->get_luminocity_hemi()<0.05f;

		int data_set				= (Random.randF()<(1.f-Environment().CurrentEnv->weight))?0:1; 
		
		CEnvDescriptor* const current_env	= Environment().Current[0]; 
		VERIFY						(current_env);

		CEnvDescriptor* const _env	= Environment().Current[data_set];
		CEnvAmbient * env_amb = nullptr;

		if (!_env)
		{
			Msg("[ERROR] Environment().Current[%d] == nullptr", data_set);
		}
		else
		{
			CEnvAmbient * env_amb = _env->env_ambient;
		}

		if (env_amb) 
		{
			CEnvAmbient::SSndChannelVec& vec	= current_env->env_ambient->get_snd_channels();
            auto I		= vec.begin();
            auto E		= vec.end();
			
			for (u32 idx=0; I!=E; ++I,++idx) 
			{
				CEnvAmbient::SSndChannel& ch	= **I;
				R_ASSERT						(idx<20);

				if(ambient_sound_next_time[idx]==0)//first
				{
					ambient_sound_next_time[idx] = Device.dwTimeGlobal + ch.get_rnd_sound_first_time();
				}
				else if(Device.dwTimeGlobal > ambient_sound_next_time[idx])
				{
					ref_sound& snd					= ch.get_rnd_sound();

					Fvector	pos;
					float	angle		= ::Random.randF(PI_MUL_2);
					pos.x				= _cos(angle);
					pos.y				= 0;
					pos.z				= _sin(angle);
					pos.normalize		().mul(ch.get_rnd_sound_dist()).add(Device.vCameraPosition);
					pos.y				+= 10.f;
					snd.play_at_pos		(0,pos);

					VERIFY							(snd._handle());
					u32 _length_ms					= iFloor(snd.get_length_sec()*1000.0f);
					ambient_sound_next_time[idx]	= Device.dwTimeGlobal + _length_ms + ch.get_rnd_sound_time();
				}
			}

			// start effect
			if ((!bIndoor) && (0==ambient_particles) && Device.dwTimeGlobal>ambient_effect_next_time){
				CEnvAmbient::SEffect* eff			= env_amb->get_rnd_effect(); 
				if (eff){
					Environment().wind_gust_factor	= eff->wind_gust_factor;
					ambient_effect_next_time		= Device.dwTimeGlobal + env_amb->get_rnd_effect_time();
					ambient_effect_stop_time		= Device.dwTimeGlobal + eff->life_time;
					ambient_effect_wind_start		= Device.fTimeGlobal;
					ambient_effect_wind_in_time		= Device.fTimeGlobal + eff->wind_blast_in_time;
					ambient_effect_wind_end			= Device.fTimeGlobal + eff->life_time/1000.f;
					ambient_effect_wind_out_time	= Device.fTimeGlobal + eff->life_time/1000.f + eff->wind_blast_out_time;
					ambient_effect_wind_on			= true;
										
					ambient_particles				= CParticlesObject::Create(eff->particles.c_str(),FALSE,false);
					Fvector pos; pos.add			(Device.vCameraPosition,eff->offset); 
					ambient_particles->play_at_pos	(pos);
					if (eff->sound._handle())		eff->sound.play_at_pos(0,pos);


					Environment().wind_blast_strength_start_value=Environment().wind_strength_factor;
					Environment().wind_blast_strength_stop_value=eff->wind_blast_strength;

					if (Environment().wind_blast_strength_start_value==0.f)
					{
						Environment().wind_blast_start_time.set(0.f,eff->wind_blast_direction.x,eff->wind_blast_direction.y,eff->wind_blast_direction.z);
					}
					else
					{
						Environment().wind_blast_start_time.set(0.f,Environment().wind_blast_direction.x,Environment().wind_blast_direction.y,Environment().wind_blast_direction.z);
					}
					Environment().wind_blast_stop_time.set(0.f,eff->wind_blast_direction.x,eff->wind_blast_direction.y,eff->wind_blast_direction.z);
				}
			}
		}
		if (Device.fTimeGlobal>=ambient_effect_wind_start && Device.fTimeGlobal<=ambient_effect_wind_in_time && ambient_effect_wind_on)
		{
			float delta=ambient_effect_wind_in_time-ambient_effect_wind_start;
			float t = 0;

			if (delta!=0.f)
			{
				float cur_in = Device.fTimeGlobal - ambient_effect_wind_start;
				t = cur_in / delta;
			}

			Environment().wind_blast_current.slerp(Environment().wind_blast_start_time,Environment().wind_blast_stop_time,t);

			Environment().wind_blast_direction.set(Environment().wind_blast_current.x,Environment().wind_blast_current.y,Environment().wind_blast_current.z);
			Environment().wind_strength_factor=Environment().wind_blast_strength_start_value+t*(Environment().wind_blast_strength_stop_value-Environment().wind_blast_strength_start_value);
		}

		// stop if time exceed or indoor
		if (bIndoor || Device.dwTimeGlobal>=ambient_effect_stop_time){
			if (ambient_particles)					ambient_particles->Stop();
			
			Environment().wind_gust_factor = 0.f;
			
		}

		if (Device.fTimeGlobal>=ambient_effect_wind_end && ambient_effect_wind_on)
		{
			Environment().wind_blast_strength_start_value=Environment().wind_strength_factor;
			Environment().wind_blast_strength_stop_value	=0.f;

			ambient_effect_wind_on = false;
		}

		if (Device.fTimeGlobal>=ambient_effect_wind_end &&  Device.fTimeGlobal<=ambient_effect_wind_out_time)
		{
			float delta=ambient_effect_wind_out_time-ambient_effect_wind_end;
			float t;
			if (delta!=0.f)
			{
				float cur_in=Device.fTimeGlobal-ambient_effect_wind_end;
				t=cur_in/delta;
			}
			else
			{
				t=0.f;
			}
			Environment().wind_strength_factor=Environment().wind_blast_strength_start_value+t*(Environment().wind_blast_strength_stop_value-Environment().wind_blast_strength_start_value);
		}
		if (Device.fTimeGlobal>ambient_effect_wind_out_time && ambient_effect_wind_out_time!=0.f )
		{			
			Environment().wind_strength_factor=0.0;
		}

		// if particles not playing - destroy
		if (ambient_particles&&!ambient_particles->IsPlaying())
			CParticlesObject::Destroy(ambient_particles);
	}
}

IC bool allow_intro ()
{
	return !strstr(Core.Params, "-nointro");
}

void CGamePersistent::start_logo_intro()
{
	if(Device.dwPrecacheFrame==0)
	{
		m_intro_event.bind(this, &CGamePersistent::update_logo_intro);
		m_intro = xr_new<CUISequencer>();
		Console->Hide();
	}
}

void CGamePersistent::update_logo_intro()
{
	if(m_intro && (!m_intro->IsActive()))
	{
		m_intro_event			= 0;
		xr_delete				(m_intro);
		Msg("intro_delete ::update_logo_intro");
		Console->Execute		("main_menu on");
	}
	else if(!m_intro)
	{
		m_intro_event			= 0;
	}
}
bool CGamePersistent::OnRenderPPUI_query()
 {
 	return MainMenu()->OnRenderPPUI_query();
 	// enable PP or not
 }

void CGamePersistent::OnRenderPPUI_PP()
{
	MainMenu()->OnRenderPPUI_PP();
}

extern int g_keypress_on_start;
void CGamePersistent::game_loaded()
{
	if(Device.dwPrecacheFrame<=2)
	{
		if(	g_pGameLevel							&&
			g_pGameLevel->bReady					&&
			(allow_intro() && g_keypress_on_start)	&&
			load_screen_renderer.b_need_user_input  )
		{
			VERIFY				(NULL==m_intro);
			m_intro				= xr_new<CUISequencer>();
			m_intro->Start		("game_loaded");
			Msg					("intro_start game_loaded");
			m_intro->m_on_destroy_event.bind(this, &CGamePersistent::update_game_loaded);
		}
		m_intro_event			= 0;
	}
}

void CGamePersistent::update_game_loaded()
{
	xr_delete				(m_intro);
	Msg("intro_delete ::update_game_loaded");
	start_game_intro		();
}

void CGamePersistent::start_game_intro		()
{
	if(!allow_intro())
	{
		m_intro_event			= 0;
		return;
	}

	if (g_pGameLevel && g_pGameLevel->bReady && Device.dwPrecacheFrame<=2)
	{
		m_intro_event.bind		(this, &CGamePersistent::update_game_intro);
		if (0==stricmp(m_game_params.m_new_or_load, "new"))
		{
			VERIFY				(NULL==m_intro);
			m_intro				= xr_new<CUISequencer>();
			m_intro->Start		("intro_game");
			Msg("intro_start intro_game");
		}
	}
}

void CGamePersistent::update_game_intro()
{
	if(m_intro && (false==m_intro->IsActive()))
	{
		xr_delete				(m_intro);
		Msg("intro_delete ::update_game_intro");
		m_intro_event			= 0;
	}
	else
	if(!m_intro)
	{
		m_intro_event			= 0;
	}
}

extern CUISequencer * g_tutorial;
extern CUISequencer * g_tutorial2;

void CGamePersistent::OnFrame	()
{
	if(Device.dwPrecacheFrame==5 && m_intro_event.empty())
	{
		SetLoadStageTitle();
		m_intro_event.bind			(this,&CGamePersistent::game_loaded);
	}

	if(g_tutorial2)
	{ 
		g_tutorial2->Destroy	();
		xr_delete				(g_tutorial2);
	}

	if(g_tutorial && !g_tutorial->IsActive())
	{
		xr_delete(g_tutorial);
	}
	if(!(Device.dwFrame % 200))
		CUITextureMaster::FreeCachedShaders();

#ifdef DEBUG
	++m_frame_counter;
#endif
	if (!m_intro_event.empty())	m_intro_event();
	
	if(Device.dwPrecacheFrame==0 && !m_intro && m_intro_event.empty())
		load_screen_renderer.stop();

	if( !m_pMainMenu->IsActive() )
		m_pMainMenu->DestroyInternal(false);

	if(!g_pGameLevel)			return;
	if(!g_pGameLevel->bReady)	return;

	if(Device.Paused())
	{
#ifndef MASTER_GOLD
		if (Level().CurrentViewEntity()) {
			if (!(!g_actor || (g_actor->ID() != Level().CurrentViewEntity()->ID())))
			{
				CCameraBase* C = nullptr;
				if (g_actor)
				{
					if(!Actor()->Holder())
						C = Actor()->cam_Active();
					else
						C = Actor()->Holder()->Camera();

					Actor()->Cameras().UpdateFromCamera		(C);
					Actor()->Cameras().ApplyDevice			(VIEWPORT_NEAR);
#ifdef DEBUG
					if(psActorFlags.test(AF_NO_CLIP))
					{
						Actor()->dbg_update_cl			= 0;
						Actor()->dbg_update_shedule		= 0;
						Device.dwTimeDelta				= 0;
						Device.fTimeDelta				= 0.01f;			
						Actor()->UpdateCL				();
						Actor()->shedule_Update			(0);
						Actor()->dbg_update_cl			= 0;
						Actor()->dbg_update_shedule		= 0;

						CSE_Abstract* e					= Level().Server->ID_to_entity(Actor()->ID());
						VERIFY							(e);
						CSE_ALifeCreatureActor*	s_actor = smart_cast<CSE_ALifeCreatureActor*>(e);
						VERIFY							(s_actor);
						xr_vector<u16>::iterator it = s_actor->children.begin();
						for(;it!=s_actor->children.end();it++)
						{
							CObject* obj = Level().Objects.net_Find(*it);
							if(obj && Engine.Sheduler.Registered(obj))
							{
								obj->dbg_update_shedule = 0;
								obj->dbg_update_cl = 0;
								obj->shedule_Update	(0);
								obj->UpdateCL();
								obj->dbg_update_shedule = 0;
								obj->dbg_update_cl = 0;
							}
						}
					}
#endif // DEBUG
				}
			}
		}
#else // MASTER_GOLD
		if (g_actor)
		{
			CCameraBase* C = nullptr;
			if(!Actor()->Holder())
				C = Actor()->cam_Active();
			else
				C = Actor()->Holder()->Camera();

			Actor()->Cameras().UpdateFromCamera			(C);
			Actor()->Cameras().ApplyDevice				(VIEWPORT_NEAR);

		}
#endif // MASTER_GOLD
	}
	MySuper::OnFrame			();

	if(!Device.Paused())
		Engine.Sheduler.Update		();

	// update weathers ambient
	if(!Device.Paused())
		WeathersUpdate				();

#ifdef DEBUG
	if ((m_last_stats_frame + 1) < m_frame_counter)
		profiler().clear		();
#endif
	UpdateDof();
}

#include "game_sv_single.h"
#include "xrServer.h"
#include "UIGameCustom.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UIPdaWnd.h"

void CGamePersistent::OnEvent(EVENT E, u64 P1, u64 P2)
{
	if(E==eQuickLoad)
	{
		if (Device.Paused())
			Device.Pause		(FALSE, TRUE, TRUE, "eQuickLoad");
		
		if(CurrentGameUI())
		{
			CurrentGameUI()->HideShownDialogs();
			CurrentGameUI()->UIMainIngameWnd->reset_ui();
			CurrentGameUI()->PdaMenu().Reset();
		}

		if(g_tutorial)
			g_tutorial->Stop();

		if(g_tutorial2)
			g_tutorial2->Stop();

		LPSTR		saved_name	= (LPSTR)(P1);

		Level().remove_objects	();
		game_sv_Single			*game = smart_cast<game_sv_Single*>(Level().Server->game);
		R_ASSERT				(game);
		game->restart_simulator	(saved_name);
		xr_free					(saved_name);
		return;
	}
}

void CGamePersistent::Statistics	(CGameFont* F)
{
#ifdef DEBUG
#	ifndef _EDITOR
		m_last_stats_frame		= m_frame_counter;
		profiler().show_stats	(F,!!psAI_Flags.test(aiStats));
#	endif
#endif
}

float CGamePersistent::MtlTransparent(u32 mtl_idx)
{
	return GMLib.GetMaterialByIdx((u16)mtl_idx)->fVisTransparencyFactor;
}
static BOOL bRestorePause	= FALSE;
static BOOL bEntryFlag		= TRUE;

void CGamePersistent::OnAppActivate		()
{
	Device.Pause(FALSE, !bRestorePause, TRUE, "CGP::OnAppActivate");

	bEntryFlag = TRUE;
}

void CGamePersistent::OnAppDeactivate	()
{
	if(!bEntryFlag) return;

	bRestorePause = FALSE;

	bRestorePause = Device.Paused();
	Device.Pause(TRUE, TRUE, TRUE, "CGP::OnAppDeactivate");
	bEntryFlag = FALSE;
}

extern void draw_wnds_rects();
void CGamePersistent::OnRenderPPUI_main()
{
	// always
	MainMenu()->OnRenderPPUI_main();
	draw_wnds_rects();
}

#include "string_table.h"
#include "../xrEngine/x_ray.h"

void CGamePersistent::SetLoadStageTitle(const char* ls_title)
{
	string256	buff;
	if (ls_title)
	{
		xr_sprintf(buff, "%s%s", CStringTable().translate(ls_title).c_str(), "...");
		pApp->SetLoadStageTitle(buff);
	}
	else
		pApp->SetLoadStageTitle("");
}

void CGamePersistent::LoadTitle(bool change_tip, shared_str map_name)
{
	pApp->LoadStage();
	if(change_tip)
	{
		string512				buff;
		u8						tip_num;
		luabind::functor<u8>	m_functor;

		R_ASSERT( ai().script_engine().functor( "loadscreen.get_tip_number", m_functor ) );
		tip_num				= m_functor(map_name.c_str());

		xr_sprintf				(buff, "%s%d:", CStringTable().translate("ls_tip_number").c_str(), tip_num);
		shared_str				tmp = buff;
		
		xr_sprintf			(buff, "ls_tip_%d", tip_num);
		pApp->LoadTitleInt		(CStringTable().translate("ls_header").c_str(), tmp.c_str(), CStringTable().translate(buff).c_str());
	}
}

void CGamePersistent::SetPickableEffectorDOF(bool bSet)
{
	m_bPickableDOF = bSet;
	if(!bSet)
		RestoreEffectorDOF();
}

void CGamePersistent::GetCurrentDof(Fvector3& dof)
{
	dof = m_dof[1];
}

void CGamePersistent::SetBaseDof(const Fvector3& dof)
{
	m_dof[0]=m_dof[1]=m_dof[2]=m_dof[3]	= dof;
}

void CGamePersistent::SetEffectorDOF(const Fvector& needed_dof)
{
	if(m_bPickableDOF)	return;
	m_dof[0]	= needed_dof;
	m_dof[2]	= m_dof[1]; //current
}

void CGamePersistent::RestoreEffectorDOF()
{
	SetEffectorDOF			(m_dof[3]);
}
#include "hudmanager.h"

//	m_dof		[4];	// 0-dest 1-current 2-from 3-original
void CGamePersistent::UpdateDof()
{
	static float diff_far	= pSettings->r_float("zone_pick_dof","far");//70.0f;
	static float diff_near	= pSettings->r_float("zone_pick_dof","near");//-70.0f;

	if(m_bPickableDOF)
	{
		Fvector pick_dof;
		pick_dof.y	= HUD().GetCurrentRayQuery().range;
		pick_dof.x	= pick_dof.y+diff_near;
		pick_dof.z	= pick_dof.y+diff_far;
		m_dof[0]	= pick_dof;
		m_dof[2]	= m_dof[1]; //current
	}
	if(m_dof[1].similar(m_dof[0]))
						return;

	float td			= Device.fTimeDelta;
	Fvector				diff;
	diff.sub			(m_dof[0], m_dof[2]);
	diff.mul			(td/0.2f); //0.2 sec
	m_dof[1].add		(diff);
	(m_dof[0].x<m_dof[2].x)?clamp(m_dof[1].x,m_dof[0].x,m_dof[2].x):clamp(m_dof[1].x,m_dof[2].x,m_dof[0].x);
	(m_dof[0].y<m_dof[2].y)?clamp(m_dof[1].y,m_dof[0].y,m_dof[2].y):clamp(m_dof[1].y,m_dof[2].y,m_dof[0].y);
	(m_dof[0].z<m_dof[2].z)?clamp(m_dof[1].z,m_dof[0].z,m_dof[2].z):clamp(m_dof[1].z,m_dof[2].z,m_dof[0].z);
}

#include "ui\uimainingamewnd.h"
void CGamePersistent::OnSectorChanged(int sector)
{
	if(CurrentGameUI())
		CurrentGameUI()->UIMainIngameWnd->OnSectorChanged(sector);
}

void CGamePersistent::OnAssetsChanged()
{
	MySuper::OnAssetsChanged	();
	CStringTable().rescan				();
}

shared_str CGamePersistent::GetServerOption() const
{
    return m_ServerOptions;
}

shared_str CGamePersistent::GetClientOption() const
{
    return m_ClientOptions;
}

void CGamePersistent::SetServerOption(const char* str)
{
    VERIFY(str);
    Msg("New server option: %s", str);
    m_ServerOptions = str;
}

void CGamePersistent::SetClientOption(const char* str)
{
    VERIFY(str);
    Msg("New client option: %s", str);
    m_ClientOptions = str;
}
