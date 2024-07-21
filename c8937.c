PasswordProcess (
  IN  UI_MENU_OPTION              *MenuOption
  )
{
  CHAR16                          *StringPtr;
  CHAR16                          *TempString;
  UINTN                           Maximum;
  EFI_STATUS                      Status;
  EFI_IFR_PASSWORD                *PasswordInfo;
  FORM_DISPLAY_ENGINE_STATEMENT   *Question;
  EFI_INPUT_KEY                   Key;

  Question     = MenuOption->ThisTag;
  PasswordInfo = (EFI_IFR_PASSWORD *) Question->OpCode;
  Maximum      = PasswordInfo->MaxSize;
  Status       = EFI_SUCCESS;

  StringPtr = AllocateZeroPool ((Maximum + 1) * sizeof (CHAR16));
  ASSERT (StringPtr);

  //
  // Use a NULL password to test whether old password is required
  //
  *StringPtr = 0;
  Status = Question->PasswordCheck (gFormData, Question, StringPtr);
  if (Status == EFI_NOT_AVAILABLE_YET || Status == EFI_UNSUPPORTED) {
    //
    // Password can't be set now.
    //
    if (Status == EFI_UNSUPPORTED) {
      do {
        CreateDialog (&Key, gEmptyString, gPasswordUnsupported, gPressEnter, gEmptyString, NULL);
      } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
    }
    FreePool (StringPtr);
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    //
    // Old password exist, ask user for the old password
    //
    Status = ReadString (MenuOption, gPromptForPassword, StringPtr);
    if (EFI_ERROR (Status)) {
      FreePool (StringPtr);
      return Status;
    }

    //
    // Check user input old password
    //
    Status = Question->PasswordCheck (gFormData, Question, StringPtr);
    if (EFI_ERROR (Status)) {
      if (Status == EFI_NOT_READY) {
        //
        // Typed in old password incorrect
        //
        PasswordInvalid ();
      } else {
        Status = EFI_SUCCESS;
      }

      FreePool (StringPtr);
      return Status;
    }
  }

  //
  // Ask for new password
  //
  ZeroMem (StringPtr, (Maximum + 1) * sizeof (CHAR16));
  Status = ReadString (MenuOption, gPromptForNewPassword, StringPtr);
  if (EFI_ERROR (Status)) {
    //
    // Reset state machine for password
    //
    Question->PasswordCheck (gFormData, Question, NULL);
    FreePool (StringPtr);
    return Status;
  }

  //
  // Confirm new password
  //
  TempString = AllocateZeroPool ((Maximum + 1) * sizeof (CHAR16));
  ASSERT (TempString);
  Status = ReadString (MenuOption, gConfirmPassword, TempString);
  if (EFI_ERROR (Status)) {
    //
    // Reset state machine for password
    //
    Question->PasswordCheck (gFormData, Question, NULL);
    FreePool (StringPtr);
    FreePool (TempString);
    return Status;
  }

  //
  // Compare two typed-in new passwords
  //
  if (StrCmp (StringPtr, TempString) == 0) {
    gUserInput->InputValue.Buffer = AllocateCopyPool (Question->CurrentValue.BufferLen, StringPtr);
    gUserInput->InputValue.BufferLen = Question->CurrentValue.BufferLen;
    gUserInput->InputValue.Type = Question->CurrentValue.Type;
    gUserInput->InputValue.Value.string = HiiSetString(gFormData->HiiHandle, gUserInput->InputValue.Value.string, StringPtr, NULL);

    Status = EFI_SUCCESS;
  } else {
    //
    // Reset state machine for password
    //
    Question->PasswordCheck (gFormData, Question, NULL);

    //
    // Two password mismatch, prompt error message
    //
    do {
      CreateDialog (&Key, gEmptyString, gConfirmError, gPressEnter, gEmptyString, NULL);
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

    Status = EFI_INVALID_PARAMETER;
  }
  ZeroMem (TempString, (Maximum + 1) * sizeof (CHAR16));
  ZeroMem (StringPtr, (Maximum + 1) * sizeof (CHAR16));
  FreePool (TempString);
  FreePool (StringPtr);

  return Status;
}