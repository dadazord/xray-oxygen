////////////////////////////////////////////////////////////////////////////
//	Module 		: object_factory_register.cpp
//	Created 	: 27.05.2004
//  Modified 	: 27.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Object factory
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "object_factory_impl.h"

// server entities includes
#include "xrServer_Objects_ALife_All.h"
#include "xrServer_Objects_ALife_Smartcovers.h"
#include "clsid_game.h"

// client entities includes
#ifndef NO_XR_GAME
#	include "../xrEngine/std_classes.h"
#	include "level.h"
#	include "gamepersistent.h"
#	include "hudmanager.h"
#	include "actor.h"

#	include "ai/monsters/flesh/flesh.h"
#	include "ai/monsters/chimera/chimera.h"
#	include "ai/monsters/dog/dog.h"
#	include "ai/stalker/ai_stalker.h"
#	include "ai/monsters/bloodsucker/bloodsucker.h"
#	include "ai/monsters/boar/boar.h"
#	include "ai/monsters/pseudodog/pseudodog.h"
#	include "ai/monsters/pseudodog/psy_dog.h"
#	include "ai/monsters/Burer/burer.h"
#	include "ai/monsters/PseudoGigant/pseudo_gigant.h"
#	include "ai/monsters/controller/controller.h"
#	include "ai/monsters/poltergeist/poltergeist.h"
#	include "ai/monsters/zombie/zombie.h"
#	include "ai/monsters/fracture/fracture.h"
#	include "ai/monsters/snork/snork.h"
#	include "ai/monsters/cat/cat.h"
#	include "ai/monsters/tushkano/tushkano.h"
#	include "ai/monsters/rats/ai_rat.h"

#	include "ai/phantom/phantom.h"

#	include "ai/trader/ai_trader.h"

#	include "ai/crow/ai_crow.h"

#	ifdef DEBUG
#		include "../xrEngine/StatGraph.h"
#		include "PHDebug.h"
#	endif // DEBUG

#	include "hit.h"
#	include "PHDestroyable.h"
#	include "car.h"

#	include "helicopter.h"

#	include "MercuryBall.h"

#	include "weaponFN2000.h"
#	include "weaponAK74.h"
#	include "weaponLR300.h"
#	include "weaponHPSA.h"
#	include "weaponPM.h"
#	include "weaponAMMO.h"
#	include "weaponFORT.h"
#	include "weaponBINOCULARS.h"
#	include "weaponShotgun.h"
#	include "weaponsvd.h"
#	include "weaponsvu.h"
#	include "weaponrpg7.h"
#	include "weaponval.h"
#	include "weaponvintorez.h"
#	include "weaponwalther.h"
#	include "weaponusp45.h"
#	include "weapongroza.h"
#	include "weaponknife.h"
#	include "weaponBM16.h"
#	include "weaponRG6.h"
#	include "WeaponStatMgun.h"

#	include "scope.h"
#	include "silencer.h"
#	include "grenadelauncher.h"

#	include "bolt.h"
#	include "medkit.h"
#	include "antirad.h"
#	include "fooditem.h"
#	include "bottleitem.h"
#	include "explosiveitem.h"

#	include "infodocument.h"
#	include "attachable_item.h"

#	include "CustomOutfit.h"
#	include "ActorHelmet.h"

#	include "f1.h"
#	include "rgd5.h"

#	include "explosiverocket.h"

#	include "customzone.h"
#	include "mosquitobald.h"
#	include "mincer.h"
#	include "gravizone.h"
#	include "radioactivezone.h"
#	include "level_changer.h"
#	include "script_zone.h"
#	include "torridZone.h"
#	include "ZoneVisual.h"
#	include "hairszone.h"
#	include "nogravityzone.h"
#	include "simpledetector.h"
#	include "elitedetector.h"
#	include "advanceddetector.h"
#	include "Dosimeter.h"
#	include "zonecampfire.h"

#	include "torch.h"
#	include "pda.h"
#	include "flare.h"

#	include "searchlight.h"

#	include "HangingLamp.h"
#	include "physicobject.h"
#	include "script_object.h"
#	include "BreakableObject.h"
#	include "PhysicsSkeletonObject.h"
#	include "DestroyablePhysicsObject.h"

#	include "game_sv_single.h"
#	include "game_cl_single.h"

