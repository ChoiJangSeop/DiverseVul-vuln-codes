content::RenderFrameHost* ElectronBrowserHandlerImpl::GetRenderFrameHost() {
  return content::RenderFrameHost::FromID(render_process_id_, render_frame_id_);
}