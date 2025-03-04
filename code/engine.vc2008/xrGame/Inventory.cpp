#include "stdafx.h"
#include "inventory.h"
#include "actor.h"
#include "CustomOutfit.h"
#include "trade.h"
#include "weaponmagazined.h"

#include "ui/UIInventoryUtilities.h"
#include "ui/UIActorMenu.h"

#include "eatable_item.h"
#include "script_engine.h"
#include "xrmessages.h"
#include "xr_level_controller.h"
#include "level.h"
#include "ai_space.h"
#include "entitycondition.h"
#include "game_base_space.h"
#include "uigamecustom.h"
#include "clsid_game.h"
#include "../FrayBuildConfig.hpp"

#ifdef DEAD_BODY_WEAPON
#	include "ai/stalker/ai_stalker.h"
#endif

#include "player_hud.h"

using namespace InventoryUtilities;

// what to block
u16	INV_STATE_BLOCK_ALL = 0xffff;
u16	INV_STATE_LADDER	= INV_STATE_BLOCK_ALL;
u16	INV_STATE_CAR		= INV_STATE_LADDER;
u16	INV_STATE_INV_WND	= INV_STATE_BLOCK_ALL;

CInventorySlot::CInventorySlot() 
{
	m_pIItem				= NULL;
	m_bAct					= true;
	m_bPersistent			= false;
}

CInventorySlot::~CInventorySlot() 
{
}

bool CInventorySlot::CanBeActivated() const 
{
	return (m_bAct);
};

CInventory::CInventory() 
{
	m_fMaxWeight								= pSettings->r_float	("inventory","max_weight");
	
	u32 sz										= pSettings->r_s32		("inventory","slots_count");
	m_slots.resize								(sz+1); //first is [1]
	
	m_iActiveSlot								= NO_ACTIVE_SLOT;
	m_iNextActiveSlot							= NO_ACTIVE_SLOT;
	m_iPrevActiveSlot							= NO_ACTIVE_SLOT;

	string256 temp;
	for(u16 i=FirstSlot(); i<=LastSlot(); ++i ) 
	{
		xr_sprintf(temp, "slot_persistent_%d", i);
		m_slots[i].m_bPersistent = !!pSettings->r_bool("inventory",temp);

		xr_sprintf			(temp, "slot_active_%d", i);
		m_slots[i].m_bAct	= !!pSettings->r_bool("inventory",temp);
	};

	m_bSlotsUseful								= true;
	m_bBeltUseful								= false;

	m_fTotalWeight								= -1.f;
	m_dwModifyFrame								= 0;
	m_drop_last_frame							= false;
	
	InitPriorityGroupsForQSwitch				();
	m_next_item_iteration_time					= 0;

	for (u16 i = 0; i < LAST_SLOT+1; ++i)
	{
		m_blocked_slots[i] = 0;
	}
}


CInventory::~CInventory() 
{
}

void CInventory::Clear()
{
	m_all.clear							();
	m_ruck.clear						();
	m_belt.clear						();
	
	for(u16 i=FirstSlot(); i<=LastSlot(); i++)
		m_slots[i].m_pIItem				= NULL;

	m_pOwner							= NULL;

	CalcTotalWeight						();
	InvalidateState						();
}

