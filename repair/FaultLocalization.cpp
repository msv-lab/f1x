/*
 * include XML parse 3th library - rapid XML
 */
#include <rapidxml/rapidxml.hpp>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
/*
 */
#include <boost/log/trivial.hpp>
#include <boost/filesystem/fstream.hpp>

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
#include "Global.h"

namespace fs = boost::filesystem;
namespace json = rapidjson;
using namespace rapidxml;

FaultLocalization::FaultLocalization(const std::vector<std::string> &tests,
									 TestingFramework &tester,
									 Project &project
									 ):
									tests(tests),
									tester(tester),
									project(project){
	this->FolderTmp = this->creatingFolderTmp();
	/*
	 * test whether folder is created
	 */
}
FaultLocalization::~FaultLocalization() {}

void FaultLocalization::clearingDataStruct()
{
	this->vSpectrumBased.clear();
	this->vTarantulaArgs.clear();
	this->vTarantulaScore.clear();
}

std::vector<struct XMLCoverageFile> FaultLocalization::getFaultLocalization(const std::vector<std::string> &vFiles)
{
/*
 * be checked
 * Does not finish
 */
	//this->getFileFromJson();
	if(project.build())
	{
		for (auto file : vFiles)
		{
			XMLCoverageFile xCovFileTmp;
			this->clearingDataStruct();
			this->executingTest(file);
			this->acquiringTarantulaArgs();
			this->calculatingTarantulaScore(vTarantulaArgs);
			xCovFileTmp.fileName = file;
			xCovFileTmp.vTScore = vTarantulaScore;
			vXMLCovFile.push_back(xCovFileTmp);
		}
		return vXMLCovFile;
	}
	return {};
}

std::vector<std::string> FaultLocalization::getFileFromJson()
{
	std::vector<std::string> fileFromJson;
	fs::path compileDB("compile_commands.json");
	json::Document db;
	/*
	 * parse JSON file to array
	 */
	fs::ifstream ifs(compileDB);
	json::IStreamWrapper isw(ifs);
	db.ParseStream(isw);

	/*
	 * collecting all file from JSON file
	 */
	for (auto &entry : db.GetArray())
	{
		std::string dbFile = entry.GetObject()["file"].GetString();
		std::stringstream fTmp;
		fTmp << std::string(basename(fs::path(dbFile.c_str())))
			 << "."
			 << std::string(fs::extension(fs::path(dbFile.c_str())));
		if (isSourceFile(fs::path(fTmp.str())))
			fileFromJson.push_back(fTmp.str());
	}
	return fileFromJson;
}

double FaultLocalization::tarantulaFormula(	const int &nf_e,
											const int &ns_e,
											const int &nf,
											const int &ns)
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

void FaultLocalization::acquiringTarantulaArgs()
{
	/**
	 * check whether Spectrum based collection is empty
	 */
	if (!vSpectrumBased.empty())
	{
		int vSpectrumSize = vSpectrumBased.size();
		for (int i = 0; i < vSpectrumSize; i++)
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
						if (vSpectrumBased[i].contentOfXML[j].hits != 0)
						{
							vSpectrumBased[i].statusOfTestCase ? vTarantulaArgs[index].ns_e++ : vTarantulaArgs[index].nf_e++;
						}
					}
				}
				else
				{
					if(vSpectrumBased[i].statusOfTestCase)
					{
						/*
						 * test case is true
						 */
						(vSpectrumBased[i].contentOfXML[j].hits > 0) ?
							vTarantulaArgs.push_back(TarantulaArgs(vSpectrumBased[i].contentOfXML[j].line,0,1)) :
							vTarantulaArgs.push_back(TarantulaArgs(vSpectrumBased[i].contentOfXML[j].line));

					}
					else
					{
						/*
						 * test case is false
						 */
						(vSpectrumBased[i].contentOfXML[j].hits > 0) ?
							vTarantulaArgs.push_back(TarantulaArgs(vSpectrumBased[i].contentOfXML[j].line,1,0)) :
							vTarantulaArgs.push_back(TarantulaArgs(vSpectrumBased[i].contentOfXML[j].line));
					}
				}
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

void FaultLocalization::executingTest(const std::string &fName)
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
			sBasedTmp.contentOfXML = this->analyzingXMLFiles(this->generatingXMLFiles(test),fName);
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
	pathToGeneration 	<< this->FolderTmp << "/"
						<< testID << ".xml";
	pathToGeneration >> tmp;
	/*
	 * Defines the root directory for source files
	 * Generate XML instead of the normal tabular output
	 * Delete the coverage files after they are processed
	 */
	cmd << "gcovr -r . -x -d > " <<  tmp;
	/*
	 * return full file path if generation is success
	 */
	return (this->executingCMD(cmd.str()) == true) ? tmp : "";
}

std::vector<struct HitsEachLine> FaultLocalization::analyzingXMLFiles(const std::string &relpath,const std::string &fName)
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
		 * pointing to classes
		 */
		xml_node<> *classRoot = doc_tmp.first_node()->first_node("packages")->first_node()->first_node()->first_node();
		xml_node<> *cRoottmp = NULL;
		while(classRoot)
		{
			std::stringstream fn;
			std::string fileName;
			fn << classRoot->first_attribute("filename")->value();
			fn >> fileName;
			if (fileName.compare(fName) == 0)
			{
				cRoottmp = classRoot;
			}
			classRoot = classRoot->next_sibling();
		}
		/*
		 * pointing to lines node
		 */
		if (cRoottmp != nullptr)
		{
			xml_node<> *lineRoot = cRoottmp->first_node("lines")->first_node();
			while(lineRoot)
			{
				struct HitsEachLine hELine_tmp;
				std::stringstream ss, sn;
				ss << lineRoot->first_attribute("hits")->value();
				ss >> hELine_tmp.hits;
				sn << lineRoot->first_attribute("number")->value();
				sn >> hELine_tmp.line;
				vHELine.push_back(hELine_tmp);
				lineRoot = lineRoot->next_sibling();
			}
		}
		//xml_node<> *lineRoot = doc_tmp.first_node()->first_node("packages")->first_node()->first_node()->first_node()->first_node("lines")->first_node();
		//xml_node<> *lineRoot = doc_tmp.first_node();
		return vHELine;
	}
	else
		return {};
}

std::string FaultLocalization::creatingFolderTmp()
{
	std::string covDir;
	std::stringstream cmd;
	covDir = cfg.dataDir + "/coverage";
	cmd << "mkdir -p " << covDir;
	if (this->executingCMD(cmd.str()))
	{
		this->folderCreateFlag = true;
	}
	return covDir;
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

bool FaultLocalization::isSourceFile(const boost::filesystem::path &relpath)
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
		std::stringstream cmd;
		cmd << "timeout 1s " << Cmd;
		unsigned long status = std::system(cmd.str().c_str());
		if (WEXITSTATUS(status) == 0)
		{
			// folder is created success
			results = true;
		}
	}
	return results;
}



