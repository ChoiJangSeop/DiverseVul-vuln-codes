readTile(T& in, bool reduceMemory , bool reduceTime)
{
    bool threw = false;
    try
    {
        const Box2i& dw = in.header().dataWindow();

        int w = dw.max.x - dw.min.x + 1;
        int h = dw.max.y - dw.min.y + 1;
        int dwx = dw.min.x;
        int dwy = dw.min.y;
        int numXLevels = in.numXLevels();
        int numYLevels = in.numYLevels();

        const TileDescription& td = in.header().tileDescription();


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
                case 0 : i.insert(c.name(),Slice(HALF, (char*)&halfChannels[-dwx] , sizeof(half) , 0 , c.channel().xSampling , c.channel().ySampling ));
                break;
                case 1 : i.insert(c.name(),Slice(FLOAT, (char*)&floatChannels[-dwx] , sizeof(float) , 0 ,  c.channel().xSampling , c.channel().ySampling));
                case 2 : i.insert(c.name(),Slice(UINT, (char*)&uintChannels[-dwx] , sizeof(unsigned int) , 0 , c.channel().xSampling , c.channel().ySampling));
                break;
            }
            channelIndex ++;
        }

        in.setFrameBuffer (i);


        //
        // limit to 2,000 tiles
        //
        size_t step = 1;

        if (reduceTime && in.numXTiles(0) * in.numYTiles(0) > 2000)
        {
            step = max(1, (in.numXTiles(0) * in.numYTiles(0)) / 2000 );
        }



        size_t tileIndex =0;
        bool isRipMap = td.mode == RIPMAP_LEVELS;

        //
        // read all tiles from all levels.
        //
        for (int ylevel = 0; ylevel < numYLevels; ++ylevel )
        {
            for (int xlevel = 0; xlevel < numXLevels; ++xlevel )
            {
                for(int y  = 0 ; y < in.numYTiles(ylevel) ; ++y )
                {
                    for(int x = 0 ; x < in.numXTiles(xlevel) ; ++x )
                    {
                        if(tileIndex % step == 0)
                        {
                            try
                            {
                                in.readTile ( x, y, xlevel , ylevel);
                            }
                            catch(...)
                            {
                                //
                                // for one level and mipmapped images,
                                // xlevel must match ylevel,
                                // otherwise an exception is thrown
                                // ignore that exception
                                //
                                if (isRipMap || xlevel==ylevel)
                                {
                                    threw = true;
                                }
                            }
                        }
                        tileIndex++;
                    }
                }
            }
        }
    }
    catch(...)
    {
        threw = true;
    }

    return threw;
}