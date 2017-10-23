#pragma once

#include "Project.h"
#include "Util.h"

#include <vector>
#include <ctime>
class FaultLocalization {
public:
	FaultLocalization(const std::vector<std::string> &tests,
					  TestingFramework &tester,
					  Project &project,
					  const boost::filesystem::path &fileName,
					  const std::string &tmpFolder);		//constructor
	~FaultLocalization();		//destructor
	/**
	*
	*/
	std::vector<struct TarantulaScore> getFaultLocalization();
	std::string getPathFolderTmp();
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
	std::string fullPathXMLFiles;
	TestingFramework tester;
	std::vector<std::string> tests;
	Project project;
	boost::filesystem::path fileName;
	std::string tmpFolder;
	std::vector<struct TarantulaArgs> vTarantulaArgs;
	std::vector<struct TarantulaScore> vTarantulaScore;
	std::vector<struct SpectrumBased> vSpectrumBased;
	std::vector<struct TestCaseInfo> vTestCaseInfo;
	void creatingNewTarantulaArgs(struct TarantulaArgs &tArgsCollection);
	double tarantulaFormula(const int &nf_e,
							const int &ns_e,
							const int &ns,
							const int &nf);
	void calculatingTarantulaScore(const std::vector<struct TarantulaArgs> &cTArgsCollection);
	void acquiringTarantulaArgs();
	std::vector<struct HitsEachLine> analyzingXMLFiles(const std::string &);
	bool isXMLFile(const std::string &relpath) const;
	bool isSourceFile(const boost::filesystem::path &) const;
	std::string generatingXMLFiles(const std::string &);
	void getFileFromJson();
	/**
	 * creating temporary folder that stores XML coverage files
	 */
	std::string creatingFolderTmp();
	bool executingGovr();
	void executingTest();
	/**
	 * executing system command
	 * return true if this command is executed success.
	 */
	bool executingCMD(const std::string &);
	/*
	*
	*/
};

/**
 * define structure for acquiring data from GCOV XML files
 */
//class HitsEachLine
//{
//public:
//	HitsEachLine()
//	{
//	}
//	void setHits(int hits)
//	{
//		this->hits = hits;
//	}
//	void setLine(int line)
//	{
//		this->line = line;
//	}
//	int getHits()
//	{
//		return this->hits;
//	}
//	int getLine()
//	{
//		return this->line;
//	}
//private:
//	int hits = 0;
//	int line = 0;
//};

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
/*
 * End define
 */


