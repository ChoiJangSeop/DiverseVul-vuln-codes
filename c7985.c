TEST(HeaderMapImplTest, PseudoHeaderOrder) {
  using MockCb = testing::MockFunction<void(const std::string&, const std::string&)>;
  MockCb cb;

  {
    LowerCaseString foo("hello");
    Http::TestHeaderMapImpl headers{};
    EXPECT_EQ(0UL, headers.size());
    EXPECT_TRUE(headers.empty());

    headers.addReferenceKey(foo, "world");
    EXPECT_EQ(1UL, headers.size());
    EXPECT_FALSE(headers.empty());

    headers.setReferenceKey(Headers::get().ContentType, "text/html");
    EXPECT_EQ(2UL, headers.size());
    EXPECT_FALSE(headers.empty());

    // Pseudo header gets inserted before non-pseudo headers
    headers.setReferenceKey(Headers::get().Method, "PUT");
    EXPECT_EQ(3UL, headers.size());
    EXPECT_FALSE(headers.empty());

    InSequence seq;
    EXPECT_CALL(cb, Call(":method", "PUT"));
    EXPECT_CALL(cb, Call("hello", "world"));
    EXPECT_CALL(cb, Call("content-type", "text/html"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);

    // Removal of the header before which pseudo-headers are inserted
    headers.remove(foo);
    EXPECT_EQ(2UL, headers.size());
    EXPECT_FALSE(headers.empty());

    EXPECT_CALL(cb, Call(":method", "PUT"));
    EXPECT_CALL(cb, Call("content-type", "text/html"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);

    // Next pseudo-header goes after other pseudo-headers, but before normal headers
    headers.setReferenceKey(Headers::get().Path, "/test");
    EXPECT_EQ(3UL, headers.size());
    EXPECT_FALSE(headers.empty());

    EXPECT_CALL(cb, Call(":method", "PUT"));
    EXPECT_CALL(cb, Call(":path", "/test"));
    EXPECT_CALL(cb, Call("content-type", "text/html"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);

    // Removing the last normal header
    headers.remove(Headers::get().ContentType);
    EXPECT_EQ(2UL, headers.size());
    EXPECT_FALSE(headers.empty());

    EXPECT_CALL(cb, Call(":method", "PUT"));
    EXPECT_CALL(cb, Call(":path", "/test"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);

    // Adding a new pseudo-header after removing the last normal header
    headers.setReferenceKey(Headers::get().Host, "host");
    EXPECT_EQ(3UL, headers.size());
    EXPECT_FALSE(headers.empty());

    EXPECT_CALL(cb, Call(":method", "PUT"));
    EXPECT_CALL(cb, Call(":path", "/test"));
    EXPECT_CALL(cb, Call(":authority", "host"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);

    // Adding the first normal header
    headers.setReferenceKey(Headers::get().ContentType, "text/html");
    EXPECT_EQ(4UL, headers.size());
    EXPECT_FALSE(headers.empty());

    EXPECT_CALL(cb, Call(":method", "PUT"));
    EXPECT_CALL(cb, Call(":path", "/test"));
    EXPECT_CALL(cb, Call(":authority", "host"));
    EXPECT_CALL(cb, Call("content-type", "text/html"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);

    // Removing all pseudo-headers
    headers.remove(Headers::get().Path);
    headers.remove(Headers::get().Method);
    headers.remove(Headers::get().Host);
    EXPECT_EQ(1UL, headers.size());
    EXPECT_FALSE(headers.empty());

    EXPECT_CALL(cb, Call("content-type", "text/html"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);

    // Removing all headers
    headers.remove(Headers::get().ContentType);
    EXPECT_EQ(0UL, headers.size());
    EXPECT_TRUE(headers.empty());

    // Adding a lone pseudo-header
    headers.setReferenceKey(Headers::get().Status, "200");
    EXPECT_EQ(1UL, headers.size());
    EXPECT_FALSE(headers.empty());

    EXPECT_CALL(cb, Call(":status", "200"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);
  }

  // Starting with a normal header
  {
    Http::TestHeaderMapImpl headers{{"content-type", "text/plain"},
                                    {":method", "GET"},
                                    {":path", "/"},
                                    {"hello", "world"},
                                    {":authority", "host"}};

    InSequence seq;
    EXPECT_CALL(cb, Call(":method", "GET"));
    EXPECT_CALL(cb, Call(":path", "/"));
    EXPECT_CALL(cb, Call(":authority", "host"));
    EXPECT_CALL(cb, Call("content-type", "text/plain"));
    EXPECT_CALL(cb, Call("hello", "world"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);
  }

  // Starting with a pseudo-header
  {
    Http::TestHeaderMapImpl headers{{":path", "/"},
                                    {"content-type", "text/plain"},
                                    {":method", "GET"},
                                    {"hello", "world"},
                                    {":authority", "host"}};

    InSequence seq;
    EXPECT_CALL(cb, Call(":path", "/"));
    EXPECT_CALL(cb, Call(":method", "GET"));
    EXPECT_CALL(cb, Call(":authority", "host"));
    EXPECT_CALL(cb, Call("content-type", "text/plain"));
    EXPECT_CALL(cb, Call("hello", "world"));

    headers.iterate(
        [](const Http::HeaderEntry& header, void* cb_v) -> HeaderMap::Iterate {
          static_cast<MockCb*>(cb_v)->Call(std::string(header.key().getStringView()),
                                           std::string(header.value().getStringView()));
          return HeaderMap::Iterate::Continue;
        },
        &cb);
  }
}