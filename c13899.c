std::wstring CreateUniqueTempDirectory()
{
    // We need to put downloaded updates into a directory of their own, because
    // if we put it in $TMP, some DLLs could be there and interfere with the
    // installer.
    //
    // This code creates a new randomized directory name and tries to create it;
    // this process is repeated if the directory already exists.
    wchar_t tmpdir[MAX_PATH+1];
    if ( GetTempPath(MAX_PATH+1, tmpdir) == 0 )
        throw Win32Exception("Cannot create temporary directory");

    for ( ;; )
    {
        std::wstring dir(tmpdir);
        dir += L"Update-";

        UUID uuid;
        UuidCreate(&uuid);
        RPC_WSTR uuidStr;
        RPC_STATUS status = UuidToString(&uuid, &uuidStr);
        dir += reinterpret_cast<wchar_t*>(uuidStr);
        RpcStringFree(&uuidStr);

        if ( CreateDirectory(dir.c_str(), NULL) )
            return dir;
        else if ( GetLastError() != ERROR_ALREADY_EXISTS )
            throw Win32Exception("Cannot create temporary directory");
    }
}