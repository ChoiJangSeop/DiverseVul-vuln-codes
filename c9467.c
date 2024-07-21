TEST_F(LuaHeaderMapWrapperTest, Methods) {
  const std::string SCRIPT{R"EOF(
    function callMe(object)
      object:add("HELLO", "WORLD")
      testPrint(object:get("hELLo"))

      object:add("header1", "")
      object:add("header2", "foo")

      for key, value in pairs(object) do
        testPrint(string.format("'%s' '%s'", key, value))
      end

      object:remove("header1")
      for key, value in pairs(object) do
        testPrint(string.format("'%s' '%s'", key, value))
      end
    end
  )EOF"};

  InSequence s;
  setup(SCRIPT);

  Http::TestRequestHeaderMapImpl headers;
  HeaderMapWrapper::create(coroutine_->luaState(), headers, []() { return true; });
  EXPECT_CALL(printer_, testPrint("WORLD"));
  EXPECT_CALL(printer_, testPrint("'hello' 'WORLD'"));
  EXPECT_CALL(printer_, testPrint("'header1' ''"));
  EXPECT_CALL(printer_, testPrint("'header2' 'foo'"));
  EXPECT_CALL(printer_, testPrint("'hello' 'WORLD'"));
  EXPECT_CALL(printer_, testPrint("'header2' 'foo'"));
  start("callMe");
}