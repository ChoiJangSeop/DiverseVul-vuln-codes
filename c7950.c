bool LBPEvaluator::read( const FileNode& node, Size _origWinSize )
{
    if (!FeatureEvaluator::read(node, _origWinSize))
        return false;
    if(features.empty())
        features = makePtr<std::vector<Feature> >();
    if(optfeatures.empty())
        optfeatures = makePtr<std::vector<OptFeature> >();
    if (optfeatures_lbuf.empty())
        optfeatures_lbuf = makePtr<std::vector<OptFeature> >();

    features->resize(node.size());
    optfeaturesPtr = 0;
    FileNodeIterator it = node.begin(), it_end = node.end();
    std::vector<Feature>& ff = *features;
    for(int i = 0; it != it_end; ++it, i++)
    {
        if(!ff[i].read(*it))
            return false;
    }
    nchannels = 1;
    localSize = lbufSize = Size(0, 0);
    if (ocl::isOpenCLActivated())
        localSize = Size(8, 8);

    return true;
}