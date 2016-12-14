/*
  This file is part of f1x.
  Copyright (C) 2016  Sergey Mechtaev, Abhik Roychoudhury

  f1x is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdlib>
#include <memory>
#include <iostream>

#include <boost/log/trivial.hpp>

#include "Repair.h"
#include "RepairUtil.h"

namespace fs = boost::filesystem;
using std::vector;
using std::string;
using std::shared_ptr;


bool repair(fs::path root,
            vector<fs::path> files,
            vector<string> tests,
            unsigned testTimeout,
            fs::path driver,
            string buildCmd,
            string& patch) {
  BOOST_LOG_TRIVIAL(info) << "repairing project " << root;
  BOOST_LOG_TRIVIAL(debug) << "with timeout " << testTimeout;

  BOOST_LOG_TRIVIAL(info) << "building with " << buildCmd;
  {
    FromDirectory dir(root);
    string cmd = "bear " + buildCmd;
    uint status = std::system(cmd.c_str());
    if (status != 0) {
      BOOST_LOG_TRIVIAL(warning) << "compilation failed";
    } else {
      BOOST_LOG_TRIVIAL(info) << "compilation succeeded";
    }
  }

  BOOST_LOG_TRIVIAL(info) << "adjusting compilation database";  
  addClangHeadersToCompileDB(root);

  BOOST_LOG_TRIVIAL(info) << "instrumenting";
  {
    FromDirectory dir(root);
    string cmd = "f1x-transform " + files[0].string() + " --instrument --file-id 0 --output /home/sergey/cl.json";
    std::cout << cmd << std::endl;
    uint status = std::system(cmd.c_str());
    if (status != 0) {
      BOOST_LOG_TRIVIAL(warning) << "transformation failed";
    } else {
      BOOST_LOG_TRIVIAL(info) << "transformation succeeded";
    }
  }

  BOOST_LOG_TRIVIAL(info) << "loading candidate locations";
  fs::path clfile("/home/sergey/cl.json");
  vector<shared_ptr<CandidateLocation>> cls = loadCondidateLocations(clfile);

  for (auto cl : cls) {
    std::cout << cl->location.beginLine << " "
              << cl->location.beginColumn << " "
              << cl->location.endLine << " "
              << cl->location.endColumn << " "
              << expressionToString(cl->original)
              << std::endl;
  }

  return false;
}
