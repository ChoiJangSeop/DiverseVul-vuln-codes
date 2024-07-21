bool HaarEvaluator::read(const FileNode& node, Size _origWinSize)
{
    if (!FeatureEvaluator::read(node, _origWinSize))
        return false;
    size_t i, n = node.size();
    CV_Assert(n > 0);
    if(features.empty())
        features = makePtr<std::vector<Feature> >();
    if(optfeatures.empty())
        optfeatures = makePtr<std::vector<OptFeature> >();
    if (optfeatures_lbuf.empty())
        optfeatures_lbuf = makePtr<std::vector<OptFeature> >();
    features->resize(n);
    FileNodeIterator it = node.begin();
    hasTiltedFeatures = false;
    std::vector<Feature>& ff = *features;
    sbufSize = Size();
    ufbuf.release();

    for(i = 0; i < n; i++, ++it)
    {
        if(!ff[i].read(*it))
            return false;
        if( ff[i].tilted )
            hasTiltedFeatures = true;
    }
    nchannels = hasTiltedFeatures ? 3 : 2;
    normrect = Rect(1, 1, origWinSize.width - 2, origWinSize.height - 2);

    localSize = lbufSize = Size(0, 0);
    if (ocl::isOpenCLActivated())
    {
        if (ocl::Device::getDefault().isAMD() || ocl::Device::getDefault().isIntel() || ocl::Device::getDefault().isNVidia())
        {
            localSize = Size(8, 8);
            lbufSize = Size(origWinSize.width + localSize.width,
                            origWinSize.height + localSize.height);
            if (lbufSize.area() > 1024)
                lbufSize = Size(0, 0);
        }
    }

    return true;
}