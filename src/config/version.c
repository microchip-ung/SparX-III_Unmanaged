//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT


#include "version.h"
/* For version in mbox*/
#include "common.h"     /* Always include common.h at the first place of user-defined herder files */






#if !defined(LUTON26_L10) && !defined(LUTON26_L16)
#if defined(LUTON26_L25)
/* Don't exceed VERSION_LENGTH of 16.    12345678901234567890123*/
char code sw_version [VERSION_LENGTH] = "Luton25 3.11.0";
#else
/* Don't exceed VERSION_LENGTH of 16.    12345678901234567890123*/
char code sw_version [VERSION_LENGTH] = "Luton25UN 3.11.0";
#endif
#elif !defined(LUTON26_L10)
/* Don't exceed VERSION_LENGTH of 16.    12345678901234567890123*/
char code sw_version [VERSION_LENGTH] = "Luton16e 3.11.0";
#else
/* Don't exceed VERSION_LENGTH of 16.    12345678901234567890123*/
char code sw_version [VERSION_LENGTH] = "Luton10 3.11.0";
#endif /* !defined(LUTON_G16) && !defined(LUTON_G16R) */
char code compile_date [COMPILE_DATE_LENGTH] = __DATE__ " " __TIME__;

