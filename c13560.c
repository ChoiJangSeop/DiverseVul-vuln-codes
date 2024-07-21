CBINDInstallDlg::UpdateService(CString StartName) {
	SC_HANDLE hSCManager;
	SC_HANDLE hService;

	if(m_toolsOnly)
		return;

	SetCurrent(IDS_OPEN_SCM);
	hSCManager= OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCManager) {
		MsgBox(IDS_ERR_OPEN_SCM, GetErrMessage());
		return;
	}

	DWORD dwStart = SERVICE_DEMAND_START;
	if (m_autoStart)
		dwStart = SERVICE_AUTO_START;

	DWORD dwServiceType = SERVICE_WIN32_OWN_PROCESS;

	CString namedLoc;
	namedLoc.Format("%s\\bin\\named.exe", m_targetDir);

	SetCurrent(IDS_OPEN_SERVICE);
	hService = OpenService(hSCManager, BIND_SERVICE_NAME,
			       SERVICE_CHANGE_CONFIG);
	if (!hService)
	{
		MsgBox(IDS_ERR_OPEN_SERVICE, GetErrMessage());
		if (hSCManager)
			CloseServiceHandle(hSCManager);
		return;
	} else {
		if (ChangeServiceConfig(hService, dwServiceType, dwStart,
			SERVICE_ERROR_NORMAL, namedLoc, NULL, NULL, NULL,
			StartName, m_accountPassword, BIND_DISPLAY_NAME)
			!= TRUE) {
			DWORD err = GetLastError();
			MsgBox(IDS_ERR_UPDATE_SERVICE, GetErrMessage());
		}
	}

	if (hService)
		CloseServiceHandle(hService);

	if (hSCManager)
		CloseServiceHandle(hSCManager);

	SetItemStatus(IDC_REG_SERVICE);
}