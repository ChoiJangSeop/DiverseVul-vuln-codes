SWFInput_readSBits(SWFInput input, int number)
{
	int num = SWFInput_readBits(input, number);

	if ( num & (1<<(number-1)) )
		return num - (1<<number);
	else
		return num;
}