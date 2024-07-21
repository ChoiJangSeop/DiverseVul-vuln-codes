CBINDInstallDlg::RegisterService() {
	SC_HANDLE hSCManager;
	SC_HANDLE hService;
	CString StartName;

	if (m_accountName == LOCAL_SERVICE)
		StartName = LOCAL_SERVICE;
	else
		StartName = ".\\" + m_accountName;
	/*
	 * We need to change the service rather than create it
	 * if the service already exists. Do nothing if we are already
	 * using that account
	 */
	if (m_serviceExists == TRUE) {
		if (m_accountUsed == FALSE) {
			UpdateService(StartName);
			SetItemStatus(IDC_REG_SERVICE);
			return;
		} else {
			SetItemStatus(IDC_REG_SERVICE);
			return;
		}
	}

	SetCurrent(IDS_OPEN_SCM);
	hSCManager= OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCManager)
		throw(Exception(IDS_ERR_OPEN_SCM, GetErrMessage()));

	DWORD dwStart = SERVICE_DEMAND_START;
	if (m_autoStart)
		dwStart = SERVICE_AUTO_START;

	DWORD dwServiceType = SERVICE_WIN32_OWN_PROCESS;

	CString namedLoc;
	namedLoc.Format("%s\\bin\\named.exe", m_targetDir);

	SetCurrent(IDS_CREATE_SERVICE);
	hService = CreateService(hSCManager, BIND_SERVICE_NAME,
		BIND_DISPLAY_NAME, SERVICE_ALL_ACCESS, dwServiceType, dwStart,
		SERVICE_ERROR_NORMAL, namedLoc, NULL, NULL, NULL, StartName,
		m_accountPassword);

	if (!hService && GetLastError() != ERROR_SERVICE_EXISTS)
		throw(Exception(IDS_ERR_CREATE_SERVICE, GetErrMessage()));

	if (hService)
		CloseServiceHandle(hService);

	if (hSCManager)
		CloseServiceHandle(hSCManager);

	SetItemStatus(IDC_REG_SERVICE);
}