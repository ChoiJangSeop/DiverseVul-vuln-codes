ModHandle CModules::OpenModule(const CString& sModule, const CString& sModPath,
                               CModInfo& Info, CString& sRetMsg) {
    // Some sane defaults in case anything errors out below
    sRetMsg.clear();

    for (unsigned int a = 0; a < sModule.length(); a++) {
        if (((sModule[a] < '0') || (sModule[a] > '9')) &&
            ((sModule[a] < 'a') || (sModule[a] > 'z')) &&
            ((sModule[a] < 'A') || (sModule[a] > 'Z')) && (sModule[a] != '_')) {
            sRetMsg =
                t_f("Module names can only contain letters, numbers and "
                    "underscores, [{1}] is invalid")(sModule);
            return nullptr;
        }
    }

    // The second argument to dlopen() has a long history. It seems clear
    // that (despite what the man page says) we must include either of
    // RTLD_NOW and RTLD_LAZY and either of RTLD_GLOBAL and RTLD_LOCAL.
    //
    // RTLD_NOW vs. RTLD_LAZY: We use RTLD_NOW to avoid ZNC dying due to
    // failed symbol lookups later on. Doesn't really seem to have much of a
    // performance impact.
    //
    // RTLD_GLOBAL vs. RTLD_LOCAL: If perl is loaded with RTLD_LOCAL and later
    // on loads own modules (which it apparently does with RTLD_LAZY), we will
    // die in a name lookup since one of perl's symbols isn't found. That's
    // worse than any theoretical issue with RTLD_GLOBAL.
    ModHandle p = dlopen((sModPath).c_str(), RTLD_NOW | RTLD_GLOBAL);

    if (!p) {
        // dlerror() returns pointer to static buffer, which may be overwritten
        // very soon with another dl call also it may just return null.
        const char* cDlError = dlerror();
        CString sDlError = cDlError ? cDlError : t_s("Unknown error");
        sRetMsg = t_f("Unable to open module {1}: {2}")(sModule, sDlError);
        return nullptr;
    }

    const CModuleEntry* (*fpZNCModuleEntry)() = nullptr;
    // man dlsym(3) explains this
    *reinterpret_cast<void**>(&fpZNCModuleEntry) = dlsym(p, "ZNCModuleEntry");
    if (!fpZNCModuleEntry) {
        dlclose(p);
        sRetMsg = t_f("Could not find ZNCModuleEntry in module {1}")(sModule);
        return nullptr;
    }
    const CModuleEntry* pModuleEntry = fpZNCModuleEntry();

    if (std::strcmp(pModuleEntry->pcVersion, VERSION_STR) ||
        std::strcmp(pModuleEntry->pcVersionExtra, VERSION_EXTRA)) {
        sRetMsg = t_f(
            "Version mismatch for module {1}: core is {2}, module is built for "
            "{3}. Recompile this module.")(
            sModule, VERSION_STR VERSION_EXTRA,
            CString(pModuleEntry->pcVersion) + pModuleEntry->pcVersionExtra);
        dlclose(p);
        return nullptr;
    }

    if (std::strcmp(pModuleEntry->pcCompileOptions,
                    ZNC_COMPILE_OPTIONS_STRING)) {
        sRetMsg = t_f(
            "Module {1} is built incompatibly: core is '{2}', module is '{3}'. "
            "Recompile this module.")(sModule, ZNC_COMPILE_OPTIONS_STRING,
                                      pModuleEntry->pcCompileOptions);
        dlclose(p);
        return nullptr;
    }

    CTranslationDomainRefHolder translation("znc-" + sModule);
    pModuleEntry->fpFillModInfo(Info);

    sRetMsg = "";
    return p;
}