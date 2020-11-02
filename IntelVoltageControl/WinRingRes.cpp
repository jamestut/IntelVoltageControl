#include "WinRingRes.h"
#include <stdexcept>
#include <Windows.h>
#include <OlsApi.h>

static bool initialized = false;

WinRingRes::~WinRingRes()
{
	if (hasResource_ && initialized)
	{
		DeinitializeOls();
		initialized = false;
	}
}

WinRingRes::WinRingRes(WinRingRes&& other) noexcept
{
	*this = std::move(other);
}

WinRingRes& WinRingRes::operator=(WinRingRes&& other) noexcept
{
	hasResource_ = other.hasResource_;
	other.hasResource_ = false;
	return *this;
}

void WinRingRes::Initialize()
{
	if (initialized)
		throw std::runtime_error("WinRing0 already initialized.");
	if (!InitializeOls())
		throw std::runtime_error("Unknown error in initializing WinRing0.");
	initialized = true;
	hasResource_ = true;
}
