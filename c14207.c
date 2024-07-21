bool StringMatching::matchString(const char* str1, const char* str2)
{
	if(fnmatch(str1,str2,FNM_NOESCAPE)==0)
		return true;
	if(fnmatch(str2,str1,FNM_NOESCAPE)==0)
		return true;
	return false;
}