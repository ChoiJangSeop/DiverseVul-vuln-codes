int __fastcall Execute()
{
  DebugAssert(StoredSessions);
  TProgramParams * Params = TProgramParams::Instance();
  DebugAssert(Params);

  // do not flash message boxes on startup
  SetOnForeground(true);

  // let installer know, that some instance of application is running
  CreateMutex(NULL, False, AppName.c_str());
  bool OnlyInstance = (GetLastError() == 0);

  UpdateStaticUsage();

  UnicodeString KeyFile;
  if (Params->FindSwitch(L"PrivateKey", KeyFile))
  {
    WinConfiguration->DefaultKeyFile = KeyFile;
  }

  UnicodeString ConsoleVersion;
  UnicodeString DotNetVersion;
  Params->FindSwitch(L"Console", ConsoleVersion);
  Params->FindSwitch(L"DotNet", DotNetVersion);
  if (!ConsoleVersion.IsEmpty() || !DotNetVersion.IsEmpty())
  {
    RecordWrapperVersions(ConsoleVersion, DotNetVersion);
  }
  if (!DotNetVersion.IsEmpty())
  {
    Configuration->Usage->Inc(L"ConsoleDotNet");
  }

  UnicodeString SwitchValue;
  if (Params->FindSwitch(L"loglevel", SwitchValue))
  {
    int StarPos = SwitchValue.Pos(L"*");
    if (StarPos > 0)
    {
      bool LogSensitive = true;
      SwitchValue.Delete(StarPos, 1);

      if ((StarPos <= SwitchValue.Length()) &&
          (SwitchValue[StarPos] == L'-'))
      {
        LogSensitive = false;
        SwitchValue.Delete(StarPos, 1);
      }

      SwitchValue = SwitchValue.Trim();

      Configuration->TemporaryLogSensitive(LogSensitive);
    }
    int LogProtocol;
    if (!SwitchValue.IsEmpty() && TryStrToInt(SwitchValue, LogProtocol) && (LogProtocol >= -1))
    {
      Configuration->TemporaryLogProtocol(LogProtocol);
    }
  }

  if (Params->FindSwitch(LOGSIZE_SWITCH, SwitchValue))
  {
    int StarPos = SwitchValue.Pos(LOGSIZE_SEPARATOR);
    int LogMaxCount = 0;
    if (StarPos > 1)
    {
      if (!TryStrToInt(SwitchValue.SubString(1, StarPos - 1), LogMaxCount))
      {
        LogMaxCount = -1;
      }
      SwitchValue.Delete(1, StarPos);
      SwitchValue = SwitchValue.Trim();
    }

    __int64 LogMaxSize;
    if ((LogMaxCount >= 0) &&
        !SwitchValue.IsEmpty() &&
        TryStrToSize(SwitchValue, LogMaxSize))
    {
      Configuration->TemporaryLogMaxCount(LogMaxCount);
      Configuration->TemporaryLogMaxSize(LogMaxSize);
    }
  }

  std::unique_ptr<TStrings> RawSettings(new TStringList());
  if (Params->FindSwitch(RAWTRANSFERSETTINGS_SWITCH, RawSettings.get()))
  {
    std::unique_ptr<TOptionsStorage> OptionsStorage(new TOptionsStorage(RawSettings.get(), false));
    GUIConfiguration->LoadDefaultCopyParam(OptionsStorage.get());
  }

  TConsoleMode Mode = cmNone;
  if (Params->FindSwitch(L"help") || Params->FindSwitch(L"h") || Params->FindSwitch(L"?"))
  {
    Mode = cmHelp;
  }
  else if (Params->FindSwitch(L"batchsettings"))
  {
    Mode = cmBatchSettings;
  }
  else if (Params->FindSwitch(KEYGEN_SWITCH))
  {
    Mode = cmKeyGen;
  }
  else if (Params->FindSwitch(FINGERPRINTSCAN_SWITCH))
  {
    Mode = cmFingerprintScan;
  }
  else if (Params->FindSwitch(DUMPCALLSTACK_SWITCH))
  {
    Mode = cmDumpCallstack;
  }
  else if (Params->FindSwitch(INFO_SWITCH))
  {
    Mode = cmInfo;
  }
  else if (Params->FindSwitch(COMREGISTRATION_SWITCH))
  {
    Mode = cmComRegistration;
  }
  // We have to check for /console only after the other options,
  // as the /console is always used when we are run by winscp.com
  // (ambiguous use to pass console version)
  else if (Params->FindSwitch(L"Console") || Params->FindSwitch(SCRIPT_SWITCH) ||
      Params->FindSwitch(COMMAND_SWITCH))
  {
    Mode = cmScripting;
  }

  if (Mode != cmNone)
  {
    return Console(Mode);
  }

  TTerminalManager * TerminalManager = NULL;
  GlyphsModule = NULL;
  NonVisualDataModule = NULL;
  TStrings * CommandParams = new TStringList;
  try
  {
    TerminalManager = TTerminalManager::Instance();
    HANDLE ResourceModule = GUIConfiguration->ChangeToDefaultResourceModule();
    try
    {
      GlyphsModule = new TGlyphsModule(Application);
    }
    __finally
    {
      GUIConfiguration->ChangeResourceModule(ResourceModule);
    }
    NonVisualDataModule = new TNonVisualDataModule(Application);

    // The default is 2.5s.
    // 20s is used by Office 2010 and Windows 10 Explorer.
    // Some applications use an infinite (Thunderbird, Firefox).
    // Overriden for some controls using THintInfo.HideTimeout
    Application->HintHidePause = 20000;
    HintWindowClass = __classid(TScreenTipHintWindow);

    UnicodeString IniFileName = Params->SwitchValue(INI_SWITCH);
    if (!IniFileName.IsEmpty() && (IniFileName != INI_NUL))
    {
      UnicodeString IniFileNameExpanded = ExpandEnvironmentVariables(IniFileName);
      if (!FileExists(ApiPath(IniFileNameExpanded)))
      {
        // this should be displayed rather at the very beginning.
        // however for simplicity (GUI-only), we do it only here.
        MessageDialog(FMTLOAD(FILE_NOT_EXISTS, (IniFileNameExpanded)), qtError, qaOK);
      }
    }

    if (Params->FindSwitch(L"UninstallCleanup"))
    {
      MaintenanceTask();
      Configuration->DontSave();
      // The innosetup cannot skip UninstallCleanup run task for silent uninstalls,
      // workaround is that we create mutex in uninstaller, if it runs silent, and
      // ignore the UninstallCleanup, when the mutex exists.
      if (OpenMutex(SYNCHRONIZE, false, L"WinSCPSilentUninstall") == NULL)
      {
        DoCleanupDialogIfAnyDataAndWanted();
      }
    }
    else if (Params->FindSwitch(L"RegisterForDefaultProtocols") ||
             Params->FindSwitch(L"RegisterAsUrlHandler")) // BACKWARD COMPATIBILITY
    {
      MaintenanceTask();
      if (CheckSafe(Params))
      {
        RegisterForDefaultProtocols();
        Configuration->DontSave();
      }
    }
    else if (Params->FindSwitch(L"UnregisterForProtocols"))
    {
      MaintenanceTask();
      if (CheckSafe(Params))
      {
        UnregisterForProtocols();
        Configuration->DontSave();
      }
    }
    else if (Params->FindSwitch(L"AddSearchPath"))
    {
      MaintenanceTask();
      if (CheckSafe(Params))
      {
        AddSearchPath(ExtractFilePath(Application->ExeName));
        Configuration->DontSave();
      }
    }
    else if (Params->FindSwitch(L"RemoveSearchPath"))
    {
      MaintenanceTask();
      if (CheckSafe(Params))
      {
        try
        {
          RemoveSearchPath(ExtractFilePath(Application->ExeName));
        }
        catch(...)
        {
          // ignore errors
          // (RemoveSearchPath is called always on uninstallation,
          // even if AddSearchPath was not used, so we would get the error
          // always for non-priviledged user)
        }
        Configuration->DontSave();
      }
    }
    else if (Params->FindSwitch(L"ImportSitesIfAny"))
    {
      MaintenanceTask();
      ImportSitesIfAny();
    }
    else if (Params->FindSwitch(L"Usage", SwitchValue))
    {
      MaintenanceTask();
      Usage(SwitchValue);
    }
    else if (Params->FindSwitch(L"Update"))
    {
      MaintenanceTask();
      CheckForUpdates(false);
    }
    else if (ShowUpdatesIfAvailable())
    {
      // noop
    }
    else if (Params->FindSwitch(L"Exit"))
    {
      // noop
      MaintenanceTask();
      Configuration->DontSave();
    }
    else if (Params->FindSwitch(L"MaintenanceTask"))
    {
      // Parameter /MaintenanceTask can be added to command-line when executing maintenance tasks
      // (e.g. from installer) just in case old version of WinSCP is called by mistake
      MaintenanceTask();
      Configuration->DontSave();
    }
    else
    {
      enum { pcNone, pcUpload, pcFullSynchronize, pcSynchronize, pcEdit, pcRefresh } ParamCommand;
      ParamCommand = pcNone;
      UnicodeString AutoStartSession;
      UnicodeString DownloadFile;
      int UseDefaults = -1;

      // do not check for temp dirs for service tasks (like RegisterAsUrlHandler)
      if (OnlyInstance &&
          WinConfiguration->TemporaryDirectoryCleanup)
      {
        TemporaryDirectoryCleanup();
      }

      WinConfiguration->CheckDefaultTranslation();
      // Loading shell image lists here (rather than only on demand when file controls are being created)
      // reduces risk of an occasional crash.
      // It seems that the point is to load the lists before any call to SHGetFileInfoWithTimeout.
      InitFileControls();

      if (!Params->Empty)
      {
        UnicodeString Value;
        if (Params->FindSwitch(DEFAULTS_SWITCH, Value) && CheckSafe(Params))
        {
          UseDefaults = StrToIntDef(Value, 0);
        }

        if (Params->FindSwitch(UPLOAD_SWITCH, CommandParams))
        {
          ParamCommand = pcUpload;
          if (CommandParams->Count == 0)
          {
            throw Exception(NO_UPLOAD_LIST_ERROR);
          }
        }
        if (Params->FindSwitch(UPLOAD_IF_ANY_SWITCH, CommandParams))
        {
          if (CommandParams->Count > 0)
          {
            ParamCommand = pcUpload;
          }
        }
        else if (Params->FindSwitch(SYNCHRONIZE_SWITCH, CommandParams, 4))
        {
          ParamCommand = pcFullSynchronize;
        }
        else if (Params->FindSwitch(KEEP_UP_TO_DATE_SWITCH, CommandParams, 4))
        {
          ParamCommand = pcSynchronize;
        }
        else if (Params->FindSwitch(L"Edit", CommandParams, 1) &&
                 (CommandParams->Count == 1))
        {
          ParamCommand = pcEdit;
        }
        else if (Params->FindSwitch(REFRESH_SWITCH, CommandParams, 1))
        {
          ParamCommand = pcRefresh;
        }
      }

      if (Params->ParamCount > 0)
      {
        AutoStartSession = Params->Param[1];
        Params->ParamsProcessed(1, 1);

        if ((ParamCommand == pcNone) &&
            (WinConfiguration->ExternalSessionInExistingInstance != OpenInNewWindow()) &&
            !Params->FindSwitch(NEWINSTANCE_SWICH) &&
            SendToAnotherInstance())
        {
          Configuration->Usage->Inc(L"SendToAnotherInstance");
          return 0;
        }
        UnicodeString CounterName;
        if (Params->FindSwitch(JUMPLIST_SWITCH))
        {
          CounterName = L"CommandLineJumpList";
        }
        else if (Params->FindSwitch(DESKTOP_SWITCH))
        {
          CounterName = L"CommandLineDesktop";
        }
        else if (Params->FindSwitch(SEND_TO_HOOK_SWITCH))
        {
          CounterName = L"CommandLineSendToHook";
        }
        else
        {
          CounterName = L"CommandLineSession2";
        }
        Configuration->Usage->Inc(CounterName);
      }
      else if (WinConfiguration->EmbeddedSessions && StoredSessions->Count)
      {
        AutoStartSession = StoredSessions->Sessions[0]->Name;
      }
      else
      {
        AutoStartSession = WinConfiguration->AutoStartSession;
      }

      if (ParamCommand == pcRefresh)
      {
        Refresh(AutoStartSession, (CommandParams->Count > 0 ? CommandParams->Strings[0] : UnicodeString()));
        return 0;
      }

      // from now flash message boxes on background
      SetOnForeground(false);

      bool NeedSession = (ParamCommand != pcNone);

      bool Retry;
      do
      {
        Retry = false;
        std::unique_ptr<TObjectList> DataList(new TObjectList());
        try
        {
          GetLoginData(AutoStartSession, Params, DataList.get(), DownloadFile, NeedSession, NULL, pufAllowStoredSiteWithProtocol);
          // GetLoginData now Aborts when session is needed and none is selected
          if (DebugAlwaysTrue(!NeedSession || (DataList->Count > 0)))
          {
            if (CheckSafe(Params))
            {
              UnicodeString LogFile;
              if (Params->FindSwitch(LOG_SWITCH, LogFile))
              {
                Configuration->TemporaryLogging(LogFile);
              }
              if (Params->FindSwitch(L"XmlLog", LogFile))
              {
                Configuration->TemporaryActionsLogging(LogFile);
              }
            }

            try
            {
              DebugAssert(!TerminalManager->ActiveTerminal);

              bool CanStart;
              bool Browse = false;
              if (DataList->Count > 0)
              {
                TManagedTerminal * Terminal = TerminalManager->NewTerminals(DataList.get());
                UnicodeString BrowseFile;
                if (Params->FindSwitch(BROWSE_SWITCH, BrowseFile) &&
                    (!BrowseFile.IsEmpty() || !DownloadFile.IsEmpty()))
                {
                  if (BrowseFile.IsEmpty())
                  {
                    BrowseFile = DownloadFile;
                  }
                  DebugAssert(Terminal->RemoteExplorerState == NULL);
                  Terminal->RemoteExplorerState = CreateDirViewStateForFocusedItem(BrowseFile);
                  DebugAssert(Terminal->LocalExplorerState == NULL);
                  Terminal->LocalExplorerState = CreateDirViewStateForFocusedItem(BrowseFile);
                  DownloadFile = UnicodeString();
                  Browse = true;
                }
                if (!DownloadFile.IsEmpty())
                {
                  Terminal->AutoReadDirectory = false;
                  DownloadFile = UnixIncludeTrailingBackslash(Terminal->SessionData->RemoteDirectory) + DownloadFile;
                  Terminal->SessionData->RemoteDirectory = L"";
                  Terminal->StateData->RemoteDirectory = Terminal->SessionData->RemoteDirectory;
                }
                TerminalManager->ActiveTerminal = Terminal;
                CanStart = (TerminalManager->Count > 0);
              }
              else
              {
                DebugAssert(!NeedSession);
                CanStart = true;
              }

              if (!CanStart)
              {
                // do not prompt with login dialog, if connection of
                // auto-start session (typically from command line) failed
                if (AutoStartSession.IsEmpty())
                {
                  Retry = true;
                }
              }
              else
              {
                // from now on, we do not support runtime interface change
                CustomWinConfiguration->CanApplyInterfaceImmediately = false;
                TCustomScpExplorerForm * ScpExplorer = CreateScpExplorer();
                CustomWinConfiguration->AppliedInterface = CustomWinConfiguration->Interface;
                try
                {
                  // moved inside try .. __finally, because it can fail as well
                  TerminalManager->ScpExplorer = ScpExplorer;

                  if ((ParamCommand != pcNone) || !DownloadFile.IsEmpty())
                  {
                    Configuration->Usage->Inc(L"CommandLineOperation");
                  }

                  if (ParamCommand == pcUpload)
                  {
                    Upload(TerminalManager->ActiveTerminal, CommandParams, UseDefaults);
                  }
                  else if (ParamCommand == pcFullSynchronize)
                  {
                    FullSynchronize(TerminalManager->ActiveTerminal, ScpExplorer,
                      CommandParams, UseDefaults);
                  }
                  else if (ParamCommand == pcSynchronize)
                  {
                    Synchronize(TerminalManager->ActiveTerminal, ScpExplorer,
                      CommandParams, UseDefaults);
                  }
                  else if (ParamCommand == pcEdit)
                  {
                    Edit(ScpExplorer, CommandParams);
                  }
                  else if (!DownloadFile.IsEmpty())
                  {
                    Download(TerminalManager->ActiveTerminal, DownloadFile,
                      UseDefaults);
                  }

                  if (Browse)
                  {
                    ScpExplorer->BrowseFile();
                  }

                  Application->Run();
                  // to allow dialog boxes show later (like from CheckConfigurationForceSave)
                  SetAppTerminated(False);
                }
                __finally
                {
                  TerminalManager->ScpExplorer = NULL;
                  SAFE_DESTROY(ScpExplorer);
                }
              }
            }
            catch (Exception &E)
            {
              ShowExtendedException(&E);
            }
          }
        }
        // Catch EAbort from Synchronize() and similar functions, so that CheckConfigurationForceSave is processed
        catch (EAbort & E)
        {
          Retry = false; // unlikely to be true, but just in case
        }
      }
      while (Retry);
    }

    // In GUI mode only
    CheckConfigurationForceSave();
  }
  __finally
  {
    delete NonVisualDataModule;
    NonVisualDataModule = NULL;
    ReleaseImagesModules();
    delete GlyphsModule;
    GlyphsModule = NULL;
    TTerminalManager::DestroyInstance();
    delete CommandParams;
  }

  return 0;
}