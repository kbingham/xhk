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

#ifndef XHK_LAYOUT_H_
#define XHK_LAYOUT_H_

enum EN_GB_LAYOUT {
    KEY_ESC = 9,
    KEY_1 = 10,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0,

    KEY_MINUS = 20,
    KEY_BACKSPACE = 22,
    KEY_TAB = 23,

    KEY_Q = 24,
    KEY_W,
    KEY_E,
    KEY_R,
    KEY_T,
    KEY_Y,
    KEY_U,
    KEY_I,
    KEY_O,
    KEY_P,

    KEY_ENTER = 36,
    KEY_CTRL  = 37,

    KEY_A = 38,
    KEY_S,
    KEY_D,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_SEMICOLON,
    KEY_AT,
    KEY_HASH,

    KEY_GRAVE = 49,

    KEY_LSHIFT = 50,
    KEY_BSLASH = 51,

    KEY_Z = 52,
    KEY_X,
    KEY_C,
    KEY_V,
    KEY_B,
    KEY_N,
    KEY_M,
    KEY_COMMA,
    KEY_STOP,
    KEY_FSLASH,
    KEY_RSHIFT,

    KEY_SPACE = 65,
    KEY_CAPS = 66,
};


#endif /* XHK_LAYOUT_H_ */
