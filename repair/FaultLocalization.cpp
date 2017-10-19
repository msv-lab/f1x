/*
 * include XML parse 3th library - rapid XML
 */
#include <rapidxml/rapidxml.hpp>
/*
 */

#include <boost/log/trivial.hpp>

#include <stdio.h>
#include <iomanip>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <string>
#include <iostream>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

#include "FaultLocalization.h"

using namespace rapidxml;

FaultLocalization::FaultLocalization(const std::vector<std::string> &tests,
										TestingFramework &tester,
										Project &project ):
										tests(tests),
										tester(tester),
										project(project){
	this->FolderTmp = this->creatingFolderTmp();
	/*
	 * test whether folder is created
	 */
}
FaultLocalization::~FaultLocalization() {}

void FaultLocalization::creatingNewTarantulaArgs(struct TarantulaArgs &tArgsCollection)
{
	tArgsCollection.line = 0;
	tArgsCollection.nf_e = 0;
	tArgsCollection.ns_e = 0;
}

std::vector<struct TarantulaScore> FaultLocalization::getFaultLocalization()
{
/*
 * be checked
 * Does not finish
 */
	if(this->executingGovr())
	{
		BOOST_LOG_TRIVIAL(info) << "execute test";
		this->executingTest();
		std::sort(vTarantulaScore.begin(),vTarantulaScore.end(),
					[](const TarantulaScore &tmp1, const TarantulaScore &tmp2)->
					bool {return &tmp1.score > &tmp2.score;});
	}
	return vTarantulaScore;
}

double FaultLocalization::tarantulaFormula(	const int &nf_e,
											const int &ns_e,
											const int &ns,
											const int &nf)
/*
 * must be check divide by zero before call this function
 */
{
	double a = (double)nf_e/(double)nf;
	double b = (double)ns_e/(double)ns;
	return a / (a + b);
}

void FaultLocalization::calculatingTarantulaScore(const std::vector<struct TarantulaArgs> &cTArgsCollection)
{
	for(auto &item : cTArgsCollection)
	{
		struct TarantulaScore tScore_tmp;
		tScore_tmp.line = item.line;
		if (item.nf_e == 0 && item.ns_e != 0)
		{
			tScore_tmp.score = 0;
		}
		else if (item.nf_e == 0 && item.ns_e == 0)
		{
			tScore_tmp.score = __INT32_MAX__;
		}
		else
		{
			tScore_tmp.score = this->tarantulaFormula(item.nf_e,item.ns_e,this->nf,this->ns);
		}
		this->vTarantulaScore.push_back(tScore_tmp);
	}
}

void FaultLocalization::acquiringTarantulaArgs(struct SpectrumBased &SBasedCollection)
{
	/**
	 * check whether Spectrum based collection is empty
	 */
	if (!vSpectrumBased.empty())
	{
		int vSpectrumSize = vSpectrumBased.size();
		if (vSpectrumSize > 1)
		{
			for (int i = 1; i < vSpectrumSize; i++)
			{
				int contentSize = vSpectrumBased[i].contentOfXML.size();
				for (int j = 0; j < contentSize; j++)
				{
					auto results = std::find_if(vTarantulaArgs.begin(), vTarantulaArgs.end(),
												[=](const TarantulaArgs &tmp) ->
												bool {return tmp.line == vSpectrumBased[i].contentOfXML[j].line;});
					/*
					 * check whether line number was existed
					 */
					if (results != vTarantulaArgs.end())
					{
						auto index = results - vTarantulaArgs.begin();
						/**
						 * check whether index is out of bound
						 */
						if (index < vTarantulaArgs.size() && index >= 0)
						{
							/**
							 * check whether line is reached in this test case
							 */
							if (vSpectrumBased[i].contentOfXML[j].hits != 0
								&& vSpectrumBased[i].contentOfXML[j].hits > vSpectrumBased[i - 1].contentOfXML[j].hits)
							{
								vSpectrumBased[i].statusOfTestCase ? vTarantulaArgs[index].ns_e++ : vTarantulaArgs[index].nf_e++;
							}
						}
						else
						{
							/**
							 * creating new structure to archive line reaching frequency
							 */
							struct TarantulaArgs tArgsTmp;
							this->creatingNewTarantulaArgs(tArgsTmp);
							tArgsTmp.line = vSpectrumBased[i].contentOfXML[j].line;
							if (vSpectrumBased[i].contentOfXML[j].hits != 0
								&& vSpectrumBased[i].contentOfXML[j].hits > vSpectrumBased[i - 1].contentOfXML[j].hits)
							{
								vSpectrumBased[i].statusOfTestCase ? tArgsTmp.ns_e++ : tArgsTmp.nf_e++;
							}
							vTarantulaArgs.push_back(tArgsTmp);
						}
					}
					/**
					 * end if()
					 */
					vSpectrumBased[i].statusOfTestCase ? this->ns++ : this->nf++;
				}
				/**
				 * end for(;;)
				 */
			}
			/**
			 * end for(;;)
			 */
		}
	}
}

