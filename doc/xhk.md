% XHK(1)
% Kieran Bingham
% January 2019

# NAME

xhk - HalfKey Xorg Driver Utility

# SYNOPSIS

**xhk** [**-m**] [**-i \<device_id\>**] [**-d** | **-dd** | **-ddd**] [**-h**]

# DESCRIPTION

**xHK** is a half keyboard mirroring implementation to help one-handed typist.

Running xhk detaches the keyboard, and intercepts all input; processing it for
space bar pushes as it goes. The space bar acts as a modifier if it is held at
the same time as a standard letter. Modifying acts to mirror the input such
that key presses are mirrored down the middle of the home row.
Pressing space, and then releasing will still provide a single space character.

The keyboard device should be guessed at launch, but if that fails, the **-i**
flag can be used to manually select the correct device.

# OPTIONS

**-d**
:   increase debug verbosity levels.

**-i N**
:   select keyboard device, **N** is the id number as listed on launch.

**-h**
:   display a friendly help message.

**-m**
:   mirror mode - all keys are changed to reversed keyboard layout.

**-t**
:   X11 related keycode test.

**-v**
:   show xHK version

