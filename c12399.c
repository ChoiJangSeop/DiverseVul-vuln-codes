ompl::geometric::VFRRT::Motion *ompl::geometric::VFRRT::extendTree(Motion *m, base::State *rstate,
                                                                   const Eigen::VectorXd &v)
{
    base::State *newState = si_->allocState();
    si_->copyState(newState, m->state);

    double d = si_->distance(m->state, rstate);
    if (d > maxDistance_)
        d = maxDistance_;

    const base::StateSpacePtr &space = si_->getStateSpace();
    for (unsigned int i = 0; i < vfdim_; i++)
        *space->getValueAddressAtIndex(newState, i) += d * v[i];
    if (!v.hasNaN() && si_->checkMotion(m->state, newState))
    {
        auto *motion = new Motion(si_);
        motion->state = newState;
        motion->parent = m;
        updateExplorationEfficiency(motion);
        nn_->add(motion);
        return motion;
    }
    else
    {
        si_->freeState(newState);
        inefficientCount_++;
        return nullptr;
    }
}