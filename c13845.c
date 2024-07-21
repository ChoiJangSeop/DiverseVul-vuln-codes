void ConnPoolImplBase::checkForIdleAndCloseIdleConnsIfDraining() {
  if (is_draining_for_deletion_) {
    closeIdleConnectionsForDrainingPool();
  }

  if (isIdleImpl()) {
    ENVOY_LOG(debug, "invoking idle callbacks - is_draining_for_deletion_={}",
              is_draining_for_deletion_);
    for (const Instance::IdleCb& cb : idle_callbacks_) {
      cb();
    }
  }
}