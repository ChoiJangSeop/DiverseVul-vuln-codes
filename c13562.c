void CBINDInstallDlg::OnInstall() {
	BOOL success = FALSE;
	int oldlen;

	if (CheckBINDService())
		StopBINDService();

	InstallTags();

	UpdateData();

	if (!m_toolsOnly && m_accountName != LOCAL_SERVICE) {
		/*
		 * Check that the Passwords entered match.
		 */
		if (m_accountPassword != m_accountPasswordConfirm) {
			MsgBox(IDS_ERR_PASSWORD);
			return;
		}

		/*
		 * Check that there is not leading / trailing whitespace.
		 * This is for compatibility with the standard password dialog.
		 * Passwords really should be treated as opaque blobs.
		 */
		oldlen = m_accountPassword.GetLength();
		m_accountPassword.TrimLeft();
		m_accountPassword.TrimRight();
		if (m_accountPassword.GetLength() != oldlen) {
			MsgBox(IDS_ERR_WHITESPACE);
			return;
		}

		/*
		 * Check the entered account name.
		 */
		if (ValidateServiceAccount() == FALSE)
			return;

		/*
		 * For Registration we need to know if account was changed.
		 */
		if (m_accountName != m_currentAccount)
			m_accountUsed = FALSE;

		if (m_accountUsed == FALSE && m_serviceExists == FALSE)
		{
		/*
		 * Check that the Password is not null.
		 */
			if (m_accountPassword.GetLength() == 0) {
				MsgBox(IDS_ERR_NULLPASSWORD);
				return;
			}
		}
	} else if (m_accountName == LOCAL_SERVICE) {
		/* The LocalService always exists. */
		m_accountExists = TRUE;
		if (m_accountName != m_currentAccount)
			m_accountUsed = FALSE;
	}

	/* Directories */
	m_etcDir = m_targetDir + "\\etc";
	m_binDir = m_targetDir + "\\bin";

	if (m_defaultDir != m_targetDir) {
		if (GetFileAttributes(m_targetDir) != 0xFFFFFFFF)
		{
			int install = MsgBox(IDS_DIREXIST,
					MB_YESNO | MB_ICONQUESTION, m_targetDir);
			if (install == IDNO)
				return;
		}
		else {
			int createDir = MsgBox(IDS_CREATEDIR,
					MB_YESNO | MB_ICONQUESTION, m_targetDir);
			if (createDir == IDNO)
				return;
		}
	}

	if (!m_toolsOnly) {
		if (m_accountExists == FALSE) {
			success = CreateServiceAccount(m_accountName.GetBuffer(30),
						       m_accountPassword.GetBuffer(30));
			if (success == FALSE) {
				MsgBox(IDS_CREATEACCOUNT_FAILED);
				return;
			}
			m_accountExists = TRUE;
		}
	}

	ProgramGroup(FALSE);

	/*
	 * Install Visual Studio libraries.  As per:
	 * http://blogs.msdn.com/astebner/archive/2006/08/23/715755.aspx
	 *
	 * Vcredist_x86.exe /q:a /c:"msiexec /i vcredist.msi /qn /l*v %temp%\vcredist_x86.log"
	 */
	/*system(".\\Vcredist_x86.exe /q:a /c:\"msiexec /i vcredist.msi /qn /l*v %temp%\vcredist_x86.log\"");*/

	/*
	 * Enclose full path to Vcredist_x86.exe in quotes as
	 * m_currentDir may contain spaces.
	 */
	if (runvcredist) {
		char Vcredist_x86[MAX_PATH];
		if (forwin64)
			sprintf(Vcredist_x86, "\"%s\\Vcredist_x64.exe\"",
				(LPCTSTR) m_currentDir);
		else
			sprintf(Vcredist_x86, "\"%s\\Vcredist_x86.exe\"",
				(LPCTSTR) m_currentDir);
		system(Vcredist_x86);
	}
	try {
		CreateDirs();
		ReadInstallFileList();
		CopyFiles();
		if (!m_toolsOnly)
			RegisterService();
		RegisterMessages();

		HKEY hKey;

		/* Create a new key for named */
		SetCurrent(IDS_CREATE_KEY);
		if (RegCreateKey(HKEY_LOCAL_MACHINE, BIND_SUBKEY,
			&hKey) == ERROR_SUCCESS) {
			// Get the install directory
			RegSetValueEx(hKey, "InstallDir", 0, REG_SZ,
					(LPBYTE)(LPCTSTR)m_targetDir,
					m_targetDir.GetLength());
			RegCloseKey(hKey);
		}


		SetCurrent(IDS_ADD_REMOVE);
		if (RegCreateKey(HKEY_LOCAL_MACHINE, BIND_UNINSTALL_SUBKEY,
				 &hKey) == ERROR_SUCCESS) {
			CString buf(BIND_DISPLAY_NAME);

			RegSetValueEx(hKey, "DisplayName", 0, REG_SZ,
					(LPBYTE)(LPCTSTR)buf, buf.GetLength());

			buf.Format("%s\\BINDInstall.exe", m_binDir);
			RegSetValueEx(hKey, "UninstallString", 0, REG_SZ,
					(LPBYTE)(LPCTSTR)buf, buf.GetLength());
			RegCloseKey(hKey);
		}

		ProgramGroup(FALSE);

		if (m_startOnInstall)
			StartBINDService();
	}
	catch(Exception e) {
		MessageBox(e.resString);
		SetCurrent(IDS_CLEANUP);
		FailedInstall();
		MsgBox(IDS_FAIL);
		return;
	}
	catch(DWORD dw)	{
		CString msg;
		msg.Format("A fatal error occured\n(%s)", GetErrMessage(dw));
		MessageBox(msg);
		SetCurrent(IDS_CLEANUP);
		FailedInstall();
		MsgBox(IDS_FAIL);
		return;
	}

	SetCurrent(IDS_INSTALL_DONE);
	MsgBox(IDS_SUCCESS);
}