void CInventory::Take(CGameObject *pObj, bool bNotActivate, bool strict_placement)
{
	CInventoryItem *pIItem				= smart_cast<CInventoryItem*>(pObj);
	VERIFY								(pIItem);
	VERIFY								(pIItem->m_pInventory==NULL);
	VERIFY								(CanTakeItem(pIItem));
	
	pIItem->m_pInventory				= this;
	pIItem->SetDropManual				(FALSE);
	pIItem->AllowTrade					();
	//if net_Import for pObj arrived then the pObj will pushed to CrPr list (correction prediction)
	//usually net_Import arrived for objects that not has a parent object..
	//for unknown reason net_Import arrived for object that has a parent, so correction prediction schema will crash
	Level().RemoveObject_From_4CrPr		(pObj);
#ifdef DEAD_BODY_WEAPON
	u16 actor_id = Level().CurrentEntity()->ID();

	if (GetOwner()->object_id()==actor_id && this->m_pOwner->object_id()==actor_id)		//actors inventory
	{
		CWeaponMagazined*	pWeapon = smart_cast<CWeaponMagazined*>(pIItem);
		if (pWeapon && pWeapon->strapped_mode())
		{
			pWeapon->strapped_mode(false);
			Ruck(pWeapon);
		}
			
	}
#endif
	m_all.push_back						(pIItem);

	if(!strict_placement)
		pIItem->m_ItemCurrPlace.type	= eItemPlaceUndefined;

	bool result							= false;
	switch(pIItem->m_ItemCurrPlace.type)
	{
	case eItemPlaceBelt:
		result							= Belt(pIItem, strict_placement); 
		if(!result)
			pIItem->m_ItemCurrPlace.type	= eItemPlaceUndefined;
#ifdef DEBUG
		if(!result) 
			Msg("cant put in belt item %s", *pIItem->object().cName());
#endif

		break;
	case eItemPlaceRuck:
		result							 = Ruck(pIItem, strict_placement);
		if(!result)
			pIItem->m_ItemCurrPlace.type = eItemPlaceUndefined;
#ifdef DEBUG
		if(!result) 
			Msg("cant put in ruck item %s", *pIItem->object().cName());
#endif

		break;
	case eItemPlaceSlot:
		result							= Slot(pIItem->m_ItemCurrPlace.slot_id, pIItem, bNotActivate, strict_placement); 
		if(!result)
			pIItem->m_ItemCurrPlace.type = eItemPlaceUndefined;
#ifdef DEBUG
		if(!result) 
			Msg("cant slot in slot item %s", *pIItem->object().cName());
#endif
		break;
	}

	if(pIItem->CurrPlace()==eItemPlaceUndefined)
	{
		if( !pIItem->RuckDefault() )
		{
			if( CanPutInSlot(pIItem, pIItem->BaseSlot()) )
			{
				result						= Slot(pIItem->BaseSlot(), pIItem, bNotActivate,strict_placement); VERIFY(result);
			}else
				if (CanPutInBelt(pIItem))
				{
					result					= Belt(pIItem,strict_placement); VERIFY(result);
				}else
				{
					result					= Ruck(pIItem,strict_placement); VERIFY(result);
				}
		}else
		{
			result						= Ruck(pIItem,strict_placement); VERIFY(result);
		}
	}
	
	m_pOwner->OnItemTake				(pIItem);

	CalcTotalWeight						();
	InvalidateState						();

	pIItem->object().processing_deactivate();
	VERIFY								(pIItem->CurrPlace() != eItemPlaceUndefined);


	if( CurrentGameUI() )
	{
		CObject* pActor_owner = smart_cast<CObject*>(m_pOwner);

		if (Level().CurrentViewEntity() == pActor_owner)
		{
			CurrentGameUI()->OnInventoryAction(pIItem, GE_OWNERSHIP_TAKE);
		}
		else if(CurrentGameUI()->ActorMenu().GetMenuMode()==mmDeadBodySearch)
		{
			if(m_pOwner==CurrentGameUI()->ActorMenu().GetPartner())
				CurrentGameUI()->OnInventoryAction(pIItem, GE_OWNERSHIP_TAKE);
		}
	};
}

bool CInventory::DropItem(CGameObject *pObj, bool just_before_destroy, bool dont_create_shell) 
{
	CInventoryItem *pIItem				= smart_cast<CInventoryItem*>(pObj);
	VERIFY								(pIItem);
	VERIFY								(pIItem->m_pInventory);
	VERIFY								(pIItem->m_pInventory==this);
	VERIFY								(pIItem->m_ItemCurrPlace.type!=eItemPlaceUndefined);
	
	pIItem->object().processing_activate(); 
	
	switch(pIItem->CurrPlace())
	{
	case eItemPlaceBelt:{
			VERIFY(InBelt(pIItem));
			TIItemContainer::iterator temp_iter = std::find(m_belt.begin(), m_belt.end(), pIItem);
			if (temp_iter != m_belt.end())
			{
				m_belt.erase(temp_iter);
			} else
			{
				Msg("! ERROR: CInventory::Drop item not found in belt...");
			}
			pIItem->object().processing_deactivate();
		}break;
	case eItemPlaceRuck:{
			VERIFY(InRuck(pIItem));
			TIItemContainer::iterator temp_iter = std::find(m_ruck.begin(), m_ruck.end(), pIItem);
			if (temp_iter != m_ruck.end())
			{
				m_ruck.erase(temp_iter);
			} else
			{
				Msg("! ERROR: CInventory::Drop item not found in ruck...");
			}
		}break;
	case eItemPlaceSlot:{
			VERIFY			(InSlot(pIItem));
			if(m_iActiveSlot == pIItem->CurrSlot())
			{
				CActor* pActor	= smart_cast<CActor*>(m_pOwner);
				if (!pActor || pActor->g_Alive())
				{
					if (just_before_destroy)
					{
#ifdef DEBUG
						Msg("---DropItem activating slot [-1], forced, Frame[%d]", Device.dwFrame);
#endif // #ifdef DEBUG
						Activate		(NO_ACTIVE_SLOT, true);
					} else 
					{
#ifdef DEBUG
						Msg("---DropItem activating slot [-1], Frame[%d]", Device.dwFrame);
#endif // #ifdef DEBUG
						Activate		(NO_ACTIVE_SLOT);
					}
				}
			}
			m_slots[pIItem->CurrSlot()].m_pIItem = NULL;							
			pIItem->object().processing_deactivate();
		}break;
	default:
		NODEFAULT;
	};
	TIItemContainer::iterator it = std::find(m_all.begin(), m_all.end(), pIItem);
	if(it!=m_all.end())
		m_all.erase(std::find(m_all.begin(), m_all.end(), pIItem));
	else
		Msg("! CInventory::Drop item not found in inventory!!!");

	pIItem->m_pInventory = NULL;


	m_pOwner->OnItemDrop	(smart_cast<CInventoryItem*>(pObj), just_before_destroy);

	CalcTotalWeight					();
	InvalidateState					();
	m_drop_last_frame				= true;

	if( CurrentGameUI() )
	{
		CObject* pActor_owner = smart_cast<CObject*>(m_pOwner);

		if (Level().CurrentViewEntity() == pActor_owner)
			CurrentGameUI()->OnInventoryAction(pIItem, GE_OWNERSHIP_REJECT);
	};
	pObj->H_SetParent(0, dont_create_shell);
	return							true;
}

