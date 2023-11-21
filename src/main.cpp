#include "brightness.h"

#include <memory>
#include <juce_gui_extra/juce_gui_extra.h>
#include "binaries.h"

using namespace juce;

MonitorControl * monitorcontrolInstance();


static int labelWidth(
    Label & label)
{
    return label.getFont().getStringWidth(label.getText()) + 
        label.getBorderSize().getLeftAndRight() + 2;
}


static String percentText(double value)
{
    return String(roundToInt(value * 100)) + "%";
}


class OurCalloutContent : public Component,
    Slider::Listener, Button::Listener
{
public:
    OurCalloutContent()
        :
        brightnessLabel("Brightness", "Brightness"),
        contrastLabel("Contrast","Contrast")
    {
        auto * mc = monitorcontrolInstance();
        setSize(250, 80);
        brightnessSlider.setRange(0, 1, 0.01);
        brightnessSlider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
        contrastSlider.setRange(0, mc->getMaxContrast(), 0.01);
        contrastSlider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);

        brightnessSlider.setValue(mc->getBrightness(), juce::dontSendNotification);
        contrastSlider.setValue(mc->getContrast(), juce::dontSendNotification);
        brightnessValueLabel.setText(percentText(mc->getBrightness()), dontSendNotification);
        contrastValueLabel.setText(percentText(mc->getContrast()), dontSendNotification);

        brightnessValueLabel.setBorderSize(BorderSize<int>(0));
        brightnessValueLabel.setColour(Label::textColourId, Colours::white.darker());
        contrastValueLabel.setBorderSize(BorderSize<int>(0));
        contrastValueLabel.setColour(Label::textColourId, Colours::white.darker());

        Image resetImg = ImageCache::getFromMemory(BinaryData::reset_png, BinaryData::reset_pngSize);
        
        brightnessSlider.addListener(this);
        contrastSlider.addListener(this);
        contrastButton.addListener(this);
        contrastButton.setImages(false, true, true,
            resetImg, 1.f, juce::Colours::transparentBlack,
            resetImg, .85f, juce::Colours::white.withAlpha(.2f),
            resetImg, 1.f, juce::Colours::transparentBlack,
            .1f);


        // layout
        auto box = getLocalBounds().reduced(4, 0);
        // line 1
        auto cline = box.removeFromTop(16);
        brightnessLabel.setBounds(cline.removeFromLeft(labelWidth(brightnessLabel)));
        brightnessValueLabel.setBounds(cline);
        // line 2
        brightnessSlider.setBounds(box.removeFromTop(24));
        // line 3
        cline = box.removeFromTop(16);
        contrastLabel.setBounds(cline.removeFromLeft(labelWidth(contrastLabel)));
        contrastValueLabel.setBounds(cline);
        // line 4
        cline = box.removeFromTop(24);
        contrastButton.setBounds(cline.removeFromRight(24).reduced(4));
        contrastSlider.setBounds(cline);

        addAndMakeVisible(brightnessLabel);
        addAndMakeVisible(brightnessValueLabel);
        addAndMakeVisible(brightnessSlider);
        addAndMakeVisible(contrastLabel);
        addAndMakeVisible(contrastValueLabel);
        addAndMakeVisible(contrastSlider);
        addAndMakeVisible(contrastButton);
    }

    void buttonClicked(Button *b) override
    {
        if (b == &contrastButton) {
            contrastSlider.setValue(1.0, juce::sendNotificationAsync);
        }
    }

    void sliderValueChanged(Slider *s) override
    {
        if (s == &brightnessSlider) {
            updateBrightness = true;
            brightnessValueLabel.setText(percentText(brightnessSlider.getValue()), dontSendNotification);
            repaint();
        }
        else if (s == &contrastSlider) {
            updateContrast = true;
            contrastValueLabel.setText(percentText(contrastSlider.getValue()), dontSendNotification);
            repaint();
        }
    }

    void paint(juce::Graphics &) override
    {
        // Updating monitor settings takes longer than the time between two mouse events.
        // So updating this during the mouse events will saturate the event loop and prevent
        // WM_PAINT being generated. Deferring until we reach the paint call makes it so
        // the GUI doesn't appear blocked, without incurring the complexity of a separate
        // thread.
        if (updateBrightness) {
            monitorcontrolInstance()->setBrightness((float) brightnessSlider.getValue());
        }
        if (updateContrast) {
            monitorcontrolInstance()->setContrast((float) contrastSlider.getValue());
        }
        updateBrightness = false;
        updateContrast = false;
    }

    Label brightnessLabel;
    Label brightnessValueLabel;
    Slider brightnessSlider;
    Label contrastLabel;
    Label contrastValueLabel;
    Slider contrastSlider;
    ImageButton contrastButton;
    bool updateBrightness = false;
    bool updateContrast = false;
    bool focusFlag = false;
};


void showInfo();
void editNeutralContrast();


