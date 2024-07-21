int __fastcall BatchSettings(TConsole * Console, TProgramParams * Params)
{
  int Result = RESULT_SUCCESS;
  try
  {
    std::unique_ptr<TStrings> Arguments(new TStringList());
    if (!DebugAlwaysTrue(Params->FindSwitch(L"batchsettings", Arguments.get())))
    {
      Abort();
    }
    else
    {
      if (Arguments->Count < 1)
      {
        throw Exception(LoadStr(BATCH_SET_NO_MASK));
      }
      else if (Arguments->Count < 2)
      {
        throw Exception(LoadStr(BATCH_SET_NO_SETTINGS));
      }
      else
      {
        TFileMasks Mask(Arguments->Strings[0]);
        Arguments->Delete(0);

        std::unique_ptr<TOptionsStorage> OptionsStorage(new TOptionsStorage(Arguments.get(), false));

        int Matches = 0;
        int Changes = 0;

        for (int Index = 0; Index < StoredSessions->Count; Index++)
        {
          TSessionData * Data = StoredSessions->Sessions[Index];
          if (!Data->IsWorkspace &&
              Mask.Matches(Data->Name, false, false))
          {
            Matches++;
            std::unique_ptr<TSessionData> OriginalData(new TSessionData(L""));
            OriginalData->CopyDataNoRecrypt(Data);
            Data->ApplyRawSettings(OptionsStorage.get());
            bool Changed = !OriginalData->IsSame(Data, false);
            if (Changed)
            {
              Changes++;
            }
            UnicodeString StateStr = LoadStr(Changed ? BATCH_SET_CHANGED : BATCH_SET_NOT_CHANGED);
            Console->PrintLine(FORMAT(L"%s - %s", (Data->Name, StateStr)));
          }
        }

        StoredSessions->Save(false, true); // explicit
        Console->PrintLine(FMTLOAD(BATCH_SET_SUMMARY, (Matches, Changes)));
      }
    }
  }
  catch (Exception & E)
  {
    Result = HandleException(Console, E);
  }

  Console->WaitBeforeExit();
  return Result;
}