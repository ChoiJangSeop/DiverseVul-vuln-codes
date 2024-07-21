TEST(Url, ParsingTest) {
  // Test url with no explicit path (with and without port).
  validateUrl("http://www.host.com", "http", "www.host.com", "/", 80);
  validateUrl("http://www.host.com:80", "http", "www.host.com", "/", 80);

  // Test url with "/" path.
  validateUrl("http://www.host.com:80/", "http", "www.host.com", "/", 80);
  validateUrl("http://www.host.com/", "http", "www.host.com", "/", 80);

  // Test url with "?".
  validateUrl("http://www.host.com:80/?", "http", "www.host.com", "/?", 80);
  validateUrl("http://www.host.com/?", "http", "www.host.com", "/?", 80);

  // Test url with "?" but without slash.
  validateUrl("http://www.host.com:80?", "http", "www.host.com", "/?", 80);
  validateUrl("http://www.host.com?", "http", "www.host.com", "/?", 80);

  // Test url with multi-character path.
  validateUrl("http://www.host.com:80/path", "http", "www.host.com", "/path", 80);
  validateUrl("http://www.host.com/path", "http", "www.host.com", "/path", 80);

  // Test url with multi-character path and ? at the end.
  validateUrl("http://www.host.com:80/path?", "http", "www.host.com", "/path?", 80);
  validateUrl("http://www.host.com/path?", "http", "www.host.com", "/path?", 80);

  // Test https scheme.
  validateUrl("https://www.host.com", "https", "www.host.com", "/", 443);

  // Test url with query parameter.
  validateUrl("http://www.host.com:80/?query=param", "http", "www.host.com", "/?query=param", 80);
  validateUrl("http://www.host.com/?query=param", "http", "www.host.com", "/?query=param", 80);

  // Test url with query parameter but without slash. It will be normalized.
  validateUrl("http://www.host.com:80?query=param", "http", "www.host.com", "/?query=param", 80);
  validateUrl("http://www.host.com?query=param", "http", "www.host.com", "/?query=param", 80);

  // Test url with multi-character path and query parameter.
  validateUrl("http://www.host.com:80/path?query=param", "http", "www.host.com",
              "/path?query=param", 80);
  validateUrl("http://www.host.com/path?query=param", "http", "www.host.com", "/path?query=param",
              80);

  // Test url with multi-character path and more than one query parameter.
  validateUrl("http://www.host.com:80/path?query=param&query2=param2", "http", "www.host.com",
              "/path?query=param&query2=param2", 80);
  validateUrl("http://www.host.com/path?query=param&query2=param2", "http", "www.host.com",
              "/path?query=param&query2=param2", 80);

  // Test url with multi-character path, more than one query parameter and fragment
  validateUrl("http://www.host.com:80/path?query=param&query2=param2#fragment", "http",
              "www.host.com", "/path?query=param&query2=param2#fragment", 80);
  validateUrl("http://www.host.com/path?query=param&query2=param2#fragment", "http", "www.host.com",
              "/path?query=param&query2=param2#fragment", 80);

  // Test url with non-default ports.
  validateUrl("https://www.host.com:8443", "https", "www.host.com:8443", "/", 8443);
  validateUrl("http://www.host.com:8080", "http", "www.host.com:8080", "/", 8080);
}