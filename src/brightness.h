#pragma once

#include <string>
#include <vector>

class MonitorControl
{
public:

    struct MonitorInfo
    {
        std::wstring name;
        bool doesBrightness = false;
        int currentBrightness = 0;
        int maxBrightness = 0;
        bool doesContrast = false;
        int currentContrast = 0;
        int maxContrast = 0;
    };

    static MonitorControl * create();
    virtual ~MonitorControl();

    virtual float getBrightness() = 0;
    virtual void setBrightness(float v) = 0;


    virtual float getContrast() = 0;
    virtual void setContrast(float v) = 0;

    virtual std::vector<MonitorInfo> monitorList() = 0;

protected:
    MonitorControl();
};
