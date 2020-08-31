/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "boot.h"
#include "video.h"
#include "xbox.h"
#include "VideoInitialization.h"
#include "BootEEPROM.h"

extern int PreferredVideoMode;
volatile CURRENT_VIDEO_MODE_DETAILS vmode;

void SetWidescreen(void *menuItemText) {
	char *text = (char *)menuItemText;
	if (!strcmp(text, "Display Size: Widescreen")) {
		strcpy(text, "Display Size: Normal");
		EepromSetWidescreen(0);	
	}
	else if (!strcmp(text, "Display Size: Normal")) {
		strcpy(text, "Display Size: Widescreen");
		EepromSetWidescreen(1);	
	}
}

void SetVideoStandard(void *menuItemText) {
	char *text = (char *)menuItemText;

	if (!strcmp(text, "TV Standard: PAL")) {
		strcpy(text, "TV Standard: NTSC-USA");
		EepromSetVideoStandard(NTSC_M);
	}
	else if (!strcmp(text, "TV Standard: NTSC-USA")) {
		strcpy(text, "TV Standard: NTSC-Japan");
		EepromSetVideoStandard(NTSC_J);
	}
	else if (!strcmp(text, "TV Standard: NTSC-Japan")) {
		strcpy(text, "TV Standard: PAL");
		EepromSetVideoStandard(PAL_I);
	}
	// We want people with broken EEPROMs to be able to recover!
	else if (!strcmp(text, "TV Standard: Unknown")) {
		strcpy(text, "TV Standard: PAL");
		EepromSetVideoStandard(PAL_I);
	}

}

void SetVideoMode(void *menuItemText) {
	char *text = (char *)menuItemText;

	if (!strcmp(text, "Set video mode 640x480")) {
		PreferredVideoMode = VIDEO_MODE_640x480;
	}
	else if (!strcmp(text, "Set video mode 640x576")) {
		PreferredVideoMode = VIDEO_MODE_640x576;
	}
	else if (!strcmp(text, "Set video mode 720x576")) {
		PreferredVideoMode = VIDEO_MODE_720x576;
	}
	else if (!strcmp(text, "Set video mode 800x600")) {
		PreferredVideoMode = VIDEO_MODE_800x600;
	}
	else if (!strcmp(text, "Set video mode 1024x576")) {
		PreferredVideoMode = VIDEO_MODE_1024x576;
	}
	else {
		PreferredVideoMode = VIDEO_MODE_UNKNOWN;
	}
	BootVgaInitializationKernelNG(&vmode);
}

