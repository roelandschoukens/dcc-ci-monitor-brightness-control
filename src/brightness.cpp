#include "brightness.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <map>
#include <regex>
#include <stdio.h>
#include <algorithm>
#include <Windows.h>
#include <lowlevelmonitorconfigurationapi.h>

// note that GetMonitorCapabilities() only works for specific (and older) MCCS versions.

static constexpr uint8_t VCP_BRIGHTNESS = 0x10;
static constexpr uint8_t VCP_CONTRAST = 0x12;


static const std::regex vcpRe("vcp\\(");
static const std::regex hexRe("[0-9A-Z]+");


MonitorControl::MonitorControl() {}
MonitorControl::~MonitorControl() {}


class MonitorControlImpl : public MonitorControl
{
    struct physicalList
    {
        PHYSICAL_MONITOR list[32];
        DWORD amount;
    };

    std::vector<physicalList> physicalMonitorLists;
    std::map<HANDLE, MonitorInfo> monitors;

    float brightness = 0, contrast = 0;
    
public:

    virtual float getBrightness() override
    {
        return brightness;
    }


    virtual void setBrightness(float v) override
    {
        brightness = v;
        for (auto & m : monitors)
        {
            if (m.second.doesBrightness)
            {
                int b = (int) std::round(v * m.second.maxBrightness);
                if (b != m.second.currentBrightness)
                {
                    m.second.currentBrightness = b;
                    SetVCPFeature(m.first, VCP_BRIGHTNESS, (DWORD) b);
                }
            }
        }
    }


    virtual float getContrast() override
    {
        return contrast;
    }


    virtual void setContrast(float v) override
    {
        contrast = v;
        for (auto & m : monitors)
        {
            if (m.second.doesContrast)
            {
                int c = (int) std::round(v * m.second.maxContrast);
                if (c != m.second.currentContrast)
                {
                    m.second.currentContrast = c;
                    SetVCPFeature(m.first, VCP_CONTRAST, (DWORD) std::round(v * m.second.maxContrast));
                }
            }
        }
    }


    virtual std::vector<MonitorInfo> monitorList() override
    {
        std::vector<MonitorInfo> info;
        info.reserve(monitors.size());
        for (const auto & p : monitors)
        {
            info.push_back(p.second);
        }
        return info;
    }


    void probe()
    {
        auto monitorProc = [](
            HMONITOR logicalMonitor,
            HDC,
            LPRECT,
            LPARAM selfPtr) -> BOOL
        {
            auto * self = reinterpret_cast<MonitorControlImpl*>(selfPtr);

            // We got a logical monitor here. Get physical monitor handles.
            self->physicalMonitorLists.push_back({});
            auto & logicalMonitorData = self->physicalMonitorLists.back();

            DWORD amount;
            bool ok = true;
            ok = ok && GetNumberOfPhysicalMonitorsFromHMONITOR(logicalMonitor, &amount);
            assert(ok);
            amount = std::min<DWORD>(amount, 32);
            logicalMonitorData.amount = amount;

            ok = ok && GetPhysicalMonitorsFromHMONITOR(logicalMonitor, amount, logicalMonitorData.list);
            assert(ok);

            for (DWORD i = 0; i < amount; ++i)
            {
                const auto physicalMonitor = logicalMonitorData.list[i].hPhysicalMonitor;

                auto pairIB = self->monitors.insert({physicalMonitor, MonitorInfo{}});

                if (!pairIB.second)
                {
                    continue;
                }

                MonitorInfo & info = pairIB.first->second;
                info.name = logicalMonitorData.list[i].szPhysicalMonitorDescription;

                // Get and parse supported VCP codes.
                // This gets some coded stuff like this (somewhat truncated for
                // brevity, and wrapped). We are interested in the "vcp()" part.
                // 
                // (prot(monitor)type(LCD)model(Blah)cmds(01 02 03 07 0C E3 F3)
                // vcp(02 04 05 08 0C 10 12 14(01 05 06 08 0B))
                // mswhql(1)asset_eep(40)mccs_ver(2.2))

                char capStr[4096];
                ok = ok && CapabilitiesRequestAndCapabilitiesReply(physicalMonitor, capStr, 4096);

                std::cmatch m;

                // find "vcp("
                const char * capIt = capStr;
                if (std::regex_search(capIt, m, vcpRe))
                {
                    // parsing loop
                    capIt = m[0].second;
                    int depth = 1;
                    while (depth > 0)
                    {
                        if (*capIt == ' ') { ++capIt; }
                        else if (*capIt == '(') { ++depth; ++capIt; }
                        else if (*capIt == ')') { --depth; ++capIt; }
                        else if (std::regex_search(capIt, m, hexRe))
                        {
                            if (depth == 1)
                            {
                                long capCode = std::stol(capIt, nullptr, 16);
                                if (capCode == VCP_BRIGHTNESS) { info.doesBrightness = true; }
                                if (capCode == VCP_CONTRAST) { info.doesContrast = true; }
                            }
                            capIt = m[0].second;
                        }
                        else
                        {
                            assert(false);
                            break;
                        }
                    }

                    // read current and max values
                    MC_VCP_CODE_TYPE type;
                    DWORD current = 0, max = 0;
                    if (info.doesBrightness)
                    {
                        ok = ok && GetVCPFeatureAndVCPFeatureReply(physicalMonitor, VCP_BRIGHTNESS, &type, &current, &max);
                        assert(ok);
                        info.currentBrightness = current;
                        info.maxBrightness = max;
                        if (self->brightness == 0) {
                            self->brightness = (float) current / max;
                        }
                    }
                    if (info.doesContrast)
                    {
                        ok = ok && GetVCPFeatureAndVCPFeatureReply(physicalMonitor, VCP_CONTRAST, &type, &current, &max);
                        assert(ok);
                        info.currentContrast = current;
                        info.maxContrast = max;
                        if (self->contrast == 0) {
                            self->contrast = (float) current / max;
                        }
                    }
                }
            }

            return true;
        };

        EnumDisplayMonitors(NULL, NULL, monitorProc, reinterpret_cast<LPARAM>(this));
    }

public:

    ~MonitorControlImpl()
    {
        for (auto & data : physicalMonitorLists)
        {
            DestroyPhysicalMonitors(data.amount, data.list);
        }
    }
};


MonitorControl * MonitorControl::create()
{
    MonitorControlImpl * impl = new MonitorControlImpl();
    impl->probe();
    return impl;
}
