#include "Client.h"

//[enc_string_enable /]
//[junk_enable /]

namespace Client
{
	//[swap_lines]
	int	iScreenWidth = 0;
	int	iScreenHeight = 0;

	string BaseDir = "";
	string LogFile = "";
	string GuiFile = "";
	string IniFile = "";

	vector<string> ConfigList;

	Vector2D	g_vCenterScreen = Vector2D(0.f, 0.f);

	CPlayers*	g_pPlayers = nullptr;
	CRender*	g_pRender = nullptr;
	CGui*		g_pGui = nullptr;

	CAimbot*	g_pAimbot = nullptr;
	CTriggerbot* g_pTriggerbot = nullptr;
	CEsp*		g_pEsp = nullptr;
	CRadar*		g_pRadar = nullptr;
	CKnifebot*	g_pKnifebot = nullptr;
	CSkin*		g_pSkin = nullptr;
	CMisc*		g_pMisc = nullptr;

	bool		bC4Timer = false;
	int			iC4Timer = 40;

	int			iWeaponID = 0;
	int			iWeaponSelectIndex = WEAPON_DEAGLE;
	int			iWeaponSelectSkinIndex = -1;
	//[/swap_lines]

	void ReadConfigs(LPCTSTR lpszFileName)
	{
		if (!strstr(lpszFileName, "gui.ini"))
		{
			ConfigList.push_back(lpszFileName);
		}
	}

	void RefreshConfigs()
	{
		ConfigList.clear();
		string ConfigDir = BaseDir + "\\*.ini";
		SearchFiles(ConfigDir.c_str(), ReadConfigs, FALSE);
	}

	bool Initialize(IDirect3DDevice9* pDevice)
	{
		g_pPlayers = new CPlayers();
		g_pRender = new CRender(pDevice);
		g_pGui = new CGui();

		g_pAimbot = new CAimbot();
		g_pTriggerbot = new CTriggerbot();
		g_pEsp = new CEsp();
		g_pRadar = new CRadar();
		g_pKnifebot = new CKnifebot();
		g_pSkin = new CSkin();
		g_pMisc = new CMisc();

		GuiFile = BaseDir + "\\" + "gui.ini";
		IniFile = BaseDir + "\\" + "settings.ini";

		g_pSkin->InitalizeSkins();

		Settings::LoadSettings(IniFile);

		iWeaponSelectSkinIndex = GetWeaponSkinIndexFromPaintKit(g_SkinChangerCfg[iWeaponSelectIndex].nFallbackPaintKit);

		g_pGui->GUI_Init(pDevice);

		RefreshConfigs();

		return true;
	}

	void Shutdown()
	{
		DELETE_MOD(g_pPlayers);
		DELETE_MOD(g_pRender);
		DELETE_MOD(g_pGui);

		DELETE_MOD(g_pAimbot);
		DELETE_MOD(g_pTriggerbot);
		DELETE_MOD(g_pEsp);
		DELETE_MOD(g_pRadar);
		DELETE_MOD(g_pKnifebot);
		DELETE_MOD(g_pSkin);
		DELETE_MOD(g_pMisc);
	}

	void OnRender()
	{
		if (g_pRender && !Interfaces::Engine()->IsTakingScreenshot() && Interfaces::Engine()->IsActiveApp())
		{
			g_pRender->BeginRender();

			if (g_pGui)
				g_pGui->GUI_Draw_Elements();

			Interfaces::Engine()->GetScreenSize(iScreenWidth, iScreenHeight);

			g_vCenterScreen.x = iScreenWidth / 2.f;
			g_vCenterScreen.y = iScreenHeight / 2.f;

			if (!Interfaces::Engine()->IsConnected())
				g_pRender->Text(3, 3, false, true, Color::Aqua(), HACK_NAME);

			if (Client::g_pPlayers && Client::g_pPlayers->GetLocal() && Interfaces::Engine()->IsInGame())
			{
				if (g_pEsp)
					g_pEsp->OnRender();

				if (g_pMisc)
				{
					g_pMisc->OnRender();
					g_pMisc->OnRenderSpectatorList();
				}
			}

			g_pRender->EndRender();
		}
	}

	void OnLostDevice()
	{
		if (g_pRender)
			g_pRender->OnLostDevice();

		if (g_pGui)
			ImGui_ImplDX9_InvalidateDeviceObjects();
	}

	void OnResetDevice()
	{
		if (g_pRender)
			g_pRender->OnResetDevice();

		if (g_pGui)
			ImGui_ImplDX9_CreateDeviceObjects();
	}

	void OnCreateMove(CUserCmd* pCmd)
	{
		if (g_pPlayers && Interfaces::Engine()->IsInGame())
		{
			g_pPlayers->Update();

			if (g_pEsp)
				g_pEsp->OnCreateMove(pCmd);

			if (IsLocalAlive())
			{
				if (!bIsGuiVisible)
				{
					int iWeaponSettingsSelectID = GetWeaponSettingsSelectID();

					if (iWeaponSettingsSelectID >= 0)
						iWeaponID = iWeaponSettingsSelectID;
				}

				if (g_pAimbot)
					g_pAimbot->OnCreateMove(pCmd, g_pPlayers->GetLocal());

				if (g_pTriggerbot)
					g_pTriggerbot->OnCreateMove(pCmd, g_pPlayers->GetLocal());

				if (g_pKnifebot)
					g_pKnifebot->OnCreateMove(pCmd);

				if (g_pMisc)
					g_pMisc->OnCreateMove(pCmd);
			}
		}
	}

	void OnFireEventClientSideThink(IGameEvent* pEvent)
	{
		if (!strcmp(pEvent->GetName(), "player_connect_full") ||
			!strcmp(pEvent->GetName(), "round_start") ||
			!strcmp(pEvent->GetName(), "cs_game_disconnected"))
		{
			if (g_pPlayers)
				g_pPlayers->Clear();

			if (g_pEsp)
				g_pEsp->OnReset();
		}

		if (Interfaces::Engine()->IsConnected())
		{
			if (g_pEsp)
				g_pEsp->OnEvents(pEvent);

			if (g_pSkin)
				g_pSkin->OnEvents(pEvent);
		}
	}