//�������� ���� � ����
bool CInventory::Slot(u16 slot_id, PIItem pIItem, bool bNotActivate, bool strict_placement) 
{
	VERIFY(pIItem);
	
	if(ItemFromSlot(slot_id) == pIItem)
		return false;

	if(!strict_placement && !CanPutInSlot(pIItem,slot_id)) 
	{
#ifdef _DEBUG
		Msg("there is item %s[%d,%x] in slot %d[%d,%x]", 
				ItemFromSlot(pIItem->GetSlot())->object().cName().c_str(), 
				ItemFromSlot(pIItem->GetSlot())->object().ID(), 
				ItemFromSlot(pIItem->GetSlot()), 
				pIItem->GetSlot(), 
				pIItem->object().ID(),
				pIItem);
#endif
		return false;
	}

	m_slots[slot_id].m_pIItem = pIItem;
	
	//������� �� ������� ��� �����
	TIItemContainer::iterator it_ruck = std::find(m_ruck.begin(), m_ruck.end(), pIItem);
	TIItemContainer::iterator it_belt = std::find(m_belt.begin(), m_belt.end(), pIItem);
	
	{
		if (it_ruck != m_ruck.end())
			m_ruck.erase(it_ruck);
		if (it_belt != m_belt.end())
			m_belt.erase(it_belt);
	}

	bool in_slot = InSlot(pIItem);
	if(in_slot && (pIItem->CurrSlot()!=slot_id)) 
	{
		if(GetActiveSlot() == pIItem->CurrSlot()) 
			Activate(NO_ACTIVE_SLOT);

		m_slots[pIItem->CurrSlot()].m_pIItem = NULL;
	}

	if (((m_iActiveSlot==slot_id) ||(m_iActiveSlot==NO_ACTIVE_SLOT) && m_iNextActiveSlot==NO_ACTIVE_SLOT) && (!bNotActivate))
		Activate				(slot_id);
	
	SInvItemPlace p					= pIItem->m_ItemCurrPlace;
	pIItem->m_ItemCurrPlace.type	= eItemPlaceSlot;
	pIItem->m_ItemCurrPlace.slot_id = slot_id;
	m_pOwner->OnItemSlot			(pIItem, p); //Abramcumner fixed
	pIItem->OnMoveToSlot			(p);
	
	pIItem->object().processing_activate();

	return						true;
}

bool CInventory::Belt(PIItem pIItem, bool strict_placement) 
{
	if(!strict_placement && !CanPutInBelt(pIItem))	return false;
	
	//���� ���� � �����
	bool in_slot = InSlot(pIItem);
	if(in_slot) 
	{
		if(GetActiveSlot() == pIItem->CurrSlot()) 
			Activate(NO_ACTIVE_SLOT);

		m_slots[pIItem->CurrSlot()].m_pIItem = NULL;
	}
	
	m_belt.insert(m_belt.end(), pIItem); 

	if(!in_slot)
	{
		TIItemContainer::iterator it = std::find(m_ruck.begin(), m_ruck.end(), pIItem); 
		if(m_ruck.end() != it) 
			m_ruck.erase(it);
	}

	CalcTotalWeight					();
	InvalidateState					();

	SInvItemPlace p					= pIItem->m_ItemCurrPlace;
	pIItem->m_ItemCurrPlace.type	= eItemPlaceBelt;
	m_pOwner->OnItemBelt			(pIItem, p);
	pIItem->OnMoveToBelt			(p);

	if(in_slot)
		pIItem->object().processing_deactivate();

	pIItem->object().processing_activate();

	return true;
}

bool CInventory::Ruck(PIItem pIItem, bool strict_placement) 
{
	if(!strict_placement && !CanPutInRuck(pIItem)) return false;

	bool in_slot = InSlot(pIItem);
	//���� ���� � �����
	if(in_slot) 
	{
		if(GetActiveSlot() == pIItem->CurrSlot()) 
			Activate(NO_ACTIVE_SLOT);

		m_slots[pIItem->CurrSlot()].m_pIItem = NULL;
	}
	else
	{
		//���� ���� �� ����� ��� ������ ������ ������� � �����
		TIItemContainer::iterator it = std::find(m_belt.begin(), m_belt.end(), pIItem); 
		if(m_belt.end() != it) 
			m_belt.erase(it);
	}
	
	m_ruck.insert									(m_ruck.end(), pIItem); 
	
	CalcTotalWeight									();
	InvalidateState									();

	m_pOwner->OnItemRuck							(pIItem, pIItem->m_ItemCurrPlace);
	SInvItemPlace prev_place						= pIItem->m_ItemCurrPlace;
	pIItem->m_ItemCurrPlace.type					= eItemPlaceRuck;
	pIItem->OnMoveToRuck							(prev_place);

	if(in_slot)
		pIItem->object().processing_deactivate();

	return true;
}

