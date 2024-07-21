generateRandomFile (int partCount, const std::string & fn)
{
    //
    // Init data.
    //
    Array2D<half> halfData;
    Array2D<float> floatData;
    Array2D<unsigned int> uintData;
    fillPixels<unsigned int>(uintData, width, height);
    fillPixels<half>(halfData, width, height);
    fillPixels<float>(floatData, width, height);

    Array2D< Array2D< half > >* tiledHalfData = new Array2D< Array2D< half > >[partCount];
    Array2D< Array2D< float > >* tiledFloatData = new Array2D< Array2D< float > >[partCount];
    Array2D< Array2D< unsigned int > >* tiledUintData = new Array2D< Array2D< unsigned int > >[partCount];

    vector<GenericOutputFile*> outputfiles;
    vector<WritingTaskData> taskList;

    pixelTypes.resize(partCount);
    partTypes.resize(partCount);
    levelModes.resize(partCount);

    //
    // Generate headers and data.
    //
    cout << "Generating headers and data " << flush;
    generateRandomHeaders(partCount, headers, taskList);

    //
    // Shuffle tasks.
    //
    cout << "Shuffling " << taskList.size() << " tasks " << flush;
    int taskListSize = taskList.size();
    for (int i = 0; i < taskListSize; i++)
    {
        int a, b;
        a = random_int(taskListSize);
        b = random_int(taskListSize);
        swap(taskList[a], taskList[b]);
    }

    remove(fn.c_str());
    MultiPartOutputFile file(fn.c_str(), &headers[0],headers.size());

    //
    // Writing tasks.
    //
    cout << "Writing tasks " << flush;

    //
    // Pre-generating frameBuffers.
    //
    vector<void *> parts;
    vector<FrameBuffer> frameBuffers(partCount);
    Array<Array2D<FrameBuffer> >tiledFrameBuffers(partCount);
    for (int i = 0; i < partCount; i++)
    {
        if (partTypes[i] == 0)
        {
            OutputPart* part = new OutputPart(file, i);
            parts.push_back((void*) part);

            FrameBuffer& frameBuffer = frameBuffers[i];

            setOutputFrameBuffer(frameBuffer, pixelTypes[i], uintData, floatData, halfData, width);

            part->setFrameBuffer(frameBuffer);
        }
        else
        {
            TiledOutputPart* part = new TiledOutputPart(file, i);
            parts.push_back((void*) part);

            int numXLevels = part->numXLevels();
            int numYLevels = part->numYLevels();

            // Allocating space.
            switch (pixelTypes[i])
            {
                case 0:
                    tiledUintData[i].resizeErase(numYLevels, numXLevels);
                    break;
                case 1:
                    tiledFloatData[i].resizeErase(numYLevels, numXLevels);
                    break;
                case 2:
                    tiledHalfData[i].resizeErase(numYLevels, numXLevels);
                    break;
            }

            tiledFrameBuffers[i].resizeErase(numYLevels, numXLevels);

            for (int xLevel = 0; xLevel < numXLevels; xLevel++)
                for (int yLevel = 0; yLevel < numYLevels; yLevel++)
                {
                    if (!part->isValidLevel(xLevel, yLevel))
                        continue;

                    int w = part->levelWidth(xLevel);
                    int h = part->levelHeight(yLevel);

                    FrameBuffer& frameBuffer = tiledFrameBuffers[i][yLevel][xLevel];

                    switch (pixelTypes[i])
                    {
                        case 0:
                            fillPixels<unsigned int>(tiledUintData[i][yLevel][xLevel], w, h);
                            break;
                        case 1:
                            fillPixels<float>(tiledFloatData[i][yLevel][xLevel], w, h);
                            break;
                        case 2:
                            fillPixels<half>(tiledHalfData[i][yLevel][xLevel], w, h);
                            break;
                    }
                    setOutputFrameBuffer(frameBuffer, pixelTypes[i],
                                         tiledUintData[i][yLevel][xLevel],
                                         tiledFloatData[i][yLevel][xLevel],
                                         tiledHalfData[i][yLevel][xLevel],
                                         w);
                }
        }
    }

    //
    // Writing tasks.
    //
    TaskGroup taskGroup;
    ThreadPool* threadPool = new ThreadPool(32);
    vector<WritingTaskData*> list;
    for (int i = 0; i < taskListSize; i++)
    {
        list.push_back(&taskList[i]);
        if (i % 10 == 0 || i == taskListSize - 1)
        {
            threadPool->addTask(
                (new WritingTask (&taskGroup, &file, list, tiledFrameBuffers)));
            list.clear();
        }
    }

    delete threadPool;

    delete[] tiledHalfData;
    delete[] tiledUintData;
    delete[] tiledFloatData;
}