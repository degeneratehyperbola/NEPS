#pragma once

template <typename T>
class UtlVector
{
public:
	constexpr T &operator[](int i) noexcept { return memory[i]; };
	constexpr const T &operator[](int i) const noexcept { return memory[i]; };

	void destructAll() noexcept { while (size) reinterpret_cast<T *>(&memory[--size])->~T(); }

	T *begin() noexcept { return memory; }
	T *end() noexcept { return memory + size; }

	T *memory;
	int allocationCount;
	int growSize;
	int size;
	T *elements;
};
