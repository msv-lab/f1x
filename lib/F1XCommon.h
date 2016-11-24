#pragma once

using std::string;


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


struct ProjectFile {
  // need to have ID, make backups and restore
  unsigned id;
};


class RepairLocation {
public:
  RepairLocation(DefectClass _dc,
                 ProjectFile _f,
                 unsigned _bl,
                 unsigned _bc,
                 unsigned _el,
                 unsigned _ec):
    defectClass(_dc),
    file(_f),
    beginLine(_bl),
    beginColumn(_bc),
    endLine(_el),
    endColumn(_ec) {}

  DefectClass getDefectClass() const { return defectClass; }
  ProjectFile getProjectFile() const { return file; }
  unsigned getBeginLine() const { return beginLine; }
  unsigned getBeginColumn() const { return beginColumn; }
  unsigned getEndLine() const { return endLine; }
  unsigned getEndColumn() const { return endColumn; }

private:
  DefectClass defectClass;
  ProjectFile file;
  unsigned beginLine;
  unsigned beginColumn;
  unsigned endLine;
  unsigned endColumn;
};
