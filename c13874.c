GetStartupData(HANDLE pipe, STARTUP_DATA *sud)
{
    size_t size, len;
    BOOL ret = FALSE;
    WCHAR *data = NULL;
    DWORD bytes, read;

    bytes = PeekNamedPipeAsync(pipe, 1, &exit_event);
    if (bytes == 0)
    {
        MsgToEventLog(M_SYSERR, TEXT("PeekNamedPipeAsync failed"));
        ReturnLastError(pipe, L"PeekNamedPipeAsync");
        goto out;
    }

    size = bytes / sizeof(*data);
    if (size == 0)
    {
        MsgToEventLog(M_SYSERR, TEXT("malformed startup data: 1 byte received"));
        ReturnError(pipe, ERROR_STARTUP_DATA, L"GetStartupData", 1, &exit_event);
        goto out;
    }

    data = malloc(bytes);
    if (data == NULL)
    {
        MsgToEventLog(M_SYSERR, TEXT("malloc failed"));
        ReturnLastError(pipe, L"malloc");
        goto out;
    }

    read = ReadPipeAsync(pipe, data, bytes, 1, &exit_event);
    if (bytes != read)
    {
        MsgToEventLog(M_SYSERR, TEXT("ReadPipeAsync failed"));
        ReturnLastError(pipe, L"ReadPipeAsync");
        goto out;
    }

    if (data[size - 1] != 0)
    {
        MsgToEventLog(M_ERR, TEXT("Startup data is not NULL terminated"));
        ReturnError(pipe, ERROR_STARTUP_DATA, L"GetStartupData", 1, &exit_event);
        goto out;
    }

    sud->directory = data;
    len = wcslen(sud->directory) + 1;
    size -= len;
    if (size <= 0)
    {
        MsgToEventLog(M_ERR, TEXT("Startup data ends at working directory"));
        ReturnError(pipe, ERROR_STARTUP_DATA, L"GetStartupData", 1, &exit_event);
        goto out;
    }

    sud->options = sud->directory + len;
    len = wcslen(sud->options) + 1;
    size -= len;
    if (size <= 0)
    {
        MsgToEventLog(M_ERR, TEXT("Startup data ends at command line options"));
        ReturnError(pipe, ERROR_STARTUP_DATA, L"GetStartupData", 1, &exit_event);
        goto out;
    }

    sud->std_input = sud->options + len;
    data = NULL; /* don't free data */
    ret = TRUE;

out:
    free(data);
    return ret;
}