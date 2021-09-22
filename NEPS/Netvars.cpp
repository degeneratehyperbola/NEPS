#include "Config.h"
#include "Netvars.h"
#include "SDK/Client.h"
#include "SDK/ClientClass.h"
#include "SDK/Entity.h"
#include "SDK/Recv.h"

static int random(int min, int max) noexcept
{
	return rand() % (max - min + 1) + min;
}

static std::unordered_map<std::uint32_t, std::pair<RecvProxy, RecvProxy *>> proxies;

static void __cdecl spottedHook(RecvProxyData &data, void *arg2, void *arg3) noexcept
{
	if (config->misc.radarHack)
		data.value._int = 1;

	constexpr auto hash{fnv::hash("CBaseEntity->m_bSpotted")};
	proxies[hash].first(data, arg2, arg3);
}

static void __cdecl viewModelSequence(RecvProxyData &data, void *outStruct, void *arg3) noexcept
{
	const auto viewModel = reinterpret_cast<Entity *>(outStruct);

	if (localPlayer && interfaces->entityList->getEntityFromHandle(viewModel->owner()) == localPlayer.get())
	{
		if (const auto weapon = interfaces->entityList->getEntityFromHandle(viewModel->weapon()))
		{
			if (config->visuals.deagleSpinner && weapon->getClientClass()->classId == ClassId::Deagle && data.value._int == 7)
				data.value._int = 8;

			SkinChanger::fixKnifeAnimation(weapon, data.value._int);
		}
	}
	constexpr auto hash{fnv::hash("CBaseViewModel->m_nSequence")};
	proxies[hash].first(data, outStruct, arg3);
}

static std::vector<std::pair<std::uint32_t, std::uint32_t>> offsets;

static void walkTable(const char *networkName, RecvTable *recvTable, const std::size_t offset = 0) noexcept
{
	for (int i = 0; i < recvTable->propCount; ++i)
	{
		auto &prop = recvTable->props[i];

		if (isdigit(prop.name[0]))
			continue;

		if (fnv::hashRuntime(prop.name) == fnv::hash("baseclass"))
			continue;

		if (prop.type == 6
			&& prop.dataTable
			&& prop.dataTable->netTableName[0] == 'D')
			walkTable(networkName, prop.dataTable, prop.offset + offset);

		const auto hash = fnv::hashRuntime((networkName + std::string{"->"} + prop.name).c_str());

		constexpr auto getHook = [](std::uint32_t hash) noexcept -> RecvProxy
		{
			switch (hash)
			{
			case fnv::hash("CBaseEntity->m_bSpotted"):
				return spottedHook;
			case fnv::hash("CBaseViewModel->m_nSequence"):
				return viewModelSequence;
			default:
				return nullptr;
			}
		};

		offsets.emplace_back(hash, offset + prop.offset);

		constexpr auto hookProperty = [](std::uint32_t hash, RecvProxy &originalProxy, RecvProxy proxy) noexcept
		{
			if (originalProxy != proxy)
			{
				proxies[hash].first = originalProxy;
				proxies[hash].second = &originalProxy;
				originalProxy = proxy;
			}
		};

		if (auto hook{getHook(hash)})
			hookProperty(hash, prop.proxy, hook);
	}
}

Netvars::Netvars() noexcept
{
	for (auto clientClass = interfaces->client->getAllClasses(); clientClass; clientClass = clientClass->next)
		walkTable(clientClass->networkName, clientClass->recvTable);

	std::sort(offsets.begin(), offsets.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
}

std::uint32_t Netvars::operator[](const std::uint32_t hash) const noexcept
{
	const auto it = std::lower_bound(offsets.begin(), offsets.end(), hash, [](const auto &p, auto hash) { return p.first < hash; });
	if (it != offsets.end() && it->first == hash)
		return it->second;
	assert(false);
	return 0;
}

void Netvars::restore() noexcept
{
	for (const auto &[hash, proxyPair] : proxies)
		*proxyPair.second = proxyPair.first;

	proxies.clear();
	offsets.clear();
}