void CInventory::Activate(u16 slot, bool bForce) 
{
	PIItem tmp_item = nullptr;
	if (slot != NO_ACTIVE_SLOT)
		tmp_item = ItemFromSlot(slot);

	if (tmp_item && IsSlotBlocked(tmp_item) && (!bForce))
	{
		//to restore after unblocking ...
		SetPrevActiveSlot(slot);
		return;
	}

	if (GetActiveSlot()==slot || (GetNextActiveSlot()==slot && !bForce))
	{
		m_iNextActiveSlot=slot;
		return;
	}

	R_ASSERT2(slot<=LastSlot(), "wrong slot number");

	if (slot != NO_ACTIVE_SLOT && !m_slots[slot].CanBeActivated()) 
		return;

	//�������� ���� �� ������
	if (GetActiveSlot() == NO_ACTIVE_SLOT)
	{
		if (tmp_item)
			m_iNextActiveSlot		= slot;
		else 
		{
			if(slot==GRENADE_SLOT)//fake for grenade
			{
				PIItem gr = SameSlot(GRENADE_SLOT, NULL, true);
				if(gr)
					Slot(GRENADE_SLOT,gr);
			}
		}
	}
	//�������� ���� ������������
	else if (slot==NO_ACTIVE_SLOT || tmp_item)
	{
		PIItem active_item = ActiveItem();
		if(active_item && !bForce)
		{
			CHudItem* tempItem = active_item->cast_hud_item();
			R_ASSERT2(tempItem, active_item->object().cNameSect().c_str());
			
			tempItem->SendDeactivateItem();
#ifdef DEBUG
//			Msg("--- Inventory owner [%s]: send deactivate item [%s]", m_pOwner->Name(), active_item->NameItem());
#endif // #ifdef DEBUG
		} else //in case where weapon is going to destroy
		{
			if (tmp_item)
				tmp_item->ActivateItem();

			m_iActiveSlot		= slot;
		}
		m_iNextActiveSlot		= slot;
	}
}


PIItem CInventory::ItemFromSlot(u16 slot) const
{
	VERIFY(NO_ACTIVE_SLOT != slot);
	return m_slots[slot].m_pIItem;
}

void CInventory::SendActionEvent(u16 cmd, u32 flags) 
{
	CActor *pActor = smart_cast<CActor*>(m_pOwner);
	if (!pActor) return;

	NET_Packet		P;
	pActor->u_EventGen		(P,GE_INV_ACTION, pActor->ID());
	P.w_u16					(cmd);
	P.w_u32					(flags);
	P.w_s32					(pActor->GetZoomRndSeed());
	P.w_s32					(pActor->GetShotRndSeed());
	pActor->u_EventSend		(P, net_flags(TRUE, TRUE, FALSE, TRUE));
};

bool CInventory::Action(u16 cmd, u32 flags) 
{
	CActor *pActor = smart_cast<CActor*>(m_pOwner);
	
	if (pActor)
	{
		switch(cmd)
		{
			case kWPN_FIRE:
			{
				pActor->SetShotRndSeed();
			}break;
			case kWPN_ZOOM : 
			{
				pActor->SetZoomRndSeed();
			}break;
		};
	};

	if (ActiveItem() &&  ActiveItem()->Action(cmd, flags)) 
		return true;

	bool b_send_event = false;
	switch(cmd) 
	{
	case kWPN_1:
	case kWPN_2:
	case kWPN_3:
	case kWPN_4:
	case kWPN_5:
	case kWPN_6:
		{
			b_send_event = true;
			
			u16 slot = u16(cmd - kWPN_1 + 1);
			if ( flags & CMD_START )
			{
				ActiveWeapon( slot );
			}
		}break;
	case kARTEFACT:
		{
		    b_send_event = true;
			if(flags&CMD_START)
			{
                if(GetActiveSlot() == ARTEFACT_SLOT && ActiveItem())
				{
					Activate(NO_ACTIVE_SLOT);
				}else {
					Activate(ARTEFACT_SLOT);
				}
			}
		}break;
	}

	return false;
}

void CInventory::ActiveWeapon( u16 slot )
{
	// weapon is in active slot
	if (GetActiveSlot() == slot && ActiveItem())
	{
		Activate(NO_ACTIVE_SLOT);
		return;
	}
	Activate(slot);
}

void CInventory::Update()
{
	if (m_iActiveSlot != m_iNextActiveSlot)
	{
		CObject* pActor_owner = smart_cast<CObject*>(m_pOwner);
		if (Level().CurrentViewEntity() == pActor_owner)
		{
			if ((m_iNextActiveSlot != NO_ACTIVE_SLOT) &&
				ItemFromSlot(m_iNextActiveSlot) &&
				!g_player_hud->allow_activation(ItemFromSlot(m_iNextActiveSlot)->cast_hud_item())
				)
				return;
		}
		if (ActiveItem())
		{
			CHudItem* hi = ActiveItem()->cast_hud_item();

			if (!hi->IsHidden())
			{
				if (hi->GetState() == CHUDState::eIdle && hi->GetNextState() == CHUDState::eIdle)
					hi->SendDeactivateItem();

				UpdateDropTasks();
				return;
			}
		}

		if (GetNextActiveSlot() != NO_ACTIVE_SLOT)
		{
			PIItem tmp_next_active = ItemFromSlot(GetNextActiveSlot());
			if (tmp_next_active)
			{
				if (IsSlotBlocked(tmp_next_active))
				{
					Activate(m_iActiveSlot);
					return;
				}
				else
				{
					tmp_next_active->ActivateItem();
				}
			}
		}

		m_iActiveSlot = GetNextActiveSlot();
	}
	if ((GetNextActiveSlot() != NO_ACTIVE_SLOT) && ActiveItem() && ActiveItem()->cast_hud_item()->IsHidden())
		ActiveItem()->ActivateItem();
	UpdateDropTasks();
}

