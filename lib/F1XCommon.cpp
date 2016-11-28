#include "F1XCommon.h"


ProjectFile::ProjectFile(std::string _p): 
  path(_p) {
  id = next_id;
  next_id++;
}

fs::path ProjectFile::getPath() const {
  return path;
}

unsigned ProjectFile::getId() const {
  return id;
}

unsigned ProjectFile::next_id = 0;


RepairLocation::RepairLocation(DefectClass _dc,
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

DefectClass RepairLocation::getDefectClass() const {
  return defectClass;
}

ProjectFile RepairLocation::getProjectFile() const {
  return file;
}

unsigned RepairLocation::getBeginLine() const {
  return beginLine;
}

unsigned RepairLocation::getBeginColumn() const {
  return beginColumn;
}

unsigned RepairLocation::getEndLine() const {
  return endLine;
}

unsigned RepairLocation::getEndColumn() const {
  return endColumn;
}
