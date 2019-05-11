// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/crash_reporter/win/crash_service_main.h"

#include "atom/common/crash_reporter/win/crash_service.h"
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/crashpad/crashpad/handler/handler_main.h"

namespace crash_service {

namespace {

const wchar_t kStandardLogFile[] = L"crashpad_logs.txt";

void InvalidParameterHandler(const wchar_t*,
                             const wchar_t*,
                             const wchar_t*,
                             unsigned int,
                             uintptr_t) {
  // noop.
}

bool CreateCrashServiceDirectory(const base::FilePath& temp_dir) {
  if (!base::PathExists(temp_dir)) {
    if (!base::CreateDirectory(temp_dir))
      return false;
  }
  return true;
}

}  // namespace.

int Main(int argc, char* argval[]) {
  // Ignore invalid parameter errors.
  _set_invalid_parameter_handler(InvalidParameterHandler);

  // Initialize all Chromium things.
  base::AtExitManager exit_manager;
  base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  // We use/create a directory under the user's temp folder, for logging.
  base::FilePath operating_dir(cmd_line->GetSwitchValueNative(
      "metrics-dir"));  // TODO BEFORE CHECKIN: Dont use the metrics dir
  CreateCrashServiceDirectory(operating_dir);
  base::FilePath log_file = operating_dir.Append(kStandardLogFile);

  // Logging to stderr (to help with debugging failures on the
  // buildbots) and to a file.
  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_ALL;
  settings.log_file = log_file.value().c_str();
  logging::InitLogging(settings);
  // Logging with pid, tid and timestamp.
  logging::SetLogItems(true, true, true, false);

  LOG(INFO) << "Ready to process crash requests";
  std::vector<base::CommandLine::StringType> argv = cmd_line->argv();
  argv.erase(std::remove(argv.begin(), argv.end(), L"--type=crashpad-handler"),
             argv.end());

  // |storage| must be declared before |argv_as_utf8|, to ensure it outlives
  // |argv_as_utf8|, which will hold pointers into |storage|.
  std::vector<std::string> storage;
  std::unique_ptr<char*[]> argv_as_utf8(new char*[argv.size() + 1]);
  storage.reserve(argv.size());
  for (size_t i = 0; i < argv.size(); ++i) {
    storage.push_back(base::UTF16ToUTF8(argv[i]));
    argv_as_utf8[i] = &storage[i][0];
  }
  argv_as_utf8[argv.size()] = nullptr;

  return crashpad::HandlerMain(argv.size(), argv_as_utf8.get(), nullptr);
}

}  // namespace crash_service
