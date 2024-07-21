void CLua::init_libraries()
{
    lua_stack_cleaner clean(state());

    // Open Crawl bindings
    cluaopen_kills(_state);
    cluaopen_you(_state);
    cluaopen_item(_state);
    cluaopen_food(_state);
    cluaopen_crawl(_state);
    cluaopen_file(_state);
    cluaopen_moninf(_state);
    cluaopen_options(_state);
    cluaopen_travel(_state);
    cluaopen_view(_state);
    cluaopen_spells(_state);

    cluaopen_globals(_state);

    execfile("dlua/macro.lua", true, true);

    // All hook names must be chk_????
    execstring("chk_startgame = { }", "base");

    lua_register(_state, "loadfile", _clua_loadfile);
    lua_register(_state, "dofile", _clua_dofile);

    lua_register(_state, "crawl_require", _clua_require);

    execfile("dlua/util.lua", true, true);
    execfile("dlua/iter.lua", true, true);
    execfile("dlua/tags.lua", true, true);
    execfile("dlua/init.lua", true, true);

    if (managed_vm)
    {
        lua_register(_state, "pcall", _clua_guarded_pcall);
        execfile("dlua/userbase.lua", true, true);
        execfile("dlua/persist.lua", true, true);
    }
}