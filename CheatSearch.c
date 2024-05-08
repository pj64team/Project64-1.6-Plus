/* Witten 20110303 ****************************************/
/**********************************************************/

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <winuser.h>
#include "main.h"
#include "CPU.h"
#include "debugger.h"
#include "resource_cheat.h"
#include "rom.h"
#include "cheats.h"
#include "cheatsearch.h"
#include "Memory.h"
#include "MemDump.h"

const char *N64GSCODETYPES8BIT[] = { "8-bit Constant Write (80)", "80",
							   	     "8-bit Uncached Write (A0)", "A0",
								     "8-bit GS Button (88)", "88",
								     "8-Bit Equal To Activator (D0)", "D0",
								     "8-Bit Different To Activator (D2)", "D2" };

const char *N64GSCODETYPES16BIT[] = { "16-bit Constant Write (81)", "81",
								      "16-bit Uncached Write (A1)", "A1",
								      "16-bit GS Button (89)", "89",
								      "16-Bit Equal To Activator (D1)", "D1",
								      "16-Bit Different To Activator (D3)", "D3" };

#define NUMBITS NUMBITSA
 typedef enum tagNUMBITS {
	bits8,
	bits16,
} NUMBITSA;
 NUMBITS searchNumBits;

#define SEARCHTYPE SEARCHTYPEA
typedef enum tagSEARCHTYPE {
	unknown,
	dec,
	hex,
	changed,
	unchanged,
	higher,
	lower
} SEARCHTYPEA;
SEARCHTYPE searchType;

#define CHEATDEVITEM CHEATDEVITEMA
typedef struct tagCHEATDEVITEM {
	char* Name;
	char* Note;
	int NumCodes;
	char* Code[];
} CHEATDEVITEMA;

#define LASTSEARCH LASTSEARCHA
typedef struct tagLASTSEARCH {
	char *SearchType;
	char *SearchValue;
	NUMBITS NumBits;
	SEARCHTYPE ValueSearchType;
	char *Results;
} LASTSEARCHA;

#define CODEENTRY CODEENTRYA
typedef struct tagCODEENTRY {
	char *Address;
	char *Value;
	char *Note;
	BYTE Enabled;
} CODEENTRYA;

#define CHTDEVENTRY CHTDEVENTRYA
typedef struct tagCHTDEVENTRY {
	char *Identifier;
	char *Name;
	LASTSEARCH *LastSearch;
	CODEENTRY *Codes;
} CHTDEVENTRYA;



CSRESULTITEM *csResults = NULL;
CSRESULTITEM *tmpResults = NULL;
long selectedResult = -1;
char passedCheatCode[256];
char passedCheatName[256];
long csNumResults = 0;
long tmpNumResults = 0;
long csAllocatedResults = 0;
long AddResultItemPreallocated(CSRESULTITEM item, long maxItems);

void Search (HWND hDlg);
void InitSearchFormatList(HWND hDlg);
void InitSearchNumBitsList(HWND hDlg);
void InitSearchResultList(HWND hDlg);
void PopulateSearchResultList(HWND hDlg);
void InitCheatCreateList(HWND hDlg);
void UpdateCheatCreateList(HWND hDlg);
void ChangeAddressRange(HWND hDlg, int index);
void SetResultsGroupBoxTitle(HWND hDlg, char *title);
void UpdateCheatCodePreview(HWND hDlg);
void ValidateHexInput(HWND hDlg, int ControlID);

CHTDEVENTRY *ReadProject64ChtDev();
void WriteProject64ChtDev(HWND hDlg);
char *ResultsToString(void);
void StringToResults(void);

BOOL CALLBACK CheatSearchDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK CheatSearch_Window_Proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK CheatSearch_Add_Proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK HexEditControlProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
WNDPROC OldHexEditControlProc;

static HWND CheatSearch_Win_hDlg;
static HWND hCheatSearchAddDialog;
static int InCheatSearchWindow = FALSE;
BOOL firstSearch = FALSE;
BOOL secondSearch = FALSE;
BOOL searched = FALSE;


DWORD EditControlValue = 0;


void Show_CheatSearchDlg (HWND hParent) {
    hCheatSearchDlg = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_Cheats_Search), NULL, CheatSearchDlgProc);
    if(hCheatSearchDlg != NULL)
    {
		Setup_CheatSearch_Window (hCheatSearchDlg);
        ShowWindow(hCheatSearchDlg, SW_SHOW);
    }
}

char *trimwhitespace(char *str) {
	char *end;

	// Trim leading space
	while(isspace(*str)) str++;
	if(*str == 0)  // All spaces? 
		return str;
	
	// Trim trailing space
	end = str + strlen(str) - 1;
	while(end > str && isspace(*end)) end--;
	
	// Write new null terminator 
	*(end+1) = 0; 

	return str;
}