void CInventory::UpdateDropTasks()
{
	//��������� �����
	for(u16 i=FirstSlot(); i<=LastSlot(); ++i)	
	{
		PIItem itm = ItemFromSlot(i);
		if(itm)
			UpdateDropItem		(itm);
	}

	for(int i = 0; i < 2; ++i)	
	{
		TIItemContainer &list			= i?m_ruck:m_belt;
		TIItemContainer::iterator it	= list.begin();
		TIItemContainer::iterator it_e	= list.end();
	
		for( ;it!=it_e; ++it)
		{
			UpdateDropItem		(*it);
		}
	}

	if (m_drop_last_frame)
	{
		m_drop_last_frame			= false;
		m_pOwner->OnItemDropUpdate	();
	}
}

void CInventory::UpdateDropItem(PIItem pIItem)
{
	if (pIItem->GetDropManual())
	{
		pIItem->SetDropManual(FALSE);
		pIItem->DenyTrade();

		NET_Packet					P;
		pIItem->object().u_EventGen(P, GE_OWNERSHIP_REJECT, pIItem->object().H_Parent()->ID());
		P.w_u16(u16(pIItem->object().ID()));
		pIItem->object().u_EventSend(P);
	}// dropManual
}

//���� �� ����� ������� ������ ����
PIItem CInventory::Same(const PIItem pIItem, bool bSearchRuck) const
{
	const TIItemContainer &list = bSearchRuck ? m_ruck : m_belt;

	for(TIItemContainer::const_iterator it = list.begin(); list.end() != it; ++it) 
	{
		const PIItem l_pIItem = *it;
		
		if((l_pIItem != pIItem) && 
				!xr_strcmp(l_pIItem->object().cNameSect(), 
				pIItem->object().cNameSect())) 
			return l_pIItem;
	}
	return NULL;
}

//���� �� ����� ���� ��� ����� 

PIItem CInventory::SameSlot(const u16 slot, PIItem pIItem, bool bSearchRuck) const
{
	if(slot == NO_ACTIVE_SLOT) 	return NULL;

	const TIItemContainer &list = bSearchRuck ? m_ruck : m_belt;
	
	for(TIItemContainer::const_iterator it = list.begin(); list.end() != it; ++it) 
	{
		PIItem _pIItem = *it;
		if(_pIItem != pIItem && _pIItem->BaseSlot() == slot) return _pIItem;
	}

	return NULL;
}

//����� � ��������� ���� � ��������� ������
PIItem CInventory::Get(LPCSTR name, bool bSearchRuck) const
{
	const TIItemContainer &list = bSearchRuck ? m_ruck : m_belt;
	
	for(TIItemContainer::const_iterator it = list.begin(); list.end() != it; ++it) 
	{
		PIItem pIItem = *it;
		if(!xr_strcmp(pIItem->object().cNameSect(), name) && 
								pIItem->Useful()) 
				return pIItem;
	}
	return NULL;
}

PIItem CInventory::Get(CLASS_ID cls_id, bool bSearchRuck) const
{
	const TIItemContainer &list = bSearchRuck ? m_ruck : m_belt;
	
	for(TIItemContainer::const_iterator it = list.begin(); list.end() != it; ++it) 
	{
		PIItem pIItem = *it;
		if(pIItem->object().CLS_ID == cls_id && 
								pIItem->Useful()) 
				return pIItem;
	}
	return NULL;
}

PIItem CInventory::Get(const u16 id, bool bSearchRuck) const
{
	const TIItemContainer &list = bSearchRuck ? m_ruck : m_belt;

	for(TIItemContainer::const_iterator it = list.begin(); list.end() != it; ++it) 
	{
		PIItem pIItem = *it;
		if(pIItem->object().ID() == id) 
			return pIItem;
	}
	return NULL;
}

//search both (ruck and belt)
PIItem CInventory::GetAny(LPCSTR name) const
{
	PIItem itm = Get(name, false);
	if(!itm)
		itm = Get(name, true);
	return itm;
}

PIItem CInventory::item(CLASS_ID cls_id) const
{
	const TIItemContainer &list = m_all;

	for(TIItemContainer::const_iterator it = list.begin(); list.end() != it; ++it) 
	{
		PIItem pIItem = *it;
		if(pIItem->object().CLS_ID == cls_id && 
			pIItem->Useful()) 
			return pIItem;
	}
	return NULL;
}

