#include <iostream>
#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>
#include "F1XConfig.h"

namespace po = boost::program_options;

using namespace std;

int main (int argc, char *argv[])
{
  po::positional_options_description positional;
  positional.add("source", 1);
  
  // Declare the supported options.
  po::options_description general("Usage: f1x SOURCE OPTIONS\n\nAllowed options");
  general.add_options()
    ("files", po::value<string>()->multitoken(), "list of source files to repair")
    ("tests", po::value<string>()->multitoken(), "list of test IDs")
    ("generic-runner", po::value<string>(), "generic test runner")
    ("google-test", po::value<string>(), "google test runner")
    ("make", po::value<string>()->default_value("make -e"), "make command")    
    ("max-exprs", po::value<int>()->default_value(10000), "maximum mutations per expression")
    ("test-timeout", po::value<int>()->default_value(5), "test execution timeout (SEC)")
    ("defect-classes", po::value<string>()->multitoken(), "list of defect classes to explore")
    ("first-found", "search until first patch found")
    ("verbose", "extended output")
    ("version", "print version")
    ("help", "produce help message")
    ;

  po::options_description hidden("Hidden options");
  hidden.add_options()  
    ("source", po::value<string>(), "source directory")
    ;

  po::options_description all("All options");
  all.add(general).add(hidden);
  
  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(all).positional(positional).run(), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << general << "\n";
    return 1;
  }

  if (vm.count("version")) {
    cout << argv[0] << " " << F1X_VERSION_MAJOR << "." << F1X_VERSION_MINOR << "\n";
    return 1;
  }

  if (!vm.count("source")) {
    BOOST_LOG_TRIVIAL(error) << "source directory is not specified (use --help)\n";
    return 1;
  }

  BOOST_LOG_TRIVIAL(info) << "test-timeout was set to " << vm["test-timeout"].as<int>();
  BOOST_LOG_TRIVIAL(info) << "source was set to " << vm["source"].as<string>();
  
  return 0;
}