	void OnFrameStageNotify(ClientFrameStage_t Stage)
	{
		if (Interfaces::Engine()->IsInGame())
		{
			Skin_OnFrameStageNotify(Stage);
			Gloves_OnFrameStageNotify(Stage);
		}
	}

	void OnDrawModelExecute(IMatRenderContext* ctx, const DrawModelState_t &state,
		const ModelRenderInfo_t &pInfo, matrix3x4_t *pCustomBoneToWorld)
	{
		if (Interfaces::Engine()->IsInGame() && ctx && pCustomBoneToWorld)
		{
			if (g_pEsp)
				g_pEsp->OnDrawModelExecute(ctx, state, pInfo, pCustomBoneToWorld);

			if (g_pMisc)
				g_pMisc->OnDrawModelExecute();
		}
	}

	void OnPlaySound(const Vector* pOrigin, const char* pszSoundName)
	{
		if (!pszSoundName || !Interfaces::Engine()->IsInGame())
			return;

		if (!strstr(pszSoundName, "bulletLtoR") &&
			!strstr(pszSoundName, "rics/ric") &&
			!strstr(pszSoundName, "impact_bullet"))
		{
			if (g_pEsp && IsLocalAlive() && Settings::Esp::esp_Sound && pOrigin)
			{
				if (!GetVisibleOrigin(*pOrigin))
					g_pEsp->SoundEsp.AddSound(*pOrigin);
			}
		}
	}

	void OnPlaySound(const char* pszSoundName)
	{
		if (g_pMisc)
			g_pMisc->OnPlaySound(pszSoundName);
	}

	void OnOverrideView(CViewSetup* pSetup)
	{
		if (g_pMisc)
			g_pMisc->OnOverrideView(pSetup);
	}

	void OnGetViewModelFOV(float& fov)
	{
		if (g_pMisc)
			g_pMisc->OnGetViewModelFOV(fov);
	}

