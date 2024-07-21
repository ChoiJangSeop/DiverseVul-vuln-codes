void dtls1_clear_record_buffer(SSL *s)
{
    pitem *item;

    for (item = pqueue_pop(s->d1->sent_messages);
         item != NULL; item = pqueue_pop(s->d1->sent_messages)) {
        dtls1_hm_fragment_free((hm_fragment *)item->data);
        pitem_free(item);
    }
}