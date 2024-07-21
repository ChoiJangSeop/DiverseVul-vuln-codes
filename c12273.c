bool __fastcall TSiteRawDialog::Execute(TSessionData * Data)
{
  std::unique_ptr<TSessionData> FactoryDefaults(new TSessionData(L""));
  std::unique_ptr<TSessionData> RawData(new TSessionData(L""));
  RawData->Assign(Data);
  // SFTP-only is not reflected by the protocol prefix, we have to use rawsettings for that
  if (RawData->FSProtocol != fsSFTPonly)
  {
    RawData->FSProtocol = FactoryDefaults->FSProtocol;
  }
  RawData->HostName = FactoryDefaults->HostName;
  RawData->PortNumber = FactoryDefaults->PortNumber;
  RawData->UserName = FactoryDefaults->UserName;
  RawData->Password = FactoryDefaults->Password;
  RawData->Ftps = FactoryDefaults->Ftps;

  std::unique_ptr<TStrings> Options(RawData->SaveToOptions(FactoryDefaults.get(), false, false));

  SettingsMemo->Lines = Options.get();

  bool Result = TCustomDialog::Execute();
  if (Result)
  {
    std::unique_ptr<TSessionData> BackupData(new TSessionData(L""));
    BackupData->Assign(Data);
    Data->DefaultSettings();

    Data->FSProtocol = BackupData->FSProtocol;
    Data->HostName = BackupData->HostName;
    Data->PortNumber = BackupData->PortNumber;
    Data->UserName = BackupData->UserName;
    Data->Password = BackupData->Password;
    Data->Ftps = BackupData->Ftps;

    Data->ApplyRawSettings(SettingsMemo->Lines);
  }
  return Result;
}