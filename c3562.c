static int ntop_generate_csrf_value(lua_State* vm) {
  char random_a[32], random_b[32], csrf[33];
  Redis *redis = ntop->getRedis();

  ntop->getTrace()->traceEvent(TRACE_INFO, "%s() called", __FUNCTION__);

#ifdef __OpenBSD__
  snprintf(random_a, sizeof(random_a), "%d", arc4random());
  snprintf(random_b, sizeof(random_b), "%lu", time(NULL)*arc4random());
#else
  snprintf(random_a, sizeof(random_a), "%d", rand());
  snprintf(random_b, sizeof(random_b), "%lu", time(NULL)*rand());
#endif

  mg_md5(csrf, random_a, random_b, NULL);

  redis->set(csrf, (char*)"1", MAX_CSRF_DURATION);
  lua_pushfstring(vm, "%s", csrf);
  return(CONST_LUA_OK);
}