BOOL CALLBACK CheatSearchDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPNMITEMACTIVATE lpnmitem;
	char s[256];
	char *trimmed;

	switch (uMsg) {
	case WM_CLOSE:
		free(csResults);
		DestroyWindow(hCheatSearchDlg);
		//hCheatSearchDlg = NULL;
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
			case NM_DBLCLK:
				switch (((LPNMHDR)lParam)->idFrom) {
					case IDL_SEARCH_RESULT_LIST:
						Edit_GetText(GetDlgItem(hDlg, IDT_CS_CHEAT_DESCRIPTION), s, 256);
						trimmed = trimwhitespace(s);
						if (strlen(s) != strlen(trimmed)) Edit_SetText(GetDlgItem(hDlg, IDT_CS_CHEAT_DESCRIPTION), trimmed);
						if (strlen(trimmed)>0) {
							lpnmitem = (LPNMITEMACTIVATE) lParam;
							selectedResult = lpnmitem->iItem;

							if(DialogBox(hInst, MAKEINTRESOURCE(IDD_Cheats_Search_Edit), hDlg, (DLGPROC)CheatSearch_Add_Proc) == IDOK) {
								UpdateCheatCreateList(hCheatSearchDlg);
							}
						}
						else {
							MessageBox(hDlg, "Please enter a name for the cheat first!", "Error", MB_OK);
						}
						break;
				}
				break;
        }
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case IDB_CS_SEARCH:
				Search(hDlg);
				WriteProject64ChtDev(hDlg);
				break;
			case IDB_CS_RESET:
				searched = FALSE;
				InitSearchFormatList(hDlg);
				ComboBox_Enable(GetDlgItem(hDlg, IDC_SEARCH_NUMBITS), 1);
				ComboBox_Enable(GetDlgItem(hDlg, IDC_ADDRESS_RANGE), 1);
				Edit_Enable(GetDlgItem(hDlg, IDT_SEARCH_VALUE), 1);
				free(csResults);
				csResults = NULL;
				csNumResults = 0;
				csAllocatedResults = 0;
				searched = FALSE;
				firstSearch = FALSE;
				secondSearch = FALSE;
				SendMessage(GetDlgItem(hDlg, IDT_SEARCH_VALUE), WM_SETTEXT, 0, (LPARAM)"");
				SendMessage(GetDlgItem(hDlg, IDL_SEARCH_RESULT_LIST), LVM_DELETEALLITEMS, 0, 0);
				SetResultsGroupBoxTitle(hDlg, "Results");
				break;
			case IDB_CS_DUMPMEM:
				Show_MemDumpDlg (hDlg);
				//SaveMemDump(hDlg);
				//DumpPCAndDisassembled(hDlg, 0x80000400, 0x80001000);
				break;
			case IDC_SEARCH_HEXDEC:
				switch (HIWORD(wParam)) {
					case CBN_SELCHANGE:
						if (!searched) {
							searchType = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
							switch (searchType) {
								case dec:
								case hex:
									Edit_Enable(GetDlgItem(hDlg, IDT_SEARCH_VALUE), 1);
									break;
								case unknown:
									Edit_Enable(GetDlgItem(hDlg, IDT_SEARCH_VALUE), 0);
									break;
							}
						}
						else {
							searchType = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
							switch (searchType) {
								case dec:
								case hex:
									Edit_Enable(GetDlgItem(hDlg, IDT_SEARCH_VALUE), 1);
									break;
								case unknown:
								case changed:
								case unchanged:
								case lower:
								case higher:
									Edit_Enable(GetDlgItem(hDlg, IDT_SEARCH_VALUE), 0);
									break;
							}
						}
						break;
				}
				break;
			case IDC_SEARCH_NUMBITS:
				switch (HIWORD(wParam)) {
					case CBN_SELCHANGE:
						searchNumBits = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
						break;
				}
				break;
			case IDL_SEARCH_RESULT_LIST:
				break;
			case IDCANCEL:
				EndDialog( hDlg, IDCANCEL );
				break;
			}
		default:
			return FALSE;
	}
	return TRUE;
}

void Setup_CheatSearch_Window (HWND hParent) {
	DWORD X, Y, dwStyle;
	DWORD WindowWidth = 683;
	DWORD WindowHeight = 341;

	CheckRadioButton(hCheatSearchDlg, IDR_SEARCH_VALUE, IDR_SEARCH_JAL, IDR_SEARCH_VALUE);

	if (RdramSize == 0x400000) {
		SendMessage(GetDlgItem(hCheatSearchDlg, IDS_RDRAMSIZE), WM_SETTEXT, 0, (LPARAM)(LPCTSTR) "RDRam size: 4MB");
		SendMessage(GetDlgItem(hCheatSearchDlg, IDT_ADDRESS_START), WM_SETTEXT, 0, (LPARAM)(LPCTSTR) "0x00000000");
		SendMessage(GetDlgItem(hCheatSearchDlg, IDT_ADDRESS_END), WM_SETTEXT, 0, (LPARAM)(LPCTSTR) "0x003FFFFF");
	}
	else {
		SendMessage(GetDlgItem(hCheatSearchDlg, IDS_RDRAMSIZE), WM_SETTEXT, 0, (LPARAM)(LPCTSTR) "RDRam size: 8MB");
		SendMessage(GetDlgItem(hCheatSearchDlg, IDT_ADDRESS_START), WM_SETTEXT, 0, (LPARAM)(LPCTSTR) "0x00000000");
		SendMessage(GetDlgItem(hCheatSearchDlg, IDT_ADDRESS_END), WM_SETTEXT, 0, (LPARAM)(LPCTSTR) "0x007FFFFF");
	}

	InitSearchFormatList(hCheatSearchDlg);

	InitSearchNumBitsList(hCheatSearchDlg);

	dwStyle = SendMessage(GetDlgItem(hCheatSearchDlg, IDL_SEARCH_RESULT_LIST), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
	dwStyle |= LVS_EX_FULLROWSELECT;
	SendMessage(GetDlgItem(hCheatSearchDlg, IDL_SEARCH_RESULT_LIST), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle);
	InitSearchResultList(hCheatSearchDlg);

	InitCheatCreateList(hCheatSearchDlg);

	X=0;
	Y=0;

	if (!GetStoredWinPos("CheatSearch", &X, &Y)) {
		X = (GetSystemMetrics(SM_CXSCREEN) - WindowWidth) / 2;
		Y = (GetSystemMetrics(SM_CYSCREEN) - WindowHeight) / 2;
	}
	SetWindowPos(hCheatSearchDlg, NULL, X, Y, WindowWidth, WindowHeight, SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOSIZE);			
}

void SetResultsGroupBoxTitle(HWND hDlg, char *title){
	SetWindowText(GetDlgItem(hDlg, IDGB_CS_RESULTS), title);
}

void InitSearchFormatList(HWND hDlg) {
	HWND hComboBox;
	int prevSelection;
	
	hComboBox=GetDlgItem(hDlg, IDC_SEARCH_HEXDEC);

	if (!searched) {
		SendMessage(hComboBox, CB_RESETCONTENT, 0, 0);
		SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM) "Unknown");
		SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM) "Decimal");
		SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM) "Hexadecimal");
		searchType = dec;
		SendMessage(hComboBox, CB_SETCURSEL , (WPARAM)searchType, 0);
	}
	else {
		prevSelection = SendMessage(hComboBox, CB_GETCURSEL, 0, 0);
		SendMessage(hComboBox, CB_RESETCONTENT, 0, 0);
		SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM) "Unknown");
		SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM) "Decimal");
		SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM) "Hexadecimal");
		SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM) "Changed");
		SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM) "Unchanged");
		SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM) "Higher");
		SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM) "lower");
		if (prevSelection == CB_ERR)
			searchType = unknown;
		SendMessage(hComboBox, CB_SETCURSEL , (WPARAM)searchType, 0);
	}
}

