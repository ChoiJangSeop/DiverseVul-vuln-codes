static void ConvertLoopSequence(ModSample &smp, STPLoopList &loopList)
{
	// This should only modify a sample if it has more than one loop
	// (otherwise, it behaves like a normal sample loop)
	if(!smp.HasSampleData() || loopList.size() < 2) return;

	ModSample newSmp = smp;
	newSmp.nLength = 0;
	newSmp.pSample = nullptr;

	size_t numLoops = loopList.size();

	// Get the total length of the sample after combining all looped sections
	for(size_t i = 0; i < numLoops; i++)
	{
		STPLoopInfo &info = loopList[i];

		// If adding this loop would cause the sample length to exceed maximum,
		// then limit and bail out
		if((newSmp.nLength + info.loopLength > MAX_SAMPLE_LENGTH) ||
		   (info.loopLength > MAX_SAMPLE_LENGTH) ||
		   (info.loopStart + info.loopLength > smp.nLength))
		{
			numLoops = i;
			break;
		}

		newSmp.nLength += info.loopLength;
	}

	if(!newSmp.AllocateSample())
	{
		return;
	}

	// start copying the looped sample data parts
	SmpLength start = 0;

	for(size_t i = 0; i < numLoops; i++)
	{
		STPLoopInfo &info = loopList[i];

		memcpy(newSmp.pSample8 + start, smp.pSample8 + info.loopStart, info.loopLength);

		// update loop info based on position in edited sample
		info.loopStart = start;
		if(i > 0 && i <= mpt::size(newSmp.cues))
		{
			newSmp.cues[i - 1] = start;
		}
		start += info.loopLength;
	}

	// replace old sample with new one
	smp.FreeSample();
	smp = newSmp;

	smp.nLoopStart = 0;
	smp.nLoopEnd = smp.nLength;
	smp.uFlags.set(CHN_LOOP);
}