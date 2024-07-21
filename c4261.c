bool chopOff(string &domain) 
{
  if(domain.empty())
    return false;

  string::size_type fdot=domain.find('.');

  if(fdot==string::npos) 
    domain="";
  else {
    string::size_type remain = domain.length() - (fdot + 1);
    char tmp[remain];
    memcpy(tmp, domain.c_str()+fdot+1, remain);
    domain.assign(tmp, remain); // don't dare to do this w/o tmp holder :-)
  }
  return true;
}