class OurSystemTrayIconComponent : public SystemTrayIconComponent
{
public:
    OurSystemTrayIconComponent()
    {
        float dpiScale = Desktop::getInstance().getGlobalScaleFactor();

        // Get icon.
        // I don't think Windows takes anything else than 16×16 or 32×32, and something is not handling
        // transparency properly so the icons also have either 0 or 100% alpha.
        // Ideally we should of course get a layer directly from our ICO resource but JUCE doesn't expose
        // this functionality.
        const auto img = (dpiScale <= 1.00f) ?
            ImageFileFormat::loadFrom(BinaryData::brightness16_png, BinaryData::brightness16_pngSize) :
            ImageFileFormat::loadFrom(BinaryData::brightness32_png, BinaryData::brightness32_pngSize);

        const auto templateImg = ImageFileFormat::loadFrom(BinaryData::brightnesssilhouette_png, BinaryData::brightnesssilhouette_pngSize);

        setIconImage(img, templateImg);

        setIconTooltip("Monitor brightness control");
    }

    virtual void mouseDown(const MouseEvent &e) override
    {
        if (e.mods.isPopupMenu())
        {
            PopupMenu m;
            m.addItem(1, "Info");
            m.addItem(2, "Edit neutral contrast");
            m.addSeparator();
            m.addItem(9, "Exit");
            m.showMenuAsync(PopupMenu::Options(), [](int result)
            {
                switch (result)
                {
                    case 1:
                        showInfo();
                        break;

                    case 2:
                        editNeutralContrast();
                        break;

                    case 9:
                        JUCEApplication::quit();
                        break;
                }
            });

        }
        else
        {
            auto content = std::make_unique<OurCalloutContent>();
            const auto pos = e.source.getScreenPosition().roundToInt();
            juce::Rectangle<int> mouseRect(pos.x, pos.y, 1, 1);
            CallOutBox& myBox
                = CallOutBox::launchAsynchronously(std::move(content), mouseRect, nullptr);
            myBox.setAlwaysOnTop(true);
        }
    }
};


//==============================================================================
class MonitorControlApplication  : public JUCEApplication
{
public:
    //==============================================================================
    MonitorControlApplication() {}

