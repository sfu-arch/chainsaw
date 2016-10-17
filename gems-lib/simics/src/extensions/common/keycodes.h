/*
  keycodes.h

  Copyright (C) 1999-2003 Virtutech AB, All Rights Reserved

  This program is a component ("Component") of Virtutech Simics and is
  being distributed under Section 1(a)(iv) of the Virtutech Simics
  Software License Agreement (the "Agreement").  You should have
  received a copy of the Agreement with this Component; if not, please
  write to Virtutech AB, Norrtullsgatan 15, 1tr, SE-113 27 STOCKHOLM,
  Sweden for a copy of the Agreement prior to using this Component.

  By using this Component, you agree to be bound by all of the terms of
  the Agreement.  If you do not agree to the terms of the Agreement, you
  may not use, copy or otherwise access the Component or any derivatives
  thereof.  You may create and use derivative works of this Component
  pursuant to the terms the Agreement provided that any such derivative
  works may only be used in conjunction with and as a part of Virtutech
  Simics for use by an authorized licensee of Virtutech.

  THIS COMPONENT AND ANY DERIVATIVES THEREOF ARE PROVIDED ON AN "AS IS"
  BASIS.  VIRTUTECH MAKES NO WARRANTIES WITH RESPECT TO THE COMPONENT OR
  ANY DERIVATIVES THEREOF AND DISCLAIMS ALL IMPLIED WARRANTIES,
  INCLUDING, WITHOUT LIMITATION, WARRANTIES OF MERCHANTABILITY AND
  FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.

*/

#ifndef _KEYCODES_H
#define _KEYCODES_H

#include <simics/devs/sim-keys.h>

#define MAX_X11_KEYS            0x100

#define MAX_SCAN_CODE_LENGTH 8

#if !defined(GULP)

int sim_key_to_scan(uint8 key, int key_up, char *buf,
                    int xt_style_conversion,
                    int shift_down, int ctrl_down,
                    int alt_down, int num_lock_on);
int sim_key_to_sun(uint8 key);
const char *sim_key_name(int keycode);
int x11_pc_key_to_sim(int keycode);
int x11_sun_key_to_sim(int keycode);
int x11_mac_key_to_sim(int keycode);
int x11_cooked_key_to_sim(int keycode);
int ascii_to_sim(int ascii);

#endif /* ! GULP */

#endif /* _KEYCODES_H */