float CInventory::TotalWeight() const
{
	VERIFY(m_fTotalWeight>=0.f);
	return m_fTotalWeight;
}


float CInventory::CalcTotalWeight()
{
	float weight = 0;
	for(TIItemContainer::const_iterator it = m_all.begin(); m_all.end() != it; ++it) 
		weight += (*it)->Weight();

	m_fTotalWeight = weight;
	return m_fTotalWeight;
}


u32 CInventory::dwfGetSameItemCount(LPCSTR caSection, bool SearchAll)
{
	u32			l_dwCount = 0;
	TIItemContainer	&l_list = SearchAll ? m_all : m_ruck;
	for(TIItemContainer::iterator l_it = l_list.begin(); l_list.end() != l_it; ++l_it) 
	{
		PIItem	l_pIItem = *l_it;
		if (!xr_strcmp(l_pIItem->object().cNameSect(), caSection))
            ++l_dwCount;
	}
	
	return		(l_dwCount);
}
u32		CInventory::dwfGetGrenadeCount(LPCSTR caSection, bool SearchAll)
{
	u32			l_dwCount = 0;
	TIItemContainer	&l_list = SearchAll ? m_all : m_ruck;
	for(TIItemContainer::iterator l_it = l_list.begin(); l_list.end() != l_it; ++l_it) 
	{
		PIItem	l_pIItem = *l_it;
		if (l_pIItem->object().CLS_ID == CLSID_GRENADE_F1 || l_pIItem->object().CLS_ID == CLSID_GRENADE_RGD5)
			++l_dwCount;
	}

	return		(l_dwCount);
}

bool CInventory::bfCheckForObject(ALife::_OBJECT_ID tObjectID)
{
	TIItemContainer	&l_list = m_all;
	for(TIItemContainer::iterator l_it = l_list.begin(); l_list.end() != l_it; ++l_it) 
	{
		PIItem	l_pIItem = *l_it;
		if (l_pIItem->object().ID() == tObjectID)
			return(true);
	}
	return		(false);
}

CInventoryItem *CInventory::get_object_by_id(ALife::_OBJECT_ID tObjectID)
{
	TIItemContainer	&l_list = m_all;
	for(TIItemContainer::iterator l_it = l_list.begin(); l_list.end() != l_it; ++l_it) 
	{
		PIItem	l_pIItem = *l_it;
		if (l_pIItem->object().ID() == tObjectID)
			return	(l_pIItem);
	}
	return		(0);
}

//������� ������� 
#include "game_object_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
bool CInventory::Eat(PIItem pIItem)
{
	//����������� �������� �� ����
	CEatableItem* pItemToEat = smart_cast<CEatableItem*>(pIItem);
	if ( !pItemToEat )			return false;

	CEntityAlive *entity_alive = smart_cast<CEntityAlive*>(m_pOwner);
	if ( !entity_alive )		return false;

	CInventoryOwner* IO	= smart_cast<CInventoryOwner*>(entity_alive);
	if ( !IO )					return false;
	
	CInventory* pInventory = pItemToEat->m_pInventory;
	if ( !pInventory || pInventory != this )	return false;
	if ( pInventory != IO->m_inventory )		return false;
	if ( pItemToEat->object().H_Parent()->ID() != entity_alive->ID() )		return false;
	
	if (!pItemToEat->UseBy(entity_alive))
		return false;

	Actor()->callback(GameObject::eUseObject)((smart_cast<CGameObject*>(pIItem))->lua_game_object());

	if(pItemToEat->Empty())
	{
		pIItem->SetDropManual(TRUE);
		return		false;
	}
	return			true;
}

bool CInventory::ClientEat(PIItem pIItem)
{
	CEatableItem* pItemToEat = smart_cast<CEatableItem*>(pIItem);
	if ( !pItemToEat )			return false;

	CEntityAlive *entity_alive = smart_cast<CEntityAlive*>(m_pOwner);
	if ( !entity_alive )		return false;

	CInventoryOwner* IO	= smart_cast<CInventoryOwner*>(entity_alive);
	if ( !IO )					return false;
	
	CInventory* pInventory = pItemToEat->m_pInventory;
	if ( !pInventory || pInventory != this )	return false;
	if ( pInventory != IO->m_inventory )		return false;
	if ( pItemToEat->object().H_Parent()->ID() != entity_alive->ID() )		return false;
	
	NET_Packet						P;
	CGameObject::u_EventGen			(P, GEG_PLAYER_ITEM_EAT, pIItem->parent_id());
	P.w_u16							(pIItem->object().ID());
	CGameObject::u_EventSend		(P);
	return true;
}

bool CInventory::InSlot(const CInventoryItem* pIItem) const
{
	if(pIItem->CurrPlace() != eItemPlaceSlot)	return false;

	VERIFY(m_slots[pIItem->CurrSlot()].m_pIItem == pIItem);

	return true;
}

bool CInventory::InBelt(const CInventoryItem* pIItem) const
{
	if(Get(pIItem->object().ID(), false)) return true;
	return false;
}

bool CInventory::InRuck(const CInventoryItem* pIItem) const
{
	if(Get(pIItem->object().ID(), true)) return true;
	return false;
}


