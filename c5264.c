  TProtocol(boost::shared_ptr<TTransport> ptrans)
    : ptrans_(ptrans), recursion_depth_(0), recursion_limit_(DEFAULT_RECURSION_LIMIT) {}