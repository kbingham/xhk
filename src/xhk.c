/*
    XHalfKey. An Xorg/XLib HalfKeyboard interpreter driver.

    Copyright (C) 2014  Kieran Bingham

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>

/* Go Real Time */
#include <sys/resource.h>

#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/XInput2.h>

#define XK_MISCELLANY
#define XK_XKB_KEYS
#include <X11/keysymdef.h>

#include <X11/XKBlib.h>

// KeyCode mappings to Layout nomenclatures
#include "xhk-layout.h"

#ifndef VERSION
#define VERSION "Undefined"
#endif

static int verbose = 1;

#define PRINT(level, x ...) 	if (verbose >= level) printf(x)
#define ERROR(...) 		PRINT(0, 	 ##__VA_ARGS__)
#define REPORT(...) 		PRINT(1, 	 ##__VA_ARGS__)
#define INFO(...) 		PRINT(2, 	 ##__VA_ARGS__)
#define DEBUG(...) 		PRINT(3, 	 ##__VA_ARGS__)
#define VERBOSE(level, ...) 	PRINT(level, ##__VA_ARGS__)

static bool ApplicationRunning = true;
static bool MirrorMode = false;

/* KeyStates == key_down / is_pressed */
#define KEYSTATE_DOWN 1
#define KEYSTATE_UP   0
static int keystates[255] = { 0 }; /* Initialised up */

static int ioErrorHandler(Display* d)
{
    printf("ERROR: Closing Down\n");
    ApplicationRunning = false;

    return 0;
}

typedef struct XWindowsScreen_s {
    Display* display;

    XIDeviceInfo keyboard;

    XKeyboardState   KBState;
    XKeyboardControl KBControl;
} XWindowsScreen_t;


static XWindowsScreen_t LocalScreen;


Display* OpenDisplay(const char* displayName)
{
    Display* display;
    // get the DISPLAY
    if (displayName == NULL) {
        displayName = getenv("DISPLAY");
        if (displayName == NULL) {
            displayName = ":0.0";
        }
    }

    // open the display
    INFO("XOpenDisplay(\"%s\")\n", displayName);
    display = XOpenDisplay(displayName);
    if (display == NULL) {
        return NULL;
    }

    // verify the availability of the XTest extension
    {
        int majorOpcode, firstEvent, firstError;
        if (!XQueryExtension(display, XTestExtensionName,
                             &majorOpcode, &firstEvent, &firstError)) {
            ERROR("XTEST extension not available");
            XCloseDisplay(display);
            return NULL;
        }
    }

    DEBUG("XOpenDisplay(\"%s\") open as %p\n", displayName, display);

    return display;
}


int
float_device(Display* dpy, int devid)
{
    XIDetachSlaveInfo c;
    int ret;

    c.type = XIDetachSlave;
    c.deviceid = devid;

    INFO("Floating device ID %d\n", devid);
    ret = XIChangeHierarchy(dpy, (XIAnyHierarchyChangeInfo*)&c, 1);
    return ret;
}

int
reattach_device(Display* dpy, int devid, int master)
{
    XIAttachSlaveInfo c;
    int ret;
    c.type = XIAttachSlave;
    c.deviceid = devid;
    c.new_master = master;

    INFO("ReAttaching device ID %d to master %d\n", devid, master);
    ret = XIChangeHierarchy(dpy, (XIAnyHierarchyChangeInfo*)&c, 1);
    return ret;
}


