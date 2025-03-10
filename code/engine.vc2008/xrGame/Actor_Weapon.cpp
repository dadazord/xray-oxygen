// Actor_Weapon.cpp:	 äëÿ ðàáîòû ñ îðóæèåì
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "actor.h"
#include "actoreffector.h"
#include "Missile.h"
#include "inventory.h"
#include "weapon.h"
#include "map_manager.h"
#include "level.h"
#include "CharacterPhysicsSupport.h"
#include "EffectorShot.h"
#include "WeaponMagazined.h"
#include "Grenade.h"
#include "game_base_space.h"
#include "Artefact.h"

static const float VEL_MAX		= 10.f;
static const float VEL_A_MAX	= 10.f;

#define GetWeaponParam(pWeapon, func_name, def_value)	((pWeapon) ? (pWeapon->func_name) : def_value)

//âîçâðàùàåò òåêóøèé ðàçáðîñ ñòðåëüáû (â ðàäèàíàõ)ñ ó÷åòîì äâèæåíèÿ
float CActor::GetWeaponAccuracy() const
{
	CWeapon* W	= smart_cast<CWeapon*>(inventory().ActiveItem());
	
	if ( IsZoomAimingMode() && W && !GetWeaponParam(W, IsRotatingToZoom(), false) )
	{
		return m_fDispAim;
	}
	float dispersion = m_fDispBase*GetWeaponParam(W, Get_PDM_Base(), 1.0f);

	CEntity::SEntityState state;
	if ( g_State(state) )
	{
		//fAVelocity = angle velocity
		dispersion *= ( 1.0f + (state.fAVelocity/VEL_A_MAX) * m_fDispVelFactor * GetWeaponParam(W, Get_PDM_Vel_F(), 1.0f) );
		//fVelocity = linear velocity
		dispersion *= ( 1.0f + (state.fVelocity/VEL_MAX) * m_fDispVelFactor * GetWeaponParam(W, Get_PDM_Vel_F(), 1.0f) );

		bool bAccelerated = isActorAccelerated( mstate_real, IsZoomAimingMode() );
		if ( bAccelerated || !state.bCrouch )
		{
			dispersion *= ( 1.0f + m_fDispAccelFactor * GetWeaponParam(W, Get_PDM_Accel_F(), 1.0f) );
		}

		if ( state.bCrouch )
		{	
			dispersion *= ( 1.0f + m_fDispCrouchFactor * GetWeaponParam(W, Get_PDM_Crouch(), 1.0f) );
			if ( !bAccelerated )
			{
				dispersion *= ( 1.0f + m_fDispCrouchNoAccelFactor * GetWeaponParam(W, Get_PDM_Crouch_NA(), 1.0f) );
			}
		}
	}
	return dispersion;
}


void CActor::g_fireParams	(const CHudItem* pHudItem, Fvector &fire_pos, Fvector &fire_dir)
{
	CWeapon				*weapon = smart_cast<CWeapon*>(inventory().ActiveItem());
	if(weapon)
	{
	if(eacFirstEye == cam_active)
	fire_pos = Cameras().Position();
	else
	fire_pos		= weapon->get_LastFP();
	}
	else{
const CMissile	*pMissile = smart_cast <const CMissile*> (pHudItem);
	if (pMissile)
	{
	Fvector act_and_cam_pos = Level().CurrentControlEntity()->Position();
	act_and_cam_pos.y    += CameraHeight();
	fire_pos = act_and_cam_pos;
	fire_pos.y += 0.14f;
	}
	}
	if (psActorFlags.test(AF_ZOOM_NEW_FD))
	{
		fire_dir = weapon->get_LastFD();
		fire_pos = weapon->get_LastFP();
	}
	else
	fire_dir		= Cameras().Direction();
}

void CActor::g_WeaponBones	(int &L, int &R1, int &R2)
{
	R1				= m_r_hand;
	R2				= m_r_finger2;
	L				= m_l_finger1;
}

BOOL CActor::g_State (SEntityState& state) const
{
	state.bJump			= !!(mstate_real&mcJump);
	state.bCrouch		= !!(mstate_real&mcCrouch);
	state.bFall			= !!(mstate_real&mcFall);
	state.bSprint		= !!(mstate_real&mcSprint);
	state.fVelocity		= character_physics_support()->movement()->GetVelocityActual();
	state.fAVelocity	= fCurAVelocity;
	return TRUE;
}

void CActor::SetCantRunState(bool bDisable)
{
	if (g_Alive() && this == Level().CurrentControlEntity())
	{
		NET_Packet	P;
		u_EventGen	(P, GEG_PLAYER_DISABLE_SPRINT, ID());
		P.w_s8		(bDisable?1:-1);
		u_EventSend	(P);
	};
}
void CActor::SetWeaponHideState (u16 State, bool bSet)
{
	if (g_Alive() && this == Level().CurrentControlEntity())
	{
		NET_Packet	P;
		u_EventGen	(P, GEG_PLAYER_WEAPON_HIDE_STATE, ID());
		P.w_u16		(State);
		P.w_u8		(u8(bSet));
		u_EventSend	(P);
	};
}
static	u16 BestWeaponSlots [] = {
	INV_SLOT_3		,		// 2
	INV_SLOT_2		,		// 1
	GRENADE_SLOT	,		// 3
	KNIFE_SLOT		,		// 0
};

