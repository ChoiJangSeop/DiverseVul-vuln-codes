void UpdateDownloader::CleanLeftovers()
{
    // Note: this is called at startup. Do not use wxWidgets from this code!

    std::wstring tmpdir;
    if ( !Settings::ReadConfigValue("UpdateTempDir", tmpdir) )
        return;

    tmpdir.append(1, '\0'); // double NULL-terminate for SHFileOperation

    SHFILEOPSTRUCT fos = {0};
    fos.wFunc = FO_DELETE;
    fos.pFrom = tmpdir.c_str();
    fos.fFlags = FOF_NO_UI | // Vista+-only
                 FOF_SILENT |
                 FOF_NOCONFIRMATION |
                 FOF_NOERRORUI;

    if ( SHFileOperation(&fos) == 0 )
    {
        Settings::DeleteConfigValue("UpdateTempDir");
    }
    // else: try another time, this is just a "soft" error
}