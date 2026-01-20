//
// gx/Logger.hh
// Copyright (C) 2026 Richard Bradley
//

#pragma once
#include <string_view>
#include <string>
#include <memory>  // unique_ptr
#include <sstream> // ostringstream
#include <ostream>


// **** Types ****
namespace gx {
  class Logger;
  class LoggerImpl;

  enum class LogLevel {
    trace,    // internal values just for developer debugging
    info,     // general operational events
    warn,     // events that could be an error
    error,    // error events when the program can continue
    fatal,    // error events when the program must terminate
    disabled  // all logging disabled
  };
}

class gx::Logger
{
 public:
  Logger();
  ~Logger();

  // disable copy/assignment/move
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  // members
  void setOStream(std::ostream& os);
    // write log to stderr
  void setFile(std::string_view fileName);
    // write log to a file
  bool rotate();
    // end current log file & start a new one
    // returns false if log isn't being writen a file

  [[nodiscard]] LogLevel level() const { return _level; }
  void setLevel(LogLevel lvl) { _level = lvl; }
  void disable() { _level = LogLevel::disabled; }
  void showMS(bool enable) { _showMS = enable; }
  void separateDate(bool enable) { _separateDate = enable; }
  void setSourcePrefix(std::string_view prefix) { _sourcePrefix = prefix; }

  // log method
  // (calling directly will always log message - only macros check log level)
  template<class... Args>
  void log(LogLevel lvl, std::string_view file, int line, const Args&... args) {
    auto os = logStream(lvl);
    ((os << args), ...);
    logMsg(os, file, line);
  }

 private:
  std::unique_ptr<LoggerImpl> _impl;
  std::string _sourcePrefix = "src/";
  int _lastDate = 0;
  LogLevel _level = LogLevel::info;
  bool _showMS = true;
  bool _separateDate = true;

  // methods
  [[nodiscard]] std::ostringstream logStream(LogLevel lvl);
  void logMsg(std::ostringstream& os, std::string_view file, int line);
};


// **** Functions ****
namespace gx {
  inline Logger& defaultLogger()
  {
    static Logger instance;
    return instance;
  }
}


// **** Macros ****
#define GX_LOGGER_LOG_FL(logger,lvl,file,line,...) \
  do { if ((lvl)>=(logger).level()) { (logger).log((lvl),(file),(line),__VA_ARGS__); } } while(0)

#define GX_LOGGER_LOG(logger,lvl,...)\
  GX_LOGGER_LOG_FL((logger),(lvl),__FILE__,__LINE__,__VA_ARGS__)


// default logger instance logging macros
//
// NOTE: messages not logged because of current log level will not evaluate
//   macro arguments.  Do not put code with side effects in macro arguments
//   if the side effects are important.
//
#define GX_LOG_TRACE(...) \
  GX_LOGGER_LOG(gx::defaultLogger(),gx::LogLevel::trace,__VA_ARGS__)
#define GX_LOG_INFO(...) \
  GX_LOGGER_LOG(gx::defaultLogger(),gx::LogLevel::info,__VA_ARGS__)
#define GX_LOG_WARN(...) \
  GX_LOGGER_LOG(gx::defaultLogger(),gx::LogLevel::warn,__VA_ARGS__)
#define GX_LOG_ERROR(...) \
  GX_LOGGER_LOG(gx::defaultLogger(),gx::LogLevel::error,__VA_ARGS__)
#define GX_LOG_FATAL(...) \
  GX_LOGGER_LOG(gx::defaultLogger(),gx::LogLevel::fatal,__VA_ARGS__)
