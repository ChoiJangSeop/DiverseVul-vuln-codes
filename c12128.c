void WebContents::MessageSync(
    bool internal,
    const std::string& channel,
    blink::CloneableMessage arguments,
    electron::mojom::ElectronBrowser::MessageSyncCallback callback,
    content::RenderFrameHost* render_frame_host) {
  TRACE_EVENT1("electron", "WebContents::MessageSync", "channel", channel);
  // webContents.emit('-ipc-message-sync', new Event(sender, message), internal,
  // channel, arguments);
  EmitWithSender("-ipc-message-sync", render_frame_host, std::move(callback),
                 internal, channel, std::move(arguments));
}