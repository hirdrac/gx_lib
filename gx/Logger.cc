//
// gx/Logger.cc
// Copyright (C) 2024 Richard Bradley
//

// TODO: threaded support
//   - IO operations handled on dedicated thread
//   - message string creation, time capture on calling thread
// TODO: naming threads
//   - at thread creation, thread ID given short name that is
//     used for logging instead of ID (main, render, log)
// TODO: replace clock_gettime() with std::chrono code?

#include "Logger.hh"
#include "ThreadID.hh"
#include <string>
#include <iostream>
#include <fstream>
#include <mutex>
#include <cstdio>
#include <ctime>
using namespace gx;


// **** Helper Functions ****
[[nodiscard]] static int dateNum(const std::tm& td)
{
  return ((td.tm_year + 1900) * 10000) + ((td.tm_mon + 1) * 100) + td.tm_mday;
}

[[nodiscard]] static std::string fileTime()
{
  const std::time_t t = std::time(nullptr);
  const std::tm td = *std::localtime(&t);
  char str[32];
  const int len = snprintf(
    str, sizeof(str), "-%d_%02d%02d%02d",
    dateNum(td), td.tm_hour, td.tm_min, td.tm_sec);
  return {str, std::size_t(len)};
}

[[nodiscard]] static constexpr std::string_view levelStr(LogLevel l)
{
  switch (l) {
    case LVL_TRACE: return " [TRACE] ";
    case LVL_INFO:  return " [INFO] ";
    case LVL_WARN:  return " [WARN] ";
    case LVL_ERROR: return " [ERROR] ";
    case LVL_FATAL: return " [FATAL] ";
    default:        return " [UNKNOWN] ";
  }
}


// **** LoggerImpl class ****
class gx::LoggerImpl
{
 public:
  void setOStream(std::ostream& os)
  {
    const std::lock_guard lg{_mutex};
    _fileName.clear();
    _fileStream.close();
    _os = &os;
  }

  void setFile(std::string_view fileName)
  {
    const std::lock_guard lg{_mutex};
    _fileName = fileName;
    _fileStream = std::ofstream(_fileName, std::ios_base::app);
    _os = &_fileStream;
  }

  void log(std::string_view s)
  {
    const std::lock_guard lg{_mutex};
    _os->write(s.data(), std::streamsize(s.size()));
    _os->flush();
  }

  void rotate()
  {
    const std::lock_guard lg{_mutex};
    if (!_fileStream) return;

    const std::size_t x = _fileName.rfind('.');
    std::string nf = _fileName.substr(0,x) + fileTime();
    if (x != std::string::npos) {
      nf += _fileName.substr(x);
    }

    _fileStream.close();
    std::rename(_fileName.c_str(), nf.c_str());
    _fileStream = std::ofstream(_fileName, std::ios_base::app);
  }

 private:
  std::ostream* _os = &std::cerr;
  std::ofstream _fileStream;
  std::string _fileName;
  std::mutex _mutex;
};


// **** Logger class ****
Logger::Logger() : _impl{new LoggerImpl} { }

Logger::~Logger() = default;
  // NOTE: default destructor needed here because LoggerImpl is not
  //   visible externally

void Logger::setOStream(std::ostream& os)
{
  _impl->setOStream(os);
}

void Logger::setFile(std::string_view fileName)
{
  _impl->setFile(fileName);
}

void Logger::rotate()
{
  _impl->rotate();
}

std::ostringstream Logger::logStream(LogLevel lvl)
{
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);

  const std::tm td = *std::localtime(&ts.tv_sec);
  char str[32];
  int len = 0;

  if (_separateDate) {
    const int date = dateNum(td);
    if (_lastDate != date) {
      _lastDate = date;
      len = snprintf(str, sizeof(str), "-- %d-%02d-%02d --\n", // 17
                     td.tm_year + 1900, td.tm_mon + 1, td.tm_mday);
    }
  } else {
    len = snprintf(str, sizeof(str), "%d-%02d-%02d ", // 11
                   td.tm_year + 1900, td.tm_mon + 1, td.tm_mday);
  }

  len += snprintf(
    str + len, sizeof(str) - std::size_t(len), "%02d:%02d:%02d", // 8
    td.tm_hour, td.tm_min, td.tm_sec);
  if (_showMS) {
    len += snprintf(str + len, sizeof(str) - std::size_t(len), ".%03ld", // 4
                    ts.tv_nsec / 1000000);
  }

  std::ostringstream ss;
  ss.write(str, std::streamsize(len)) << levelStr(lvl);
  return ss;
}

void Logger::logMsg(std::ostringstream& ss, std::string_view file, int line)
{
  while (file.substr(0, 3) == "../") {
    file.remove_prefix(3);
  }

  const auto sz = _sourcePrefix.size();
  if (sz > 0 && file.substr(0, sz) == _sourcePrefix) {
    file.remove_prefix(sz);
  }

  // footer
  ss << " (";
  const auto tid = getThreadID();
  if (tid != mainThreadID) { ss << "t=" << tid << ' '; }
  ss << file << ':' << line << ")\n";

  // output to stream
#if __cplusplus >= 202002L
  _impl->log(ss.view()); // C++20
#else
  _impl->log(ss.str());
#endif
}
