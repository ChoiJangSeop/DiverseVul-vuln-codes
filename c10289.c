readScanline(T& in, bool reduceMemory , bool reduceTime)
{

    bool threw = false;

    try
    {
        const Box2i &dw = in.header().dataWindow();

        int w = dw.max.x - dw.min.x + 1;
        int dx = dw.min.x;

        if (reduceMemory && w > (1 << 10))
        {
            return false;
        }

        FrameBuffer i;


        // read all channels present (later channels will overwrite earlier ones)
        vector<half> halfChannels(w);
        vector<float> floatChannels(w);
        vector<unsigned int> uintChannels(w);

        int channelIndex = 0;
        const ChannelList& channelList = in.header().channels();
        for (ChannelList::ConstIterator c = channelList.begin() ; c != channelList.end() ; ++c )
        {
            switch (channelIndex % 3)
            {
                case 0 : i.insert(c.name(),Slice(HALF, (char*)&halfChannels[-dx] , sizeof(half) , 0 , c.channel().xSampling , c.channel().ySampling ));
                break;
                case 1 : i.insert(c.name(),Slice(FLOAT, (char*)&floatChannels[-dx] , sizeof(float) , 0 , c.channel().xSampling , c.channel().ySampling ));
                case 2 : i.insert(c.name(),Slice(UINT, (char*)&uintChannels[-dx] , sizeof(unsigned int) , 0 , c.channel().xSampling , c.channel().ySampling ));
                break;
            }
            channelIndex ++;
        }

        in.setFrameBuffer(i);

        int step = getStep( dw , reduceTime );



        //
        // try reading scanlines. Continue reading scanlines
        // even if an exception is encountered
        //
        for (int y = dw.min.y; y <= dw.max.y; y+=step )
        {
            try
            {
               in.readPixels (y);
            }
            catch(...)
            {
                threw = true;
            }
        }
    }
    catch(...)
    {
        threw = true;
    }

    return threw;
}