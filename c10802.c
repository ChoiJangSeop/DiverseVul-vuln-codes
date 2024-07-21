char *redisProtocolToLuaType(lua_State *lua, char* reply) {
    char *p = reply;

    switch(*p) {
    case ':': p = redisProtocolToLuaType_Int(lua,reply); break;
    case '$': p = redisProtocolToLuaType_Bulk(lua,reply); break;
    case '+': p = redisProtocolToLuaType_Status(lua,reply); break;
    case '-': p = redisProtocolToLuaType_Error(lua,reply); break;
    case '*': p = redisProtocolToLuaType_Aggregate(lua,reply,*p); break;
    case '%': p = redisProtocolToLuaType_Aggregate(lua,reply,*p); break;
    case '~': p = redisProtocolToLuaType_Aggregate(lua,reply,*p); break;
    case '_': p = redisProtocolToLuaType_Null(lua,reply); break;
    case '#': p = redisProtocolToLuaType_Bool(lua,reply,p[1]); break;
    case ',': p = redisProtocolToLuaType_Double(lua,reply); break;
    }
    return p;
}