#pragma once
#include "UIProgressBar.h"
#include "UIProgressShape.h"

class CUIMotionIcon: public CUIWindow
{
	using inherited = CUIWindow;

private:

	CUIProgressBar		m_luminosity_bar;
	CUIProgressBar		m_noise_bar;
	CUIProgressShape 	m_luminosity_shape;
	CUIProgressShape 	m_noise_shape;

	struct _npc_visibility
	{
		u16 id;
		float value;
		bool operator == (const u16& _id)
		{
			return id == _id;
		}
		bool operator < (const _npc_visibility& m) const
		{
			return (value < m.value);
		}
	};
	xr_vector<_npc_visibility>	m_npc_visibility;
	bool						m_bchanged;
	float						m_luminosity;
	float						cur_pos;

public:
	virtual					~CUIMotionIcon		();
							CUIMotionIcon		();
	virtual	void			Update				();
	virtual void			Draw				();
			void			Init				(Frect const& rect);
			void			SetNoise			(float Pos);
			void			SetLuminosity		(float Pos);
			void			SetActorVisibility	(u16 who_id, float value);
			void			ResetVisibility		();
};
