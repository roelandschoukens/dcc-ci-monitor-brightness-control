# Simple brightness control

This is a simple application to set monitor brightness on Windows desktop PCs. Many monitors allow
sending commands using [DDC/CI](https://en.wikipedia.org/wiki/Display_Data_Channel), a standard from
all the way back in 1998.

## Usage notes

Recent Windows versions do not by default display icons in the notification area (aka. system tray).

In Windows 11 you will have to open Taskbar settings, open “Other system tray icons” and enable the
*brightness_slider.exe* item. You’ll probably want to do this after installing the executable in
its final position.

In older versions the icon will show up in the overflow area.

Putting utilities like this over there is officially frowned upon, and the official recommendation
is to use a task bar icon. But guess where the volume control is. In the system tray it goes.

If you want to run this on start-up, the old trick of dropping a shortcut in
`"%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup"` still works. (you can open this folder by entering `shell:startup` in the Run dialog.)

## Building

This project depends on JUCE, see the [JUCE CMake documentation](https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md) on
how to find it. If you don’t have a system-wide JUCE install you can clone their repository and set `USE_JUCE_DIR` to the path to the checkout.

## Technical notes

This uses the [_Low-Level Monitor Configuration Functions_](https://learn.microsoft.com/en-us/windows/win32/monitor/using-the-low-level-monitor-configuration-functions)
in the Windows API to send VCP codes to your monitors using DDC/CI commands. There are also
[_High-Level Monitor Configuration Functions_](https://learn.microsoft.com/en-us/windows/win32/monitor/using-the-high-level-monitor-configuration-functions)
but they work only if your monitor reports one of a few specific supported MCCS versions.

You can find the standard somewhere, or ask VESA kindly if you can have a copy, but the relevant part for
us is that code `0x10` sets the brightness, and code `0x12` sets the contrast.

The brightness is always set to the same percentage for all monitors, which may not mean they actually get
the same brightness, but for now no compensation for that was implemented yet. For the contrast setting
you can specify which level counts as ‘neutral’ (i.e. using the full panel brightness, but with no
clipped highlights).

## Monitor support

This works with all monitors I tested. The oldest one probably from around 2010.

So why does Windows normally only show a brightness slider for laptop screens? For one thing, the OEM knows
which screen is in that laptop and can provide some known working thing, somewhere. Probably the write cycle
thing doesn’t apply to laptop screens. Maybe this is broken for sufficiently many monitors that Microsoft
decided it is not worth the trouble ¹. Who knows.

## Write cycles

There is some speculation that monitors may store these settings in EEPROM. This may only be rated for
100,000 write cycles. That is a bit more than 50 per day, for 5 years. If you are the type of person who
tinkers around with these settings, you will easily achieve that rate. The same goes for some widget
that measures ambient light and updates the brightness.

I don’t know if this is an actual thing, but still, there is always some risk in doing uncommon things
like this.

-----------

¹ There is precedent for this. Windows 95 did not issue halt instructions to save CPU power, this was
because it would cause many laptops to lock up.