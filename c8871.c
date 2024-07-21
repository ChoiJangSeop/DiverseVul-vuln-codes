bytesPerLineTable (const Header &header,
		   vector<size_t> &bytesPerLine)
{
    const Box2i &dataWindow = header.dataWindow();
    const ChannelList &channels = header.channels();

    bytesPerLine.resize (dataWindow.max.y - dataWindow.min.y + 1);

    for (ChannelList::ConstIterator c = channels.begin();
	 c != channels.end();
	 ++c)
    {
	int nBytes = pixelTypeSize (c.channel().type) *
		     (dataWindow.max.x - dataWindow.min.x + 1) /
		     c.channel().xSampling;

	for (int y = dataWindow.min.y, i = 0; y <= dataWindow.max.y; ++y, ++i)
	    if (modp (y, c.channel().ySampling) == 0)
		bytesPerLine[i] += nBytes;
    }

    size_t maxBytesPerLine = 0;

    for (int y = dataWindow.min.y, i = 0; y <= dataWindow.max.y; ++y, ++i)
	if (maxBytesPerLine < bytesPerLine[i])
	    maxBytesPerLine = bytesPerLine[i];

    return maxBytesPerLine;
}