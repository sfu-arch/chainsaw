/*
  gfx-console.h

  Copyright (C) 2000-2003 Virtutech AB, All Rights Reserved

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

#ifndef _GFX_CONSOLE_H
#define _GFX_CONSOLE_H

#ifdef CONSOLE_OPENGL
#include <GL/gl.h>
#include <GL/glx.h>
#include "../graphics-console/glfuncs.h"

#define FUN_PTR_DEF(r, f, p) r (*f) p;

typedef struct {
	int version;
	int nr_functions;
	WITH_GL_FUNCS(FUN_PTR_DEF)
} gl_interface_t;

#endif

/* target formats supported by the gfx console */

#define GFX_8BIT_INDEXED 8
#define GFX_RGB_565      16
#define GFX_RGB_888      24
#define GFX_xRGB_8888    32

/* Graphical breakpoint support */

#define GBP_MAGIC       0xe0e0e0e0

#define GBP_FMT_VGA_4   1

#define GBP_FMT_V3_8    8
#define GBP_FMT_V3_16   16
#define GBP_FMT_V3_24   24
#define GBP_FMT_V3_32   32

typedef struct gfx_breakpoint {
        int     id, enabled, format;
        uint32  minx, miny, maxx, maxy;
        uint8   *data;
        struct gfx_breakpoint *next, *prev;
} gfx_breakpoint_t;

#define GBP_COMMENT_OFFS 0
#define GBP_MAGIC_OFFS   32
#define GBP_FORMAT_OFFS  36
#define GBP_BYTES_OFFS   40
#define GBP_MINX_OFFS    44
#define GBP_MINY_OFFS    48
#define GBP_MAXX_OFFS    52
#define GBP_MAXY_OFFS    56

#define GBP_HEADER_SIZE  (GBP_MAXY_OFFS + 4)

/*
typedef struct {
        char            comment[32];
        uint32          magic;
        uint32          format, bytes;
        int             minx, miny, maxx, maxy;
} gbp_header_t;
*/

#define GFX_CONSOLE_INTERFACE "gfx-console"

typedef struct gfx_console_interface {
        int (*set_color)(conf_object_t *, int, int, int, int);
        void (*set_size)(conf_object_t *, int, int, int, int, int);
        void (*set_cursor_bitmap)(conf_object_t *, int, int, char bitmap[64][64], uint32, uint32);
        void (*set_cursor_location)(conf_object_t *, int, int);
        void (*cursor_toggle)(conf_object_t *, int);

        void (*put_pixel)(conf_object_t *, int, int, int);
        void (*put_pixel_rgb)(conf_object_t *, int, int, uint32);
        void (*put_block)(conf_object_t *, uint8 *, int, int, int, int, int,
			  int, int);
        int (*select_rectangle)(conf_object_t *_con, uint32 *x1, uint32 *y1, uint32 *x2, uint32 *y2);

#ifdef CONSOLE_OPENGL
        gl_interface_t* (*gl_init)(void);
        int (*set_gl_buffer)(void *buffer, unsigned int stride, unsigned int w, unsigned int h);
        void (*update_gl_buffer)(void);
#endif
        void (*redraw)(conf_object_t *);
        void (*keyboard_info)(conf_object_t *, int);
} gfx_console_interface_t;

#endif /* _GFX_CONSOLE_H */