void FaultLocalization::executingTest()
{
	/**
	 * main of class
	 */
	if (!this->tests.empty())
	{
		struct SpectrumBased sBasedTmp;
		TestStatus status;
		for(auto &test : tests)
		{
			sBasedTmp.contentOfXML.clear();
			status = tester.execute(test);
			BOOST_LOG_TRIVIAL(info) << "test is executing";
			sBasedTmp.contentOfXML = this->analyzingXMLFiles("/tmp/" + this->generatingXMLFiles(test));
			if (status == TestStatus::PASS)
			{
				/*
				 * test success
				 */
				this->ns++;
				sBasedTmp.statusOfTestCase = true;
			}
			else
			{
				/*
				 * test failure
				 */
				this->nf++;
				sBasedTmp.statusOfTestCase = false;
			}
			this->vSpectrumBased.push_back(sBasedTmp);
		}
		if (vTarantulaArgs.empty())
		{
			if (status == TestStatus::PASS)
			{
				this->ns = 1;
				for (auto &item : sBasedTmp.contentOfXML)
				{
					struct TarantulaArgs tArgsTmp;
					this->creatingNewTarantulaArgs(tArgsTmp);
					tArgsTmp.line = item.line;
					if (item.hits > 0)
					{
						tArgsTmp.ns_e = 1;
					}
					vTarantulaArgs.push_back(tArgsTmp);
				}
			}
			else
			{
				for (auto &item : sBasedTmp.contentOfXML)
				{
					struct TarantulaArgs tArgsTmp;
					this->creatingNewTarantulaArgs(tArgsTmp);
					tArgsTmp.line = item.line;
					if (item.hits > 0)
					{
						tArgsTmp.nf_e = 1;
					}
					vTarantulaArgs.push_back(tArgsTmp);
				}
			}
		}
	}
	else
	{
		BOOST_LOG_TRIVIAL(info) << "don't have any test case";
	}
}
/**
 * expecting XML output file of gcov
 * set full patch of XML file when was created success
 * be called after each test case was executed
 */
std::string FaultLocalization::generatingXMLFiles(const std::string &testID)
{
	std::stringstream cmd;
	std::stringstream pathToGeneration;
	std::string tmp;
	/**
	 * creating xml file
	 */
	pathToGeneration 	<< "/tmp/" << this->FolderTmp << "/"
						<< testID << ".xml";
	pathToGeneration >> tmp;
	cmd << "gcovr -r . --xml-pretty > " <<  tmp;
	/*
	 * return full file path if generation is success
	 */
	BOOST_LOG_TRIVIAL(info) << tmp.c_str();
	return (this->executingCMD(cmd.str()) == true) ? tmp : "";
}

std::vector<struct HitsEachLine> FaultLocalization::analyzingXMLFiles(const std::string &relpath)
{
	std::vector<struct HitsEachLine> vHELine;
	xml_document<> doc_tmp;
	if(this->isXMLFile(relpath))
	{
		std::ifstream file_tmp(relpath.c_str());
		std::stringstream buff_file;
		buff_file << file_tmp.rdbuf();
		file_tmp.close();
		std::string content(buff_file.str());
		doc_tmp.parse<0>(&content[0]);
		/*
		 * pointing to lines node
		 */
		xml_node<> *lineRoot = doc_tmp.first_node()->first_node("packages")->first_node()->first_node()->first_node()->first_node("lines")->first_node();
		while(lineRoot)
		{
			struct HitsEachLine hELine_tmp;
			std::stringstream ss, sn;
			ss << lineRoot->first_attribute("hits")->value();
			ss >> hELine_tmp.hits;
			sn << lineRoot->first_attribute("number")->value();
			sn >> hELine_tmp.line;
			vHELine.push_back(hELine_tmp);
		}
		return vHELine;
	}
	else
		return {};
}

std::string FaultLocalization::creatingFolderTmp()
{
	std::time_t curTime = std::time(NULL);
	char strBuff[128];
	if (std::strftime(strBuff, 128, "%d_%m_%Y_%T", std::localtime(&curTime)))
	{
		std::stringstream cmd;
		cmd << "mkdir -p" << " /tmp/" << strBuff;
		BOOST_LOG_TRIVIAL(info) << cmd.str().c_str();
		if (this->executingCMD(cmd.str()))
		{
			this->folderCreateFlag = true;
		}
	}
	return strBuff;
}

bool FaultLocalization::isXMLFile(const std::string &relpath) const
{
	bool results = false;
	if (!relpath.empty())
	{
		if (relpath.find(".XML") != std::string::npos || relpath.find(".xml") != std::string::npos)
		{
			results = true;
		}
	}
	return results;
}

bool FaultLocalization::isSourceFile(const boost::filesystem::path &relpath) const
{
	bool results = false;
	std::string p_tmp = relpath.string();
	if (!p_tmp.empty())
	{
		if (p_tmp.find(".c") != std::string::npos || p_tmp.find(".cpp") != std::string::npos)
		{
			results = true;
		}
	}
	return results;
}

std::string FaultLocalization::getPathFolderTmp()
{
	return this->FolderTmp;
}

bool FaultLocalization::executingCMD(const std::string &Cmd)
{
	bool results = false;
	if (!Cmd.empty())
	{
		unsigned long status = std::system(Cmd.c_str());
		if (WEXITSTATUS(status) == 0)
		{
			// folder is created success
			results = true;
		}
	}
	return results;
}


bool FaultLocalization::executingGovr()
{
	bool result = false;
	int numSourceFile = 0;
	std::stringstream cmd;
	cmd << "gcc -fprofile-arcs -ftest-coverage -o ";
	for (auto &file : project.getFiles())
	{
		if(isSourceFile(file.relpath))
		{
			numSourceFile++;
			std::string sFile = file.relpath.string();
			int pos = sFile.find(".");
			sFile.erase(sFile.begin()+pos,sFile.end());
			cmd << sFile.c_str() << " " << file.relpath.string().c_str();
			if (executingCMD(cmd.str()))
			{
				result = true;
			}
		}
	}
	return result;
}