void InitSearchNumBitsList(HWND hDlg) {
	HWND hComboBox;
	
	hComboBox = GetDlgItem(hDlg, IDC_SEARCH_NUMBITS);
	SendMessage(hComboBox, CB_RESETCONTENT, 0, 0);
	SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)"8bit");
	SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)"16bit");

	searchNumBits = bits8;
	SendMessage(hComboBox, CB_SETCURSEL , (WPARAM)searchNumBits, 0);
}

void InitSearchResultList(HWND hDlg) {
	LV_COLUMN  col;
	HWND hListView;

	hListView = GetDlgItem(hDlg, IDL_SEARCH_RESULT_LIST);

	col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	col.fmt  = LVCFMT_LEFT;

	col.pszText  = "Address";
	col.cx       = 110;
	col.iSubItem = 0;
	ListView_InsertColumn (hListView, 0, &col);

	col.pszText  = "Value (Hex)";
	col.cx       = 70;
	col.iSubItem = 1;
	ListView_InsertColumn (hListView, 1, &col);

	col.pszText  = "Value (Dec)";
	col.cx       = 70;
	col.iSubItem = 2;
	ListView_InsertColumn (hListView, 2, &col);

	col.pszText  = "Old (Hex)";
	col.cx       = 70;
	col.iSubItem = 3;
	ListView_InsertColumn (hListView, 3, &col);
}

void PopulateSearchResultList(HWND hDlg) {
	HWND hListView;
	LV_ITEM item;
	int count;
	char s[9];
	char title[256];

	hListView = GetDlgItem(hDlg, IDL_SEARCH_RESULT_LIST);
	SendMessage(hListView, LVM_DELETEALLITEMS, 0, 0);

	for (count=0; count<csNumResults; count++) {
		item.mask      = LVIF_TEXT;
		item.iItem     = count;
		item.iSubItem  = 0;

		sprintf_s(s, 9, "%08X", csResults[count].Address);
		item.pszText = (LPSTR) &s;
		ListView_InsertItem(hListView,&item);

		if (searchNumBits == bits8)
			sprintf_s(s, 9, "%02X", csResults[count].Value);
		else
			sprintf_s(s, 9, "%04X", csResults[count].Value);
		item.pszText = (LPSTR) &s;
		item.iSubItem  = 1;
		ListView_SetItem(hListView,&item);

		sprintf_s(s, 9,"%u", csResults[count].Value);
		item.pszText = (LPSTR) &s;
		item.iSubItem  = 2;
		ListView_SetItem(hListView,&item);

		if (secondSearch) {
			if (searchNumBits == bits8)
				sprintf_s(s, 9, "%02X", csResults[count].OldValue);
			else
				sprintf_s(s, 9, "%04X", csResults[count].OldValue);
			item.pszText = (LPSTR) &s;
			item.iSubItem  = 3;
			ListView_SetItem(hListView,&item);
		}

		if (count > 0xFFE)
			break;
	}
	
	if (csNumResults > 0x1000)
		sprintf(title, "Results (found %u / displaying first 4096)", csNumResults);
	else
		sprintf(title, "Results (found %u)", csNumResults);
	SetResultsGroupBoxTitle(hDlg, title);
}

void InitCheatCreateList(HWND hDlg) {
	HWND hListView;
	LVCOLUMN col;

	hListView = GetDlgItem(hDlg, IDL_CS_CHEAT_CREATE);

	col.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH  | LVCF_ORDER;
	col.iOrder = 0;
	col.pszText = "Act.";
	col.fmt = LVCFMT_IMAGE;
	col.cx = 40;
	SendMessage(hListView, LVM_INSERTCOLUMN, 0, (LPARAM)&col);

	col.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH  | LVCF_ORDER;
	col.iOrder = 1;
	col.pszText = "Code";
	col.fmt = LVCFMT_LEFT;
	col.cx = 180;
	SendMessage(hListView, LVM_INSERTCOLUMN, 0, (LPARAM)&col);

	col.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH  | LVCF_ORDER;
	col.iOrder = 2;
	col.pszText = "Note";
	col.fmt = LVCFMT_LEFT;
	col.cx = 160;
	SendMessage(hListView, LVM_INSERTCOLUMN, 0, (LPARAM)&col);
}

void UpdateCheatCreateList(HWND hDlg) {
	HWND hListView;

	hListView = GetDlgItem(hDlg, IDL_CS_CHEAT_CREATE);
}