#	include "UIGameSP.h"
#	include	"climableobject.h"
#	include "space_restrictor.h"
#	include "smart_zone.h"
#	include "InventoryBox.h"

#	include "smart_cover_object.h"
#endif // NO_XR_GAME

#ifndef NO_XR_GAME
#	define ADD(a,b,c,d)			add<a,b>(c,d)
#else
#	define ADD(a,b,c,d)			add<b>(c,d)
#endif

void CObjectFactory::register_classes	()
{
#ifndef NO_XR_GAME
	// client entities
	add<CLevel>													(CLSID_GAME_LEVEL				,"level");
	add<CGamePersistent>										(CLSID_GAME_PERSISTANT			,"game");
	add<CHUDManager>											(CLSID_HUDMANAGER				,"hud_manager");
	//Server Game type
	
	add<game_sv_Single>											(CLSID_SV_GAME_SINGLE			,"game_sv_single");
	//Client Game type
	add<game_cl_Single>											(CLSID_CL_GAME_SINGLE			,"game_cl_single");
	add<CUIGameSP>												(CLSID_GAME_UI_SINGLE			,"game_ui_single");
#else // NO_XR_GAME
	ADD(CActor					,CSE_ALifeCreatureActor			,CLSID_OBJECT_ACTOR				,"actor");
#endif // NO_XR_GAME

	// server entities
	add<CSE_ALifeGroupTemplate<CSE_ALifeMonsterBase> >			(CLSID_AI_FLESH_GROUP			,"flesh_group");
	add<CSE_ALifeGraphPoint>									(CLSID_AI_GRAPH					,"graph_point");
	add<CSE_ALifeOnlineOfflineGroup>							(CLSID_ONLINE_OFFLINE_GROUP		,"online_offline_group");
	
	// client and server entities
	ADD(CAI_Flesh				,CSE_ALifeMonsterBase			,CLSID_AI_FLESH					,"flesh");
	ADD(CChimera				,CSE_ALifeMonsterBase			,CLSID_AI_CHIMERA				,"chimera");
	ADD(CAI_Dog					,CSE_ALifeMonsterBase			,CLSID_AI_DOG_RED				,"dog_red");
	ADD(CAI_Stalker				,CSE_ALifeHumanStalker			,CLSID_AI_STALKER				,"stalker");
	ADD(CAI_Bloodsucker			,CSE_ALifeMonsterBase			,CLSID_AI_BLOODSUCKER			,"bloodsucker");
	ADD(CAI_Boar				,CSE_ALifeMonsterBase			,CLSID_AI_BOAR					,"boar");
	ADD(CAI_PseudoDog			,CSE_ALifeMonsterBase			,CLSID_AI_DOG_BLACK				,"dog_black");
	ADD(CPsyDog					,CSE_ALifeMonsterBase			,CLSID_AI_DOG_PSY				,"psy_dog");
	ADD(CPsyDogPhantom			,CSE_ALifePsyDogPhantom			,CLSID_AI_DOG_PSY_PHANTOM		,"psy_dog_phantom");
	ADD(CBurer					,CSE_ALifeMonsterBase			,CLSID_AI_BURER					,"burer");
	ADD(CPseudoGigant			,CSE_ALifeMonsterBase			,CLSID_AI_GIANT					,"pseudo_gigant");
	ADD(CController				,CSE_ALifeMonsterBase			,CLSID_AI_CONTROLLER			,"controller");
	ADD(CPoltergeist			,CSE_ALifeMonsterBase			,CLSID_AI_POLTERGEIST			,"poltergeist");
	ADD(CZombie					,CSE_ALifeMonsterBase			,CLSID_AI_ZOMBIE				,"zombie");
	ADD(CFracture				,CSE_ALifeMonsterBase			,CLSID_AI_FRACTURE				,"fracture");
	ADD(CSnork					,CSE_ALifeMonsterBase			,CLSID_AI_SNORK					,"snork");
	ADD(CCat					,CSE_ALifeMonsterBase			,CLSID_AI_CAT					,"cat");
	ADD(CTushkano				,CSE_ALifeMonsterBase			,CLSID_AI_TUSHKANO				,"tushkano");
	
	ADD(CPhantom				,CSE_ALifeCreaturePhantom		,CLSID_AI_PHANTOM				,"phantom");

	// Trader
	ADD(CAI_Trader				,CSE_ALifeTrader				,CLSID_AI_TRADER				,"trader");

	ADD(CAI_Crow				,CSE_ALifeCreatureCrow			,CLSID_AI_CROW					,"crow");
	ADD(CAI_Rat					,CSE_ALifeMonsterRat			,CLSID_AI_RAT					,"rat");
	ADD(CCar					,CSE_ALifeCar					,CLSID_CAR						,"car");

	ADD(CHelicopter				,CSE_ALifeHelicopter			,CLSID_VEHICLE_HELICOPTER		,"helicopter");

	// Artefacts
	ADD(CArtefact				,CSE_ALifeItemArtefact			,CLSID_ARTEFACT					,"artefact");

	//  [8/15/2006]
	ADD(CWeaponMagazined		,CSE_ALifeItemWeaponMagazined	,CLSID_OBJECT_W_MAGAZINED		,"wpn_wmagaz");
	//  [8/15/2006]
	//  [8/17/2006]
	ADD(CWeaponMagazinedWGrenade,CSE_ALifeItemWeaponMagazinedWGL,CLSID_OBJECT_W_MAGAZWGL		,"wpn_wmaggl");
	//  [8/17/2006]
	ADD(CWeaponFN2000			,CSE_ALifeItemWeaponMagazinedWGL	,CLSID_OBJECT_W_FN2000			,"wpn_fn2000");
	ADD(CWeaponAK74				,CSE_ALifeItemWeaponMagazinedWGL	,CLSID_OBJECT_W_AK74			,"wpn_ak74");
	ADD(CWeaponLR300			,CSE_ALifeItemWeaponMagazined	,CLSID_OBJECT_W_LR300			,"wpn_lr300");
	ADD(CWeaponHPSA				,CSE_ALifeItemWeaponMagazined	,CLSID_OBJECT_W_HPSA			,"wpn_hpsa");
	ADD(CWeaponPM				,CSE_ALifeItemWeaponMagazined	,CLSID_OBJECT_W_PM				,"wpn_pm");
	ADD(CWeaponFORT				,CSE_ALifeItemWeaponMagazined	,CLSID_OBJECT_W_FORT			,"wpn_fort");
	ADD(CWeaponBinoculars		,CSE_ALifeItemWeaponMagazined	,CLSID_OBJECT_W_BINOCULAR		,"wpn_binocular");
	ADD(CWeaponShotgun			,CSE_ALifeItemWeaponShotGun		,CLSID_OBJECT_W_SHOTGUN			,"wpn_shotgun");
	ADD(CWeaponSVD				,CSE_ALifeItemWeaponMagazined	,CLSID_OBJECT_W_SVD				,"wpn_svd");
	ADD(CWeaponSVU				,CSE_ALifeItemWeaponMagazined	,CLSID_OBJECT_W_SVU				,"wpn_svu");
	ADD(CWeaponRPG7				,CSE_ALifeItemWeaponMagazined	,CLSID_OBJECT_W_RPG7			,"wpn_rpg7");
	ADD(CWeaponVal				,CSE_ALifeItemWeaponMagazined	,CLSID_OBJECT_W_VAL				,"wpn_val");
	ADD(CWeaponVintorez			,CSE_ALifeItemWeaponMagazined	,CLSID_OBJECT_W_VINTOREZ		,"wpn_vintorez");
	ADD(CWeaponWalther			,CSE_ALifeItemWeaponMagazined	,CLSID_OBJECT_W_WALTHER			,"wpn_walther");
	ADD(CWeaponUSP45			,CSE_ALifeItemWeaponMagazined	,CLSID_OBJECT_W_USP45			,"wpn_usp45");
	ADD(CWeaponGroza			,CSE_ALifeItemWeaponMagazinedWGL,CLSID_OBJECT_W_GROZA			,"wpn_groza");
	ADD(CWeaponKnife			,CSE_ALifeItemWeapon			,CLSID_OBJECT_W_KNIFE			,"wpn_knife");
	ADD(CWeaponBM16				,CSE_ALifeItemWeaponShotGun		,CLSID_OBJECT_W_BM16			,"wpn_bm16");
	ADD(CWeaponRG6				,CSE_ALifeItemWeaponShotGun		,CLSID_OBJECT_W_RG6				,"wpn_rg6");
	//-----------------------------------------------------------------------------------------------------
	ADD(CWeaponAmmo				,CSE_ALifeItemAmmo				,CLSID_OBJECT_AMMO				,"wpn_ammo");
	ADD(CWeaponAmmo				,CSE_ALifeItemAmmo				,CLSID_OBJECT_A_VOG25			,"wpn_ammo_vog25");
	ADD(CWeaponAmmo				,CSE_ALifeItemAmmo				,CLSID_OBJECT_A_OG7B			,"wpn_ammo_og7b");
	ADD(CWeaponAmmo				,CSE_ALifeItemAmmo				,CLSID_OBJECT_A_M209			,"wpn_ammo_m209");
	//-----------------------------------------------------------------------------------------------------

	//Weapons Add-on
	ADD(CScope					,CSE_ALifeItem					,CLSID_OBJECT_W_SCOPE			,"wpn_scope");
	ADD(CSilencer				,CSE_ALifeItem					,CLSID_OBJECT_W_SILENCER		,"wpn_silencer");
	ADD(CGrenadeLauncher		,CSE_ALifeItem					,CLSID_OBJECT_W_GLAUNCHER		,"wpn_grenade_launcher");

	// Inventory
	ADD(CBolt					,CSE_ALifeItemBolt				,CLSID_IITEM_BOLT				,"obj_bolt");
	ADD(CMedkit					,CSE_ALifeItem					,CLSID_IITEM_MEDKIT				,"obj_medkit");
	ADD(CMedkit					,CSE_ALifeItem					,CLSID_IITEM_BANDAGE			,"obj_bandage");
	ADD(CAntirad				,CSE_ALifeItem					,CLSID_IITEM_ANTIRAD			,"obj_antirad");
	ADD(CFoodItem				,CSE_ALifeItem					,CLSID_IITEM_FOOD				,"obj_food");
	ADD(CBottleItem				,CSE_ALifeItem					,CLSID_IITEM_BOTTLE				,"obj_bottle");
	ADD(CExplosiveItem			,CSE_ALifeItemExplosive			,CLSID_IITEM_EXPLOSIVE			,"obj_explosive");
	
	//Info Document
	ADD(CInfoDocument			,CSE_ALifeItemDocument			,CLSID_IITEM_DOCUMENT			,"obj_document");
	ADD(CInventoryItemObject	,CSE_ALifeItem					,CLSID_IITEM_ATTACH				,"obj_attachable");

	//Equipment outfit
	ADD(CCustomOutfit			,CSE_ALifeItemCustomOutfit		,CLSID_EQUIPMENT_STALKER		,"equ_stalker");
	ADD(CHelmet					,CSE_ALifeItem					,CLSID_EQUIPMENT_HELMET			,"helmet");

	// Grenades
	ADD(CF1						,CSE_ALifeItemGrenade			,CLSID_GRENADE_F1				,"wpn_grenade_f1");
	ADD(CRGD5					,CSE_ALifeItemGrenade			,CLSID_GRENADE_RGD5				,"wpn_grenade_rgd5");

	// Rockets
	ADD(CExplosiveRocket		,CSE_Temporary					,CLSID_OBJECT_G_RPG7			,"wpn_grenade_rpg7");
	ADD(CExplosiveRocket		,CSE_Temporary					,CLSID_OBJECT_G_FAKE			,"wpn_grenade_fake");

	// Zones
	ADD(CCustomZone				,CSE_ALifeCustomZone			,CLSID_ZONE						,"zone");
	ADD(CMosquitoBald			,CSE_ALifeAnomalousZone			,CLSID_Z_MBALD					,"zone_mosquito_bald");
	ADD(CMincer					,CSE_ALifeAnomalousZone			,CLSID_Z_MINCER					,"zone_mincer");
	ADD(CMosquitoBald			,CSE_ALifeAnomalousZone			,CLSID_Z_ACIDF					,"zone_acid_fog");
	ADD(CMincer					,CSE_ALifeAnomalousZone			,CLSID_Z_GALANT					,"zone_galantine");
	ADD(CRadioactiveZone		,CSE_ALifeAnomalousZone			,CLSID_Z_RADIO					,"zone_radioactive");
	ADD(CHairsZone				,CSE_ALifeZoneVisual			,CLSID_Z_BFUZZ					,"zone_bfuzz");
	ADD(CHairsZone				,CSE_ALifeZoneVisual			,CLSID_Z_RUSTYH					,"zone_rusty_hair");
	ADD(CMosquitoBald			,CSE_ALifeAnomalousZone			,CLSID_Z_DEAD					,"zone_dead");
#ifndef	BENCHMARK_BUILD
	ADD(CLevelChanger			,CSE_ALifeLevelChanger			,CLSID_LEVEL_CHANGER			,"level_changer");
#endif	//	BENCHMARK_BUILD
	ADD(CScriptZone				,CSE_ALifeSpaceRestrictor		,CLSID_SCRIPT_ZONE				,"script_zone");
	ADD(CSmartZone				,CSE_ALifeSmartZone				,CLSID_SMART_ZONE				,"smart_zone");
	ADD(CTorridZone				,CSE_ALifeTorridZone			,CLSID_Z_TORRID					,"torrid_zone");
	ADD(CSpaceRestrictor		,CSE_ALifeSpaceRestrictor		,CLSID_SPACE_RESTRICTOR			,"space_restrictor");
//.	ADD(CAmebaZone				,CSE_ALifeZoneVisual			,CLSID_Z_AMEBA					,"ameba_zone");
	ADD(CNoGravityZone			,CSE_ALifeAnomalousZone			,CLSID_Z_NOGRAVITY				,"nogravity_zone");
	ADD(CZoneCampfire			,CSE_ALifeAnomalousZone			,CLSID_Z_CAMPFIRE				,"zone_campfire");
	// Detectors
	ADD(CSimpleDetector			,CSE_ALifeItemDetector			,CLSID_DETECTOR_SIMPLE			,"device_detector_simple");
	ADD(CAdvancedDetector		,CSE_ALifeItemDetector			,CLSID_DETECTOR_ADVANCED		,"device_detector_advanced");
	ADD(CEliteDetector			,CSE_ALifeItemDetector			,CLSID_DETECTOR_ELITE			,"device_detector_elite");
	ADD(CScientificDetector		,CSE_ALifeItemDetector			,CLSID_DETECTOR_SCIENTIFIC		,"device_detector_scientific");
	ADD(CDosimeter              ,CSE_ALifeItem                  ,CLSID_DEVICE_DOSIMETER         ,"device_dosimeter");

	// Devices
	ADD(CTorch					,CSE_ALifeItemTorch				,CLSID_DEVICE_TORCH				,"device_torch");
	ADD(CPda					,CSE_ALifeItemPDA				,CLSID_DEVICE_PDA				,"device_pda");
	ADD(CFlare					,CSE_ALifeItem					,CLSID_DEVICE_FLARE				,"device_flare");

	// objects
	ADD(CProjector				,CSE_ALifeObjectProjector		,CLSID_OBJECT_PROJECTOR			,"projector");
	ADD(CWeaponStatMgun			,CSE_ALifeStationaryMgun		,CLSID_OBJECT_W_STATMGUN		,"wpn_stat_mgun");

	// entity
	ADD(CHangingLamp			,CSE_ALifeObjectHangingLamp		,CLSID_OBJECT_HLAMP				,"hanging_lamp");
	ADD(CPhysicObject			,CSE_ALifeObjectPhysic			,CLSID_OBJECT_PHYSIC			,"obj_physic");
	ADD(CScriptObject			,CSE_ALifeDynamicObjectVisual	,CLSID_SCRIPT_OBJECT			,"script_object");
	ADD(CBreakableObject		,CSE_ALifeObjectBreakable		,CLSID_OBJECT_BREAKABLE			,"obj_breakable");
	ADD(CClimableObject			,CSE_ALifeObjectClimable		,CLSID_OBJECT_CLIMABLE			,"obj_climable");
	ADD(CPhysicsSkeletonObject	,CSE_ALifePHSkeletonObject		,CLSID_PH_SKELETON_OBJECT		,"obj_phskeleton");
	ADD(CDestroyablePhysicsObject,CSE_ALifeObjectPhysic			,CLSID_PHYSICS_DESTROYABLE		,"obj_phys_destroyable");

	ADD(CInventoryBox			,CSE_ALifeInventoryBox			,CLSID_INVENTORY_BOX			,"inventory_box");
	ADD(smart_cover::object		,CSE_SmartCover					,TEXT2CLSID("SMRTCOVR")			,"smart_cover");
}
