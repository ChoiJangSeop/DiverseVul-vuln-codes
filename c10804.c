void luaReplyToRedisReply(client *c, lua_State *lua) {
    int t = lua_type(lua,-1);

    switch(t) {
    case LUA_TSTRING:
        addReplyBulkCBuffer(c,(char*)lua_tostring(lua,-1),lua_strlen(lua,-1));
        break;
    case LUA_TBOOLEAN:
        if (server.lua_client->resp == 2)
            addReply(c,lua_toboolean(lua,-1) ? shared.cone :
                                               shared.null[c->resp]);
        else
            addReplyBool(c,lua_toboolean(lua,-1));
        break;
    case LUA_TNUMBER:
        addReplyLongLong(c,(long long)lua_tonumber(lua,-1));
        break;
    case LUA_TTABLE:
        /* We need to check if it is an array, an error, or a status reply.
         * Error are returned as a single element table with 'err' field.
         * Status replies are returned as single element table with 'ok'
         * field. */

        /* Handle error reply. */
        lua_pushstring(lua,"err");
        lua_gettable(lua,-2);
        t = lua_type(lua,-1);
        if (t == LUA_TSTRING) {
            sds err = sdsnew(lua_tostring(lua,-1));
            sdsmapchars(err,"\r\n","  ",2);
            addReplySds(c,sdscatprintf(sdsempty(),"-%s\r\n",err));
            sdsfree(err);
            lua_pop(lua,2);
            return;
        }
        lua_pop(lua,1); /* Discard field name pushed before. */

        /* Handle status reply. */
        lua_pushstring(lua,"ok");
        lua_gettable(lua,-2);
        t = lua_type(lua,-1);
        if (t == LUA_TSTRING) {
            sds ok = sdsnew(lua_tostring(lua,-1));
            sdsmapchars(ok,"\r\n","  ",2);
            addReplySds(c,sdscatprintf(sdsempty(),"+%s\r\n",ok));
            sdsfree(ok);
            lua_pop(lua,2);
            return;
        }
        lua_pop(lua,1); /* Discard field name pushed before. */

        /* Handle double reply. */
        lua_pushstring(lua,"double");
        lua_gettable(lua,-2);
        t = lua_type(lua,-1);
        if (t == LUA_TNUMBER) {
            addReplyDouble(c,lua_tonumber(lua,-1));
            lua_pop(lua,2);
            return;
        }
        lua_pop(lua,1); /* Discard field name pushed before. */

        /* Handle map reply. */
        lua_pushstring(lua,"map");
        lua_gettable(lua,-2);
        t = lua_type(lua,-1);
        if (t == LUA_TTABLE) {
            int maplen = 0;
            void *replylen = addReplyDeferredLen(c);
            lua_pushnil(lua); /* Use nil to start iteration. */
            while (lua_next(lua,-2)) {
                /* Stack now: table, key, value */
                lua_pushvalue(lua,-2);        /* Dup key before consuming. */
                luaReplyToRedisReply(c, lua); /* Return key. */
                luaReplyToRedisReply(c, lua); /* Return value. */
                /* Stack now: table, key. */
                maplen++;
            }
            setDeferredMapLen(c,replylen,maplen);
            lua_pop(lua,2);
            return;
        }
        lua_pop(lua,1); /* Discard field name pushed before. */

        /* Handle set reply. */
        lua_pushstring(lua,"set");
        lua_gettable(lua,-2);
        t = lua_type(lua,-1);
        if (t == LUA_TTABLE) {
            int setlen = 0;
            void *replylen = addReplyDeferredLen(c);
            lua_pushnil(lua); /* Use nil to start iteration. */
            while (lua_next(lua,-2)) {
                /* Stack now: table, key, true */
                lua_pop(lua,1);               /* Discard the boolean value. */
                lua_pushvalue(lua,-1);        /* Dup key before consuming. */
                luaReplyToRedisReply(c, lua); /* Return key. */
                /* Stack now: table, key. */
                setlen++;
            }
            setDeferredSetLen(c,replylen,setlen);
            lua_pop(lua,2);
            return;
        }
        lua_pop(lua,1); /* Discard field name pushed before. */

        /* Handle the array reply. */
        void *replylen = addReplyDeferredLen(c);
        int j = 1, mbulklen = 0;
        while(1) {
            lua_pushnumber(lua,j++);
            lua_gettable(lua,-2);
            t = lua_type(lua,-1);
            if (t == LUA_TNIL) {
                lua_pop(lua,1);
                break;
            }
            luaReplyToRedisReply(c, lua);
            mbulklen++;
        }
        setDeferredArrayLen(c,replylen,mbulklen);
        break;
    default:
        addReplyNull(c);
    }
    lua_pop(lua,1);
}