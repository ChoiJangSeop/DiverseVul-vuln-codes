NOEXPORT void tray_update(const int num) {
    NOTIFYICONDATA nid;
    static ICON_TYPE previous_icon=ICON_NONE;
    ICON_TYPE current_icon;
    LPTSTR tip;

    if(!global_options.option.taskbar) { /* currently disabled */
        tray_delete(); /* remove the taskbark icon if exists */
        return;
    }
    if(!tray_menu_handle) /* initialize taskbar */
        tray_menu_handle=LoadMenu(ghInst, MAKEINTRESOURCE(IDM_TRAYMENU));
    if(!tray_menu_handle) {
        ioerror("LoadMenu");
        return;
    }
    if(cmdline.service)
        EnableMenuItem(tray_menu_handle, IDM_EDIT_CONFIG, MF_GRAYED);

    ZeroMemory(&nid, sizeof nid);
    nid.cbSize=sizeof nid;
    nid.uID=1; /* application-defined icon ID */
    nid.uFlags=NIF_MESSAGE|NIF_TIP;
    nid.uCallbackMessage=WM_SYSTRAY; /* notification message */
    nid.hWnd=hwnd; /* window to receive notifications */
    if(num<0) {
        tip=str_tprintf(TEXT("Server is down"));
        current_icon=ICON_ERROR;
    } else if(num>0) {
        tip=str_tprintf(TEXT("%d active session(s)"), num);
        current_icon=ICON_ACTIVE;
    } else {
        tip=str_tprintf(TEXT("Server is idle"));
        current_icon=ICON_IDLE;
    }
    _tcsncpy(nid.szTip, tip, 63);
    nid.szTip[63]=TEXT('\0');
    str_free(tip);
    nid.hIcon=global_options.icon[current_icon];
    if(current_icon!=previous_icon) {
        nid.uFlags|=NIF_ICON;
        previous_icon=current_icon;
    }
    if(Shell_NotifyIcon(NIM_MODIFY, &nid)) /* modify tooltip */
        return; /* OK: taskbar icon exists */
    /* tooltip update failed - try to create the icon */
    nid.uFlags|=NIF_ICON;
    Shell_NotifyIcon(NIM_ADD, &nid);
}