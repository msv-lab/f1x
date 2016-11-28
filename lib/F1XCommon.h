#pragma once

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

enum class LocalizationFormula {
  OCHIAI,
  JACCARD,
  TARANTULA  
};


enum class CandidateSelection {
  FIXED_ORDER,         // order defined by search space ordering
  PREDICATE_SWITCHING, // predicate switching for failing test
  MAX_PASSING,         // prioritize candidates that pass maximim number of tests
  LEAST_COVERED        // prioritize candidates that are evaluated with least number of tests
};


enum class TestSelection {
  FIXED_ORDER,      // failing, then passing
  MAX_FAILING,      // prioritize tests that failed maximum number of times
  LARGEST_PARTITION // prioritize tests with largest average partition size
};


enum class DefectClass {
  CONDITION,  // existing program conditions (e.g. if, for, while, ...)
  EXPRESSION, // right side of assignments, call arguments
  GUARD       // adding guard for existing statement
};


class ProjectFile {
public:
  ProjectFile(std::string _p);
  fs::path getPath() const;
  unsigned getId() const;

private:
  static unsigned next_id;
  unsigned id;
  fs::path path;
};


class RepairLocation {
public:
  RepairLocation(DefectClass _dc,
                 ProjectFile _f,
                 unsigned _bl,
                 unsigned _bc,
                 unsigned _el,
                 unsigned _ec);
  DefectClass getDefectClass() const;
  ProjectFile getProjectFile() const;
  unsigned getBeginLine() const;
  unsigned getBeginColumn() const;
  unsigned getEndLine() const;
  unsigned getEndColumn() const;

private:
  DefectClass defectClass;
  ProjectFile file;
  unsigned beginLine;
  unsigned beginColumn;
  unsigned endLine;
  unsigned endColumn;
};
