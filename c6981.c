std::string HttpConnection::eraseConfidentialInfo(const std::string& request)
{
  std::istringstream istr(request);
  std::string result;
  std::string line;
  while (getline(istr, line)) {
    if (util::startsWith(line, "Authorization: Basic")) {
      result += "Authorization: Basic ********\n";
    }
    else if (util::startsWith(line, "Proxy-Authorization: Basic")) {
      result += "Proxy-Authorization: Basic ********\n";
    }
    else {
      result += line;
      result += "\n";
    }
  }
  return result;
}