static int ConfigureKeyboard(XWindowsScreen_t * screen)
{
    int ret;
    int devid = screen->keyboard.deviceid;

    XGetKeyboardControl(screen->display, &screen->KBState);

    /* Select to receive all events from device N */

    XIEventMask eventmask;
    unsigned char mask[1] = { 0 }; /* the actual mask */

    eventmask.deviceid = devid;
    eventmask.mask_len = sizeof(mask); /* always in bytes */
    eventmask.mask = mask;
    /* now set the mask */
    XISetMask(mask, XI_KeyPress);
    XISetMask(mask, XI_KeyRelease);

    /* select on the window */
    ret = XISelectEvents(screen->display, DefaultRootWindow(screen->display), &eventmask, 1);

    DEBUG("XISelectEvents returned %d which could be %s\n", ret, (ret == 0 ? "Ok" : ret == BadValue ? "BadValue" : ret == BadWindow ? "BadWindow" : "Unknown"));

    /* Detach the keyboard so that no one else receives input from this Keyboard */
    float_device(screen->display, devid);

    return 0;
}

static XWindowsScreen_t * construct()
{
    LocalScreen = (XWindowsScreen_t) {
        0
    };

    // set the X I/O error handler so we catch the display disconnecting
    XSetIOErrorHandler(&ioErrorHandler);

    LocalScreen.display = OpenDisplay(NULL);
    if (LocalScreen.display == NULL)
        return NULL;

    return &LocalScreen;
}

int destruct(XWindowsScreen_t * screen)
{
    if (screen->display) {
        // Please Sir, can I have my Keyboard back?
        /* Attachment describes what the device was attached to before we caused it to float */
        reattach_device(screen->display, screen->keyboard.deviceid, screen->keyboard.attachment);

        XCloseDisplay(screen->display);
    }

    return 0;
}

char * keycode_to_char(XWindowsScreen_t * screen, int keycode)
{
    KeySym ks = XkbKeycodeToKeysym(screen->display, keycode, 0, 0);
    return XKeysymToString(ks);
}

static int SendKey(XWindowsScreen_t * screen, int keycode, int key_down, unsigned long time)
{
    DEBUG("Sending keycode %s %d, (%s) to XTest at %lu\n", key_down ? "Down" : "Up", keycode, keycode_to_char(screen, keycode), time);

    int ret = XTestFakeKeyEvent(screen->display, keycode, key_down ? True : False, CurrentTime);

    if (ret == 0)
        ERROR("XTestFakeKeyEvent failed to submit keycode %s %d at %lu\n", key_down ? "Down" : "Up", keycode, time);

    XFlush(screen->display);

    /* Record this action in our state table */
    keystates[keycode] = key_down;

    return ret;
}

int mirror_key(int keycode)
{
    /************************
    * Half-Key heavily based on code by John Meacham
    * john@foo.net
    * Adapted for X Keycodes, special cases and extra rows.
    ***********************/
    int t=0;

    if(keycode >= KEY_1 && keycode <= KEY_0) t = KEY_1; // numbers
    if(keycode >= KEY_Q && keycode <= KEY_P) t = KEY_Q; // top row
    if(keycode >= KEY_A && keycode <= KEY_SEMICOLON) t = KEY_A; // middle row
    if(keycode >= KEY_Z && keycode <= KEY_FSLASH) t = KEY_Z; // bottom row
    if(t) {
        int temp = keycode;
        temp -=t+4;
        if(temp < 1) temp--;
        temp = -temp;
        if(temp < 1) temp++;
        temp +=t+4;
        keycode = temp;
    }

    /* Swap special cases */
    switch(keycode) {
    case KEY_BACKSPACE:
        keycode = KEY_TAB;
        break;
    case KEY_TAB:
        keycode = KEY_BACKSPACE;
        break;

    case KEY_ENTER:
        keycode = KEY_CAPS;
        break;
    case KEY_CAPS:
        keycode = KEY_ENTER;
        break;

    case KEY_MINUS:
        keycode = KEY_GRAVE;
        break;
    case KEY_GRAVE:
        keycode = KEY_MINUS;
        break;
    }

    return keycode;
}

#define SPACE_STATE_START    0
#define SPACE_STATE_PRESSED  1
#define SPACE_STATE_MODIFIED 2

char * SpaceStateNames[] = {
    "Start",
    "Pressed",
    "Modified",
};