void Search(HWND hDlg) {
	BYTE *buffer;
	long bufferSize;
	MIPS_WORD word;
	char startAddress[11], endAddress[11];
	DWORD dwstartAddress;
	long state;
	long count, maxcount, numfound;
	DWORD bufferAddress;

	SendMessage(GetDlgItem(hDlg, IDT_ADDRESS_START), WM_GETTEXT, 11, (LPARAM)startAddress);
	SendMessage(GetDlgItem(hDlg, IDT_ADDRESS_END), WM_GETTEXT, 11, (LPARAM)endAddress);
	dwstartAddress = strtoul(startAddress, NULL, 16);
	bufferSize = strtoul(endAddress, NULL, 16)-strtoul(startAddress, NULL, 16);

	buffer = (BYTE *)malloc(bufferSize);
	for (count=0; count<bufferSize-4; count+=4) {
		r4300i_LW_PAddr(strtoul(startAddress, NULL, 16)+count, (DWORD *)&word);
		buffer[count]=word.UB[2];
		buffer[count+1]=word.UB[3];
		buffer[count+2]=word.UB[0];
		buffer[count+3]=word.UB[1];
	}
	//***********************************************

	if (!searched) {
		state = SendMessage(GetDlgItem(hDlg, IDR_SEARCH_VALUE), BM_GETCHECK, 0, 0);
		if (state == BST_CHECKED) {
			char searchString[9];
			DWORD searchValue;
			SendMessage(GetDlgItem(hDlg, IDT_SEARCH_VALUE), WM_GETTEXT, 9, (LPARAM)searchString);
			switch (searchType) {
				case dec:
				case hex:
					if (searchType == hex) {
						searchValue = strtoul(searchString, NULL, 16);
					}
					else {
						searchValue = strtoul(searchString, NULL, 10);
					}

					switch (searchNumBits) {
						case bits8:
							count = 0;
							maxcount = bufferSize-1;
							numfound = 0;
							while (count < maxcount) {
								if ((BYTE)searchValue == (BYTE)buffer[count]) {
									numfound++;
								}
								count++;
							}
							count = 0;
							while (count < maxcount) {
								if ((BYTE)searchValue == (BYTE)buffer[count]) {
									CSRESULTITEM item;
									
									item.Address = (DWORD)count + dwstartAddress + 1;
									item.Value = (DWORD)searchValue;
									item.OldValue = NULL;
									AddResultItemPreallocated(item, numfound);
								}
								count++;
							}
							break;
						case bits16:
							count=0;
							maxcount = bufferSize-2;
							numfound = 0;
							while (count < maxcount) {
								if ((WORD)searchValue == (WORD)((WORD)(buffer[count+1]<<8)+buffer[count])) {
									numfound++;
								}
								count += 2;
							}
							count = 0;
							while (count < maxcount) {
								if ((WORD)searchValue == (WORD)((WORD)(buffer[count+1]<<8)+buffer[count])) {
									CSRESULTITEM item;
									
									item.Address = (DWORD)count + dwstartAddress;
									item.Value = (DWORD)searchValue;
									item.OldValue = NULL;
									AddResultItemPreallocated(item, numfound);
								}
								count += 2;
							}
							break;
					}
					break;
				case unknown:
					switch (searchNumBits) {
						case bits8:
							count=0;
							maxcount = bufferSize-1;
							while (count < maxcount) {
								CSRESULTITEM item;
								
								item.Address = (DWORD)count + dwstartAddress + 1;
								item.Value = (DWORD)(BYTE)buffer[count];
								item.OldValue = NULL;
								AddResultItemPreallocated(item, bufferSize);

								count++;
							}
							break;
						case bits16:
							count=0;
							maxcount = bufferSize-2;
							while (count < maxcount) {
								CSRESULTITEM item;
								
								item.Address = (DWORD)count + dwstartAddress;
								item.Value = (DWORD)(WORD)((WORD)(buffer[count+1]<<8)+buffer[count]);
								item.OldValue = NULL;
								AddResultItemPreallocated(item, bufferSize-1);

								count += 2;
							}
							break;
					}
					break;
			}
			searched = TRUE;
			ComboBox_Enable(GetDlgItem(hDlg, IDC_SEARCH_NUMBITS), 0);
			ComboBox_Enable(GetDlgItem(hDlg, IDC_ADDRESS_RANGE), 0);
		}
		state = SendMessage(GetDlgItem(hDlg, IDR_SEARCH_TEXT), BM_GETCHECK,  0, 0);
		if (state == BST_CHECKED) {
			searched = TRUE;
		}
		state = SendMessage(GetDlgItem(hDlg, IDR_SEARCH_JAL), BM_GETCHECK, 0, 0);
		if (state == BST_CHECKED) {
			searched = TRUE;
		}
	}
	else {
		tmpResults = csResults;
		tmpNumResults = csNumResults;
		csResults  = NULL;
		csNumResults = 0;
		csAllocatedResults = 0;

		state = SendMessage(GetDlgItem(hDlg, IDR_SEARCH_VALUE), BM_GETCHECK, 0, 0);
		if (state == BST_CHECKED) {
			char searchString[9];
			DWORD searchValue;
			SendMessage(GetDlgItem(hDlg, IDT_SEARCH_VALUE), WM_GETTEXT, 9, (LPARAM)searchString);
			
			dwstartAddress = strtoul(startAddress, NULL, 16);

			switch (searchNumBits) {
				case bits8:
					count = 0;
					maxcount = tmpNumResults;

					switch (searchType) {
						case dec:
						case hex:
							if (searchType == dec)
								searchValue = strtoul(searchString, NULL, 10);
							else
								searchValue = strtoul(searchString, NULL, 16);

							numfound = 0;
							while (count < maxcount) {
								bufferAddress = tmpResults[count].Address+dwstartAddress-1;
								if ((BYTE)searchValue == (BYTE)buffer[bufferAddress]) {
									numfound++;
								}
								count++;
							}
							count = 0;
							while (count < maxcount) {
								bufferAddress = tmpResults[count].Address+dwstartAddress-1;
								if ((BYTE)searchValue == (BYTE)buffer[bufferAddress]) {
									CSRESULTITEM item;
									
									item.Address = tmpResults[count].Address;
									item.Value = (DWORD)(BYTE)searchValue;
									item.OldValue = tmpResults[count].Value;
									AddResultItemPreallocated(item, numfound);
								}
								count++;
							}
							break;
						case unknown:
							count = 0;
							while (count < maxcount) {
								CSRESULTITEM item;
								bufferAddress = tmpResults[count].Address+dwstartAddress-1;
								item.Address = tmpResults[count].Address;
								item.Value = (DWORD)(BYTE)buffer[bufferAddress];
								item.OldValue = tmpResults[count].Value;
								AddResultItemPreallocated(item, maxcount);
								count++;
							}
							break;
						case changed:
							numfound = 0;
							while (count < maxcount) {
								bufferAddress = tmpResults[count].Address+dwstartAddress-1;
								if ((BYTE)tmpResults[count].Value != (BYTE)buffer[bufferAddress]) {
									numfound++;
								}
								count++;
							}
							count = 0;
							while (count < maxcount) {
								bufferAddress = tmpResults[count].Address+dwstartAddress-1;
								if ((BYTE)tmpResults[count].Value != (BYTE)buffer[bufferAddress]) {
									CSRESULTITEM item;
									
									item.Address = tmpResults[count].Address;
									item.Value = (DWORD)(BYTE)buffer[bufferAddress];
									item.OldValue = tmpResults[count].Value;
									AddResultItemPreallocated(item, numfound);
								}
								count++;
							}
							break;
						case unchanged:
							numfound = 0;
							while (count < maxcount) {
								bufferAddress = tmpResults[count].Address+dwstartAddress-1;
								if ((BYTE)tmpResults[count].Value == (BYTE)buffer[bufferAddress]) {
									numfound++;
								}
								count++;
							}
							count = 0;
							while (count < maxcount) {
								bufferAddress = tmpResults[count].Address+dwstartAddress-1;
								if ((BYTE)tmpResults[count].Value == (BYTE)buffer[bufferAddress]) {
									CSRESULTITEM item;
									
									item.Address = tmpResults[count].Address;
									item.Value = (DWORD)(BYTE)buffer[bufferAddress];
									item.OldValue = tmpResults[count].Value;
									AddResultItemPreallocated(item, numfound);
								}
								count++;
							}
							break;
						case lower:
							numfound = 0;
							while (count < maxcount) {
								bufferAddress = tmpResults[count].Address+dwstartAddress-1;
								if ((BYTE)tmpResults[count].Value > (BYTE)buffer[bufferAddress]) {
									numfound++;
								}
								count++;
							}
							count = 0;
							while (count < maxcount) {
								bufferAddress = tmpResults[count].Address+dwstartAddress-1;
								if ((BYTE)tmpResults[count].Value > (BYTE)buffer[bufferAddress]) {
									CSRESULTITEM item;
									
									item.Address = tmpResults[count].Address;
									item.Value = (DWORD)(BYTE)buffer[bufferAddress];
									item.OldValue = tmpResults[count].Value;
									AddResultItemPreallocated(item, numfound);
								}
								count++;
							}
							break;
						case higher:
							numfound = 0;
							while (count < maxcount) {
								bufferAddress = tmpResults[count].Address+dwstartAddress-1;
								if ((BYTE)tmpResults[count].Value < (BYTE)buffer[bufferAddress]) {
									numfound++;
								}
								count++;
							}
							count = 0;
							while (count < maxcount) {
								bufferAddress = tmpResults[count].Address+dwstartAddress-1;
								if ((BYTE)tmpResults[count].Value < (BYTE)buffer[bufferAddress]) {
									CSRESULTITEM item;
									
									item.Address = tmpResults[count].Address;
									item.Value = (DWORD)(BYTE)buffer[bufferAddress];
									item.OldValue = tmpResults[count].Value;
									AddResultItemPreallocated(item, numfound);
								}
								count++;
							}
							break;
					}
					break;
				case bits16:
					maxcount = tmpNumResults;

					switch (searchType) {
						case dec:
						case hex:
							if (searchType == dec)
								searchValue = strtoul(searchString, NULL, 10);
							else
								searchValue = strtoul(searchString, NULL, 16);

							numfound = 0;
							count=0;
							while (count < maxcount) {
								bufferAddress = tmpResults[count].Address+dwstartAddress;
								if ((WORD)searchValue == (WORD)((WORD)(buffer[bufferAddress+1]<<8)+buffer[bufferAddress])) {
									numfound++;
								}
								count++;
							}
							count = 0;
							while (count < maxcount) {
								bufferAddress = tmpResults[count].Address+dwstartAddress;
								if ((WORD)searchValue == (WORD)((WORD)(buffer[bufferAddress+1]<<8)+buffer[bufferAddress])) {
									CSRESULTITEM item;
									
									item.Address = tmpResults[count].Address;
									item.Value = (DWORD)(WORD)searchValue;
									item.OldValue = tmpResults[count].Value;
									AddResultItemPreallocated(item, numfound);
								}
								count++;
							}
							break;
						case unknown:
							count = 0;
							while (count < maxcount) {
								CSRESULTITEM item;
								bufferAddress = tmpResults[count].Address+dwstartAddress;
								item.Address = tmpResults[count].Address;
								item.Value = (DWORD)(WORD)((WORD)(buffer[bufferAddress+1]<<8)+buffer[bufferAddress]);
								item.OldValue = tmpResults[count].Value;
								AddResultItemPreallocated(item, maxcount);
								count++;
							}
							break;
						case changed:
							numfound = 0;
							count=0;
							while (count < maxcount) {
								bufferAddress = tmpResults[count].Address+dwstartAddress;
								if ((WORD)tmpResults[count].Value != (WORD)((WORD)(buffer[bufferAddress+1]<<8)+buffer[bufferAddress])) {
									numfound++;
								}
								count++;
							}
							count = 0;
							while (count < maxcount) {
								bufferAddress = tmpResults[count].Address+dwstartAddress;
								if ((WORD)tmpResults[count].Value != (WORD)((WORD)(buffer[bufferAddress+1]<<8)+buffer[bufferAddress])) {
									CSRESULTITEM item;
									
									item.Address = tmpResults[count].Address;
									item.Value = (DWORD)(WORD)((WORD)(buffer[bufferAddress+1]<<8)+buffer[bufferAddress]);
									item.OldValue = tmpResults[count].Value;
									AddResultItemPreallocated(item, numfound);
								}
								count++;
							}
							break;
						case unchanged:
							numfound = 0;
							count=0;
							while (count < maxcount) {
								bufferAddress = tmpResults[count].Address+dwstartAddress;
								if ((WORD)tmpResults[count].Value == (WORD)((WORD)(buffer[bufferAddress+1]<<8)+buffer[bufferAddress])) {
									numfound++;
								}
								count++;
							}
							count = 0;
							while (count < maxcount) {
								bufferAddress = tmpResults[count].Address+dwstartAddress;
								if ((WORD)tmpResults[count].Value == (WORD)((WORD)(buffer[bufferAddress+1]<<8)+buffer[bufferAddress])) {
									CSRESULTITEM item;
									
									item.Address = tmpResults[count].Address;
									item.Value = (DWORD)(WORD)((WORD)(buffer[bufferAddress+1]<<8)+buffer[bufferAddress]);
									item.OldValue = tmpResults[count].Value;
									AddResultItemPreallocated(item, numfound);
								}
								count++;
							}
							break;
						case lower:
							numfound = 0;
							count=0;
							while (count < maxcount) {
								bufferAddress = tmpResults[count].Address+dwstartAddress;
								if ((WORD)tmpResults[count].Value > (WORD)((WORD)(buffer[bufferAddress+1]<<8)+buffer[bufferAddress])) {
									numfound++;
								}
								count++;
							}
							count = 0;
							while (count < maxcount) {
								bufferAddress = tmpResults[count].Address+dwstartAddress;
								if ((WORD)tmpResults[count].Value > (WORD)((WORD)(buffer[bufferAddress+1]<<8)+buffer[bufferAddress])) {
									CSRESULTITEM item;
									
									item.Address = tmpResults[count].Address;
									item.Value = (DWORD)(WORD)((WORD)(buffer[bufferAddress+1]<<8)+buffer[bufferAddress]);
									item.OldValue = tmpResults[count].Value;
									AddResultItemPreallocated(item, numfound);
								}
								count++;
							}
							break;
						case higher:
							numfound = 0;
							count=0;
							while (count < maxcount) {
								bufferAddress = tmpResults[count].Address+dwstartAddress;
								if ((WORD)tmpResults[count].Value < (WORD)((WORD)(buffer[bufferAddress+1]<<8)+buffer[bufferAddress])) {
									numfound++;
								}
								count++;
							}
							count = 0;
							while (count < maxcount) {
								bufferAddress = tmpResults[count].Address+dwstartAddress;
								if ((WORD)tmpResults[count].Value < (WORD)((WORD)(buffer[bufferAddress+1]<<8)+buffer[bufferAddress])) {
									CSRESULTITEM item;
									
									item.Address = tmpResults[count].Address;
									item.Value = (DWORD)(WORD)((WORD)(buffer[bufferAddress+1]<<8)+buffer[bufferAddress]);
									item.OldValue = tmpResults[count].Value;
									AddResultItemPreallocated(item, numfound);
								}
								count++;
							}
							break;
					}
					break;
			}
			secondSearch = TRUE;
		}
		state = SendMessage(GetDlgItem(hDlg, IDR_SEARCH_TEXT), BM_GETCHECK,  0, 0);
		if (state == BST_CHECKED) {
			secondSearch = TRUE;
		}
		state = SendMessage(GetDlgItem(hDlg, IDR_SEARCH_JAL), BM_GETCHECK, 0, 0);
		if (state == BST_CHECKED) {
			secondSearch = TRUE;
		}
		free(tmpResults);
	}

	free(buffer);

	PopulateSearchResultList(hDlg);
	firstSearch = TRUE;
	InitSearchFormatList(hDlg);
}

