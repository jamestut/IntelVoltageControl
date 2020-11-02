#pragma once

class WinRingRes
{
public:
	WinRingRes() = default;
	~WinRingRes();

	// no copy construct/assign
	WinRingRes(WinRingRes const&) = delete;
	WinRingRes& operator=(WinRingRes const&) = delete;

	// move assign
	WinRingRes(WinRingRes &&) noexcept;
	WinRingRes& operator=(WinRingRes&&) noexcept;

	void Initialize();

private:
	bool hasResource_ = false;
};

