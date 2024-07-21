agoo_con_create(agooErr err, int sock, uint64_t id, agooBind b) {
    agooCon	c;

    if (NULL == (c = (agooCon)AGOO_CALLOC(1, sizeof(struct _agooCon)))) {
	AGOO_ERR_MEM(err, "Connection");
    } else {
	c->sock = sock;
	c->id = id;
	c->timeout = dtime() + CON_TIMEOUT;
	c->bind = b;
	c->loop = NULL;
	pthread_mutex_init(&c->res_lock, 0);
    }
    return c;
}