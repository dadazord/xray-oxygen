#pragma once

#include "inventory_item_object.h"
#include "../xrScripts/export/script_export_space.h"

struct SBoneProtections;
class CBinocularsVision;

class CCustomOutfit: public CInventoryItemObject {
private:
    typedef	CInventoryItemObject inherited;
public:
							CCustomOutfit		();
	virtual					~CCustomOutfit		();

	virtual void			Load				(LPCSTR section);
	
	//����������� ������ ����, ��� ������, ����� ������ ����� �� ���������
	virtual void			Hit					(float P, ALife::EHitType hit_type);

	//������������ �� ������� ����������� ���
	//��� ��������������� ���� �����������
	//���� �� ��������� ����� ������
	float					GetHitTypeProtection		(ALife::EHitType hit_type, s16 element);
	float					GetDefHitTypeProtection		(ALife::EHitType hit_type);
	float					GetBoneArmor				(s16 element);

	float					HitThroughArmor		(float hit_power, s16 element, float ap, bool& add_wound, ALife::EHitType hit_type);

	virtual void			OnMoveToSlot		(const SInvItemPlace& prev);
	virtual void			OnMoveToRuck		(const SInvItemPlace& previous_place);
	virtual void			OnH_A_Chield		();

protected:
	HitImmunity::HitTypeSVec		m_HitTypeProtection;

	shared_str				m_ActorVisual;
	shared_str				m_PlayerHudSection;
	SBoneProtections*		m_boneProtection;	
	
protected:
	u32						m_ef_equipment_type;
	u32						m_artefact_count;

public:
	float					m_fPowerLoss;
	float					m_additional_weight;
	float					m_additional_weight2;

	float					m_fHealthRestoreSpeed;
	float 					m_fRadiationRestoreSpeed;
	float 					m_fSatietyRestoreSpeed;
	float					m_fPowerRestoreSpeed;
	float					m_fBleedingRestoreSpeed;

	shared_str				m_BonesProtectionSect;
	shared_str				m_NightVisionSect;
	float					m_fShowNearestEnemiesDistance;

	bool					bIsHelmetAvaliable;
	bool                    m_reload_on_sprint;

	virtual u32				ef_equipment_type		() const;
	virtual	BOOL			BonePassBullet			(int boneID);
	u32						get_artefact_count		() const	{ return m_artefact_count; }

	virtual BOOL			net_Spawn				(CSE_Abstract* DC);
	virtual void			net_Export				(NET_Packet& P);
	virtual void			net_Import				(NET_Packet& P);
			void			ApplySkinModel			(CActor* pActor, bool bDress, bool bHUDOnly);
			void			ReloadBonesProtection	();
			void			AddBonesProtection		(LPCSTR bones_section);

protected:
	virtual bool			install_upgrade_impl( LPCSTR section, bool test );
	
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CCustomOutfit)
#undef script_type_list
#define script_type_list save_type_list(CCustomOutfit)