long AddResultItemPreallocated(CSRESULTITEM item, long maxItems) {
	CSRESULTITEM *_tmp;

	if (csAllocatedResults != maxItems) {
		csAllocatedResults = maxItems;
		_tmp = (CSRESULTITEM*)realloc(csResults, (csAllocatedResults * sizeof(CSRESULTITEM)));
		if (!_tmp) {
                //fprintf(stderr, "ERROR: Couldn't realloc memory!\n");
                return(-1);
        }
		if (csResults != NULL) {
			free(csResults);
		}
        csResults = (CSRESULTITEM*)_tmp;
	}

    csResults[csNumResults] = item;
    csNumResults++;

    return csNumResults;
}

LRESULT CALLBACK CheatSearch_Add_Proc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	HWND hComboBox, hHexEdit1;
	int Count;
	char strAddress[9];
	char strValue[5];
	char code[14];
	char note[238];

	switch (uMsg) {
	case WM_INITDIALOG:
		// Populate ComboBox with GS Cheat Types
		hComboBox = GetDlgItem(hDlg, IDC_CSE_GSCODETYPE);
		SendMessage(hComboBox, CB_RESETCONTENT, 0, 0);
		switch (searchNumBits) {
		case bits8:
			for (Count = 0; Count < 5; Count++) {
				SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) N64GSCODETYPES8BIT[Count*2]);
			}
			break;
		case bits16:
			for (Count = 0; Count < 5; Count++) {
				SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) N64GSCODETYPES16BIT[Count*2]);
			}
			break;
		}
		SendMessage(hComboBox, CB_SETCURSEL , (WPARAM)0, 0);

		sprintf((char *)&strAddress, "%06X", csResults[selectedResult].Address);
		SendMessage(GetDlgItem(hDlg, IDT_CSE_ADDRESS), WM_SETTEXT, 0, (LPARAM)strAddress);
		EditControlValue = csResults[selectedResult].Value;
		sprintf((char *)&strValue, "%X", EditControlValue);
		SendMessage(GetDlgItem(hDlg, IDT_CSE_VALUE_HEX), WM_SETTEXT, 0, (LPARAM)strValue);
		sprintf((char *)&strValue, "%u", EditControlValue);
		SendMessage(GetDlgItem(hDlg, IDT_CSE_VALUE_DEC), WM_SETTEXT, 0, (LPARAM)strValue);

		UpdateCheatCodePreview(hDlg);

		// Subclass HexEditControls
		hHexEdit1 = GetDlgItem(hDlg, IDT_CSE_VALUE_HEX);
		OldHexEditControlProc =  (WNDPROC)SetWindowLongPtr (hHexEdit1, GWLP_WNDPROC, (LONG_PTR)HexEditControlProc);

		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
        case IDOK: 
			Edit_GetText(GetDlgItem(hDlg, IDT_CSE_PREVIEW), code, 14);
			Edit_GetText(GetDlgItem(hDlg, IDT_CSE_NOTE), note, 238);
			sprintf_s(passedCheatCode, 256, "%s;%s;0", code, note);
				
			// Pass through

        case IDCANCEL: 
            EndDialog(hDlg, wParam); 
            return TRUE;

		case IDC_CSE_GSCODETYPE:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				UpdateCheatCodePreview(hDlg);
			}
			break;

		case IDT_CSE_VALUE_HEX:
			if (HIWORD(wParam) == EN_CHANGE) {
				DWORD value;
				Edit_GetText((HWND)lParam, strValue, 5);
				value = AsciiToHex(strValue);
				if (value != EditControlValue) {
					DWORD maxValue;
					if (searchNumBits == bits8) {maxValue = 0xFF;} else {maxValue = 0xFFFF;}
					if (value > maxValue) {
						sprintf_s(strValue, 5, "%X", EditControlValue);
						Edit_SetText((HWND)lParam, strValue);
						Edit_SetSel((HWND)lParam, strlen(strValue), strlen(strValue));
					}
					else {
						EditControlValue = value;
						sprintf_s(strValue, 5, "%u", value);
						Edit_SetText(GetDlgItem(hDlg, IDT_CSE_VALUE_DEC), strValue);
						UpdateCheatCodePreview(hDlg);
					}
				}
			}
			break;

		case IDT_CSE_VALUE_DEC:
			if (HIWORD(wParam) == EN_CHANGE) {
				DWORD value;
				Edit_GetText((HWND)lParam, strValue, 5);
				value = atoi(strValue);
				if (value != EditControlValue) {
					DWORD maxValue;
					if (searchNumBits == bits8) {maxValue = 0xFF;} else {maxValue = 0xFFFF;}
					if (value > maxValue) {
						sprintf_s(strValue, 5, "%u", EditControlValue);
						Edit_SetText((HWND)lParam, strValue);
						Edit_SetSel((HWND)lParam, strlen(strValue), strlen(strValue));
					}
					else {
						EditControlValue = value;
						sprintf_s(strValue, 5, "%X", value);
						Edit_SetText(GetDlgItem(hDlg, IDT_CSE_VALUE_HEX), strValue);
						UpdateCheatCodePreview(hDlg);
					}
				}
			}
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void UpdateCheatCodePreview(HWND hDlg) {
	int ndxCodeType;
	char address[7];
	char value[5];
	char s[256];
	char *pEnd;

	ndxCodeType = SendMessage(GetDlgItem(hDlg, IDC_CSE_GSCODETYPE), CB_GETCURSEL, 0, 0);
	SendMessage(GetDlgItem(hDlg, IDT_CSE_ADDRESS), WM_GETTEXT, 7, (LPARAM)address);
	SendMessage(GetDlgItem(hDlg, IDT_CSE_VALUE_HEX), WM_GETTEXT, 5, (LPARAM)value);

	switch (searchNumBits) {
	case bits8:
		sprintf((char *)&s,
			    "%X %04X",
				(DWORD)strtoul(N64GSCODETYPES8BIT[ndxCodeType*2+1], &pEnd, 16)<<24 ^ ((DWORD)strtoul(address, &pEnd, 16) & 0x00FFFFFF),
				(DWORD)strtoul(value, &pEnd, 16));
		break;
	case bits16:
		sprintf((char *)&s,
			    "%X %04X",
				(DWORD)strtoul(N64GSCODETYPES16BIT[ndxCodeType*2+1], &pEnd, 16)<<24 ^ ((DWORD)strtoul(address, &pEnd, 16) & 0x00FFFFFF),
				(DWORD)strtoul(value, &pEnd, 16));
		break;
	}
	SendMessage(GetDlgItem(hDlg, IDT_CSE_PREVIEW), WM_SETTEXT, 0, (LPARAM)s);
}

