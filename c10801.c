void ldbRedis(lua_State *lua, sds *argv, int argc) {
    int j, saved_rc = server.lua_replicate_commands;

    lua_getglobal(lua,"redis");
    lua_pushstring(lua,"call");
    lua_gettable(lua,-2);       /* Stack: redis, redis.call */
    for (j = 1; j < argc; j++)
        lua_pushlstring(lua,argv[j],sdslen(argv[j]));
    ldb.step = 1;               /* Force redis.call() to log. */
    server.lua_replicate_commands = 1;
    lua_pcall(lua,argc-1,1,0);  /* Stack: redis, result */
    ldb.step = 0;               /* Disable logging. */
    server.lua_replicate_commands = saved_rc;
    lua_pop(lua,2);             /* Discard the result and clean the stack. */
}