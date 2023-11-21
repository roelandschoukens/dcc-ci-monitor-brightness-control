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
        contrastSlider.setRange(0, 1, 0.01);
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
            contrastSlider.setValue(.5, juce::sendNotificationAsync);
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


void showInfo()
{
    auto * mc = monitorcontrolInstance();
    auto list = mc->monitorList();
    juce::String text;
    for (const auto & m : list)
    {
        if (text.isNotEmpty()) { text << "\n"; }

        text << "Monitor: " << m.name.c_str() << "\n";
        text << "  Brightness supported: " << (m.doesBrightness ? "Yes" : "No");
        if (m.doesBrightness) { text << " (0 - " << m.maxBrightness << ")"; }
        text << "\n";
        text << "  Contrast supported: " << (m.doesContrast ? "Yes" : "No");
        if (m.doesContrast) { text << " (0 - " << m.maxContrast << ")"; }
        text << "\n";
    }
    juce::AlertWindow::showMessageBoxAsync(AlertWindow::NoIcon, "Monitor brightness control", text);
}


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
            m.addSeparator();
            m.addItem(2, "Exit");
            m.showMenuAsync(PopupMenu::Options(), [](int result)
            {
                switch (result)
                {
                    case 1:
                        showInfo();
                        break;

                    case 2:
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
        monitorcontrol.reset(MonitorControl::create());

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


    static MonitorControl * monitorcontrolInstance()
    {
        return static_cast<MonitorControlApplication*>(JUCEApplication::getInstance())->monitorcontrol.get();
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

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (MonitorControlApplication)
