static void free_clt(struct rtrs_clt_sess *clt)
{
	free_permits(clt);
	free_percpu(clt->pcpu_path);
	mutex_destroy(&clt->paths_ev_mutex);
	mutex_destroy(&clt->paths_mutex);
	/* release callback will free clt in last put */
	device_unregister(&clt->dev);
}