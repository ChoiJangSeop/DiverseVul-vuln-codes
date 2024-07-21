bool StringMatching::matchString(const char* str1, const char* str2)
{
	 if(PathMatchSpec(str1,str2))
		 return true;
	 if(PathMatchSpec(str2,str1))
	 	 return true;
	 return false;
}