    const String getApplicationName() override       { return JUCE_APPLICATION_NAME_STRING; }
    const String getApplicationVersion() override    { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override       { return false; }

    //==============================================================================
    void initialise (const String&) override
    {
        PropertiesFile::Options sOptions;
        sOptions.applicationName = getApplicationName();
        settings.setStorageParameters(sOptions);

        MonitorControl::Settings mcSettings;
        auto * userSettings = settings.getUserSettings();
        if (userSettings)
        {
            for (int i = 0; i < 100; ++i)
            {
                const String monitorName = userSettings->getValue("monitorkey" + String(i));
                if (monitorName.isEmpty()) break;

                const int refContrast = userSettings->getIntValue("monitorRefContrast" + String(i));
                mcSettings.savedNeutralContrast[monitorName.toUTF16().getAddress()] = refContrast;
            }
        }

        monitorcontrol.reset(MonitorControl::create(std::move(mcSettings)));

        // look-and-feel (use the light one)
        lookAndFeel = std::make_unique<LookAndFeel_V3>();
        lookAndFeel->setDefaultSansSerifTypefaceName("Segoe UI");

        Colour highlight(0xff4499dd);
        Colour textButtonColour(0xffbbddff);
        lookAndFeel->setColour(Label::textColourId, Colours::white);
        lookAndFeel->setColour(Slider::textBoxOutlineColourId, Colours::white);
        lookAndFeel->setColour(TextEditor::highlightColourId, highlight);
        lookAndFeel->setColour(PopupMenu::highlightedBackgroundColourId, highlight);

        lookAndFeel->setColour(Slider::textBoxHighlightColourId, highlight);
        lookAndFeel->setColour(Slider::textBoxOutlineColourId, highlight);
        lookAndFeel->setColour(TextButton::buttonColourId, textButtonColour);
        lookAndFeel->setColour(TextButton::buttonOnColourId, Colour(0xffbbffaa));
        lookAndFeel->setColour(TextEditor::focusedOutlineColourId, textButtonColour);
        lookAndFeel->setColour(Slider::thumbColourId, textButtonColour);
        lookAndFeel->setColour(TextButton::buttonColourId, textButtonColour);
        lookAndFeel->setColour(TextButton::buttonColourId, textButtonColour);
        lookAndFeel->setColour(TextButton::buttonColourId, textButtonColour);
        lookAndFeel->setColour(TextButton::buttonColourId, textButtonColour);

        LookAndFeel::setDefaultLookAndFeel(lookAndFeel.get());

        icon = std::make_unique<OurSystemTrayIconComponent>();
    }

    void shutdown() override
    {
        icon = nullptr;
        lookAndFeel = nullptr;
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const String&) override
    {
        showInfo();
    }


    static MonitorControlApplication & instance()
    {
        return *static_cast<MonitorControlApplication*>(JUCEApplication::getInstance());
    }


    static PropertiesFile * getUserSettings()
    {
        return instance().settings.getUserSettings();
    }


    static MonitorControl * monitorcontrolInstance()
    {
        return instance().monitorcontrol.get();
    }


    static LookAndFeel & lookAndFeelInstance()
    {
        return *instance().lookAndFeel.get();
    }

private:
    std::unique_ptr<OurSystemTrayIconComponent> icon;
    std::unique_ptr<LookAndFeel> lookAndFeel;
    std::unique_ptr<MonitorControl> monitorcontrol;
    ApplicationProperties settings;
};


MonitorControl * monitorcontrolInstance()
{
    return MonitorControlApplication::monitorcontrolInstance();
}


// actions

void showInfo()
{
    auto * mc = monitorcontrolInstance();
    auto list = mc->monitorList();
    juce::String text;
    for (const auto & m : list)
    {
        if (text.isNotEmpty()) { text << "\n"; }

        text << "Monitor: " << m.name.c_str() << "\n";
        text << "  MCCS version: " << (m.version.empty() ? std::string(u8"—") : m.version) << "\n";
        text << "  Brightness supported: " << (m.doesBrightness ? "Yes" : "No");
        if (m.doesBrightness) { text << " (0 - " << m.maxBrightness << ")"; }
        text << "\n";
        text << "  Contrast supported: " << (m.doesContrast ? "Yes" : "No");
        if (m.doesContrast)
        {
            text << " (0 - " << m.maxContrast << ") / " << m.neutralContrast;
        }
        text << "\n";
    }
    juce::AlertWindow::showMessageBoxAsync(AlertWindow::NoIcon, "Monitor brightness control", text);
}


void editNeutralContrast()
{
    auto * mc = monitorcontrolInstance();
    mc->setContrast(1.0);
    auto list = mc->monitorList();

    class OurComponent : public Component, public Button::Listener
    {
    public:
        Label infoLabel;
        PropertyPanel propertyPanel;
        TextButton applyBtn{"Apply", "Apply this contrast setting"};

        // this one is sorted by name
        std::map<std::wstring, std::pair<Value, int>> neutralContrastValues;

        OurComponent()
        {
            infoLabel.setText(CharPointer_UTF8(u8"Set the monitor contrast to the desired “neutral” level."), juce::dontSendNotification);
            addAndMakeVisible(infoLabel);
            addAndMakeVisible(propertyPanel);
            addAndMakeVisible(applyBtn);
            applyBtn.addListener(this);
        }

        virtual void buttonClicked (Button*)
        {
            auto * userSettings = MonitorControlApplication::getUserSettings();
            if (!userSettings) return;

            for (int i = 0; i < 100; ++i)
            {
                const String monitorName = userSettings->getValue("monitorkey" + String(i));
                if (monitorName.isEmpty()) break;
                userSettings->removeValue("monitorkey" + String(i));
                userSettings->removeValue("monitorRefContrast" + String(i));
            }

            MonitorControl::Settings mcSettings;
            int i = 0;
            for (const auto & m : neutralContrastValues)
            {
                const int nc =  m.second.first.getValue();
                userSettings->setValue("monitorkey" + String(i), m.first.c_str());
                userSettings->setValue("monitorRefContrast" + String(i), nc);
                mcSettings.savedNeutralContrast[m.first] = nc;
                ++i;
            }
            MonitorControlApplication::monitorcontrolInstance()->updateSettings(std::move(mcSettings));
        }
    };

    juce::OptionalScopedPointer<OurComponent> content(new OurComponent, true);
    auto & neutralContrastValues = content->neutralContrastValues;

    for (const auto & m : list)
    {
        neutralContrastValues[m.name] = {Value(m.neutralContrast), m.maxContrast};
    }
    juce::Colour bgColor = MonitorControlApplication::lookAndFeelInstance().findColour(DialogWindow::backgroundColourId);

    juce::Array<PropertyComponent*> properties;
    for (auto pair : neutralContrastValues)
    {
        String jName = CharPointer_UTF16(pair.first.c_str());
        properties.add(new SliderPropertyComponent(pair.second.first, jName, 0.0, pair.second.second, 1.0));
    }

    int propertyHeight = 5 + 25 * (int) neutralContrastValues.size();
    content->propertyPanel.addProperties(properties);

    // layout (depends on amount of values)
    int y = 0;
    content->infoLabel.setBounds(0, y, 400, 25);
    y += 25;
    content->propertyPanel.setBounds(0, y, 400, propertyHeight);
    y += propertyHeight;
    content->applyBtn.setBounds(juce::Rectangle<int>(0, y, 400, 30).withSizeKeepingCentre(120, 24));
    y += 30;
    content->setSize(400, y);

    content->applyBtn.addListener(content);

    DialogWindow::LaunchOptions dlo;
    dlo.dialogTitle = "Monitor neutral contrast";
    dlo.dialogBackgroundColour = bgColor;
    dlo.content.setOwned(content.release());
    dlo.resizable = false;
    dlo.escapeKeyTriggersCloseButton = true;
    // use non-native title bar, or else the layout will be wrong
    dlo.useNativeTitleBar = false;

    dlo.launchAsync();
}

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (MonitorControlApplication)