static int space = SPACE_STATE_START;

int ProcessKeycode(XWindowsScreen_t * screen, int keycode, int up_flag)
{
    int mirrored_key;
    bool mirrored;

    DEBUG("Entering ProcessKeycode in state %s\n", SpaceStateNames[space]);

    /* MirrorMode mirrors all keys before the state machine operates */
    if (MirrorMode)
        keycode = mirror_key(keycode);

    /*
     * SPACE State Table
     *
     * 			START		PRESSED		MODIFIED
     *
     * SpaceDown	Discarded	Discarded	Discarded
     * 			-> Pressed	-> Pressed	-> Modified
     *
     * SpaceUp		Invalid?	InjectSpace	UpAllModifiedDowns?
     * 			-> Start	-> Start	-> Start
     *
     * MirroredKey	InjectKey	MirrorKey	MirrorKey
     * 			-> Start	-> Modified	-> Modified
     *
     * OtherKey		InjectKey	InjectKey	InjectKey
     * 			-> Start	-> Pressed	-> Modified
     */

    if(keycode == KEY_SPACE) {
        switch(space) {
        case SPACE_STATE_START:
            space = SPACE_STATE_PRESSED;
            return -1; /* Change state but swallow the Space Input Event */
        case SPACE_STATE_PRESSED:
            if(up_flag) {
                /* Space released before any other key */
                /* We discarded the original Space Down event, so provide one now */
                SendKey(screen, keycode, True, CurrentTime);
                space = SPACE_STATE_START;
                return keycode; /* Space bar released, allow it to be pressed */
            } else
                return -1; /* Ignore and swallow repeated space down events */
            break;
        case SPACE_STATE_MODIFIED:
            if(up_flag)
                space = SPACE_STATE_START;
            return -1;
        }
    }

    /* Mirror the key once to prevent excess checking */
    mirrored_key = mirror_key(keycode);
    /* Determine if the key was modified by our mirror - Not all keys flip */
    mirrored = (mirrored_key != keycode);

    /* Only change state if this action would mirror a key */
    if( mirrored && (space != SPACE_STATE_START) ) {
        space = SPACE_STATE_MODIFIED; /* Space bar can no longer insert a space char */
        keycode = mirrored_key;
    }
    if(keycode == KEY_ESC && space != SPACE_STATE_START) {
        space = SPACE_STATE_MODIFIED;
        return -1;
    }
    /* Verify that we are only releasing keys that we pressed */
    if (up_flag && keystates[keycode] == KEYSTATE_UP) { /* Perhaps this was the wrong key */
        int mirror = mirror_key(keycode);
        if (keystates[mirror] == KEYSTATE_DOWN) {
            DEBUG("StateInversion: Releasing key %s instead of %s\n", keycode_to_char(screen, mirror), keycode_to_char(screen, keycode));
            keycode = mirror; /* We will 'up' this key instead */

            /* because of the inversion, we take the SPACE state back a level */
            if (space == SPACE_STATE_MODIFIED)
                space = SPACE_STATE_PRESSED;
        }
    }

    return keycode;
}

static int handle_key_release(XWindowsScreen_t * screen, XIDeviceEvent *event)
{
    int keycode = event->detail;
    keycode = ProcessKeycode(screen, keycode, 1);

    INFO("Keyrelease %d (%s), keycode = %d (%s) time=%ld\n", event->detail, keycode_to_char(screen, event->detail),
         keycode, keycode_to_char(screen, keycode),
         event->time);

    if (keycode < 0)
        return -1;

    SendKey(screen, keycode, False, event->time);

    INFO("\n");

    return 1;
}