bool CInventory::CanPutInSlot(PIItem pIItem, u16 slot_id) const
{
	if(!m_bSlotsUseful) return false;

	if( !GetOwner()->CanPutInSlot(pIItem, slot_id ) ) return false;


	if(slot_id==HELMET_SLOT)
	{
		CCustomOutfit* pOutfit = m_pOwner->GetOutfit();
		if(pOutfit && !pOutfit->bIsHelmetAvaliable)
			return false;
	}

	if(slot_id!=NO_ACTIVE_SLOT && 
		NULL==ItemFromSlot(slot_id) )
		return true;
	
	return false;
}
//��������� ����� �� ��������� ���� �� ����,
//��� ���� ������� ������ �� ��������
bool CInventory::CanPutInBelt(PIItem pIItem)
{
	if(InBelt(pIItem))					return false;
	if(!m_bBeltUseful)					return false;
	if(!pIItem || !pIItem->Belt())		return false;
	if(m_belt.size() >= BeltWidth())	return false;

	return FreeRoom_inBelt(m_belt, pIItem, BeltWidth(), 1);
}
//��������� ����� �� ��������� ���� � ������,
//��� ���� ������� ������ �� ��������
bool CInventory::CanPutInRuck(PIItem pIItem) const
{
	if(InRuck(pIItem)) return false;
	return true;
}

u32	CInventory::dwfGetObjectCount()
{
	return		u32(m_all.size());
}

CInventoryItem	*CInventory::tpfGetObjectByIndex(int iIndex)
{
	if ((iIndex >= 0) && (iIndex < (int)m_all.size())) {
		TIItemContainer	&l_list = m_all;
		int			i = 0;
		for(TIItemContainer::iterator l_it = l_list.begin(); l_list.end() != l_it; ++l_it, ++i) 
			if (i == iIndex)
                return	(*l_it);
	}
	else {
		ai().script_engine().script_log	(ScriptStorage::eLuaMessageTypeError,"invalid inventory index!");
		return	(0);
	}
	R_ASSERT	(false);
	return		(0);
}

CInventoryItem	*CInventory::GetItemFromInventory(LPCSTR caItemName)
{
	TIItemContainer	&l_list = m_all;

	u32 crc = crc32(caItemName, xr_strlen(caItemName));

	for(TIItemContainer::iterator l_it = l_list.begin(); l_list.end() != l_it; ++l_it)
		if ((*l_it)->object().cNameSect()._get()->dwCRC == crc){
			VERIFY(	0 == xr_strcmp( (*l_it)->object().cNameSect().c_str(), caItemName)  );
			return	(*l_it);
		}
	return	(0);
}


bool CInventory::CanTakeItem(CInventoryItem *inventory_item) const
{
	VERIFY(inventory_item);
	VERIFY(m_pOwner);

	if (inventory_item->object().getDestroy()) 
		return false;

	if (!inventory_item->CanTake()) 
		return false;

    TIItemContainer::const_iterator it = m_all.begin();
	for(; it != m_all.end(); ++it)
		if ((*it)->object().ID() == inventory_item->object().ID()) 
			break;

	VERIFY3(it == m_all.end(), "item already exists in inventory", *inventory_item->object().cName());

	CActor* pActor = smart_cast<CActor*>(m_pOwner);
	//����� ������ ����� ����� ����
	if(!pActor && (TotalWeight() + inventory_item->Weight() > m_pOwner->MaxCarryWeight()))
		return false;

	return	true;
}


u32  CInventory::BeltWidth() const
{
	CActor* pActor = smart_cast<CActor*>( m_pOwner );
	if ( pActor )
	{
		CCustomOutfit* outfit = pActor->GetOutfit();
		if ( outfit )
		{
			return outfit->get_artefact_count();
		}
	}
	return 0; //m_iMaxBelt;
}

void  CInventory::AddAvailableItems(TIItemContainer& items_container, bool for_trade) const
{
	for(TIItemContainer::const_iterator it = m_ruck.begin(); m_ruck.end() != it; ++it) 
	{
		PIItem pIItem = *it;
		if(!for_trade || pIItem->CanTrade())
			items_container.push_back(pIItem);
	}

	if(m_bBeltUseful)
	{
		for(TIItemContainer::const_iterator it = m_belt.begin(); m_belt.end() != it; ++it) 
		{
			PIItem pIItem = *it;
			if(!for_trade || pIItem->CanTrade())
				items_container.push_back(pIItem);
		}
	}
#ifdef DEAD_BODY_WEAPON
	CAI_Stalker* pOwner = smart_cast<CAI_Stalker*>(m_pOwner);
	if (pOwner && !pOwner->g_Alive())
	{
		u16 I = FirstSlot();
		u16 E = LastSlot();
		for(;I<=E;++I)
		{
			PIItem item = ItemFromSlot(I);
			if(item && (item->BaseSlot() == INV_SLOT_3 || item->BaseSlot() == GRENADE_SLOT))
			{
				items_container.push_back(item);
			}
		}
	}
	else 
#endif
	if (m_bSlotsUseful)
	{
		u16 I = FirstSlot();
		u16 E = LastSlot();
		for(;I<=E;++I)
		{
			PIItem item = ItemFromSlot(I);
			if(item && (!for_trade || item->CanTrade()))
			{
				if(!SlotIsPersistent(I) || item->BaseSlot() == GRENADE_SLOT )
					items_container.push_back(item);
			}
		}
	}
}

