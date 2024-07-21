int unit_watch_pid(Unit *u, pid_t pid) {
        int q, r;

        assert(u);
        assert(pid >= 1);

        r = set_ensure_allocated(&u->pids, trivial_hash_func, trivial_compare_func);
        if (r < 0)
                return r;

        /* Watch a specific PID. We only support one unit watching
         * each PID for now. */

        r = set_put(u->pids, LONG_TO_PTR(pid));

        q = hashmap_put(u->manager->watch_pids, LONG_TO_PTR(pid), u);
        if (q < 0)
                return q;

        return r;
}