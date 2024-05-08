#ifndef __debugger_cheatsearch_h 
#define __debugger_cheatsearch_h 

extern HWND hCheatSearchDlg;
void Show_CheatSearchDlg (HWND hParent);
void Setup_CheatSearch_Window (HWND hParent);

#define CSRESULTITEM CSRESULTITEMA
typedef struct tagCSRESULTITEM {
  DWORD Address;
  DWORD Value;
  DWORD OldValue;
} CSRESULTITEMA;


#endif