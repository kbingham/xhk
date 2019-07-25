[xHK] - An Xlib halfkeyboard implementation
===========================================

Getting started
---------------

To tryout xhk, run the following commands:

    git clone https://github.com/kbingham/xhk.git
    cd xhk
    sudo apt-get install build-essential autoconf automake pkg-config
    sudo apt-get install libx11-dev libxi-dev libxtst-dev
    ./autogen.sh
    ./configure
    make
    src/xhk -ddd  # -ddd executes with the highest debug level to see it working :)

In early 2014, I had an operation on my right elbow to remove some
bone fragments. These were remaining from an accident in my teenage
years - but had started to cause me some pain and grief. The operation
went well - but has a 6 month full recovery time, and for the first 6
weeks my right arm was pretty much immobile. A serious blocker to my
coding. There are it would seem some solutions to this. A few companies
sell physical keyboards which provide one-handed typing functionality -
but these are expensive. Regardless of the price, I have a keyboard
already attached to my Laptop - and plugging in an external keyboard
isn’t really feasible when sat on the sofa; So I need a better solution
- one which uses my existing keyboard. Now - again - there are a few
software implementations of half-keyboard / mirrors using different
methods, but not one for linux:

Alternative Software Solutions
------------------------------

-   [http://www.onehandkeyboard.org/] has both a Mac and Windows
    solution
-   <http://www.onehandkeyboard.org/linux-one-handed-keyboards/> does
    link to some other linux solutions - but they are either expired or
    not appropriate.
-   [http://warped.org/] has a version utilising AutoHotkeys for Windows
-   [http://blog.xkcd.com/] provides an XKB mapping - but can only use
    the caps key as a modifier - and doesn’t meet my needs!

My Solution
-----------

I write C code. I use linux, it seemed only reasonable that as a version
didn’t exist in this space I would create it and open-source it for all
to use (and improve) You can get the sources from [GitHub] and build it
yourself by following the instructions at my [xhk page][xHK] Once the
application is running - it is processing all your keypresses and
decides if it should mirror them. ‘Backspace’ is mirrored with the ‘Tab’
key, whilst ‘Enter’ is mirrored with ‘Caps-Lock’ If you use this - do
drop me a mail to let me know how you get on, and if you find any
problems I’ll look into them to fix them. Perhaps in the future I’ll
pull together a GTK frontend for it, but it depends on how much use it
gets.

  [xHK]: http://kieranbingham.co.uk/xhk/ "xhk"
  [http://www.onehandkeyboard.org/]: http://www.onehandkeyboard.org/download/
    "MacOS/Windows software"
  [http://warped.org/]: http://warped.org/blog/2008/10/06/the-free-one-handed-keyboard/
    "Autokeys implementation"
  [http://blog.xkcd.com/]: http://blog.xkcd.com/2007/08/14/mirrorboard-a-one-handed-keyboard-layout-for-the-lazy/
    "XKCD MirrorBoard"
  [GitHub]: https://github.com/kbingham/xhk "xhk @ GitHub"



This code is released under the GNU GPL v2 or later

www.kieranbingham.co.uk
