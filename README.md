# GPU Panel

GPU monitoring app for Arch and Cachy OS made in C++!

It has:

Multi-GPU sidebar (Ctrl B, open on start, click your gpu to see it!)
Live stats: temps, core/memory clocks, load, VRAM, power, voltage, fan, throttle
History graphs for every reading since the app started!
Full graph view and Export PNG!
Per-GPU info dialog (name, ID, vendor, PCI slot, driver, details)
Overlay widgets (GPU stats): per GPU, choose which corner, optional CPU/RAM stats.
Auto-starts lactd if it's not running!

## Setup!

You need:
```
Qt 6 (Widgets)
CMake 3.16+
C++20 compiler (GCC or Clang)
LACT (lactd service)
X11 or Wayland (via XWayland)
```
...and a bit of luck (required for building!)


Build:
```
bash build.sh
```

Install (after build):
```
bash install.sh
```

## Using!

type in terminal:
```
gpupanel
```
or search app and open it!


Click any gpu on sidebar to view stats!

To see graphs scroll down!
Click Full graphs to see full version!

To add widget with gpu temps and other click button with 3 `-` and add widget!

Enjoy!
