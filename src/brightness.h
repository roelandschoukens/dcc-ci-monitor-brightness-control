#pragma once

#include <string>
#include <vector>
#include <unordered_map>

class MonitorControl
{
public:

    struct MonitorInfo
    {
        std::wstring name;
        std::string version;
        bool doesBrightness = false;
        int currentBrightness = 0;
        int maxBrightness = 0;
        bool doesContrast = false;
        int currentContrast = 0;
        int maxContrast = 0;
        int neutralContrast = 0;
    };

    struct Settings
    {
        std::unordered_map<std::wstring, int> savedNeutralContrast;
    };

    static MonitorControl * create(Settings && settings);
    virtual ~MonitorControl();

    virtual float getBrightness() = 0;
    virtual void setBrightness(float v) = 0;

    virtual void updateSettings(Settings && settings) = 0;

    virtual float getContrast() = 0;
    virtual float getMaxContrast() = 0;
    virtual void setContrast(float v) = 0;

    virtual std::vector<MonitorInfo> monitorList() = 0;

protected:
    MonitorControl();
};
