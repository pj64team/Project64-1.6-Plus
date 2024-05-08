#ifndef ROMTOOLS_H
#define ROMTOOLS_H

#define Unknown_Region 0
#define NTSC_Region 1
#define PAL_Region 2

void CountryCodeToString (char string[], BYTE Country, int length);
int RomRegion (BYTE Country);

#endif