//void ValidateHexInput(HWND hDlg, int ControlID) {
//	char s[255];
//	char tmp[255];
//	int count = 0;
//	int tmpcount = 0;
//	SendMessage(GetDlgItem(hDlg, ControlID), WM_GETTEXT, 255, (LPARAM)s);
//
//	while (s[count] != '\0') {
//		switch (s[count]) {
//			case '0':
//			case '1':
//			case '2':
//			case '3':
//			case '4':
//			case '5':
//			case '6':
//			case '7':
//			case '8':
//			case '9':
//			case 'A':
//			case 'B':
//			case 'C':
//			case 'D':
//			case 'E':
//			case 'F':
//				tmp[tmpcount] = s[count];
//				tmpcount++;
//				break;
//		}
//		count++;
//	}
//	tmp[tmpcount] = '\0';
//	if (strcmp(s, tmp) != 0)
//		SendMessage(GetDlgItem(hDlg, ControlID), WM_SETTEXT, 0, (LPARAM)tmp);
//}

LRESULT CALLBACK HexEditControlProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
	case WM_CHAR:
				switch (wParam) {
			case 0x08: // VK_BACKSPACE
			case 0x30: //0
			case 0x31: //1
			case 0x32: //2
			case 0x33: //3
			case 0x34: //4
			case 0x35: //5
			case 0x36: //6
			case 0x37: //7
			case 0x38: //8
			case 0x39: //9
			case 0x41: //A
			case 0x42: //B
			case 0x43: //C
			case 0x44: //D
			case 0x45: //E
			case 0x46: //F
			case 0x61: //a
			case 0x62: //b
			case 0x63: //c
			case 0x64: //d
			case 0x65: //e
			case 0x66: //f
				return CallWindowProc(OldHexEditControlProc, hWnd, uMsg, wParam, lParam);
				break;
		}

		return 0;
    } 
    return CallWindowProc(OldHexEditControlProc, hWnd, uMsg, wParam, lParam);
}

