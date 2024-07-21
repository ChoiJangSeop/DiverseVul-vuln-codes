int HeaderMapWrapper::luaGet(lua_State* state) {
  const char* key = luaL_checkstring(state, 2);
  const Http::HeaderEntry* entry = headers_.get(Http::LowerCaseString(key));
  if (entry != nullptr) {
    lua_pushlstring(state, entry->value().getStringView().data(),
                    entry->value().getStringView().length());
    return 1;
  } else {
    return 0;
  }
}