#include <stdio.h>
#include <Windows.h>
#include <OlsApi.h>
#include <OlsDef.h>
#include <stdexcept>
#include <stdint.h>
#include <cstring>
#include <string>
#include <string_view>
#include <memory>
#include "WinRingRes.h"

#define VOLTAGE_OFFSET_MSR_INDEX 0x150
#define PLANE_COUNT 5

using namespace std::string_literals;

BOOL CheckDllError()
{
    auto status = GetDllStatus();
    if (status == OLS_DLL_NO_ERROR)
        return TRUE;

    switch (status) {
    case OLS_DLL_UNSUPPORTED_PLATFORM:
        puts("Platform not supported.");
        break;
    case OLS_DLL_DRIVER_NOT_LOADED:
        puts("Driver not loaded.");
        break;
    case OLS_DLL_DRIVER_NOT_FOUND:
        puts("Driver not found.");
        break;
    case OLS_DLL_DRIVER_NOT_LOADED_ON_NETWORK:
        puts("Please start this application on a local drive.");
        break;
    default:
        puts("Unknown error.");
        break;
    }
    return FALSE;
}

DWORD VoltageToMsrWord(float mv)
{
    return (static_cast<uint32_t>(static_cast<int32_t>(mv * 1.024)) & 0xFFF) << 21;
}

float MsrWordToVoltage(DWORD msr)
{
    return static_cast<float>(static_cast<int32_t>((msr >> 21) | (msr & 0x80000000 ? 0xFFFFF800 : 0))) / 1.024f;
}

DWORD VoltageOffsetControlWord(uint8_t plane, bool write)
{
    uint8_t writeBit = write ? 1 : 0;
    return 0x80000010 | (static_cast<uint32_t>(plane) << 8) | static_cast<uint32_t>(writeBit);
}

BOOL ReadVoltageOffset(uint8_t plane, float* out)
{
    if (!Wrmsr(VOLTAGE_OFFSET_MSR_INDEX, 0, VoltageOffsetControlWord(plane, false)))
        return FALSE;
    DWORD rdEax, rdEdx;
    if (!Rdmsr(VOLTAGE_OFFSET_MSR_INDEX, &rdEax, &rdEdx))
        return FALSE;
    *out = MsrWordToVoltage(rdEax);
    return TRUE;
}

std::unique_ptr<WinRingRes> InitializeWinRingRes()
{
    auto ret = std::make_unique<WinRingRes>();
    try
    {
        ret->Initialize();
    }
    catch (std::exception& ex)
    {
        puts(ex.what());
        return nullptr;
    }
    if (!CheckDllError())
        return nullptr;

    if (!IsMsr())
    {
        puts("MSR control not supported.");
        return nullptr;
    }
    return std::move(ret);
}

void PrintUsage()
{
    puts("Usage:");
    puts("  IntelVoltageControl show");
    puts("  IntelVoltageControl set [--allow-overvolt] [--commit] ([plane_number] [voltage_offset])...");
    puts("    voltage_offset is in mV.");
    puts("    plane_number for Intel Skylake systems are as follows:");
    puts("    - 0 = CPU core");
    puts("    - 1 = iGPU core/slice");
    puts("    - 2 = CPU cache");
    puts("    - 3 = System agent");
    puts("    - 4 = iGPU uncore/unslice");
    puts("  Example to set voltage offset for both CPU core and iGPU core:");
    puts("    IntelVoltageControl set --commit 0 -100 1 -50");
}

void ShowVoltageOffsets()
{
    auto winRingRes = InitializeWinRingRes();
    if (!winRingRes.get())
        return;
    for (int i = 0; i < PLANE_COUNT; ++i)
    {
        float voltage;
        if (!ReadVoltageOffset(i, &voltage))
        {
            puts("Error reading voltage offset value.");
            break;
        }
        printf("Plane %d: %.1f mV\n", i, voltage);
    }
}

void SetVoltageOffset(int argc, char** argv)
{
    argv += 2;
    argc -= 2;
    bool commit = false;
    bool allowOvervolt = false;
    DWORD reqVoltages[PLANE_COUNT];
    bool setVoltages[PLANE_COUNT];
    bool hasVoltageSet = false;
    memset(reqVoltages, 0, sizeof(reqVoltages));
    memset(setVoltages, 0, sizeof(setVoltages));

    // parse args
    try 
    {
        for (int i = 0; i < argc; ++i)
        {
            int planeNumber;
            float reqVoltage;
            auto const arg = std::string_view(argv[i]);
            if (arg == "--commit")
                commit = true;
            else if (arg == "--allow-overvolt")
                allowOvervolt = true;
            else if(sscanf_s(argv[i], "%d", &planeNumber))
            {
                if (planeNumber < 0 || planeNumber >= PLANE_COUNT)
                {
                    printf("Plane number must be between 0 and %d.\n", planeNumber);
                    return;
                }
                ++i;
                if ((i >= argc) || !sscanf_s(argv[i], "%f", &reqVoltage))
                {
                    PrintUsage();
                    return;
                }
                if (abs(reqVoltage) > 999.0)
                {
                    puts("Voltage offset must between -999 mV to 999 mV.");
                    return;
                }
                if (reqVoltage > 0 && !allowOvervolt)
                {
                    puts("Use --allow-overvolt to enable overvolting.");
                    return;
                }
                reqVoltages[planeNumber] = VoltageToMsrWord(reqVoltage);
                hasVoltageSet = setVoltages[planeNumber] = true;
            }
        }
    }
    catch (std::exception)
    {
        PrintUsage();
        return;
    }

    if (!hasVoltageSet)
    {
        PrintUsage();
        return;
    }

    for (int i = 0; i < PLANE_COUNT; ++i)
    {
        if (setVoltages[i])
        {
            printf("Set offset plane %d to %.1f mV\n", i, MsrWordToVoltage(reqVoltages[i]));
        }
    }    

    if (commit)
    {
        auto const winRingRes = InitializeWinRingRes();
        if (!winRingRes)
            return;
        for (int i = 0; i < PLANE_COUNT; ++i)
        {
            if (setVoltages[i]) 
            {
                if (!Wrmsr(VOLTAGE_OFFSET_MSR_INDEX, reqVoltages[i], VoltageOffsetControlWord(i, true)))
                {
                    puts("Error setting voltage offset.");
                    return;
                }
            }
        }
    }
    else
    {
        puts("Changes not applied. Use --commit to apply changes.");
    }
}

int main(int argc, char ** argv)
{
    if (argc < 2)
    {
        PrintUsage();
        return 0;
    }

    auto const cmd = std::string_view(argv[1]);
    if (cmd == "show"s)
        ShowVoltageOffsets();
    else if (cmd == "set"s)
        SetVoltageOffset(argc, argv);
    return 0;
}
