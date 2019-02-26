// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_resource_dispatcher_host_delegate.h"

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/web_contents_preferences.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/render_frame_host.h"
#include "electron/buildflags/buildflags.h"
#include "net/base/escape.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "atom/common/atom_constants.h"
#include "base/guid.h"
#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "content/public/browser/stream_info.h"
#include "content/public/common/transferrable_url_loader.mojom.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_stream_manager.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_guest.h"
#include "net/url_request/url_request.h"
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)

using content::BrowserThread;

namespace atom {

AtomResourceDispatcherHostDelegate::AtomResourceDispatcherHostDelegate() {}
AtomResourceDispatcherHostDelegate::~AtomResourceDispatcherHostDelegate() {
#if BUILDFLAG(ENABLE_PDF_VIEWER)
  CHECK(stream_target_info_.empty());
#endif
}

bool AtomResourceDispatcherHostDelegate::ShouldInterceptResourceAsStream(
    net::URLRequest* request,
    const std::string& mime_type,
    GURL* origin,
    std::string* payload) {
#if BUILDFLAG(ENABLE_PDF_VIEWER)
  if (mime_type == "application/pdf") {
    StreamTargetInfo target_info;
    *origin = GURL(base::StrCat({"chrome-extension://", kPdfExtensionId}));
    target_info.extension_id = kPdfExtensionId;
    target_info.view_id = base::GenerateGUID();
    *payload = target_info.view_id;
    stream_target_info_[request] = target_info;
    return true;
  }
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)
  return false;
}

void AtomResourceDispatcherHostDelegate::OnStreamCreated(
    net::URLRequest* request,
    std::unique_ptr<content::StreamInfo> stream) {
#if BUILDFLAG(ENABLE_PDF_VIEWER)
  content::ResourceRequestInfo* info =
      content::ResourceRequestInfo::ForRequest(request);
  auto ix = stream_target_info_.find(request);
  CHECK(ix != stream_target_info_.end());
  bool embedded = info->GetResourceType() != content::RESOURCE_TYPE_MAIN_FRAME;
  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(
          &AtomResourceDispatcherHostDelegate::OnPdfResourceIntercepted,
          ix->second.extension_id, ix->second.view_id, embedded,
          info->GetFrameTreeNodeId(), info->GetChildID(),
          info->GetRenderFrameID(), std::move(stream),
          nullptr /* transferrable_loader */, GURL()));
  stream_target_info_.erase(request);
#endif
}

#if BUILDFLAG(ENABLE_PDF_VIEWER)
void AtomResourceDispatcherHostDelegate::OnPdfResourceIntercepted(
    const std::string& extension_id,
    const std::string& view_id,
    bool embedded,
    int frame_tree_node_id,
    int render_process_id,
    int render_frame_id,
    std::unique_ptr<content::StreamInfo> stream,
    content::mojom::TransferrableURLLoaderPtr transferrable_loader,
    const GURL& original_url) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  content::WebContents* web_contents = nullptr;
  if (frame_tree_node_id != -1) {
    web_contents =
        content::WebContents::FromFrameTreeNodeId(frame_tree_node_id);
  } else {
    web_contents = content::WebContents::FromRenderFrameHost(
        content::RenderFrameHost::FromID(render_process_id, render_frame_id));
  }
  if (!web_contents)
    return;

  // If the mime handler uses MimeHandlerViewGuest, the MimeHandlerViewGuest
  // will take ownership of the stream.
  GURL handler_url(
      GURL(base::StrCat({"chrome-extension://", extension_id})).spec() +
      "index.html");
  int tab_id = -1;
  std::unique_ptr<extensions::StreamContainer> stream_container(
      new extensions::StreamContainer(
          std::move(stream), tab_id, embedded, handler_url, extension_id,
          std::move(transferrable_loader), original_url));
  auto* browser_context = web_contents->GetBrowserContext();
  extensions::MimeHandlerStreamManager::Get(browser_context)
      ->AddStream(view_id, std::move(stream_container), frame_tree_node_id,
                  render_process_id, render_frame_id);
}
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)

}  // namespace atom
