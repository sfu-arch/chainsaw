/*
 * simics/devs/cdrom.h
 *
 * This file is part of Virtutech Simics
 *
 * Copyright (C) 1998-2005 Virtutech AB, All Rights Reserved
 *
 * The Virtutech Simics API is distributed under license. Please refer to
 * the 'LICENSE' file for details.
 *
 * For documentation on the Virtutech Simics API, please refer to the
 * Simics Reference Manual. Please report any difficulties you encounter
 * with this API through the Virtutech Simics website at www.simics.com
 *
 */

#ifndef _SIMICS_DEVS_CDROM_H
#define _SIMICS_DEVS_CDROM_H

#define CDROM_MEDIA_INTERFACE "cdrom-media"

typedef struct {
        int (*read_toc)(conf_object_t *media, char *buf, int msf, int start_track);
        uint32 (*capacity)(conf_object_t *media);
        int (*read_block)(conf_object_t *media, char *buf, int lba);
        int (*read_raw_block)(conf_object_t *media, char *buf, int lba);
	int (*insert)(conf_object_t *media);
	void (*eject)(conf_object_t *media);	
} cdrom_media_interface_t;

// ADD INTERFACE cdrom_media_interface

#endif /* _SIMICS_DEVS_CDROM_H */