/********************************************************************************************
  GetCheatDevIniFileName

  Purpose: 
  Parameters:
  Returns:

********************************************************************************************/
char * GetCheatDevIniFileName(void) {
	char path_buffer[_MAX_PATH], drive[_MAX_DRIVE] ,dir[_MAX_DIR];
	char fname[_MAX_FNAME],ext[_MAX_EXT];
	static char IniFileName[_MAX_PATH];

	GetModuleFileName(NULL,path_buffer,sizeof(path_buffer));
	_splitpath( path_buffer, drive, dir, fname, ext );
	sprintf(IniFileName,"%s%s%s",drive,dir,CheatDevIniName);
	return IniFileName;
}

CHTDEVENTRY *ReadProject64ChtDev() {
	FILE * pFile;
	long lSize;
	char * buffer;
	size_t result;
	//CHTDEVENTRY * ChtDev;

	pFile = fopen (GetCheatDevIniFileName(), "rb");
	if (pFile == NULL) {
		// file error
		return NULL;
	}

	// obtain file size:
	fseek (pFile, 0, SEEK_END);
	lSize = ftell (pFile);
	rewind (pFile);

	// allocate memory to contain the whole file:
	buffer = (char*) malloc (sizeof(char)*lSize);
	if (buffer == NULL) {
		// Memory error
		return NULL;
	}

	// copy the file into the buffer:
	result = fread (buffer,1,lSize,pFile);
	if (result != lSize) {
		// Reading error
		return NULL;
	}

	/* the whole file is now loaded in the memory buffer. */

	// terminate
	fclose (pFile);
	free (buffer);

	return NULL; //ChtDev;
}