static int handle_key_press(XWindowsScreen_t * screen, XIDeviceEvent *event)
{
    int keycode = event->detail;

    if ( (event->flags & XIKeyRepeat) && (keycode == KEY_SPACE) ) // Ignore SPACE key repeats
        return -1;

    keycode = ProcessKeycode(screen, keycode, 0);

    INFO("Keypress %d (%s), keycode = %d (%s) time=%ld\n",  event->detail, keycode_to_char(screen, event->detail),
         keycode, keycode_to_char(screen, keycode),
         event->time);

    if (keycode < 0)
        return -1;

    SendKey(screen, keycode, True, event->time);

    INFO("\n");

    return 1;
}


static int process_event(XWindowsScreen_t * screen)
{
    XEvent ev;
    Window current_window;
    int revert;

    XNextEvent(screen->display, &ev);

    XGetInputFocus(screen->display, &current_window, &revert);

    if (ev.xcookie.type == GenericEvent &&
//        ev.xcookie.extension == opcode &&
        XGetEventData(screen->display, &ev.xcookie)) {
        switch(ev.xcookie.evtype) {
        case XI_KeyPress:
            handle_key_press(screen, ev.xcookie.data);

            break;
        case XI_KeyRelease:
            handle_key_release(screen, ev.xcookie.data);
            break;
        default:
            INFO("Unhandled Event Received of type %d\n", ev.xcookie.type);
            break;
        }

        XFreeEventData(screen->display, &ev.xcookie);
    }

    return 0;
}

static int enumerate_keyboards(XWindowsScreen_t * screen)
{
    int ndevices;
    XIDeviceInfo *devices, *device;

    devices = XIQueryDevice(screen->display, XIAllDevices, &ndevices);

    for (int i = 0; i < ndevices; i++) {
        device = &devices[i];

        /* We're only interested in Keyboards */
        if (device->use != XISlaveKeyboard)
            continue;

        printf("Device %s (id: %d) is a ", device->name, device->deviceid);

        switch(device->use) {
        case XIMasterPointer:
            printf("master pointer\n");
            break;
        case XIMasterKeyboard:
            printf("master keyboard\n");
            break;
        case XISlavePointer:
            printf("slave pointer\n");
            break;
        case XISlaveKeyboard:
            printf("slave keyboard\n");
            break;
        case XIFloatingSlave:
            printf("floating slave\n");
            break;
        }

        printf("Device is attached to/paired with %d\n", device->attachment);
    }

    XIFreeDeviceInfo(devices);

    return 0;
}


static int identify_local_keyboard(XWindowsScreen_t * screen)
{
    int keyboard_id = -1;

    int ndevices;
    XIDeviceInfo *devices, *device;

    devices = XIQueryDevice(screen->display, XIAllDevices, &ndevices);

    for (int i = 0; i < ndevices; i++) {
        device = &devices[i];

        /* We're only interested in Keyboards */
        if (device->use != XISlaveKeyboard)
            continue;

        /* We're only interested in keyboards which are presented as keyboards too! */
        if (strcasestr(device->name, "keyboard") == 0 )
            continue;

        /* And I'm afraid we aren't going to deal with anything which isn't real... */
        if (strcasestr(device->name, "virtual") != 0 )
            continue;

        /* However, If we have come this far, we likely have a keyboard! */
        keyboard_id = device->deviceid;
        screen->keyboard = *device; /* Copy the device structure for destruction */
    }

    XIFreeDeviceInfo(devices);

    return keyboard_id;
}

int xlib_halfkey(void)
{
    XWindowsScreen_t * screen = construct();

    if (screen->display == NULL) {
        ERROR("Couldn't connect to XServer\n");
        return -1;
    }

    /* Which version of XI2? We support 2.0 */
    int major = 2, minor = 0;
    if (XIQueryVersion(screen->display, &major, &minor) == BadRequest) {
        printf("XI2 not available. Server supports %d.%d\n", major, minor);
        return -1;
    }

    INFO("XI Version %d.%d\n", major, minor);

    enumerate_keyboards(screen);

    if (identify_local_keyboard(screen) < 0) {
        destruct(screen);
        return -1;
    }

    ConfigureKeyboard(screen);

    fflush(stdout);

    DEBUG("Entering Event Loop...\n");

    // Loop until exit receiving and responding to events...
    while (ApplicationRunning)
        process_event(screen);

    destruct(screen);

    return 0;
}

