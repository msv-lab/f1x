#pragma once

#include "Project.h"
#include "Util.h"

#include <vector>
#include <ctime>
class FaultLocalization {
public:
	FaultLocalization(const std::vector<std::string> &tests,
					  TestingFramework &tester,
					  Project &project);		//constructor
	~FaultLocalization();		//destructor
	/**
	*
	*/
	std::vector<struct XMLCoverageFile> getFaultLocalization(const std::vector<std::string> &vFiles);
	std::string getPathFolderTmp();
	static std::vector<std::string> getFileFromJson(const boost::filesystem::path &);
	/**
	*
	*/
private:
	/**
	* Signature function for calculating tarantula score
	*/
	int ns = 0;
	int nf = 0;
	bool folderCreateFlag = false;
	std::string FolderTmp;
	TestingFramework tester;
	std::vector<std::string> tests;
	Project project;
	std::vector<struct TarantulaArgs> vTarantulaArgs;
	std::vector<struct TarantulaScore> vTarantulaScore;
	std::vector<struct SpectrumBased> vSpectrumBased;
	std::vector<struct TestCaseInfo> vTestCaseInfo;
	std::vector<struct XMLCoverageFile> vXMLCovFile;
	double tarantulaFormula(const int &nf_e,
							const int &ns_e,
							const int &ns,
							const int &nf);
	void calculatingTarantulaScore(const std::vector<struct TarantulaArgs> &cTArgsCollection);
	void acquiringTarantulaArgs();
	std::vector<struct HitsEachLine> analyzingXMLFiles(const std::string &,const std::string &);
	bool isXMLFile(const std::string &relpath) const;
	static bool isSourceFile(const boost::filesystem::path &);
	std::string generatingXMLFiles(const std::string &);
	/**
	 * creating temporary folder that stores XML coverage files
	 */
	std::string creatingFolderTmp();
	/**
	 * executing system command
	 * return true if this command is executed success.
	 */
	bool executingCMD(const std::string &);
	bool executingGovr();
	void executingTest(const std::string &relPath);
	void clearingDataStruct();
	/*
	*
	*/
};

struct HitsEachLine {
	int hits;
	int line;
};		//Storing number of hit each line

struct SpectrumBased {
	std::vector<HitsEachLine> contentOfXML;
	bool statusOfTestCase;
};		//Storing

struct TarantulaArgs {
	int line;
	int nf_e;
	int ns_e;
public:
	TarantulaArgs(int &line, int nf_e = 0, int ns_e = 0):
		line(line), nf_e(nf_e), ns_e(ns_e){}
};

struct TarantulaScore {
	int line;
	double score;
};

struct TestCaseInfo
{
	boost::filesystem::path fullPath;
	bool status;
};

struct XMLCoverageFile
{
	std::string fileName;
	std::vector<struct TarantulaScore> vTScore;
};
/*
 * End define
 */