BOOL	g_bShowHitSectors	= TRUE;

void	CActor::HitSector(CObject* who, CObject* weapon)
{
	if (!g_bShowHitSectors) return;
	if (!g_Alive()) return;

	bool bShowHitSector = true;
	
	CEntityAlive* pEntityAlive = smart_cast<CEntityAlive*>(who);

	if (!pEntityAlive || this == who) bShowHitSector = false;

	if (weapon)
	{
		CWeapon* pWeapon = smart_cast<CWeapon*> (weapon);
		if (pWeapon)
		{
			if (pWeapon->IsSilencerAttached())
			{
				bShowHitSector = false;

			}
		}
	}
// #TODO: Remove it!
}

void CActor::on_weapon_shot_start		(CWeapon *weapon)
{	
	//CWeaponMagazined* pWM = smart_cast<CWeaponMagazined*> (weapon);
	CameraRecoil const& camera_recoil = ( IsZoomAimingMode() )? weapon->zoom_cam_recoil : weapon->cam_recoil;
		
	CCameraShotEffector* effector = smart_cast<CCameraShotEffector*>( Cameras().GetCamEffector(eCEShot) );
	if ( !effector )
	{
		effector = (CCameraShotEffector*)Cameras().AddCamEffector( xr_new<CCameraShotEffector>( camera_recoil ) );
	}
	else
	{
		if( effector->m_WeaponID != weapon->ID() )
		{
			effector->Initialize( camera_recoil );
		}
	}
	
	effector->m_WeaponID = weapon->ID();
	R_ASSERT(effector);

	effector->SetRndSeed	(GetShotRndSeed());
	effector->SetActor		(this);
	effector->Shot			(weapon);
}

void CActor::on_weapon_shot_update		()
{
	CCameraShotEffector* effector = smart_cast<CCameraShotEffector*>( Cameras().GetCamEffector(eCEShot) );
	if ( effector )
	{
		update_camera( effector );
	}
}

void CActor::on_weapon_shot_remove		(CWeapon *weapon)
{
	Cameras().RemoveCamEffector(eCEShot);
}

void CActor::on_weapon_shot_stop		()
{
	CCameraShotEffector				*effector = smart_cast<CCameraShotEffector*>(Cameras().GetCamEffector(eCEShot)); 
	if (effector && effector->IsActive())
	{
		effector->StopShoting();
	}
}

void CActor::on_weapon_hide				(CWeapon *weapon)
{
	CCameraShotEffector				*effector = smart_cast<CCameraShotEffector*>(Cameras().GetCamEffector(eCEShot)); 
	if (effector && effector->IsActive())
		effector->Reset				();
}

Fvector CActor::weapon_recoil_delta_angle	()
{
	CCameraShotEffector				*effector = smart_cast<CCameraShotEffector*>(Cameras().GetCamEffector(eCEShot));
	Fvector							result = {0.f,0.f,0.f};

	if (effector)
		effector->GetDeltaAngle		(result);

	return							(result);
}

Fvector CActor::weapon_recoil_last_delta()
{
	CCameraShotEffector				*effector = smart_cast<CCameraShotEffector*>(Cameras().GetCamEffector(eCEShot));
	Fvector							result = {0.f,0.f,0.f};

	if (effector)
		effector->GetLastDelta		(result);

	return							(result);
}
//////////////////////////////////////////////////////////////////////////

void	CActor::SpawnAmmoForWeapon	(CInventoryItem *pIItem)
{
	if (!pIItem) return;

	CWeaponMagazined* pWM = smart_cast<CWeaponMagazined*> (pIItem);
	if (!pWM || !pWM->AutoSpawnAmmo()) return;

	pWM->SpawnAmmo(0xffffffff, NULL, ID());
};

void	CActor::RemoveAmmoForWeapon	(CInventoryItem *pIItem)
{
	if (!pIItem) return;

	CWeaponMagazined* pWM = smart_cast<CWeaponMagazined*> (pIItem);
	if (!pWM || !pWM->AutoSpawnAmmo()) return;

	CWeaponAmmo* pAmmo = smart_cast<CWeaponAmmo*>(inventory().GetAny( pWM->m_ammoTypes[0].c_str() ));
	if (!pAmmo) return;
	//--- ìû íàøëè ïàòðîíû ê òåêóùåìó îðóæèþ	
	/*
	//--- ïðîâåðÿåì íå ïîäõîäÿò ëè îíè ê ÷åìó-òî åùå
	bool CanRemove = true;
	TIItemContainer::const_iterator I = inventory().m_all.begin();//, B = I;
	TIItemContainer::const_iterator E = inventory().m_all.end();
	for ( ; I != E; ++I)
	{
	CInventoryItem* pItem = (*I);//->m_pIItem;
	CWeaponMagazined* pWM = smart_cast<CWeaponMagazined*> (pItem);
	if (!pWM || !pWM->AutoSpawnAmmo()) continue;
	if (pWM == pIItem) continue;
	if (pWM->m_ammoTypes[0] != pAmmo->CInventoryItem::object().cNameSect()) continue;
	CanRemove = false;
	break;
	};

	if (!CanRemove) return;
	*/
	pAmmo->DestroyObject();
	//	NET_Packet			P;
	//	u_EventGen			(P,GE_DESTROY,pAmmo->ID());
	//	u_EventSend			(P);
};