void WriteProject64ChtDev(HWND hDlg) {
	FILE * pFile;

	char Identifier[100];
	char s[16];
	CHTDEVENTRY *ChtDev;

	//ChtDev = ReadProject64ChtDev(GetCheatDevIniFileName());
	ChtDev = (CHTDEVENTRY *)malloc(sizeof(CHTDEVENTRY));

	sprintf(Identifier,"%08X-%08X-C:%X",*(DWORD *)(&RomHeader[0x10]),*(DWORD *)(&RomHeader[0x14]),RomHeader[0x3D]);
	ChtDev->Identifier = Identifier;
	
	ChtDev->Name = RomName;

	ChtDev->LastSearch = (LASTSEARCH *)malloc(sizeof(LASTSEARCH));

	ChtDev->LastSearch->SearchType = "Value";

	SendMessage(GetDlgItem(hDlg, IDT_SEARCH_VALUE), WM_GETTEXT, 9, (LPARAM)s);
	ChtDev->LastSearch->SearchValue = s;

	ChtDev->LastSearch->ValueSearchType = searchType;

	ChtDev->LastSearch->NumBits = searchNumBits;

	ChtDev->LastSearch->Results = ResultsToString();

	pFile = fopen (GetCheatDevIniFileName(), "w");
	if (pFile == NULL) {
		// file error
		return;
	}

	fprintf(pFile, "<?xml version=""1.0"" encoding=""ISO-8859-1""?>\n");
	fprintf(pFile, "<CheatDev>\n");

	fprintf(pFile, "   <Game>\n");
	fprintf(pFile, "      <ID>%s</ID>\n", ChtDev->Identifier);
	fprintf(pFile, "      <Name>%s</Name>\n", ChtDev->Name);
	fprintf(pFile, "      <LastSearch>\n");
	fprintf(pFile, "         <SearchType>%s</SearchType>\n", ChtDev->LastSearch->SearchType);
	fprintf(pFile, "         <Value>%s</Value>\n", ChtDev->LastSearch->SearchValue);
	fprintf(pFile, "         <ValueSearchType>%d</ValueSearchType>\n", ChtDev->LastSearch->ValueSearchType);
	fprintf(pFile, "         <NumBits>%d</NumBits>\n", ChtDev->LastSearch->NumBits);
	fprintf(pFile, "         <Results>%s</Results>\n", ChtDev->LastSearch->Results);
	fprintf(pFile, "      </LastSearch>\n");
	fprintf(pFile, "   </Game>\n");

	fprintf(pFile, "</CheatDev>\n");

	// terminate
	fclose (pFile);

	free(ChtDev->LastSearch);
	free(ChtDev);
}

char *ResultsToString(void) {
	int count;
	char s[16];
	char *result;
	long maxlength = 0;

	if (searchNumBits == bits8) {
		maxlength= (9+3+3)*csNumResults+1;
	} else {
		maxlength= (9+5+5)*csNumResults+1;
	}
	result = (char *)calloc(maxlength, sizeof(char));

	for (count=0; count<csNumResults; count++) {
		sprintf_s(s, 16, "%08X,", csResults[count].Address);
		strcat_s(result, maxlength, s);

		if (searchNumBits == bits8)
			sprintf_s(s, 16, "%02X,", csResults[count].Value);
		else
			sprintf_s(s, 16, "%04X,", csResults[count].Value);

		strcat_s(result, maxlength, s);

		if (secondSearch) {
			if (searchNumBits == bits8)
				sprintf_s(s, 16, "%02X,", csResults[count].OldValue);
			else
				sprintf_s(s, 16, "%04X,", csResults[count].OldValue);

			strcat_s(result, maxlength, s);
		} else {
			sprintf_s(s, 16, "  ,");
			strcat_s(result, maxlength, s);
		}
	}
	
	return result;
}

void StringToResults(void) {
	//TODO
}