#define BOLD "\033[1m"
#define NORMAL "\033[0m"

void version(void)
{
    printf( "xhk, version %s\n", VERSION);
    printf( "Copyright (C) 2014 Kieran Bingham.\n" );
    printf( "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
            "\n"
            "This is free software; you are free to change and redistribute it.\n"
            "There is NO WARRANTY, to the extent permitted by law.\n" );
}

void usage(void)
{
    printf("%s", BOLD);
    printf("\tusage:\n");
    printf("\t\t-m mirror mode - all keys reversed\n");
    printf("\t\t-d increase debug verbosity levels\n");
    printf("\t\t-h this help\n");
    printf("%s", NORMAL);
}

static void handle_signal(int signum)
{
    ERROR("Received Signal %d\n", signum);

    switch (signum) {
    case SIGTERM: /* 15 */
        ERROR("Really want me to die huh?\n");
        /* Fall Through */
    default:
        if (LocalScreen.display)
            reattach_device(LocalScreen.display, LocalScreen.keyboard.deviceid, LocalScreen.keyboard.attachment);

        ApplicationRunning = False;
        break;
    }
}

void install_signal_handlers(void)
{
    signal(SIGTERM, handle_signal);
    signal(SIGQUIT, handle_signal);
    signal(SIGINT, handle_signal);
}

int KeycodeTest(XWindowsScreen_t * screen, int keycode, int up_flag, int expected, int expected_state)
{
    int returned_code = ProcessKeycode(screen, keycode, up_flag);

    if (returned_code != expected)
        ERROR("ProcessKeyCode for %d (%s) returned %d {%s} but expected %d {%s}\n", keycode, up_flag ? "Up" : "Down",
              returned_code, keycode_to_char(screen, returned_code), expected, keycode_to_char(screen, expected));

    if (space != expected_state)
        ERROR("ProcessKeyCode for %d (%s %s) returned in state %d {%s} but expected state %d {%s}\n",
              keycode, keycode_to_char(screen, keycode), up_flag ? "Up" : "Down",
              space, SpaceStateNames[space], expected_state, SpaceStateNames[expected_state]);

    return (returned_code != expected);
}

#define UPFLAG_KEYDOWN 0
#define UPFLAG_KEYUP 1

