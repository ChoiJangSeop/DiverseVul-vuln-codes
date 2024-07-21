void ElectronBrowserClient::RegisterBrowserInterfaceBindersForFrame(
    content::RenderFrameHost* render_frame_host,
    mojo::BinderMapWithContext<content::RenderFrameHost*>* map) {
  map->Add<network_hints::mojom::NetworkHintsHandler>(
      base::BindRepeating(&BindNetworkHintsHandler));
  map->Add<blink::mojom::BadgeService>(
      base::BindRepeating(&badging::BadgeManager::BindFrameReceiver));
  map->Add<electron::mojom::ElectronBrowser>(
      base::BindRepeating(&BindElectronBrowser));
  map->Add<blink::mojom::KeyboardLockService>(base::BindRepeating(
      &content::KeyboardLockServiceImpl::CreateMojoService));
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  map->Add<extensions::mime_handler::MimeHandlerService>(
      base::BindRepeating(&BindMimeHandlerService));
  map->Add<extensions::mime_handler::BeforeUnloadControl>(
      base::BindRepeating(&BindBeforeUnloadControl));

  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents)
    return;

  const GURL& site = render_frame_host->GetSiteInstance()->GetSiteURL();
  if (!site.SchemeIs(extensions::kExtensionScheme))
    return;

  content::BrowserContext* browser_context =
      render_frame_host->GetProcess()->GetBrowserContext();
  auto* extension = extensions::ExtensionRegistry::Get(browser_context)
                        ->enabled_extensions()
                        .GetByID(site.host());
  if (!extension)
    return;
  extensions::ExtensionsBrowserClient::Get()
      ->RegisterBrowserInterfaceBindersForFrame(map, render_frame_host,
                                                extension);
#endif
}