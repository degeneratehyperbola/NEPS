#include "Debug.h"

#include <shared_lib/imgui/imgui.h>
#include <shared_lib/imgui/imgui_stdlib.h>
#include <shared_lib/Texture/TextureDX9.h>

#include "lib/ImguiCustom.hpp"
#include "GameData.h"
#include "Hooks.h"
#include "Hacks/Animations.h"

#include "SDK/Client.h"
#include "SDK/ClientClass.h"
#include "SDK/ClientMode.h"
#include "SDK/ConVar.h"
#include "SDK/Cvar.h"
#include "SDK/Effects.h"
#include "SDK/EngineTrace.h"
#include "SDK/Entity.h"
#include "SDK/EntityList.h"
#include "SDK/NetworkStringTable.h"
#include "SDK/PlayerResource.h"
#include "SDK/Surface.h"
#include "SDK/Engine.h"

#include "resource.h"

void Debug::drawGUI() noexcept
{
	ImGui::ShowDemoWindow();

	ImGui::Columns(3, nullptr, false);

	{
		if (ImGui::Button("Test chat virtual methods", { -1, 0 }))
			memory->clientMode->getHudChat()->printf(0, "\x1N \x2N \x3N \x4N \x5N \x6N \x7N \x8N \x9N \xAN \xBN \xCN \xDN \xEN \xFN \x10N \x1");

		if (ImGui::Button("List client classes", { -1, 0 }))
		{
			for (int i = 0; i <= interfaces->entityList->getHighestEntityIndex(); i++)
			{
				auto entity = interfaces->entityList->getEntity(i);
				if (!entity) continue;

				memory->conColorMsg({ 0, 200, 0, 255 }, std::to_string(i).c_str());
				memory->debugMsg(": ");
				memory->conColorMsg({ 0, 120, 255, 255 }, entity->getClientClass()->networkName);
				memory->debugMsg(" -> ");
				memory->conColorMsg({ 255, 120, 255, 255 }, std::to_string((int)entity->getClientClass()->classId).c_str());
				memory->debugMsg("   ");
			}
		}

		static const char *entName;
		static ClassId entClassId;
		static int idx = -1;
		if (ImGui::Button("Select...") && localPlayer)
		{
			const Vector start = localPlayer->getEyePosition();
			const Vector end = start + Vector::fromAngle(interfaces->engine->getViewAngles()) * 1000.0f;

			Trace trace;
			interfaces->engineTrace->traceRay({ start, end }, ALL_VISIBLE_CONTENTS | CONTENTS_MOVEABLE | CONTENTS_DETAIL, localPlayer.get(), trace);

			if (trace.entity)
			{
				auto clientClass = trace.entity->getClientClass();
				entName = clientClass->networkName;
				entClassId = clientClass->classId;
				idx = trace.entity->index();
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Select self") && localPlayer)
		{
			auto clientClass = localPlayer->getClientClass();
			entName = clientClass->networkName;
			entClassId = clientClass->classId;
			idx = localPlayer->index();
		}

		Entity *entity = interfaces->entityList->getEntity(idx);
		if (entName)
		{
			ImGui::TextUnformatted("Selected:");
			ImGui::SameLine();
			ImGui::TextColored({ 1.0f, 1.0f, 0.0f, 1.0f }, "%s", entName);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("In entity list at %i", idx);
			ImGui::SameLine();
			ImGui::TextColored({ 0.0f, 0.3f, 1.0f, 1.0f }, "%i", entClassId);
		}

		static std::array<float, 3> lightColor = { 1.0f, 1.0f, 1.0f };
		static float radius = 120.0f;
		static float life = 20.0f;
		static int exponent = 2;

		ImGuiCustom::colorPicker("Light color", lightColor.data());
		ImGui::PushItemWidth(-1);
		ImGui::SliderFloat("##radius", &radius, 0.0f, 5000.0f, "Light radius %.3f", ImGuiSliderFlags_Logarithmic);
		ImGui::SliderInt("##exponent", &exponent, 0, 12, "Light exponent %d");
		ImGui::SliderFloat("##life", &life, 0.0f, 100.0f, "Light lifetime %.3f");
		ImGui::PopItemWidth();

		static DynamicLight *dlight = nullptr;
		if (entity && entClassId != ClassId::World && ImGui::Button("Allocade d-light for selected entity", { -1, 0 }))
		{
			dlight = interfaces->effects->allocDlight(idx);
			if (dlight)
			{
				dlight->outerAngle = 0.0f;
				dlight->flags = 0;
				dlight->decay = 0.0f;
				dlight->die = memory->globalVars->currentTime + life;
				dlight->origin = entity->getAbsOrigin();
				dlight->radius = radius;
				dlight->color.r = static_cast<unsigned char>(lightColor[0] * 255);
				dlight->color.g = static_cast<unsigned char>(lightColor[1] * 255);
				dlight->color.b = static_cast<unsigned char>(lightColor[2] * 255);
				dlight->color.exponent = exponent;
			}
		}

		if (entity && entity->isPlayer())
		{
			if (ImGui::Button("Resolve selected", { -1, 0 }))
				Animations::resolveAnimations(entity);
		}

		if (ImGui::Button("Precache info", { -1, 0 }))
			interfaces->engine->clientCmdUnrestricted("sv_precacheinfo");

		const auto &colors = ImGui::GetStyle().Colors;
		std::ostringstream ss;

		for (int i = 0; i < ImGuiCol_COUNT; ++i)
		{
			ss << "colors[ImGuiCol_" << ImGui::GetStyleColorName(i) << "] = {";
			ss.precision(2);
			ss << std::fixed << colors[i].x;
			ss << "f, ";
			ss << colors[i].y;
			ss << "f, ";
			ss << colors[i].z;
			ss << "f, ";
			ss << colors[i].w;
			ss << "f};\n";
		}

		if (ImGui::Button("Copy style colors", { -1, 0 }))
			ImGui::SetClipboardText(ss.str().c_str());
	}

	ImGui::NextColumn();

	{
		{
			GameData::Lock lock;

			auto playerResource = *memory->playerResource;

			if (localPlayer && playerResource)
			{
				if (ImGui::BeginTable("shrek", 4))
				{
					ImGui::TableSetupColumn("Name");
					ImGui::TableSetupColumn("Wins");
					ImGui::TableSetupColumn("Level");
					ImGui::TableSetupColumn("Ranking");
					ImGui::TableHeadersRow();

					ImGui::TableNextRow();
					ImGui::PushID(ImGui::TableGetRowIndex());

					if (ImGui::TableNextColumn())
						ImGui::TextUnformatted("Local player");

					if (ImGui::TableNextColumn())
						ImGui::Text("%i", playerResource->competitiveWins()[localPlayer->index()]);

					if (ImGui::TableNextColumn())
						ImGui::Text("%i", playerResource->level()[localPlayer->index()]);

					if (ImGui::TableNextColumn())
						ImGui::Text("%i", playerResource->competitiveRanking()[localPlayer->index()]);

					for (auto &player : GameData::players())
					{
						ImGui::TableNextRow();
						ImGui::PushID(ImGui::TableGetRowIndex());

						auto *entity = interfaces->entityList->getEntityFromHandle(player.handle);
						if (!entity) continue;

						if (ImGui::TableNextColumn())
							ImGui::TextUnformatted(player.name.c_str());

						if (ImGui::TableNextColumn())
							ImGui::Text("%i", playerResource->competitiveWins()[entity->index()]);

						if (ImGui::TableNextColumn())
							ImGui::Text("%i", playerResource->level()[entity->index()]);

						if (ImGui::TableNextColumn())
							ImGui::Text("%i", playerResource->competitiveRanking()[entity->index()]);
					}

					ImGui::EndTable();
				}

				ImGui::InputInt("Wins", &playerResource->competitiveWins()[localPlayer->index()]);
				ImGui::InputInt("Level", &playerResource->level()[localPlayer->index()]);
				ImGui::InputInt("Ranking", &playerResource->competitiveRanking()[localPlayer->index()]);
			}
		}

		ImGui::TextColored({ 1.0f, 0.8f, 0.0f, 1.0f }, "Local player");
		ImGui::SameLine();
		ImGui::TextUnformatted("at");
		ImGui::SameLine();
		ImGui::TextColored({ 0.0f, 0.2f, 1.0f, 1.0f }, "0x%p", localPlayer.get());
		ImGui::SameLine();

		char buffer[9];
		sprintf(buffer, "%p", localPlayer.get());
		if (ImGui::Button("Copy"))
			ImGui::SetClipboardText(buffer);
	}

	ImGui::NextColumn();

	{
		if (localPlayer)
		{
			const auto layers = localPlayer->animLayers();

			if (ImGui::BeginTable("shrek2", 5))
			{
				ImGui::TableSetupColumn("Name", 0, 4.0f);
				ImGui::TableSetupColumn("Weight");
				ImGui::TableSetupColumn("Rate");
				ImGui::TableSetupColumn("Seq");
				ImGui::TableSetupColumn("Cycle");
				ImGui::TableHeadersRow();

				for (int i = 0; i < localPlayer->getAnimLayerCount(); ++i)
				{
					ImGui::TableNextRow();
					ImGui::PushID(ImGui::TableGetRowIndex());

					if (ImGui::TableNextColumn())
					{
						switch (i)
						{
						case 0: ImGui::TextUnformatted("AIMMATRIX"); break;
						case 1: ImGui::TextUnformatted("WEAPON_ACTION"); break;
						case 2: ImGui::TextUnformatted("WEAPON_ACTION_RECROUCH"); break;
						case 3: ImGui::TextUnformatted("ADJUST"); break;
						case 4: ImGui::TextUnformatted("MOVEMENT_JUMP_OR_FALL"); break;
						case 5: ImGui::TextUnformatted("MOVEMENT_LAND_OR_CLIMB"); break;
						case 6: ImGui::TextUnformatted("MOVEMENT_MOVE"); break;
						case 7: ImGui::TextUnformatted("MOVEMENT_STRAFECHANGE"); break;
						case 8: ImGui::TextUnformatted("WHOLE_BODY"); break;
						case 9: ImGui::TextUnformatted("FLASHED"); break;
						case 10: ImGui::TextUnformatted("FLINCH"); break;
						case 11: ImGui::TextUnformatted("ALIVELOOP"); break;
						case 12: ImGui::TextUnformatted("LEAN"); break;
						case 13: ImGui::TextUnformatted("???"); break;
						case 14: ImGui::TextUnformatted("???"); break;
						case 15: ImGui::TextUnformatted("???"); break;
						}
					}

					if (ImGui::TableNextColumn())
						ImGui::Text("%.4f", layers[i].weight);

					if (ImGui::TableNextColumn())
						ImGui::Text("%.4f", layers[i].playbackRate);

					if (ImGui::TableNextColumn())
						ImGui::Text("%i", layers[i].sequence);

					if (ImGui::TableNextColumn())
						ImGui::Text("%.4f", layers[i].cycle);
				}

				ImGui::EndTable();
			}

			ImGui::Text("PoseParam_BodyYaw = %.4f", localPlayer->poseParams()[PoseParam_BodyYaw]);
		}

		static std::string soundPath;

		ImGui::SetNextItemWidth(-1);
		ImGui::InputTextWithHint("##snd_path", "Relative sound path", &soundPath);

		if (ImGui::Button("Play sound", { -1, 0 }))
			interfaces->surface->playSound(soundPath.c_str());

		if (ImGui::Button("Precache sound", { -1, 0 }))
		{
			if (const auto soundprecache = interfaces->networkStringTableContainer->findTable("soundprecache"))
				soundprecache->addString(false, soundPath.c_str());
		}
	}
}

void Debug::drawAdditionalContextMenuItems() noexcept
{
	if (ImGui::MenuItem("Fog UI"))
		interfaces->engine->clientCmdUnrestricted("fogui");
	if (ImGui::MenuItem("Loaded textures"))
		interfaces->cvar->findVar("mat_texture_list")->setValue(true);
}

void Debug::drawOverlay() noexcept
{
	static Texture debugNotice = { hooks->getDllHandle(), IDB_PNG2, L"PNG"};
	if (debugNotice.get())
		ImGui::GetBackgroundDrawList()->AddImage(debugNotice.get(), { 0, 0 }, { 256, 256 });
}
