/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Copyright (C) 2020 Stanislav Motylkov                                 *
 *                                                                         *
 ***************************************************************************/

#include "boot.h"
#include "memory_layout.h"
#include <shared.h>

extern volatile int nInteruptable;

unsigned int callback_key = 0;

#define CALLBACK_DEBUG         0
#define CALLBACK_START_SERVICE 1
#define CALLBACK_EXIT_SERVICE  2
#define CALLBACK_CALL_IRQ      3
#define CALLBACK_CHECK_KEY     4
#define CALLBACK_GET_KEY       5

/* USB scan code to PC/AT translation */
int usbToPc[256] = {
//  0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
	   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  // 0
	   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  // 1
	   0,    0,    0,    0,    0,    0,    0,    0, 0x0D, 0x1B, 0x08, 0x09, 0x20,    0,    0,    0,  // 2
	   0,    0,    0,    0,    0,    0,    0,    0,    0, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40,  // 3
	0x41, 0x42, 0x43, 0x44, 0x57, 0x58,    0,    0,    0,    0, 0x47,    0, 0x53, 0x4F,    0, 0x4D,  // 4
	0x4B, 0x50, 0x48,    0,    0,    0,    0, 0x4E,    0,    0,    0,    0,    0,    0,    0,    0,  // 5
	   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  // 6
	   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  // 7
	   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  // 8
	   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  // 9
	   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  // A
	   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  // B
	   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  // C
	   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  // D
	   0, 0x2A,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  // E
	   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,  // F
};

int __attribute__((fastcall)) CallbackFunction(int Reason, void *Param) {
	switch (Reason)
	{
		case CALLBACK_DEBUG:
			if (Param)
				bprintf("DEBUG: %s\r", Param);
			return Param ? 1 : 0;

		case CALLBACK_START_SERVICE:
			if (nInteruptable)
				return 0;
			nInteruptable = 1;
			BootStartUSB();
			return 1;

		case CALLBACK_EXIT_SERVICE:
			if (!nInteruptable)
				return 0;
			BootStopUSB();
			nInteruptable = 0;
			return 1;

		case CALLBACK_CALL_IRQ:
			if (!nInteruptable)
				return 0;
			switch ((int)Param)
			{
				case 0:
					IntHandlerCTimer0();
					return 1;
				case 1:
					IntHandler1C();
					return 1;
				case 2:
					IntHandler2C();
					return 1;
				case 3:
					IntHandler3VsyncC();
					return 1;
				case 4:
					IntHandler4C();
					return 1;
				case 5:
					IntHandler5C();
					return 1;
				case 6:
					IntHandler6C();
					return 1;
				case 7:
					IntHandler7C();
					return 1;
				case 8:
					IntHandler8C();
					return 1;
				case 9:
					IntHandler9C();
					return 1;
				case 10:
					IntHandler10C();
					return 1;
				case 11:
					IntHandlerCI2C();
					return 1;
				case 12:
					IntHandlerCSmc();
					return 1;
				case 13:
					IntHandler13C();
					return 1;
				case 14:
					IntHandlerCIde();
					return 1;
				case 15:
					IntHandler15C();
					return 1;
				default:
					break;
			}
			return 0;

		case CALLBACK_CHECK_KEY:
			callback_key = get_keyboard_key();

			if (!callback_key)
			{
				if (risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_A) == 1)
					callback_key = 0x28;
				else if (risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_B) == 1)
					callback_key = 0x29;
				else if (risefall_xpad_BUTTON(TRIGGER_XPAD_KEY_BACK) == 1)
					callback_key = 0x29;
				else if (risefall_xpad_BUTTON(TRIGGER_XPAD_PAD_UP) == 1)
					callback_key = 0x52;
				else if (risefall_xpad_BUTTON(TRIGGER_XPAD_PAD_DOWN) == 1)
					callback_key = 0x51;
				else if (risefall_xpad_BUTTON(TRIGGER_XPAD_PAD_LEFT) == 1)
					callback_key = 0x50;
				else if (risefall_xpad_BUTTON(TRIGGER_XPAD_PAD_RIGHT) == 1)
					callback_key = 0x4F;
			}

			if (!callback_key)
				return 0;

			/* Convert USB scan code to PC/AT */
			callback_key = usbToPc[callback_key & 0xFF];
			return (callback_key ? 1 : 0);

		case CALLBACK_GET_KEY:
			return (int)callback_key;
	}
	return 5;
}

char CalcParamChecksum(char *Data) {
	char Checksum = 0;
	int i;

	for (i = 0; i < sizeof(PARAMETER_BLOCK) - sizeof(int); i++, Data++)
		Checksum += *Data;
	return Checksum ^ 0x55;
}

void GetParameterBlock(PARAMETER_BLOCK *Param) {
	memcpy(Param, (void*)PARAM_BLOCK_OFFSET, sizeof(*Param));

	if ((Param->Signature != PARAM_BLOCK_SIGNATURE) ||
		(Param->Checksum != CalcParamChecksum(Param)))
	{
		Param->Signature = 0;
		Param->BiosCallback = NULL;
		Param->Checksum = 0;
		return;
	}
}

void SetParameterBlock(PARAMETER_BLOCK *Param) {
	Param->Signature = PARAM_BLOCK_SIGNATURE;
	Param->BiosCallback = (void *)CallbackFunction;
	Param->Checksum = CalcParamChecksum(Param);

	memcpy((void*)PARAM_BLOCK_OFFSET, Param, sizeof(*Param));
}
