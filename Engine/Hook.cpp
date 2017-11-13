#include "Hook.h"

//[enc_string_enable /]
//[junk_enable /]

#define MAXBACKTRACKTICKS 32
#define TICK_INTERVAL			(Interfaces::GlobalVars()->interval_per_tick)
#define TIME_TO_TICKS( dt )		( (int)( 0.5f + (float)(dt) / TICK_INTERVAL ) )
backtrackData headPositions[64][16];

namespace Engine
{
	namespace Hook
	{
		CSX::Hook::VTable IDirect3DDevice9Table;
		CSX::Hook::VTable SoundTable;
		CSX::Hook::VTable ClientModeTable;
		CSX::Hook::VTable GameEventTable;
		CSX::Hook::VTable ModelRenderTable;
		CSX::Hook::VTable ClientTable;
		CSX::Hook::VTable SurfaceTable;

		IDirect3DDevice9* g_pDevice = nullptr;

		typedef HRESULT(WINAPI* Present_t)(IDirect3DDevice9* pDevice, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
		Present_t Present_o;

		typedef HRESULT(WINAPI* Reset_t)(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
		Reset_t Reset_o;

		HRESULT WINAPI Hook_Present(IDirect3DDevice9* pDevice, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
		{
			Client::OnRender();

			return Present_o(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
		}

		HRESULT WINAPI Hook_Reset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
		{
			Client::OnLostDevice();

			HRESULT hRes = Reset_o(pDevice, pPresentationParameters);

			Client::OnResetDevice();

			return hRes;
		}

		bool WINAPI Hook_CreateMove(float flInputSampleTime, CUserCmd* pCmd)
		{
			ClientModeTable.UnHook();

			int bestTargetIndex = -1;
			float bestFov = FLT_MAX;
			CBaseEntity* local = (CBaseEntity*)Interfaces::EntityList()->GetClientEntity(Interfaces::Engine()->GetLocalPlayer());
			PlayerInfo info;
			for (int i = 0; i < SDK::Interfaces::Engine()->GetMaxClients(); i++)
			{
				auto entity = (CBaseEntity*)SDK::Interfaces::EntityList()->GetClientEntity(i);

				if (!entity || !local)
					continue;

				if (entity == local)
					continue;

				if (!Interfaces::Engine()->GetPlayerInfo(i, &info))
					continue;

				if (entity->IsDormant())
					continue;

				/*	if (checkteams)
				{
				if (entity->m_iTeamNum() == local->m_iTeamNum())
				continue;
				}*/

				if (!entity->IsDead())
				{
					float simtime = entity->GetSimTime();
					Vector hitboxPos = entity->GetHitboxPosition(0);

					headPositions[i][pCmd->command_number % 13] = backtrackData{ simtime, hitboxPos };
					Vector ViewDir = AngleVector(pCmd->viewangles + (local->GetAimPunchAngle() * 2.f));
					float FOVDistance = DistancePointToLine(hitboxPos, local->GetEyePosition(), ViewDir);

					if (bestFov > FOVDistance)
					{
						bestFov = FOVDistance;
						bestTargetIndex = i;
					}
				}
			}

			float bestTargetSimTime;
			if (bestTargetIndex != -1)
			{
				float tempFloat = FLT_MAX;

				Vector ViewDir = AngleVector(pCmd->viewangles + (local->GetAimPunchAngle() * 2.f));

				for (int t = 0; t <= 12; ++t)
				{
					float tempFOVDistance = DistancePointToLine(headPositions[bestTargetIndex][t].hitboxPos, local->GetEyePosition(), ViewDir);

					if (tempFloat > tempFOVDistance && headPositions[bestTargetIndex][t].simtime > Interfaces::GlobalVars()->curtime - 1)
					{

						tempFloat = tempFOVDistance;
						bestTargetSimTime = headPositions[bestTargetIndex][t].simtime;
					}
				}
				/*if (cmd->buttons & IN_ATTACK2)
				{
				QAngle imgay;
				math::vector_angles((headPositions[bestTargetIndex][12].hitboxPos - local->get_eye_pos()), imgay);
				g_EngineClient->SetViewAngles(imgay);
				}*/

				if (pCmd->buttons & IN_ATTACK)
				{
					pCmd->tick_count = TIME_TO_TICKS(bestTargetSimTime);

				}

			}



			Client::OnCreateMove(pCmd);

			bool ret = Interfaces::ClientMode()->CreateMove(flInputSampleTime, pCmd);
			ClientModeTable.ReHook();
			return ret;
		}

		bool WINAPI Hook_OverrideView(CViewSetup* pSetup)
		{
			Client::OnOverrideView(pSetup);

			ClientModeTable.UnHook();
			bool ret = Interfaces::ClientMode()->OverrideView(pSetup);
			ClientModeTable.ReHook();
			return ret;
		}

		float WINAPI Hook_GetViewModelFOV()
		{
			ClientModeTable.UnHook();
			float fov = Interfaces::ClientMode()->GetViewModelFOV();
			ClientModeTable.ReHook();

			Client::OnGetViewModelFOV(fov);

			return fov;
		}

		bool WINAPI Hook_FireEventClientSideThink(IGameEvent* pEvent)
		{
			bool ret = false;

			if (!pEvent)
			{
				GameEventTable.UnHook();
				ret = Interfaces::GameEvent()->FireEventClientSide(pEvent);
				GameEventTable.ReHook();
				return ret;
			}

			Client::OnFireEventClientSideThink(pEvent);

			GameEventTable.UnHook();
			ret = Interfaces::GameEvent()->FireEventClientSide(pEvent);
			GameEventTable.ReHook();

			return ret;
		}

		void WINAPI Hook_FrameStageNotify(ClientFrameStage_t Stage)
		{
			Client::OnFrameStageNotify(Stage);

			ClientTable.UnHook();
			Interfaces::Client()->FrameStageNotify(Stage);
			ClientTable.ReHook();
		}

		int WINAPI Hook_EmitSound1(IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSoundEntry, unsigned int nSoundEntryHash, const char *pSample,
			float flVolume, soundlevel_t iSoundlevel, int nSeed, int iFlags = 0, int iPitch = PITCH_NORM,
			const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1)
		{

			if (pSample)
				Client::OnPlaySound(pOrigin, pSample);

			SoundTable.UnHook();

			int ret = Interfaces::Sound()->EmitSound1(filter, iEntIndex, iChannel, pSoundEntry, nSoundEntryHash, pSample,
				flVolume, iSoundlevel, nSeed, iFlags, iPitch,
				pOrigin, pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity);

			SoundTable.ReHook();

			return ret;
		}

		int WINAPI Hook_EmitSound2(IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSoundEntry, unsigned int nSoundEntryHash, const char *pSample,
			float flVolume, float flAttenuation, int nSeed, int iFlags = 0, int iPitch = PITCH_NORM,
			const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1)
		{
			if (pSample)
				Client::OnPlaySound(pOrigin, pSample);

			SoundTable.UnHook();

			int ret = Interfaces::Sound()->EmitSound2(filter, iEntIndex, iChannel, pSoundEntry, nSoundEntryHash, pSample,
				flVolume, flAttenuation, nSeed, iFlags, iPitch,
				pOrigin, pDirection, pUtlVecOrigins, bUpdatePositions, soundtime, speakerentity);

			SoundTable.ReHook();

			return ret;
		}

		void WINAPI Hook_DrawModelExecute(IMatRenderContext* ctx, const DrawModelState_t &state,
			const ModelRenderInfo_t &pInfo, matrix3x4_t *pCustomBoneToWorld = NULL)
		{
			ModelRenderTable.UnHook();

			if (ctx && pCustomBoneToWorld)
				Client::OnDrawModelExecute(ctx, state, pInfo, pCustomBoneToWorld);

			Interfaces::ModelRender()->DrawModelExecute(ctx, state, pInfo, pCustomBoneToWorld);

			if (ctx && pCustomBoneToWorld && Client::g_pEsp && Settings::Esp::esp_Chams)
			{
				Interfaces::ModelRender()->ForcedMaterialOverride(0);
			}

			ModelRenderTable.ReHook();
		}

		void WINAPI Hook_PlaySound(const char* pszSoundName)
		{
			SurfaceTable.UnHook();

			if (pszSoundName)
				Client::OnPlaySound(pszSoundName);

			Interfaces::Surface()->PlaySound(pszSoundName);

			SurfaceTable.ReHook();
		}

		bool Initialize()
		{
			if (!CSX::Utils::IsModuleLoad(D3D9_DLL))
				return false;

			if (!CSX::Utils::IsModuleLoad(SHADERPIDX9_DLL))
				return false;

			if (!CSX::Utils::IsModuleLoad(GAMEOVERLAYRENDERER_DLL))
				return false;

			DWORD d3d9TablePtrPtr = CSX::Memory::FindPattern(SHADERPIDX9_DLL, D3D9_PATTERN, D3D9_MASK, 1);
			DWORD_PTR** dwPresent_o = (DWORD_PTR**)CSX::Memory::FindPattern(GAMEOVERLAYRENDERER_DLL, GMOR_PATTERN, GMOR_MASK, 1);

			if (d3d9TablePtrPtr && dwPresent_o)
			{
				g_pDevice = (IDirect3DDevice9*)(**(PDWORD*)d3d9TablePtrPtr);

				if (IDirect3DDevice9Table.InitTable(g_pDevice))
				{
					DWORD_PTR* dwAddress = *dwPresent_o;
					Present_o = (Present_t)(*dwAddress);
					*dwAddress = (DWORD_PTR)(&Hook_Present);

					Reset_o = (Reset_t)IDirect3DDevice9Table.GetHookIndex(D3D9::TABLE::Reset, Hook_Reset);

					if (!ClientModeTable.InitTable(Interfaces::ClientMode()))
						return false;

					ClientModeTable.HookIndex(TABLE::IClientMode::CreateMove, Hook_CreateMove);
					ClientModeTable.HookIndex(TABLE::IClientMode::OverrideView, Hook_OverrideView);
					ClientModeTable.HookIndex(TABLE::IClientMode::GetViewModelFOV, Hook_GetViewModelFOV);

					if (!GameEventTable.InitTable(Interfaces::GameEvent()))
						return false;

					GameEventTable.HookIndex(TABLE::IGameEventManager2::FireEventClientSide, Hook_FireEventClientSideThink);

					if (!ClientTable.InitTable(Interfaces::Client()))
						return false;

					ClientTable.HookIndex(TABLE::IBaseClientDLL::FrameStageNotify, Hook_FrameStageNotify);

					if (!SoundTable.InitTable(Interfaces::Sound()))
						return false;

					SoundTable.HookIndex(TABLE::IEngineSound::EmitSound1, Hook_EmitSound1);
					SoundTable.HookIndex(TABLE::IEngineSound::EmitSound2, Hook_EmitSound2);

					if (!ModelRenderTable.InitTable(Interfaces::ModelRender()))
						return false;

					ModelRenderTable.HookIndex(TABLE::IVModelRender::DrawModelExecute, Hook_DrawModelExecute);

					if (!SurfaceTable.InitTable(Interfaces::Surface()))
						return false;

					SurfaceTable.HookIndex(TABLE::ISurface::PlaySound, Hook_PlaySound);

					if (Client::Initialize(g_pDevice))
						return true;
				}
			}

			return false;
		}

		void Shutdown()
		{
			IDirect3DDevice9Table.UnHook();
			SoundTable.UnHook();
			ClientModeTable.UnHook();
			GameEventTable.UnHook();
			ModelRenderTable.UnHook();
			ClientTable.UnHook();
		}
	}
}