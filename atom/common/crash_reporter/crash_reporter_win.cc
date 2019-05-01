// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/crash_reporter/crash_reporter_win.h"

#include <memory>

#include "base/memory/singleton.h"
#include "base/path_service.h"
#include "crashpad/client/crashpad_client.h"
#include "crashpad/client/crashpad_info.h"

namespace crash_reporter {

CrashReporterWin::CrashReporterWin() {}

CrashReporterWin::~CrashReporterWin() {}

void CrashReporterWin::InitBreakpad(const std::string& product_name,
                                    const std::string& version,
                                    const std::string& company_name,
                                    const std::string& submit_url,
                                    const base::FilePath& crashes_dir,
                                    bool upload_to_server,
                                    bool skip_system_crash_handler) {
  // check whether crashpad has been initialized.
  // Only need to initialize once.
  if (simple_string_dictionary_)
    return;

  if (is_browser_) {
    base::FilePath handler_path;
    base::PathService::Get(base::FILE_EXE, &handler_path);

    std::vector<std::string> args = {
        "--no-rate-limit",
        "--no-upload-gzip",  // not all servers accept gzip
    };

    crashpad::CrashpadClient crashpad_client;
    crashpad_client.StartHandler(handler_path, crashes_dir, crashes_dir,
                                 submit_url, StringMap(), args, true, false);
  }

  crashpad::CrashpadInfo* crashpad_info =
      crashpad::CrashpadInfo::GetCrashpadInfo();
  if (skip_system_crash_handler) {
    crashpad_info->set_system_crash_reporter_forwarding(
        crashpad::TriState::kDisabled);
  }

  simple_string_dictionary_.reset(new crashpad::SimpleStringDictionary());
  crashpad_info->set_simple_annotations(simple_string_dictionary_.get());

  SetCrashKeyValue("prod", ATOM_PRODUCT_NAME);
  SetCrashKeyValue("process_type", is_browser_ ? "browser" : "renderer");
  SetCrashKeyValue("ver", version);

  for (const auto& upload_parameter : upload_parameters_) {
    SetCrashKeyValue(upload_parameter.first, upload_parameter.second);
  }
  if (is_browser_) {
    database_ = crashpad::CrashReportDatabase::Initialize(crashes_dir);
    SetUploadToServer(upload_to_server);
  }
}

// static
CrashReporterWin* CrashReporterWin::GetInstance() {
  return base::Singleton<CrashReporterWin>::get();
}

// static
CrashReporter* CrashReporter::GetInstance() {
  return CrashReporterWin::GetInstance();
}

}  // namespace crash_reporter
