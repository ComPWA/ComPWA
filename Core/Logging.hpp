// Copyright (c) 2015, 2017 The ComPWA Team.
// This file is part of the ComPWA framework, check
// https://github.com/ComPWA/ComPWA/license.txt for details.

#ifndef LOGGING_HPP_
#define LOGGING_HPP_

#include <boost/log/trivial.hpp>
#include <boost/log/common.hpp>

namespace ComPWA {

#define LOG(lvl) Logging::log(lvl)
#define info lvl::info
// Redefine BOOST_LOG_TRIVIAL(level) to LOG(level)
//#define LOG(lvl)                                                               \
//  BOOST_LOG_STREAM_WITH_PARAMS(                                                \
//      ::boost::log::trivial::logger::get(),                                    \
//      (::boost::log::keywords::severity = ::boost::log::trivial::lvl))

//#define BOOST_LOG_STREAM_WITH_PARAMS(logger, params_seq)                       \
//  BOOST_LOG_STREAM_WITH_PARAMS_INTERNAL(                                       \
//      logger, BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_record_),            \
//      params_seq)
//
//#define BOOST_LOG_STREAM_WITH_PARAMS_INTERNAL(logger, rec_var, params_seq)     \
//  for (::boost::log::record rec_var =                                          \
//           (logger).open_record((BOOST_PP_SEQ_ENUM(params_seq)));              \
//       !!rec_var;)                                                             \
//  ::boost::log::aux::make_record_pump((logger), rec_var).stream()
///
/// \class Logging
/// Logging class privides an interface for logging all over the framework.
/// Behind the scenes boost::log is currently used which allows a detailed
/// on logging format and log levels
///
operator<<
class Logging {
public:
  Logging(std::string outFileName = "output.log",
          std::string minLevel = "debug");

  enum logLvl {Trace, Debug, Info, Warning, Error, Fatal};

  static void log(logLvl level);

  void setLogLevel(std::string minLevel);
  
};

} // namespace ComPWA

#endif
