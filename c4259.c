bool chopOffDotted(string &domain)
{
  if(domain.empty() || (domain.size()==1 && domain[0]=='.'))
    return false;

  string::size_type fdot=domain.find('.');
  if(fdot == string::npos)
    return false;

  if(fdot==domain.size()-1) 
    domain=".";
  else  {
    string::size_type remain = domain.length() - (fdot + 1);
    char tmp[remain];
    memcpy(tmp, domain.c_str()+fdot+1, remain);
    domain.assign(tmp, remain);
  }
  return true;
}