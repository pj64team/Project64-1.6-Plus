#if defined(__cplusplus)
extern "C" {
#endif

	BOOL RegDelnodeRecurse(HKEY hKeyRoot, LPTSTR lpSubKey);
	BOOL RegDelnode(HKEY hKeyRoot, LPCTSTR lpSubKey);

#if defined(__cplusplus)
}
#endif