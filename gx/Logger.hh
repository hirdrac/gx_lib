//
// gx/Logger.hh
// Copyright (C) 2020 Richard Bradley
//

#pragma once
#include <string_view>
#include <memory>  // unique_ptr
#include <sstream> // ostringstream
#include <ostream>


// **** Types ****
namespace gx {
  class Logger;
  class LoggerImpl;

  enum LogLevel {
    LVL_TRACE,
    LVL_INFO,
    LVL_WARN,
    LVL_ERROR,
    LVL_FATAL,

    LVL_DISABLED
  };
}

class gx::Logger
{
 public:
  Logger();
  ~Logger();

  // members
  void setOStream(std::ostream& os);
  void setFile(std::string_view fileName);
  void rotate();

  [[nodiscard]] LogLevel level() const { return _level; }
  void setLevel(LogLevel lvl) { _level = lvl; }
  void disable() { _level = LVL_DISABLED; }

  // log method
  template<typename... Args>
  void log(LogLevel lvl, const char* file, int line, const Args&... args) {
    std::ostringstream os;
    header(os, lvl);
    ((os << args),...);
    logMsg(os, file, line);
  }

 private:
  std::unique_ptr<LoggerImpl> _impl;
  LogLevel _level;

  // methods
  void header(std::ostringstream& os, LogLevel lvl);
  void logMsg(std::ostringstream& os, const char* file, int line);

  // disable copy/assignment
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;
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
#define LOGGER_LOG_FL(logger,lvl,file,line,...) \
  do { gx::Logger& lg(logger); if ((lvl)>=lg.level()) { lg.log((lvl),(file),(line),__VA_ARGS__); } } while(0)

#define LOGGER_LOG(logger,lvl,...)\
  LOGGER_LOG_FL((logger),(lvl),__FILE__,__LINE__,__VA_ARGS__)


// default logger instance logging macros
#define LOG_TRACE(...) \
  LOGGER_LOG(gx::defaultLogger(),gx::LVL_TRACE,__VA_ARGS__)
#define LOG_INFO(...) \
  LOGGER_LOG(gx::defaultLogger(),gx::LVL_INFO,__VA_ARGS__)
#define LOG_WARN(...) \
  LOGGER_LOG(gx::defaultLogger(),gx::LVL_WARN,__VA_ARGS__)
#define LOG_ERROR(...) \
  LOGGER_LOG(gx::defaultLogger(),gx::LVL_ERROR,__VA_ARGS__)
#define LOG_FATAL(...) \
  LOGGER_LOG(gx::defaultLogger(),gx::LVL_FATAL,__VA_ARGS__)
