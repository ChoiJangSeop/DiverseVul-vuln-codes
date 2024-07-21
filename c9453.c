TEST(Url, ParsingForConnectTest) {
  validateConnectUrl("host.com:443", "host.com:443", 443);
  validateConnectUrl("host.com:80", "host.com:80", 80);
}