	void OnRenderGUI()
	{
		ImGui::SetNextWindowSize(ImVec2(560.f, 325.f));

		if (ImGui::Begin(HACK_NAME, &bIsGuiVisible, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
		{
			if (Settings::Aimbot::weapon_aim_settings[iWeaponID].aim_FovType > 1)
				Settings::Aimbot::weapon_aim_settings[iWeaponID].aim_FovType = 1;

			if (Settings::Aimbot::weapon_aim_settings[iWeaponID].aim_BestHit > 1)
				Settings::Aimbot::weapon_aim_settings[iWeaponID].aim_BestHit = 1;

			if (Settings::Aimbot::weapon_aim_settings[iWeaponID].aim_Spot > 5)
				Settings::Aimbot::weapon_aim_settings[iWeaponID].aim_Spot = 5;

			const char* tabNames[] = {
				AIMBOT_TEXT , TRIGGER_TEXT , VISUAL_TEXT , RADAR_TEXT ,
				KNIFEBOT_TEXT , SKIN_TEXT , MISC_TEXT , CONFIG_TEXT };

			static int tabOrder[] = { 0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 };
			static int tabSelected = 0;
			const bool tabChanged = ImGui::TabLabels(tabNames,
				sizeof(tabNames) / sizeof(tabNames[0]),
				tabSelected, tabOrder);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			float SpaceLineOne = 120.f;
			float SpaceLineTwo = 220.f;
			float SpaceLineThr = 320.f;

			if (tabSelected == 0) // Aimbot
			{
				ImGui::PushItemWidth(110.f);
				ImGui::Text("Current Weapon: ");
				ImGui::SameLine();
				ImGui::Combo("##AimWeapon", &iWeaponID, pWeaponData, IM_ARRAYSIZE(pWeaponData));
				ImGui::PopItemWidth();

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::Checkbox("Deathmatch", &Settings::Aimbot::aim_Deathmatch);
				ImGui::SameLine(SpaceLineOne);
				ImGui::Checkbox("WallAttack", &Settings::Aimbot::aim_WallAttack);
				ImGui::SameLine(SpaceLineTwo);
				ImGui::Checkbox("CheckSmoke", &Settings::Aimbot::aim_CheckSmoke);

				ImGui::Checkbox("AntiJump", &Settings::Aimbot::aim_AntiJump);
				ImGui::SameLine(SpaceLineOne);
				ImGui::Checkbox("Draw Fov", &Settings::Aimbot::aim_DrawFov);
				ImGui::SameLine(SpaceLineTwo);
				ImGui::Checkbox("DrawSpot", &Settings::Aimbot::aim_DrawSpot);

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::Checkbox("Active", &Settings::Aimbot::weapon_aim_settings[iWeaponID].aim_Active);

				if (iWeaponID <= 9)
				{
					ImGui::SameLine();
					ImGui::Checkbox("Autopistol", &Settings::Aimbot::weapon_aim_settings[iWeaponID].aim_AutoPistol);
				}

				ImGui::PushItemWidth(362.f);
				ImGui::SliderInt("Smooth", &Settings::Aimbot::weapon_aim_settings[iWeaponID].aim_Smooth, 1, 300);
				ImGui::SliderInt("Fov", &Settings::Aimbot::weapon_aim_settings[iWeaponID].aim_Fov, 1, 300);
				ImGui::PopItemWidth();

				const char* AimFovType[] = { "Dynamic" , "Static" };
				ImGui::PushItemWidth(362.f);
				ImGui::Combo("Fov Type", &Settings::Aimbot::weapon_aim_settings[iWeaponID].aim_FovType, AimFovType, IM_ARRAYSIZE(AimFovType));
				ImGui::PopItemWidth();

				const char* BestHit[] = { "Disable" , "Enable" };
				ImGui::PushItemWidth(362.f);
				ImGui::Combo("BestHit", &Settings::Aimbot::weapon_aim_settings[iWeaponID].aim_BestHit, BestHit, IM_ARRAYSIZE(BestHit));

				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("if disabled then used Aimspot");

				ImGui::PopItemWidth();

				const char* Aimspot[] = { "Head" , "Neck" , "Low Neck" , "Body" , "Thorax" , "Chest" };
				ImGui::PushItemWidth(362.f);
				ImGui::Combo("Aimspot", &Settings::Aimbot::weapon_aim_settings[iWeaponID].aim_Spot, Aimspot, IM_ARRAYSIZE(Aimspot));
				ImGui::PopItemWidth();

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::PushItemWidth(362.f);
				ImGui::SliderInt("ShotDelay", &Settings::Aimbot::weapon_aim_settings[iWeaponID].aim_Delay, 0, 200);
				ImGui::SliderInt("Rcs", &Settings::Aimbot::weapon_aim_settings[iWeaponID].aim_Rcs, 0, 100);
				ImGui::PopItemWidth();

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				if (iWeaponID >= 10 && iWeaponID <= 30)
				{
					ImGui::PushItemWidth(362.f);
					ImGui::SliderInt("Rcs Fov", &Settings::Aimbot::weapon_aim_settings[iWeaponID].aim_RcsFov, 1, 300);
					ImGui::SliderInt("Rcs Smooth", &Settings::Aimbot::weapon_aim_settings[iWeaponID].aim_RcsSmooth, 1, 300);
					ImGui::PopItemWidth();

					const char* ClampType[] = { "All Target" , "Shot" , "Shot + Target" };
					ImGui::PushItemWidth(362.f);
					ImGui::Combo("Rcs Clamp", &Settings::Aimbot::weapon_aim_settings[iWeaponID].aim_RcsClampType, ClampType, IM_ARRAYSIZE(ClampType));
					ImGui::PopItemWidth();

					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();
				}
			}
			else if (tabSelected == 1) // Trigger
			{
				const char* TriggerEnable[] = { "Disable" , "Fov" , "Trace" };
				ImGui::PushItemWidth(80.f);
				ImGui::Combo("Enable", &Settings::Triggerbot::trigger_Enable, TriggerEnable, IM_ARRAYSIZE(TriggerEnable));
				ImGui::PopItemWidth();
				ImGui::SameLine();

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::Checkbox("Deathmatch", &Settings::Triggerbot::trigger_Deathmatch);
				ImGui::SameLine(SpaceLineOne);
				ImGui::Checkbox("WallAttack", &Settings::Triggerbot::trigger_WallAttack);
				ImGui::SameLine(SpaceLineTwo);
				ImGui::Checkbox("FastZoom", &Settings::Triggerbot::trigger_FastZoom);

				ImGui::Checkbox("SmokCheck", &Settings::Triggerbot::trigger_SmokCheck);
				ImGui::SameLine(SpaceLineOne);
				ImGui::Checkbox("DrawFov", &Settings::Triggerbot::trigger_DrawFov);
				ImGui::SameLine(SpaceLineTwo);
				ImGui::Checkbox("DrawSpot", &Settings::Triggerbot::trigger_DrawSpot);
				ImGui::SameLine(SpaceLineThr);
				ImGui::Checkbox("DrawFovAssist", &Settings::Triggerbot::trigger_DrawFovAssist);

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				const char* items1[] = { CVAR_KEY_MOUSE3 , CVAR_KEY_MOUSE4 , CVAR_KEY_MOUSE5 };
				ImGui::PushItemWidth(80.f);
				ImGui::Combo("Key", &Settings::Triggerbot::trigger_Key, items1, IM_ARRAYSIZE(items1));
				ImGui::PopItemWidth();
				ImGui::SameLine();

				const char* items2[] = { "Hold" , "Press" };
				ImGui::PushItemWidth(80.f);
				ImGui::Combo("Key Mode", &Settings::Triggerbot::trigger_KeyMode, items2, IM_ARRAYSIZE(items2));
				ImGui::PopItemWidth();
				ImGui::SameLine();

				ImGui::PushItemWidth(110.f);
				ImGui::Combo("Weapon", &iWeaponID, pWeaponData, IM_ARRAYSIZE(pWeaponData));
				ImGui::PopItemWidth();

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::PushItemWidth(362.f);
				ImGui::SliderInt("Min Disstance", &Settings::Triggerbot::weapon_trigger_settings[iWeaponID].trigger_DistanceMin, 0, 5000);
				ImGui::SliderInt("Max Disstance", &Settings::Triggerbot::weapon_trigger_settings[iWeaponID].trigger_DistanceMax, 0, 5000);
				ImGui::SliderInt("Fov", &Settings::Triggerbot::weapon_trigger_settings[iWeaponID].trigger_Fov, 1, 100);
				ImGui::SliderInt("Delay Before", &Settings::Triggerbot::weapon_trigger_settings[iWeaponID].trigger_DelayBefore, 0, 200);
				ImGui::SliderInt("Delay After", &Settings::Triggerbot::weapon_trigger_settings[iWeaponID].trigger_DelayAfter, 0, 1000);
				ImGui::PopItemWidth();

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::Checkbox("HeadOnly", &Settings::Triggerbot::weapon_trigger_settings[iWeaponID].trigger_HeadOnly);
				ImGui::SameLine();

				const char* AssistMode[] = { "Disable" , "One Shot" , "Auto" };
				ImGui::PushItemWidth(80.f);
				ImGui::Combo("Assist", &Settings::Triggerbot::weapon_trigger_settings[iWeaponID].trigger_Assist, AssistMode, IM_ARRAYSIZE(AssistMode));
				ImGui::PopItemWidth();
				ImGui::SameLine();

				const char* AssistFovType[] = { "Dynamic" , "Static" };
				ImGui::PushItemWidth(80.f);
				ImGui::Combo("Assist Fov Type", &Settings::Triggerbot::weapon_trigger_settings[iWeaponID].trigger_AssistFovType, AssistFovType, IM_ARRAYSIZE(AssistFovType));
				ImGui::PopItemWidth();

				const char* HitGroup[] = { "All" , "Head + Body" , "Head" };
				ImGui::Combo("HitGroup", &Settings::Triggerbot::weapon_trigger_settings[iWeaponID].trigger_HitGroup, HitGroup, IM_ARRAYSIZE(HitGroup));
				//ImGui::PopItemWidth();

				ImGui::PushItemWidth(362.f);
				ImGui::SliderInt("Assist Rcs", &Settings::Triggerbot::weapon_trigger_settings[iWeaponID].trigger_AssistRcs, 0, 100);
				ImGui::SliderInt("Assist Fov", &Settings::Triggerbot::weapon_trigger_settings[iWeaponID].trigger_AssistFov, 1, 300);
				ImGui::SliderInt("Assist Smooth", &Settings::Triggerbot::weapon_trigger_settings[iWeaponID].trigger_AssistSmooth, 1, 300);
				ImGui::PopItemWidth();

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
			}
			else if (tabSelected == 2) // Visuals
			{
				string style_1 = "Box";
				string style_2 = "CoalBox";

				const char* items1[] = { style_1.c_str() , style_2.c_str() };

				ImGui::PushItemWidth(339.f);
				ImGui::Combo("Esp Type", &Settings::Esp::esp_Style, items1, IM_ARRAYSIZE(items1));
				ImGui::PopItemWidth();

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::Checkbox("Esp Team", &Settings::Esp::esp_Team);
				ImGui::SameLine(SpaceLineOne);
				ImGui::Checkbox("Esp Enemy", &Settings::Esp::esp_Enemy);
				ImGui::SameLine(SpaceLineTwo);
				ImGui::Checkbox("Esp Bomb", &Settings::Esp::esp_Bomb);
				ImGui::SameLine(SpaceLineThr);
				ImGui::Checkbox("Esp Sound", &Settings::Esp::esp_Sound);

				ImGui::Checkbox("Esp Line", &Settings::Esp::esp_Line);
				ImGui::SameLine(SpaceLineOne);
				ImGui::Checkbox("Esp OutLine", &Settings::Esp::esp_Outline);
				ImGui::SameLine(SpaceLineTwo);
				ImGui::Checkbox("Esp Name", &Settings::Esp::esp_Name);
				ImGui::SameLine(SpaceLineThr);
				ImGui::Checkbox("Esp Rank", &Settings::Esp::esp_Rank);

				ImGui::Checkbox("Esp Weapon", &Settings::Esp::esp_Weapon);
				ImGui::SameLine(SpaceLineOne);
				ImGui::Checkbox("Esp Ammo", &Settings::Esp::esp_Ammo);
				ImGui::SameLine(SpaceLineTwo);
				ImGui::Checkbox("Esp Distance", &Settings::Esp::esp_Distance);
				ImGui::SameLine(SpaceLineThr);
				ImGui::Checkbox("Esp Skeleton", &Settings::Esp::esp_Skeleton);

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::Checkbox("Esp World Weapon", &Settings::Esp::esp_WorldWeapons);
				ImGui::Checkbox("Esp World Grenade", &Settings::Esp::esp_WorldGrenade);

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				string visible_1 = "Enemy";
				string visible_2 = "Team";
				string visible_3 = "All";
				string visible_4 = "Only Visible";

				const char* items2[] = { visible_1.c_str() , visible_2.c_str() , visible_3.c_str() , visible_4.c_str() };

				ImGui::PushItemWidth(339.f);
				ImGui::Combo("Esp Visible", &Settings::Esp::esp_Visible, items2, IM_ARRAYSIZE(items2));

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::SliderInt("Esp Size", &Settings::Esp::esp_Size, 0, 10);
				ImGui::SliderInt("Esp BombTimer", &Settings::Esp::esp_BombTimer, 0, 65);
				ImGui::SliderInt("Esp BulletTrace", &Settings::Esp::esp_BulletTrace, 0, 3000);

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				string hpbar_1 = "None";
				string hpbar_2 = "Number";
				string hpbar_3 = "Bottom";
				string hpbar_4 = "Left";

				const char* items3[] = { hpbar_1.c_str() , hpbar_2.c_str() , hpbar_3.c_str() , hpbar_4.c_str() };
				ImGui::Combo("Esp Health", &Settings::Esp::esp_Health, items3, IM_ARRAYSIZE(items3));

				string arbar_1 = "None";
				string arbar_2 = "Number";
				string arbar_3 = "Bottom";
				string arbar_4 = "Right";

				const char* items4[] = { arbar_1.c_str() , arbar_2.c_str() , arbar_3.c_str() , arbar_4.c_str() };
				ImGui::Combo("Esp Armor", &Settings::Esp::esp_Armor, items4, IM_ARRAYSIZE(items4));

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				string chams_1 = "None";
				string chams_2 = "Flat";
				string chams_3 = "Texture";

				const char* items5[] = { chams_1.c_str() , chams_2.c_str() , chams_3.c_str() };
				ImGui::Combo("Chams", &Settings::Esp::esp_Chams, items5, IM_ARRAYSIZE(items5));

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::ColorEdit3("Esp Color CT", Settings::Esp::esp_Color_CT);
				ImGui::ColorEdit3("Esp Color TT", Settings::Esp::esp_Color_TT);
				ImGui::ColorEdit3("Esp Color Visible CT", Settings::Esp::esp_Color_VCT);
				ImGui::ColorEdit3("Esp Color Visible TT", Settings::Esp::esp_Color_VTT);

				ImGui::ColorEdit3("Chams Color CT", Settings::Esp::chams_Color_CT);
				ImGui::ColorEdit3("Chams Color TT", Settings::Esp::chams_Color_TT);
				ImGui::ColorEdit3("Chams Color Visible CT", Settings::Esp::chams_Color_VCT);
				ImGui::ColorEdit3("Chams Color Visible TT", Settings::Esp::chams_Color_VTT);
				ImGui::PopItemWidth();

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
			}
			else if (tabSelected == 3) // Radar
			{
				ImGui::Checkbox("Active", &Settings::Radar::rad_Active);
				ImGui::SameLine();
				ImGui::Checkbox("Team", &Settings::Radar::rad_Team);
				ImGui::SameLine();
				ImGui::Checkbox("Enemy", &Settings::Radar::rad_Enemy);
				ImGui::SameLine();
				ImGui::Checkbox("Sound", &Settings::Radar::rad_Sound);
				ImGui::SameLine();
				ImGui::Checkbox("InGame", &Settings::Radar::rad_InGame);

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::PushItemWidth(339.f);
				ImGui::SliderInt("Range", &Settings::Radar::rad_Range, 1, 5000);
				ImGui::SliderInt("Alpha", &Settings::Radar::rad_Alpha, 1, 255);

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::ColorEdit3("Color CT", Settings::Radar::rad_Color_CT);
				ImGui::ColorEdit3("Color TT", Settings::Radar::rad_Color_TT);
				ImGui::ColorEdit3("Color Visible CT", Settings::Radar::rad_Color_VCT);
				ImGui::ColorEdit3("Color Visible TT", Settings::Radar::rad_Color_VTT);
				ImGui::PopItemWidth();

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
			}
			else if (tabSelected == 4) // knifebot
			{
				ImGui::Checkbox("Active", &Settings::Knifebot::knf_Active);
				ImGui::Checkbox("Attack Team", &Settings::Knifebot::knf_Team);

				ImGui::Separator();

				string attack_1 = "Attack 1";
				string attack_2 = "Attack 2";
				string attack_3 = "Attack 1 + Attack 2";

				const char* items[] = { attack_1.c_str() , attack_2.c_str() , attack_3.c_str() };
				ImGui::Combo("Attack type", &Settings::Knifebot::knf_Attack, items, IM_ARRAYSIZE(items));

				ImGui::Separator();

				ImGui::SliderInt("Dist to attack 1", &Settings::Knifebot::knf_DistAttack, 1, 100);
				ImGui::SliderInt("Dist to attack 2", &Settings::Knifebot::knf_DistAttack2, 1, 100);
			}
			//Fixed by smef#2433 and Troll Face#1136 on discord
			else if (tabSelected == 5) // Skins
			{
				//[enc_string_disable /]
				const char* knife_models_items[] =
				{
					"Default","Bayonet","Flip","Gut","Karambit" ,"M9 Bayonet",
					"Huntsman","Falchion","Bowie","Butterfly","Shadow Daggers"
				};

				const char* quality_items[] =
				{
					"Normal","Genuine","Vintage","Unusual","Community","Developer",
					"Self-Made","Customized","Strange","Completed","Tournament"
				};

				const char* skins_items[] =
				{
					"", "",  "2: Groundwater","3: Candy Apple", "",
					"5: Forest DDPAT","6: Arctic Camo",  "",
					"8: Desert Storm","9: Bengal Tiger","10: Copperhead",
					"11: Skulls","12: Crimson Web","13: Blue Streak","14: Red Laminate"
					,"15: Gunsmoke","16: Jungle Tiger","17: Urban DDPAT",  "",
					"", "20: Virus","21: Granite Marbleized","22: Contrast Spray",
					"",  "", "25: Forest Leaves","26: Lichen Dashed",
					"27: Bone Mask","28: Anodized Navy",  "",  "30: Snake Camo",
					"",  "32: Silver","33: Hot Rod","34: Metallic DDPAT",  "",
					"36: Ossified","37: Blaze","38: Fade","39: Bulldozer","40: Night",
					"41: Copper","42: Blue Steel","43: Stained","44: Case Hardened",
					"",  "46: Contractor","47: Colony","48: Dragon Tattoo",
					"",   "",  "51: Lightning Strike",  "",
					"",   "",   "",   "",   "",
					"",    "59: Slaughter","60: Dark Water"
					,"61: Hypnotic","62: Bloomstick",  "",   ""
					,   "",   "",  "67: Cold Blooded",  "",   ""
					,  "70: Carbon Fiber","71: Scorpion","72: Safari Mesh","73: Wings","74: Polar Camo"
					,"75: Blizzard Marbleized","76: Winter Forest","77: Boreal Forest","78: Forest Night"
					,  "",   "",   "",   "",  "83: Orange DDPAT","84: Pink DDPAT"
					,  "",   "",   "",   "",  "",  "90: Mudder",  ""
					,  "92: Cyanospatter","93: Caramel",  "", "95: Grassland","96: Blue Spruce",  ""
					,  "98: Ultraviolet","99: Sand Dune","100: Storm","101: Tornado","102: Whiteout",  ""
					,  "104: Grassland Leaves",  "",   "", "107: Polar Mesh",  "",   ""
					, "110: Condemned","111: Glacier Mesh",  "",   "",   "",   "",  "116: Sand Mesh",  "",   "", "119: Sage Spray",  "",   "", "122: Jungle Spray",  "",  "124: Sand Spray",  ""
					,  "",  "",  "",  "",  "",  "",  "",  "",  "", "135: Urban Perforated"
					,"136: Waves Perforated",  "",   "",   "",  "", "141: Orange Peel",  "",  "143: Urban Masked", "", "", "", "147: Jungle Dashed"
					,"148: Sand Dashed","149: Urban Dashed", "", "151: Jungle", "", "153: Demolition","154: Afterimage","155: Bullet Rain","156: Death by Kitty","157: Palm","158: Walnut","159: Brass", "", "", "162: Splash", "","164: Modern Hunter","165: Splash Jam","166: Blaze Orange","167: Radiation Hazard","168: Nuclear Threat","169: Fallout Warning","170: Predator","171: Irradiated Alert","172: Black Laminate", "","174: BOOM","175: Scorched","176: Faded Zebra","177: Memento","178: Doomkitty","179: Nuclear Threat","180: Fire Serpent","181: Corticera","182: Emerald Dragon","183: Overgrowth","184: Corticera","185: Golden Koi","186: Wave Spray","187: Zirka","188: Graven","189: Bright Water","190: Black Limba","191: Tempest","192: Shattered","193: Bone Pile","194: Spitfire","195: Demeter","196: Emerald","197: Anodized Navy","198: Hazard","199: Dry Season","200: Mayan Dreams","201: Palm","202: Jungle DDPAT","203: Rust Coat","204: Mosaico","205: Jungle","206: Tornado","207: Facets","208: Sand Dune","209: Groundwater","210: Anodized Gunmetal","211: Ocean Foam","212: Graphite","213: Ocean Foam","214: Graphite","215: X-Ray","216: Blue Titanium","217: Blood Tiger","218: Hexane","219: Hive","220: Hemoglobin","221: Serum","222: Blood in the Water","223: Nightshade","224: Water Sigil","225: Ghost Camo","226: Blue Laminate","227: Electric Hive","228: Blind Spot","229: Azure Zebra","230: Steel Disruption","231: Cobalt Disruption","232: Crimson Web","233: Tropical Storm","234: Ash Wood","235: VariCamo","236: Night Ops","237: Urban Rubble","238: VariCamo Blue", "", "240: CaliCamo","241: Hunting Blind","242: Army Mesh","243: Gator Mesh","244: Teardown","245: Army Recon","246: Amber Fade","247: Damascus Steel","248: Red Quartz","249: Cobalt Quartz","250: Full Stop","251: Pit Viper","252: Silver Quartz","253: Acid Fade","254: Nitro","255: Asiimov","256: The Kraken","257: Guardian","258: Mehndi","259: Redline","260: Pulse","261: Marina","262: Rose Iron","263: Rising Skull","264: Sandstorm","265: Kami","266: Magma","267: Cobalt Halftone","268: Tread Plate","269: The Fuschia Is Now","270: Victoria","271: Undertow","272: Titanium Bit","273: Heirloom","274: Copper Galaxy","275: Red FragCam","276: Panther","277: Stainless","278: Blue Fissure","279: Asiimov","280: Chameleon","281: Corporal","282: Redline","283: Trigon","284: Heat","285: Terrain","286: Antique","287: Pulse","288: Sergeant","289: Sandstorm","290: Guardian","291: Heaven Guard", "", "293: Death Rattle","294: Green Apple","295: Franklin","296: Meteorite","297: Tuxedo","298: Army Sheen","299: Caged Steel","300: Emerald Pinstripe","301: Atomic Alloy","302: Vulcan","303: Isaac","304: Slashed","305: Torque","306: Antique","307: Retribution","308: Kami","309: Howl","310: Curse","311: Desert Warfare","312: Cyrex","313: Orion","314: Heaven Guard","315: Poison Dart","316: Jaguar","317: Bratatat","318: Road Rash","319: Detour","320: Red Python","321: Master Piece","322: Nitro","323: Rust Coat", "", "325: Chalice","326: Knight","327: Chainmail","328: Hand Cannon","329: Dark Age","330: Briar", "", "332: Royal Blue","333: Indigo","334: Twist","335: Module","336: Desert-Strike","337: Tatter","338: Pulse","339: Caiman","340: Jet Set","341: First Class","342: Leather","343: Commuter","344: Dragon Lore","345: First Class","346: Coach Class","347: Pilot","348: Red Leather","349: Osiris","350: Tigris","351: Conspiracy","352: Fowl Play","353: Water Elemental","354: Urban Hazard","355: Desert-Strike","356: Koi","357: Ivory","358: Supernova","359: Asiimov","360: Cyrex","361: Abyss","362: Labyrinth","363: Traveler","364: Business Class","365: Olive Plaid","366: Green Plaid","367: Reactor","368: Setting Sun","369: Nuclear Waste","370: Bone Machine","371: Styx","372: Nuclear Garden","373: Contamination","374: Toxic","375: Radiation Hazard","376: Chemical Green","377: Hot Shot","378: Fallout Warning","379: Cerberus","380: Wasteland Rebel","381: Grinder","382: Murky","383: Basilisk","384: Griffin","385: Firestarter","386: Dart","387: Urban Hazard","388: Cartel","389: Fire Elemental","390: Highwayman","391: Cardiac","392: Delusion","393: Tranquility","394: Cartel","395: Man-o'-war","396: Urban Shock","397: Naga","398: Chatterbox","399: Catacombs","400: 龍王 (Dragon King)","401: System Lock","402: Malachite","403: Deadly Poison","404: Muertos","405: Serenity","406: Grotto","407: Quicksilver","", "409: Tiger Tooth","410: Damascus Steel","411: Damascus Steel", "", "413: Marble Fade","414: Rust Coat","415: Doppler","416: Doppler","417: Doppler","418: Doppler","419: Doppler","420: Doppler","421: Doppler","422: Elite Build","423: Armor Core","424: Worm God","425: Bronze Deco","426: Valence","427: Monkey Business","428: Eco","429: Djinn","430: Hyper Beast","431: Heat","432: Man-o'-war","433: Neon Rider","434: Origami","435: Pole Position","436: Grand Prix","437: Twilight Galaxy","438: Chronos","439: Hades","440: Icarus Fell","441: Minotaur's Labyrinth","442: Asterion","443: Pathfinder","444: Daedalus","445: Hot Rod","446: Medusa","447: Duelist","448: Pandora's Box","449: Poseidon","450: Moon in Libra","451: Sun in Leo","452: Shipping Forecast","453: Emerald","454: Para Green","455: Akihabara Accept","456: Hydroponic","457: Bamboo Print","458: Bamboo Shadow","459: Bamboo Forest","460: Aqua Terrace", "", "462: Counter Terrace","463: Terrace","464: Neon Kimono","465: Orange Kimono","466: Crimson Kimono","467: Mint Kimono","468: Midnight Storm","469: Sunset Storm 壱","470: Sunset Storm 弐","471: Daybreak","472: Impact Drill","473: Seabird","474: Aquamarine Revenge","475: Hyper Beast","476: Yellow Jacket","477: Neural Net","478: Rocket Pop","479: Bunsen Burner","480: Evil Daimyo","481: Nemesis","482: Ruby Poison Dart","483: Loudmouth","484: Ranger","485: Handgun","486: Elite Build","487: Cyrex","488: Riot","489: Torque","490: Frontside Misty","491: Dualing Dragons","492: Survivor Z","493: Flux","494: Stone Cold","495: Wraiths","496: Nebula Crusader","497: Golden Coil","498: Rangeen","499: Cobalt Core","500: Special Delivery","501: Wingshot","502: Green Marine","503: Big Iron","504: Kill Confirmed","505: Scumbria","506: Point Disarray","507: Ricochet","508: Fuel Rod","509: Corinthian","510: Retrobution","511: The Executioner","512: Royal Paladin", "", "514: Power Loader","515: Imperial","516: Shapewood","517: Yorick","518: Outbreak","519: Tiger Moth","520: Avalanche","521: Teclu Burner","522: Fade","523: Amber Fade","524: Fuel Injector","525: Elite Build","526: Photic Zone","527: Kumicho Dragon","528: Cartel","529: Valence","530: Triumvirate", "", "532: Royal Legion","533: The Battlestar","534: Lapis Gator","535: Praetorian","536: Impire","537: Hyper Beast","538: Necropos","539: Jambiya","540: Lead Conduit","541: Fleet Flock","542: Judgement of Anubis","543: Red Astor","544: Ventilators","545: Orange Crash","546: Firefight","547: Spectre","548: Chantico's Fire","549: Bioleak","550: Oceanic","551: Asiimov","552: Fubar","553: Atlas","554: Ghost Crusader","555: Re-Entry","556: Primal Saber","557: Black Tie","558: Lore","559: Lore","560: Lore","561: Lore","562: Lore","563: Black Laminate","564: Black Laminate","565: Black Laminate","566: Black Laminate","567: Black Laminate","568: Gamma Doppler","569: Gamma Doppler","570: Gamma Doppler","571: Gamma Doppler","572: Gamma Doppler","573: Autotronic","574: Autotronic","575: Autotronic","576: Autotronic","577: Autotronic","578: Bright Water","579: Bright Water","580: Freehand","581: Freehand","582: Freehand","583: Aristocrat","584: Phobos","585: Violent Daimyo","586: Wasteland Rebel","587: Mecha Industries","588: Desolate Space","589: Carnivore","590: Exo","591: Imperial Dragon","592: Iron Clad","593: Chopper","594: Harvester","595: Reboot","596: Limelight","597: Bloodsport","598: Aerial","599: Ice Cap","600: Neon Revolution","601: Syd Mead","602: Imprint","603: Directive","604: Roll Cage","605: Scumbria","606: Ventilator","607: Weasel","608: Petroglyph","609: Airlock","610: Dazzle","611: Grim","612: Powercore","613: Triarch","614: Fuel Injector","615: Briefing","616: Slipstream","617: Doppler","618: Doppler","619: Doppler","620: Ultraviolet","621: Ultraviolet","622: Polymer","623: Ironwork","624: Dragonfire","625: Royal Consorts","626: Mecha Industries","627: Cirrus","628: Stinger","629: Black Sand","630: Sand Scale","631: Flashback","632: Buzz Kill","633: Sonar","634: Gila","635: Turf","636: Shallow Grave","637: Cyrex","638: Wasteland Princess","639: Bloodsport","640: Fever Dream","641: Jungle Slipstream","642: Blueprint","643: Xiangliu","644: Decimator","645: Oxide Blaze","646: Capillary","647: Crimson Tsunami","648: Emerald Poison Dart","649: Akoben","650: Ripple","651: Last Dive","652: Scaffold","653: Neo-Noir","654: Seasons","655: Zander","656: Orbit Mk01","657: Blueprint","658: Cobra Strike","659: Macabre","660: Hyper Beast","661: Sugar Rush","662: Oni Taiji","663: Briefing","664: Hellfire","665: Aloha","666: Hard Water","667: Woodsman","668: Red Rock","669: Death Grip","670: Death's Head","671: Cut Out","672: Metal Flowers","673: Morris","674: Triqua","675: The Empress","676: High Roller","677: Hunter","678: See Ya Later","679: Goo","680: Off World","681: Leaded Glass","682: Oceanic","683: Llama Cannon","684: Cracked Opal","685: Jungle Slipstream","686: Phantom","687: Tacticat","688: Exposure","689: Ziggy"
				};

				const char* gloves_listbox_items[25] =
				{
					"default",
					"bloodhound_black_silver","bloodhound_snakeskin_brass","bloodhound_metallic","handwrap_leathery",
					"handwrap_camo_grey","slick_black","slick_military","slick_red","sporty_light_blue","sporty_military",
					"handwrap_red_slaughter","motorcycle_basic_black","motorcycle_mint_triangle","motorcycle_mono_boom",
					"motorcycle_triangle_blue","specialist_ddpat_green_camo","specialist_kimono_diamonds_red",
					"specialist_emerald_web","specialist_orange_white","handwrap_fabric_orange_camo","sporty_purple",
					"sporty_green","bloodhound_guerrilla","slick_snakeskin_yellow"
				};
				//[enc_string_enable /]

			
				ImGui::Text("Current Weapon: %s", pWeaponData[iWeaponID]);

				ImGui::PushItemWidth(362.f);

				ImGui::Separator();

				ImGui::Combo("Knife CT Model", &Settings::Skin::knf_ct_model, knife_models_items, IM_ARRAYSIZE(knife_models_items));
				ImGui::Combo("Knife TT Model", &Settings::Skin::knf_tt_model, knife_models_items, IM_ARRAYSIZE(knife_models_items));

				ImGui::Combo("Custom CT Knife", &Settings::Skin::knf_ct_skin, skins_items, IM_ARRAYSIZE(skins_items));
				ImGui::Combo("Custom TT Knife", &Settings::Skin::knf_tt_skin, skins_items, IM_ARRAYSIZE(skins_items));

				ImGui::Separator();

				static int iSelectKnifeCTSkinIndex = -1;
				static int iSelectKnifeTTSkinIndex = -1;

				int iKnifeCTModelIndex = Settings::Skin::knf_ct_model;
				int iKnifeTTModelIndex = Settings::Skin::knf_tt_model;

				static int iOldKnifeCTModelIndex = -1;
				static int iOldKnifeTTModelIndex = -1;

				if (iOldKnifeCTModelIndex != iKnifeCTModelIndex && Settings::Skin::knf_ct_model)
					iSelectKnifeCTSkinIndex = GetKnifeSkinIndexFromPaintKit(Settings::Skin::knf_ct_skin, false);

				if (iOldKnifeTTModelIndex != iKnifeTTModelIndex && Settings::Skin::knf_tt_model)
					iSelectKnifeTTSkinIndex = GetKnifeSkinIndexFromPaintKit(Settings::Skin::knf_ct_skin, true);

				iOldKnifeCTModelIndex = iKnifeCTModelIndex;
				iOldKnifeTTModelIndex = iKnifeTTModelIndex;

				string KnifeCTModel = knife_models_items[Settings::Skin::knf_ct_model];
				string KnifeTTModel = knife_models_items[Settings::Skin::knf_tt_model];

				KnifeCTModel += " Skin##KCT";
				KnifeTTModel += " Skin##KTT";

				ImGui::SliderFloat("Knife CT Wear", &g_SkinChangerCfg[WEAPON_KNIFE].flFallbackWear, 0.001f, 1.f);
				ImGui::Combo("Knife CT Quality", &g_SkinChangerCfg[WEAPON_KNIFE].iEntityQuality, quality_items, IM_ARRAYSIZE(quality_items));
				ImGui::ComboBoxArray(KnifeCTModel.c_str(), &iSelectKnifeCTSkinIndex, KnifeSkins[iKnifeCTModelIndex].SkinNames);

				ImGui::Separator();

				ImGui::SliderFloat("Knife TT Wear", &g_SkinChangerCfg[WEAPON_KNIFE_T].flFallbackWear, 0.001f, 1.f);
				ImGui::Combo("Knife TT Quality", &g_SkinChangerCfg[WEAPON_KNIFE_T].iEntityQuality, quality_items, IM_ARRAYSIZE(quality_items));
				ImGui::ComboBoxArray(KnifeTTModel.c_str(), &iSelectKnifeTTSkinIndex, KnifeSkins[iKnifeTTModelIndex].SkinNames);

				ImGui::Separator();

				static int iOldWeaponID = -1;

				ImGui::Combo("Weapon##WeaponSelect", &iWeaponID, pWeaponData, IM_ARRAYSIZE(pWeaponData));

				iWeaponSelectIndex = pWeaponItemIndexData[iWeaponID];

				if (iOldWeaponID != iWeaponID)
					iWeaponSelectSkinIndex = GetWeaponSkinIndexFromPaintKit(g_SkinChangerCfg[iWeaponSelectIndex].nFallbackPaintKit);

				iOldWeaponID = iWeaponID;

				string WeaponSkin = pWeaponData[iWeaponID];
				WeaponSkin += " Skin";

				ImGui::ComboBoxArray(WeaponSkin.c_str(), &iWeaponSelectSkinIndex, WeaponSkins[iWeaponID].SkinNames);

				ImGui::Combo("Weapon Qality", &g_SkinChangerCfg[pWeaponItemIndexData[iWeaponID]].iEntityQuality, quality_items, IM_ARRAYSIZE(quality_items));
				ImGui::SliderFloat("Weapon Wear", &g_SkinChangerCfg[pWeaponItemIndexData[iWeaponID]].flFallbackWear, 0.001f, 1.f);
				ImGui::InputInt("Weapon StatTrak", &g_SkinChangerCfg[pWeaponItemIndexData[iWeaponID]].nFallbackStatTrak, 1, 100, ImGuiInputTextFlags_CharsDecimal);
				ImGui::Combo("Custom Skin", &g_SkinChangerCfg[pWeaponItemIndexData[iWeaponID]].nFallbackPaintKit, skins_items, IM_ARRAYSIZE(skins_items));

				ImGui::Separator();

				ImGui::Combo("Gloves Skin", &Settings::Skin::gloves_skin, gloves_listbox_items,
					IM_ARRAYSIZE(gloves_listbox_items));

				ImGui::Separator();

				ImGui::PopItemWidth();

				if (ImGui::Button("Apply##Skin"))
				{
					if (iWeaponSelectSkinIndex >= 0) {
						g_SkinChangerCfg[iWeaponSelectIndex].nFallbackPaintKit = WeaponSkins[iWeaponID].SkinPaintKit[iWeaponSelectSkinIndex];
					}
					if (iSelectKnifeCTSkinIndex > 0) {
						Settings::Skin::knf_ct_skin = KnifeSkins[iKnifeCTModelIndex].SkinPaintKit[iSelectKnifeCTSkinIndex];
					}
					if (iSelectKnifeTTSkinIndex > 0) {
						Settings::Skin::knf_tt_skin = KnifeSkins[iKnifeTTModelIndex].SkinPaintKit[iSelectKnifeTTSkinIndex];
					}
					ForceFullUpdate();
				}
			}
			else if (tabSelected == 6) // Misc
			{
				ImGui::Checkbox("Bhop", &Settings::Misc::misc_Bhop);
				ImGui::Checkbox("Punch", &Settings::Misc::misc_Punch);
				ImGui::Checkbox("AwpAim", &Settings::Misc::misc_AwpAim);
				ImGui::Checkbox("NoFlash", &Settings::Misc::misc_NoFlash);
				ImGui::Checkbox("AutoStrafe", &Settings::Misc::misc_AutoStrafe);
				ImGui::Checkbox("AutoAccept", &Settings::Misc::misc_AutoAccept);
				ImGui::Checkbox("Spectators", &Settings::Misc::misc_Spectators);
				ImGui::Checkbox("Fov Changer", &Settings::Misc::misc_FovChanger);
				ImGui::PushItemWidth(362.f);
				ImGui::SliderInt("Fov View", &Settings::Misc::misc_FovView, 1, 190);
				ImGui::SliderInt("Fov Model View", &Settings::Misc::misc_FovModelView, 1, 190);
				ImGui::Separator();
				ImGui::ColorEdit3("Awp Aim Color", Settings::Misc::misc_AwpAimColor);
				ImGui::PopItemWidth();
			}
			else if (tabSelected == 7) // Config
			{
				static int iConfigSelect = 0;
				static int iMenuSheme = 1;
				static char ConfigName[64] = { 0 };

				ImGui::ComboBoxArray("Select Config", &iConfigSelect, ConfigList);

				ImGui::Separator();

				if (ImGui::Button("Load Config"))
				{
					Settings::LoadSettings(BaseDir + "\\" + ConfigList[iConfigSelect]);
				}
				ImGui::SameLine();
				if (ImGui::Button("Save Config"))
				{
					Settings::SaveSettings(BaseDir + "\\" + ConfigList[iConfigSelect]);
				}
				ImGui::SameLine();
				if (ImGui::Button("Refresh Config List"))
				{
					RefreshConfigs();
				}

				ImGui::Separator();

				ImGui::InputText("Config Name", ConfigName, 64);

				if (ImGui::Button("Create & Save new Config"))
				{
					string ConfigFileName = ConfigName;

					if (ConfigFileName.size() < 1)
					{
						ConfigFileName = "settings";
					}

					Settings::SaveSettings(BaseDir + "\\" + ConfigFileName + ".ini");
					RefreshConfigs();
				}

				ImGui::Separator();

				const char* ThemesList[] = { "Default" , "Red"  , "Blue" };

				ImGui::Combo("Menu Color Sheme", &iMenuSheme, ThemesList, IM_ARRAYSIZE(ThemesList));

				ImGui::Separator();

				if (ImGui::Button("Apply Sheme"))
				{
					if (iMenuSheme == 0)
					{
						g_pGui->DefaultSheme();
					}
					else if (iMenuSheme == 1)
					{
						g_pGui->RedSheme();
					}
					else if (iMenuSheme == 2)
					{
						g_pGui->BlackSheme();
					}
				}
			}

			ImGui::End();
		}
	}
}