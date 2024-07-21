void vnc_disconnect_finish(VncState *vs)
{
    int i;

    trace_vnc_client_disconnect_finish(vs, vs->ioc);

    vnc_jobs_join(vs); /* Wait encoding jobs */

    vnc_lock_output(vs);
    vnc_qmp_event(vs, QAPI_EVENT_VNC_DISCONNECTED);

    buffer_free(&vs->input);
    buffer_free(&vs->output);

    qapi_free_VncClientInfo(vs->info);

    vnc_zlib_clear(vs);
    vnc_tight_clear(vs);
    vnc_zrle_clear(vs);

#ifdef CONFIG_VNC_SASL
    vnc_sasl_client_cleanup(vs);
#endif /* CONFIG_VNC_SASL */
    audio_del(vs);
    qkbd_state_lift_all_keys(vs->vd->kbd);

    if (vs->mouse_mode_notifier.notify != NULL) {
        qemu_remove_mouse_mode_change_notifier(&vs->mouse_mode_notifier);
    }
    QTAILQ_REMOVE(&vs->vd->clients, vs, next);
    if (QTAILQ_EMPTY(&vs->vd->clients)) {
        /* last client gone */
        vnc_update_server_surface(vs->vd);
    }

    vnc_unlock_output(vs);

    qemu_mutex_destroy(&vs->output_mutex);
    if (vs->bh != NULL) {
        qemu_bh_delete(vs->bh);
    }
    buffer_free(&vs->jobs_buffer);

    for (i = 0; i < VNC_STAT_ROWS; ++i) {
        g_free(vs->lossy_rect[i]);
    }
    g_free(vs->lossy_rect);

    object_unref(OBJECT(vs->ioc));
    vs->ioc = NULL;
    object_unref(OBJECT(vs->sioc));
    vs->sioc = NULL;
    vs->magic = 0;
    g_free(vs);
}