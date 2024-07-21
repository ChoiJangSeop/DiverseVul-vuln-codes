static int iovw_put(struct iovec_wrapper *iovw, void* data, size_t len) {
        if (!GREEDY_REALLOC(iovw->iovec, iovw->size_bytes, iovw->count + 1))
                return log_oom();

        iovw->iovec[iovw->count++] = IOVEC_MAKE(data, len);
        return 0;
}