int xlib_test(void)
{
    int errors = 0;

    XWindowsScreen_t * screen = construct();

    if (screen->display == NULL) {
        ERROR("Couldn't connect to XServer\n");
        return -1;
    }

    /* Which version of XI2? We support 2.0 */
    int major = 2, minor = 0;
    if (XIQueryVersion(screen->display, &major, &minor) == BadRequest) {
        printf("XI2 not available. Server supports %d.%d\n", major, minor);
        return -1;
    }

    INFO("XI Version %d.%d\n", major, minor);

    fflush(stdout);

    INFO("Entering Test Loop...\n");

    DEBUG("Check a key returns as expected\n");
    errors += KeycodeTest(screen, KEY_0, UPFLAG_KEYDOWN, KEY_0, SPACE_STATE_START);
    errors += KeycodeTest(screen, KEY_0, UPFLAG_KEYUP, KEY_0, SPACE_STATE_START);


    DEBUG("\nCheck space works alone\n");
    errors += KeycodeTest(screen, KEY_SPACE, UPFLAG_KEYDOWN, -1, SPACE_STATE_PRESSED);
    errors += KeycodeTest(screen, KEY_SPACE, UPFLAG_KEYUP, KEY_SPACE, SPACE_STATE_START);
    /* If we expect a space - it gets sent by the state machine, so we have to cancel it out */
    SendKey(screen, KEY_SPACE, KEYSTATE_UP, CurrentTime);

    DEBUG("\nVerify a key gets mirrored\n");
    errors += KeycodeTest(screen, KEY_SPACE, UPFLAG_KEYDOWN, -1, SPACE_STATE_PRESSED);
    errors += KeycodeTest(screen, KEY_F, UPFLAG_KEYDOWN, KEY_J, SPACE_STATE_MODIFIED);
    errors += KeycodeTest(screen, KEY_F, UPFLAG_KEYUP, KEY_J, SPACE_STATE_MODIFIED);
    errors += KeycodeTest(screen, KEY_SPACE, UPFLAG_KEYUP, -1, SPACE_STATE_START);
    /* If we expect a space - it gets sent by the state machine, so we have to cancel it out */
    SendKey(screen, KEY_SPACE, KEYSTATE_UP, CurrentTime);

    DEBUG("\nVerify a non-mirrored key doesn't break the space bar\n");
    errors += KeycodeTest(screen, KEY_SPACE, UPFLAG_KEYDOWN, -1, SPACE_STATE_PRESSED);
    errors += KeycodeTest(screen, KEY_LSHIFT, UPFLAG_KEYDOWN, KEY_LSHIFT, SPACE_STATE_PRESSED);
    errors += KeycodeTest(screen, KEY_LSHIFT, UPFLAG_KEYUP, KEY_LSHIFT, SPACE_STATE_PRESSED);
    errors += KeycodeTest(screen, KEY_SPACE, UPFLAG_KEYUP, KEY_SPACE, SPACE_STATE_START);

    DEBUG("\nVerify pressing paired mirror keys sequentially still works\n");
    errors += KeycodeTest(screen, KEY_F, UPFLAG_KEYDOWN, KEY_F, SPACE_STATE_START);
    errors += KeycodeTest(screen, KEY_J, UPFLAG_KEYDOWN, KEY_J, SPACE_STATE_START);
    errors += KeycodeTest(screen, KEY_F, UPFLAG_KEYUP, KEY_F, SPACE_STATE_START);
    errors += KeycodeTest(screen, KEY_J, UPFLAG_KEYUP, KEY_J, SPACE_STATE_START);
    /* Try the other way too */
    errors += KeycodeTest(screen, KEY_R, UPFLAG_KEYDOWN, KEY_R, SPACE_STATE_START);
    errors += KeycodeTest(screen, KEY_U, UPFLAG_KEYDOWN, KEY_U, SPACE_STATE_START);
    errors += KeycodeTest(screen, KEY_U, UPFLAG_KEYUP, KEY_U, SPACE_STATE_START);
    errors += KeycodeTest(screen, KEY_R, UPFLAG_KEYUP, KEY_R, SPACE_STATE_START);

    INFO("\nExiting Test Loop with %d errors...\n", errors);

    XCloseDisplay(screen->display);

    return 0;
}


int main(int argc, char **argv)
{
    char opt;

    REPORT("\n-- HalfKey Xorg Driver Utility %s --\n", VERSION);

    while((opt = getopt(argc, argv, "dvhmt")) != -1)
        switch(opt) {
        case 'd':
            verbose++;
            break;
        case 'v':
            version();
            exit(0);
        case 'h':			//print help page
            usage();
            exit(0);
        case 'm':
            MirrorMode = true;
            break;
        case 't':
            xlib_test();
            exit(0);
            break;
        default:
            usage();
            exit(1);
        }

    install_signal_handlers();

    int ret = setpriority(PRIO_PROCESS, getpid(), -20);
    if (ret)
        ERROR("SetPriority call failed : %d\n", ret);

    INFO("Process Priority set at %d\n", getpriority(PRIO_PROCESS, getpid()));

    xlib_halfkey();


    REPORT("\n-- Terminating --\n");

    return 0;
}

