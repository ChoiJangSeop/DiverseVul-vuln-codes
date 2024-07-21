void readFile (int channelCount,
               bool bulkRead,
               bool relativeCoords,
               bool randomChannels,
               const std::string & filename)
{
    if (relativeCoords)
        assert(bulkRead == false);

    cout << "reading " << flush;

    DeepTiledInputFile file (filename.c_str(), 4);

    const Header& fileHeader = file.header();
    assert (fileHeader.displayWindow() == header.displayWindow());
    assert (fileHeader.dataWindow() == header.dataWindow());
    assert (fileHeader.pixelAspectRatio() == header.pixelAspectRatio());
    assert (fileHeader.screenWindowCenter() == header.screenWindowCenter());
    assert (fileHeader.screenWindowWidth() == header.screenWindowWidth());
    assert (fileHeader.lineOrder() == header.lineOrder());
    assert (fileHeader.compression() == header.compression());
    assert (fileHeader.channels() == header.channels());
    assert (fileHeader.type() == header.type());
    assert (fileHeader.tileDescription() == header.tileDescription());

    Array2D<unsigned int> localSampleCount;
    localSampleCount.resizeErase(height, width);
    
    // also test filling channels. Generate up to 2 extra channels
    int fillChannels=random_int(3);
    
    Array<Array2D< void* > > data(channelCount+fillChannels);
    for (int i = 0; i < channelCount+fillChannels; i++)
        data[i].resizeErase(height, width);

    DeepFrameBuffer frameBuffer;

    int memOffset;
    if (relativeCoords)
        memOffset = 0;
    else
        memOffset = dataWindow.min.x + dataWindow.min.y * width;
    frameBuffer.insertSampleCountSlice (Slice (IMF::UINT,
                                        (char *) (&localSampleCount[0][0] - memOffset),
                                        sizeof (unsigned int) * 1,
                                        sizeof (unsigned int) * width,
                                        1, 1,
                                        0,
                                        relativeCoords,
                                        relativeCoords));

    
    vector<int> read_channel(channelCount);
    int channels_added=0;
    for (int i = 0; i < channelCount; i++)
    {
         if(randomChannels)
        {
	     read_channel[i] = random_int(2);
	     
        }
        if(!randomChannels || read_channel[i]==1)
        {
            PixelType type = IMF::NUM_PIXELTYPES;
            if (channelTypes[i] == 0)
                type = IMF::UINT;
            if (channelTypes[i] == 1)
                type = IMF::HALF;
            if (channelTypes[i] == 2)
                type = IMF::FLOAT;

            stringstream ss;
            ss << i;
            string str = ss.str();

            int sampleSize = 0;
            if (channelTypes[i] == 0) sampleSize = sizeof (unsigned int);
            if (channelTypes[i] == 1) sampleSize = sizeof (half);
            if (channelTypes[i] == 2) sampleSize = sizeof (float);

            int pointerSize = sizeof (char *);

            frameBuffer.insert (str,
                                DeepSlice (type,
                                (char *) (&data[i][0][0] - memOffset),
                                pointerSize * 1,
                                pointerSize * width,
                                sampleSize,
                                1, 1,
                                0,
                                relativeCoords,
                                relativeCoords));
                channels_added++;
            }
    }
     if(channels_added==0)
    {
      cout << "skipping " <<flush;
      return;
    }
    for(int i = 0 ; i < fillChannels ; ++i )
    { 
        PixelType type  = IMF::FLOAT;
        int sampleSize = sizeof(float);            
        int pointerSize = sizeof (char *);
        stringstream ss;
        // generate channel names that aren't in file but (might) interleave with existing file
        ss << i << "fill";
        string str = ss.str();
        frameBuffer.insert (str,
                            DeepSlice (type,
                            (char *) (&data[i+channelCount][0][0] - memOffset),
                            pointerSize * 1,
                            pointerSize * width,
                            sampleSize,
                            1, 1,
                            0,
                            relativeCoords,
                            relativeCoords));
    }

    file.setFrameBuffer(frameBuffer);

    if (bulkRead)
        cout << "bulk " << flush;
    else
    {
        if (relativeCoords == false)
            cout << "per-tile " << flush;
        else
            cout << "per-tile with relative coordinates " << flush;
    }

    for (int ly = 0; ly < file.numYLevels(); ly++)
        for (int lx = 0; lx < file.numXLevels(); lx++)
        {
            Box2i dataWindowL = file.dataWindowForLevel(lx, ly);

            if (bulkRead)
            {
                //
                // Testing bulk read (without relative coordinates).
                //

                file.readPixelSampleCounts(0, file.numXTiles(lx) - 1, 0, file.numYTiles(ly) - 1, lx, ly);

                for (int i = 0; i < file.numYTiles(ly); i++)
                {
                    for (int j = 0; j < file.numXTiles(lx); j++)
                    {
                        Box2i box = file.dataWindowForTile(j, i, lx, ly);
                        for (int y = box.min.y; y <= box.max.y; y++)
                            for (int x = box.min.x; x <= box.max.x; x++)
                            {
                                int dwy = y - dataWindowL.min.y;
                                int dwx = x - dataWindowL.min.x;
                                assert(localSampleCount[dwy][dwx] == sampleCountWhole[ly][lx][dwy][dwx]);

                                for (size_t k = 0; k < channelTypes.size(); k++)
                                {
                                    if (channelTypes[k] == 0)
                                        data[k][dwy][dwx] = new unsigned int[localSampleCount[dwy][dwx]];
                                    if (channelTypes[k] == 1)
                                        data[k][dwy][dwx] = new half[localSampleCount[dwy][dwx]];
                                    if (channelTypes[k] == 2)
                                        data[k][dwy][dwx] = new float[localSampleCount[dwy][dwx]];
                                }
                                
                                for( int f = 0 ; f < fillChannels ; ++f )
                                {
                                     data[f + channelTypes.size()][dwy][dwx] = new float[localSampleCount[dwy][dwx]];
                                }
                                
                            }
                            
                    }
                }

                file.readTiles(0, file.numXTiles(lx) - 1, 0, file.numYTiles(ly) - 1, lx, ly);
            }
            else if (bulkRead == false)
            {
                if (relativeCoords == false)
                {
                    //
                    // Testing per-tile read without relative coordinates.
                    //

                    for (int i = 0; i < file.numYTiles(ly); i++)
                    {
                        for (int j = 0; j < file.numXTiles(lx); j++)
                        {
                            file.readPixelSampleCount(j, i, lx, ly);

                            Box2i box = file.dataWindowForTile(j, i, lx, ly);
                            for (int y = box.min.y; y <= box.max.y; y++)
                                for (int x = box.min.x; x <= box.max.x; x++)
                                {
                                    int dwy = y - dataWindowL.min.y;
                                    int dwx = x - dataWindowL.min.x;
                                    assert(localSampleCount[dwy][dwx] == sampleCountWhole[ly][lx][dwy][dwx]);

                                    for (size_t k = 0; k < channelTypes.size(); k++)
                                    {
                                        if (channelTypes[k] == 0)
                                            data[k][dwy][dwx] = new unsigned int[localSampleCount[dwy][dwx]];
                                        if (channelTypes[k] == 1)
                                            data[k][dwy][dwx] = new half[localSampleCount[dwy][dwx]];
                                        if (channelTypes[k] == 2)
                                            data[k][dwy][dwx] = new float[localSampleCount[dwy][dwx]];
                                    }
                                    for( int f = 0 ; f < fillChannels ; ++f )
                                    {
                                       data[f + channelTypes.size()][dwy][dwx] = new float[localSampleCount[dwy][dwx]];
                                    }
                                }

                            file.readTile(j, i, lx, ly);
                        }
                    }
                }
                else if (relativeCoords)
                {
                    //
                    // Testing per-tile read with relative coordinates.
                    //

                    for (int i = 0; i < file.numYTiles(ly); i++)
                    {
                        for (int j = 0; j < file.numXTiles(lx); j++)
                        {
                            file.readPixelSampleCount(j, i, lx, ly);

                            Box2i box = file.dataWindowForTile(j, i, lx, ly);
                            for (int y = box.min.y; y <= box.max.y; y++)
                                for (int x = box.min.x; x <= box.max.x; x++)
                                {
                                    int dwy = y - dataWindowL.min.y;
                                    int dwx = x - dataWindowL.min.x;
                                    int ty = y - box.min.y;
                                    int tx = x - box.min.x;
                                    assert(localSampleCount[ty][tx] == sampleCountWhole[ly][lx][dwy][dwx]);

                                    for (size_t k = 0; k < channelTypes.size(); k++)
                                    {
                                          if( !randomChannels || read_channel[k]==1)
                                          {
                                                if (channelTypes[k] == 0)
                                                    data[k][ty][tx] = new unsigned int[localSampleCount[ty][tx]];
                                                if (channelTypes[k] == 1)
                                                    data[k][ty][tx] = new half[localSampleCount[ty][tx]];
                                                if (channelTypes[k] == 2)
                                                    data[k][ty][tx] = new float[localSampleCount[ty][tx]];
                                          }
                                            
                                         
                                    }
                                    for( int f = 0 ; f < fillChannels ; ++f )
                                    {
                                        data[f + channelTypes.size()][ty][tx] = new float[localSampleCount[ty][tx]];
                                    }
                                }

                            file.readTile(j, i, lx, ly);

                            for (int y = box.min.y; y <= box.max.y; y++)
                                for (int x = box.min.x; x <= box.max.x; x++)
                                {
                                    int dwy = y - dataWindowL.min.y;
                                    int dwx = x - dataWindowL.min.x;
                                    int ty = y - box.min.y;
                                    int tx = x - box.min.x;

                                    for (size_t k = 0; k < channelTypes.size(); k++)
                                    {
                                         if( !randomChannels || read_channel[k]==1)
                                         {
                                                checkValue(data[k][ty][tx],
                                                        localSampleCount[ty][tx],
                                                        channelTypes[k],
                                                        dwx, dwy);
                                                if (channelTypes[k] == 0)
                                                    delete[] (unsigned int*) data[k][ty][tx];
                                                if (channelTypes[k] == 1)
                                                    delete[] (half*) data[k][ty][tx];
                                                if (channelTypes[k] == 2)
                                                    delete[] (float*) data[k][ty][tx];
                                         }
                                    }
                                    for( int f = 0 ; f < fillChannels ; ++f )
                                    {
                                         delete[] (float*) data[f+channelTypes.size()][ty][tx];
                                    }
                                }
                        }
                    }
                }
            }

            if (relativeCoords == false)
            {
                for (int i = 0; i < file.levelHeight(ly); i++)
                    for (int j = 0; j < file.levelWidth(lx); j++)
                        for (int k = 0; k < channelCount; k++)
                        {
                            if( !randomChannels || read_channel[k]==1)
                            {
                                for (unsigned int l = 0; l < localSampleCount[i][j]; l++)
                                {
                                    if (channelTypes[k] == 0)
                                    {
                                        unsigned int* value = (unsigned int*)(data[k][i][j]);
                                        if (value[l] != static_cast<unsigned int>(i * width + j) % 2049)
                                            cout << j << ", " << i << " error, should be "
                                                << (i * width + j) % 2049 << ", is " << value[l]
                                                << endl << flush;
                                        assert (value[l] == static_cast<unsigned int>(i * width + j) % 2049);
                                    }
                                    if (channelTypes[k] == 1)
                                    {
                                        half* value = (half*)(data[k][i][j]);
                                        if (value[l] != (i * width + j) % 2049)
                                            cout << j << ", " << i << " error, should be "
                                                << (i * width + j) % 2049 << ", is " << value[l]
                                                << endl << flush;
                                        assert (((half*)(data[k][i][j]))[l] == (i * width + j) % 2049);
                                    }
                                    if (channelTypes[k] == 2)
                                    {
                                        float* value = (float*)(data[k][i][j]);
                                        if (value[l] != (i * width + j) % 2049)
                                            cout << j << ", " << i << " error, should be "
                                                << (i * width + j) % 2049 << ", is " << value[l]
                                                << endl << flush;
                                        assert (((float*)(data[k][i][j]))[l] == (i * width + j) % 2049);
                                    }
                                }
                            }
                        }

                for (int i = 0; i < file.levelHeight(ly); i++)
                    for (int j = 0; j < file.levelWidth(lx); j++)
                    {
                        for (int k = 0; k < channelCount; k++)
                        {
                            if( !randomChannels || read_channel[k]==1)
                            {
                                if (channelTypes[k] == 0)
                                    delete[] (unsigned int*) data[k][i][j];
                                if (channelTypes[k] == 1)
                                    delete[] (half*) data[k][i][j];
                                if (channelTypes[k] == 2)
                                    delete[] (float*) data[k][i][j];
                            }
                        }
                        for( int f = 0 ; f < fillChannels ; ++f )
                        {
                            delete[] (float*) data[f+channelTypes.size()][i][j];
                        }
                    }
            }
        }
}