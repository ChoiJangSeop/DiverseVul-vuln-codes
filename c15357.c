bool CModules::GetModPathInfo(CModInfo& ModInfo, const CString& sModule,
                              const CString& sModPath, CString& sRetMsg) {
    ModInfo.SetName(sModule);
    ModInfo.SetPath(sModPath);

    ModHandle p = OpenModule(sModule, sModPath, ModInfo, sRetMsg);
    if (!p) return false;

    dlclose(p);

    return true;
}