#pragma once

#include <cassert>
#include <memory>
#include <vector>

#include "lib/fnv.hpp"

struct RecvProp;
struct RecvTable;

struct Netvars
{
	Netvars() noexcept;

	void restore() noexcept;

	std::uint32_t operator[](const std::uint32_t hash) const noexcept;
};

inline std::unique_ptr<Netvars> netvars;

#define PNETVAR_OFFSET(funcname, class_name, var_name, offset, type) \
[[nodiscard]] auto funcname() noexcept \
{ \
    constexpr auto hash = fnv::hash(class_name "->" var_name); \
    return reinterpret_cast<std::add_pointer_t<type>>(this + netvars->operator[](hash) + offset); \
}

#define PNETVAR(funcname, class_name, var_name, type) \
    PNETVAR_OFFSET(funcname, class_name, var_name, 0, type)

#define NETVAR_OFFSET(funcname, class_name, var_name, offset, type) \
[[nodiscard]] std::add_lvalue_reference_t<type> funcname() noexcept \
{ \
    constexpr auto hash = fnv::hash(class_name "->" var_name); \
    return *reinterpret_cast<std::add_pointer_t<type>>(this + netvars->operator[](hash) + offset); \
}

#define NETVAR(funcname, class_name, var_name, type) \
    NETVAR_OFFSET(funcname, class_name, var_name, 0, type)