bool CInventory::isBeautifulForActiveSlot	(CInventoryItem *pIItem)
{
	u16 I = FirstSlot();
	u16 E = LastSlot();
	for(;I<=E;++I)
	{
		PIItem item = ItemFromSlot(I);
		if (item && item->IsNecessaryItem(pIItem))
			return		(true);
	}
	return				(false);
}

//.#include "WeaponHUD.h"
void CInventory::Items_SetCurrentEntityHud(bool current_entity)
{
	TIItemContainer::iterator it;
	for(it = m_all.begin(); m_all.end() != it; ++it) 
	{
		PIItem pIItem = *it;
		CWeapon* pWeapon = smart_cast<CWeapon*>(pIItem);
		if (pWeapon)
		{
			pWeapon->InitAddons();
			pWeapon->UpdateAddonsVisibility();
		}
	}
};

//call this only via Actor()->SetWeaponHideState()
void CInventory::SetSlotsBlocked(u16 mask, bool bBlock)
{
	for(u16 i = FirstSlot(), ie = LastSlot(); i <= ie; ++i)
	{
		if(mask & (1<<i))
		{
			if (bBlock)
				BlockSlot(i);
			else
				UnblockSlot(i);
		}
	}
	
	if (bBlock)
	{
		TryDeactivateActiveSlot();	
	} else
	{
		TryActivatePrevSlot();
	}
}

void CInventory::TryActivatePrevSlot()
{
	u16 ActiveSlot		= GetActiveSlot();
	u16 PrevActiveSlot	= GetPrevActiveSlot();
	u16 NextActiveSlot	= GetNextActiveSlot();
	if ((
			(ActiveSlot == NO_ACTIVE_SLOT) ||
			(NextActiveSlot == NO_ACTIVE_SLOT)
		) &&
		(PrevActiveSlot != NO_ACTIVE_SLOT))
	{
		PIItem prev_active_item = ItemFromSlot(PrevActiveSlot);
		if (prev_active_item &&
			!IsSlotBlocked(prev_active_item) &&
			m_slots[PrevActiveSlot].CanBeActivated())
		{
#ifndef MASTER_GOLD
			Msg("Set slots blocked: activating prev slot [%d], Frame[%d]", PrevActiveSlot, Device.dwFrame);
#endif // #ifndef MASTER_GOLD
			Activate(PrevActiveSlot);
			SetPrevActiveSlot(NO_ACTIVE_SLOT);
		}
	}
}

void CInventory::TryDeactivateActiveSlot	()
{
	u16 ActiveSlot		= GetActiveSlot();
	u16 NextActiveSlot	= GetNextActiveSlot();

	if ((ActiveSlot == NO_ACTIVE_SLOT) && (NextActiveSlot == NO_ACTIVE_SLOT))
		return;
	
	PIItem		active_item = (ActiveSlot != NO_ACTIVE_SLOT) ? 
		ItemFromSlot(ActiveSlot) : NULL;
	PIItem		next_active_item = (NextActiveSlot != NO_ACTIVE_SLOT) ?
		ItemFromSlot(NextActiveSlot) : NULL;

	if (active_item &&
		(IsSlotBlocked(active_item) || !m_slots[ActiveSlot].CanBeActivated())
		)
	{
#ifndef MASTER_GOLD
		Msg("Set slots blocked: activating slot [-1], Frame[%d]", Device.dwFrame);
#endif // #ifndef MASTER_GOLD
		ItemFromSlot(ActiveSlot)->DiscardState();
		Activate			(NO_ACTIVE_SLOT);
		SetPrevActiveSlot	(ActiveSlot);
	} else if (next_active_item &&
		(IsSlotBlocked(next_active_item) || !m_slots[NextActiveSlot].CanBeActivated())
		)
	{
		Activate			(NO_ACTIVE_SLOT);
		SetPrevActiveSlot	(NextActiveSlot);
	}
}

void CInventory::BlockSlot(u16 slot_id)
{
	VERIFY(slot_id <= LAST_SLOT);
	
	++m_blocked_slots[slot_id];
	
	VERIFY2(m_blocked_slots[slot_id] < 5,
		make_string("blocked slot [%d] overflow").c_str());	
}

void CInventory::UnblockSlot(u16 slot_id)
{
	VERIFY(slot_id <= LAST_SLOT);
	VERIFY2(m_blocked_slots[slot_id] > 0,
		make_string("blocked slot [%d] underflow").c_str());	
	
	--m_blocked_slots[slot_id];	
}

bool CInventory::IsSlotBlocked(u16 slot_id) const
{
	VERIFY(slot_id <= LAST_SLOT);
	return m_blocked_slots[slot_id] > 0;
}

bool CInventory::IsSlotBlocked(PIItem const iitem) const
{
	VERIFY(iitem);
	return IsSlotBlocked(iitem->BaseSlot());
}
