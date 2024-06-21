#include <Windows.h>
#include <stdio.h>
#include "RomTools_Common.h"

void CountryCodeToString (char string[], BYTE Country, int length) {
	switch (Country) {
	case '7': strncpy(string, "Beta", length); break;
	case 'A': strncpy(string, "NTSC", length); break;
	case 'B': strncpy(string, "Brazil", length); break;
	case 'C': strncpy(string, "China", length); break;
	case 'D': strncpy(string, "Germany", length); break;
	case 'E': strncpy(string, "USA", length); break;
	case 'F': strncpy(string, "France", length); break;
	case 'G': strncpy(string, "Gateway (NTSC)", length); break;
	case 'I': strncpy(string, "Italy", length); break;
	case 'J': strncpy(string, "Japan", length); break;
	case 'K': strncpy(string, "Korea", length); break;
	case 'L': strncpy(string, "Lodgenet (PAL)", length); break;
	case 'P': strncpy(string, "Europe", length); break;
	case 'S': strncpy(string, "Spain", length); break;
	case 'U': strncpy(string, "Australia", length); break;
	case 'W': strncpy(string, "Taiwan", length); break;
	case 'X': strncpy(string, "PAL", length); break;
	case 'Y': strncpy(string, "PAL", length); break;
	case ' ': strncpy(string, "None (PD by NAN)", length); break;
	case 0: strncpy(string, "None (PD)", length); break;
		default:
			if (length > 20)
				sprintf(string, "Unknown %c (%02X)", Country, Country);
			break;
	}
}

void CountryCodeToShortString(char string[], BYTE Country, int length) {
	switch (Country) {
	case '7': strncpy(string, "(Beta)", length); break;
	case 'A': strncpy(string, "(NTSC)", length); break;
	case 'B': strncpy(string, "(B)", length); break;
	case 'C': strncpy(string, "(C)", length); break;
	case 'D': strncpy(string, "(G)", length); break;
	case 'E': strncpy(string, "(U)", length); break;
	case 'F': strncpy(string, "(F)", length); break;
	case 'I': strncpy(string, "(I)", length); break;
	case 'J': strncpy(string, "(J)", length); break;
	case 'K': strncpy(string, "(K)", length); break;
	case 'P': strncpy(string, "(E)", length); break;
	case 'S': strncpy(string, "(S)", length); break;
	case 'U': strncpy(string, "(A)", length); break;
	case 'W': strncpy(string, "(TW)", length); break;
	case 'X': strncpy(string, "(PAL)", length); break;
	case 'Y': strncpy(string, "(PAL)", length); break;
	case ' ': strncpy(string, "(PD by NAN)", length); break;
	case 0: strncpy(string, "(PD)", length); break;
	default:
		if (length > 20)
			sprintf(string, "Unknown %c (%02X)", Country, Country);
		break;
	}
}

int RomRegion (BYTE Country) {
	switch(Country)
	{
		case 0x44: // Germany
		case 0x46: // French
		case 0x49: // Italian
		case 0x50: // Europe
		case 0x4C: // Lodgenet (PAL)
		case 0x53: // Spanish
		case 0x55: // Australia
		case 0x58: // X (PAL)
		case 0x59: // Y (PAL)
			return PAL_Region;
	
		case 0x37:	// 7 (Beta)
		case 0x41:	// NTSC (Only 1080 JU?)
		case 0x42:	// Brazil
		case 0x43:	// China
		case 0x4B:	// Korea
		case 0x57:	// Taiwan
		case 0x45:	// USA
		case 0x47:  // Gateway (NTSC)
		case 0x4A:	// Japan
		case 0x20:	// (PD)
		case 0x0:	// (PD)
			return NTSC_Region;

		default:
			return Unknown_Region;
	}
}