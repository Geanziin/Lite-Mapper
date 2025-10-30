#include "tray.h"

HMENU CreateTrayMenu() {
	HMENU menu = CreatePopupMenu();
	AppendMenu(menu, MF_STRING, 1002, L"Open");
	AppendMenu(menu, MF_STRING, 1001, L"Exit");
	return menu;
}
