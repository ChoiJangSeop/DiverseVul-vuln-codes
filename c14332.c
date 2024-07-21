static void clear_rulist(rzip_control *control)
{
	while (control->ruhead) {
		struct runzip_node *node = control->ruhead;
		struct stream_info *sinfo = node->sinfo;

		dealloc(sinfo->ucthreads);
		dealloc(node->pthreads);
		dealloc(sinfo->s);
		dealloc(sinfo);
		control->ruhead = node->prev;
		dealloc(node);
	}
}