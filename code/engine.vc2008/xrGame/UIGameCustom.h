#pragma once

#include "../xrScripts/export/script_export_space.h"
#include "object_interfaces.h"
#include "inventory_space.h"
#include "gametype_chooser.h"
#include "UIDialogHolder.h"
#include "../xrEngine/CustomHUD.h"

// refs
class CUI;
class game_cl_GameState;
class CUIDialogWnd;
class CUICaption;
class CUIStatic;
class CUIWindow;
class CUIXml;
class CUIActorMenu;
class CUIPdaWnd;
struct KillMessageStruct;
class CUIMainIngameWnd;
class CUIMessagesWindow;

struct SDrawStaticStruct :public IPureDestroyableObject
{
	float m_endTime;
	shared_str m_name;
	CUIStatic *m_static;

	SDrawStaticStruct();
	virtual	void destroy();

	void Draw();
	void Update();
	inline CUIStatic *wnd() { return m_static; }
	bool IsActual()	const;
	void SetText(LPCSTR);
};

struct SGameTypeMaps
{
	shared_str m_game_type_name;
	EGameIDs m_game_type_id;

	struct SMapItm
	{
		shared_str	map_name;
		shared_str	map_ver;
		bool operator ==(const SMapItm& other) { return map_name == other.map_name && map_ver == other.map_ver; }
	};

	xr_vector<SMapItm> m_map_names;
};

struct SGameWeathers
{
	shared_str m_weather_name;
	shared_str m_start_time;
};

typedef xr_vector<SGameWeathers> GAME_WEATHERS;
typedef xr_vector<SGameWeathers>::iterator GAME_WEATHERS_IT;
typedef xr_vector<SGameWeathers>::const_iterator GAME_WEATHERS_CIT;

class CUIGameCustom :public DLL_Pure, public CDialogHolder
{
protected:
	CUIWindow * m_window;
	CUIXml *m_msgs_xml;
	typedef xr_vector<SDrawStaticStruct*> st_vec;
	typedef st_vec::iterator st_vec_it;
	st_vec m_custom_statics;

	CUIActorMenu *m_ActorMenu;
	CUIPdaWnd *m_PdaMenu;

	bool m_bShowGameIndicators;

public:
	CUIMainIngameWnd * UIMainIngameWnd;
	CUIMessagesWindow *m_pMessagesWnd;

	virtual void SetClGame(game_cl_GameState* g);
	virtual void OnInventoryAction(PIItem item, u16 action_type);

	CUIGameCustom();
	virtual ~CUIGameCustom();

	virtual	void Init(int stage) {};

	virtual void Render();
	virtual void __stdcall OnFrame();

	inline CUIActorMenu& ActorMenu() const { return *m_ActorMenu; }
	inline CUIPdaWnd& PdaMenu() const { return *m_PdaMenu; }

	bool ShowActorMenu();
	void HideActorMenu();
	bool ShowPdaMenu();
	void HidePdaMenu();
	void ShowMessagesWindow();
	void HideMessagesWindow();

	void ShowGameIndicators(bool b) { m_bShowGameIndicators = b; };
	bool GameIndicatorsShown() { return m_bShowGameIndicators; };
	void ShowCrosshair(bool b) { psHUD_Flags.set(HUD_CROSSHAIR_RT, b); }
	bool CrosshairShown() { return !!psHUD_Flags.test(HUD_CROSSHAIR_RT); }

	virtual void HideShownDialogs() {};

	SDrawStaticStruct*	AddCustomStatic(LPCSTR id, bool bSingleInstance);
	SDrawStaticStruct*	GetCustomStatic(LPCSTR id);
	void RemoveCustomStatic(LPCSTR id);

	void CommonMessageOut(LPCSTR text);

	virtual void ChangeTotalMoneyIndicator(LPCSTR newMoneyString) {};
	virtual void DisplayMoneyChange(LPCSTR deltaMoney) {};
	virtual void DisplayMoneyBonus(KillMessageStruct* bonus) {};

	virtual void UnLoad();
	void Load();

	void OnConnected();

	void UpdatePda();
	void update_fake_indicators(u8 type, float power);
	void enable_fake_indicators(bool enable);

	DECLARE_SCRIPT_REGISTER_FUNCTION
}; // class CUIGameCustom

extern CUIGameCustom* CurrentGameUI();