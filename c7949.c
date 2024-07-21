bool CascadeClassifierImpl::Data::read(const FileNode &root)
{
    static const float THRESHOLD_EPS = 1e-5f;

    // load stage params
    String stageTypeStr = (String)root[CC_STAGE_TYPE];
    if( stageTypeStr == CC_BOOST )
        stageType = BOOST;
    else
        return false;

    String featureTypeStr = (String)root[CC_FEATURE_TYPE];
    if( featureTypeStr == CC_HAAR )
        featureType = FeatureEvaluator::HAAR;
    else if( featureTypeStr == CC_LBP )
        featureType = FeatureEvaluator::LBP;
    else if( featureTypeStr == CC_HOG )
    {
        featureType = FeatureEvaluator::HOG;
        CV_Error(Error::StsNotImplemented, "HOG cascade is not supported in 3.0");
    }
    else
        return false;

    origWinSize.width = (int)root[CC_WIDTH];
    origWinSize.height = (int)root[CC_HEIGHT];
    CV_Assert( origWinSize.height > 0 && origWinSize.width > 0 );

    // load feature params
    FileNode fn = root[CC_FEATURE_PARAMS];
    if( fn.empty() )
        return false;

    ncategories = fn[CC_MAX_CAT_COUNT];
    int subsetSize = (ncategories + 31)/32,
        nodeStep = 3 + ( ncategories>0 ? subsetSize : 1 );

    // load stages
    fn = root[CC_STAGES];
    if( fn.empty() )
        return false;

    stages.reserve(fn.size());
    classifiers.clear();
    nodes.clear();
    stumps.clear();

    FileNodeIterator it = fn.begin(), it_end = fn.end();
    minNodesPerTree = INT_MAX;
    maxNodesPerTree = 0;

    for( int si = 0; it != it_end; si++, ++it )
    {
        FileNode fns = *it;
        Stage stage;
        stage.threshold = (float)fns[CC_STAGE_THRESHOLD] - THRESHOLD_EPS;
        fns = fns[CC_WEAK_CLASSIFIERS];
        if(fns.empty())
            return false;
        stage.ntrees = (int)fns.size();
        stage.first = (int)classifiers.size();
        stages.push_back(stage);
        classifiers.reserve(stages[si].first + stages[si].ntrees);

        FileNodeIterator it1 = fns.begin(), it1_end = fns.end();
        for( ; it1 != it1_end; ++it1 ) // weak trees
        {
            FileNode fnw = *it1;
            FileNode internalNodes = fnw[CC_INTERNAL_NODES];
            FileNode leafValues = fnw[CC_LEAF_VALUES];
            if( internalNodes.empty() || leafValues.empty() )
                return false;

            DTree tree;
            tree.nodeCount = (int)internalNodes.size()/nodeStep;
            minNodesPerTree = std::min(minNodesPerTree, tree.nodeCount);
            maxNodesPerTree = std::max(maxNodesPerTree, tree.nodeCount);

            classifiers.push_back(tree);

            nodes.reserve(nodes.size() + tree.nodeCount);
            leaves.reserve(leaves.size() + leafValues.size());
            if( subsetSize > 0 )
                subsets.reserve(subsets.size() + tree.nodeCount*subsetSize);

            FileNodeIterator internalNodesIter = internalNodes.begin(), internalNodesEnd = internalNodes.end();

            for( ; internalNodesIter != internalNodesEnd; ) // nodes
            {
                DTreeNode node;
                node.left = (int)*internalNodesIter; ++internalNodesIter;
                node.right = (int)*internalNodesIter; ++internalNodesIter;
                node.featureIdx = (int)*internalNodesIter; ++internalNodesIter;
                if( subsetSize > 0 )
                {
                    for( int j = 0; j < subsetSize; j++, ++internalNodesIter )
                        subsets.push_back((int)*internalNodesIter);
                    node.threshold = 0.f;
                }
                else
                {
                    node.threshold = (float)*internalNodesIter; ++internalNodesIter;
                }
                nodes.push_back(node);
            }

            internalNodesIter = leafValues.begin(), internalNodesEnd = leafValues.end();

            for( ; internalNodesIter != internalNodesEnd; ++internalNodesIter ) // leaves
                leaves.push_back((float)*internalNodesIter);
        }
    }

    if( maxNodesPerTree == 1 )
    {
        int nodeOfs = 0, leafOfs = 0;
        size_t nstages = stages.size();
        for( size_t stageIdx = 0; stageIdx < nstages; stageIdx++ )
        {
            const Stage& stage = stages[stageIdx];

            int ntrees = stage.ntrees;
            for( int i = 0; i < ntrees; i++, nodeOfs++, leafOfs+= 2 )
            {
                const DTreeNode& node = nodes[nodeOfs];
                stumps.push_back(Stump(node.featureIdx, node.threshold,
                                       leaves[leafOfs], leaves[leafOfs+1]));
            }
        }
    }

    return true;
}