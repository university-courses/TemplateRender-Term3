/*
#define _CRT_SECURE_NO_WARNINGS
#include "Functions.h"
#include <fstream>
#include <iostream>
#include <Windows.h>

std::string HelperFunctions::getCppHtmlCode(const std::string& fileName)
{
	std::string result("");
	if (fileName.size() != 0)
	{
		std::ifstream file;
		file.open(fileName);

		if (file.is_open())
		{
			result.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			file.close();
		}
		else
		{
			std::cerr << "Error occurred in 'HelperFunctions::getCppHtmlCode()' function: can not open file " << fileName << '\n';
		}
	}
	else
	{
		std::cerr << "Error occurred in 'HelperFunctions::getCppHtmlCode()' function: incorrect path to html file.\n";
	}
	return result;
}

bool HelperFunctions::createHtmlPage(const std::string & htmlCode, const std::string& fileName)
{
	std::ofstream file(fileName);
	if (file.is_open())
	{
		file << htmlCode;
		file.close();
		return true;
	}
	std::cerr << "Error occurred in 'HelperFunctions::createHtmlPage()' function: can not write data to file '" << fileName << "'\n";
	return false;
}

bool HelperFunctions::createCpp(const std::string& cppCode, const std::string& fileName)
{
	if (!HelperFunctions::directoryExists(CONSTANT::TEMP_DIR))
	{
		if (!HelperFunctions::run("mkdir " + CONSTANT::TEMP_DIR))
		{
			std::cerr << "Error occurred in 'HelperFunctions::createCpp()' function: can not create folder.\n";
			return false;
		}
	}
	std::ofstream file(CONSTANT::TEMP_DIR + "\\" + fileName);
	if (file.is_open())
	{
		file << cppCode + '\n';
		file.close();
		return true;
	}
	std::cerr << "Error occurred in 'HelperFunctions::createCpp()' function: file was not opened for writting.\n";
	return false;
}

std::string HelperFunctions::createCompletedCppCode(const std::string& mainPartOfCppCode)
{
	return std::string(PROGRAM_BEGIN + mainPartOfCppCode + PROGRAM_END);
}

bool HelperFunctions::compile(const std::string& cppFilePath)
{
	if (cppFilePath.size() != 0)
	{
		std::string command(CONSTANT::MINGW_PATH + "\\g++ " + CONSTANT::TEMP_DIR + "\\" + cppFilePath +
			" -o " + CONSTANT::TEMP_DIR + "\\" + cppFilePath.substr(0, cppFilePath.size() - 4));
		if (HelperFunctions::run(command))
		{
			return true;
		}
		else
		{
			std::cerr << "Error occurred in 'HelperFunctions::compile()' function: compilation error.\n";
		}
	}
	else
	{
		std::cerr << "Error occurred in 'HelperFunctions::compile()' function: path to cpp file is none.\n";
	}
	return false;
}

bool HelperFunctions::run(const std::string& command)
{
	if (command.size() != 0 || command.find_first_not_of(' ') != std::string::npos)
	{
		STARTUPINFO startupInfo;
		PROCESS_INFORMATION processInfo;
		ZeroMemory(&startupInfo, sizeof(startupInfo));
		startupInfo.cb = sizeof(startupInfo);
		ZeroMemory(&processInfo, sizeof(processInfo));
		if (CreateProcess(NULL, const_cast<char*>(("cmd /c " + command).c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo))
		{
			WaitForSingleObject(processInfo.hProcess, INFINITE);
			if (CloseHandle(processInfo.hProcess))
			{
				if (CloseHandle(processInfo.hThread))
				{
					return true;
				}
				else
				{
					std::cerr << "Error occurred in 'HelperFunctions::run()' function: thread of process '" << command << 
						"' was not closed properly. Error: #" + GetLastError() + '\n';
				}
			}
			else
			{
				std::cerr << "Error occurred in 'HelperFunctions::run()' function: process '" << command <<
					"' was not closed properly. Error: #" + GetLastError() + '\n';
			}
		}
		else
		{
			std::cerr << "Error occurred in 'HelperFunctions::run()' function: process '" << command <<
				"' was not executed. Error: #" << GetLastError() << '\n';
		}
	}
	else
	{
		std::cerr << "Error occurred in 'HelperFunctions::run()' function: incorrect command.\n";
	}
	return false;
}

bool HelperFunctions::directoryExists(const std::string& directory)
{
	DWORD dirAttributes = GetFileAttributesA(directory.c_str());
	return dirAttributes != INVALID_FILE_ATTRIBUTES && dirAttributes & FILE_ATTRIBUTE_DIRECTORY;
}*/

#include <iostream>
#include <fstream>
#include <regex>
#include <algorithm>
#include <stack>
#include "Functions.h"
#include "Constants.h"
#include "../Render/Parser.h"

std::string HelperFunctions::retrieveBodyForLoop(const std::string& code, int& numberOfIteration, bool& increment, bool& fewer)
{
	string result = "";
	std::regex forRegex(CONSTANT::FOR_REGEX);
	std::smatch loopCondt;
	std::regex_search(code, loopCondt, forRegex);
	string loopCondition = loopCondt.str();
	int startCount = stoi(loopCondition.substr(loopCondition.find('=')+1, loopCondition.find(';', loopCondition.find('='))));
	int endCount;
	if (loopCondition.find('<') != std::string::npos)
	{
		fewer = true;
		endCount = stoi(loopCondition.substr(loopCondition.find('<')+1, loopCondition.find(';', loopCondition.find('<'))));
		if (loopCondition.find("++") != std::string::npos)
		{
			numberOfIteration = endCount - startCount;
			increment = true;
		}
		else
		{
			throw  std::exception("Incorrect loop condition");
		}
	}
	else if(loopCondition.find('>') != std::string::npos)
	{
		fewer = false;
		endCount = stoi(loopCondition.substr(loopCondition.find('>') + 1, loopCondition.find(';', loopCondition.find('>'))));
		if (loopCondition.find("--") != std::string::npos)
		{
			increment = false;
			numberOfIteration = startCount - endCount;
		}
		else
		{
			throw  std::exception("Incorrect loop condition");
		}
	}
	else
	{
		throw  std::exception("Incorrect loop condition");
	}
	std::copy(code.begin() + loopCondition.length(), code.end() - 1, result);	
	return result;
}

size_t HelperFunctions::codeType(const std::string& code)
{
	int result;
	
	bool checkFor = Parser::regexCheck(code, CONSTANT::FOR_REGEX);
	//bool checkForeach = Parser::regexCheck(code, foreachRegex);
	bool checkIf = Parser::regexCheck(code, CONSTANT::IF_REGEX);
	if (checkFor)
	{
		result = 1;
	}
	/*else if (checkForeach)
	{
		result = 2;
	}*/
	else if (checkIf)
	{
		result = 3;
	}
	return result;
}

std::string HelperFunctions::runCode(const std::string& code)
{
	int numberOfIters = 0;
	bool increment, fewer;
	std::string body;
	std::string result = "";
	switch (HelperFunctions::codeType(code))
	{
	case 1:
		body = HelperFunctions::retrieveBodyForLoop(code, numberOfIters, increment, fewer);
		HelperFunctions::forLoop(body, result, numberOfIters, increment, fewer);
		break;
	case 2:
		// TODO: foreach statement (result += ...)
		break;
	case 3:
		// TODO: if statement (result += ...)
		break;
	default:
		throw exception("Incorrect type of code");
		break;
	}
	return result;
}

std::string HelperFunctions::parse(const std::string& code)
{
	std::smatch data;
	std::regex regexBegin("(" + CONSTANT::FOR_REGEX + ")|(" + CONSTANT::IF_REGEX + ")");
	std::regex regexEnd("(" + CONSTANT::END_FOR_REGEX + ")|(" + CONSTANT::IF_REGEX + ")");
	std::string result("");
	size_t end = code.find("{%");
	if (end == string::npos)
	{
		result += code;
	}
	else
	{
		std::list<std::string> blocks;
		HelperFunctions::findAllBlocks(blocks, code);
		for (std::list<std::string>::iterator i = blocks.begin(); i != blocks.end(); i++)
		{
			end = code.find("{%");
			for (size_t i = 0; i < end; i++)
			{
				result += code[i];
			}
			if (std::regex_search(code, data, regexBegin))
			{
				size_t begin = data.position(0);
				if (std::regex_search(code, data, regexEnd))
				{
					size_t end = data.position(data.size() - 1);
					std::string completedPart = HelperFunctions::runCode(string(begin, end));
					result += HelperFunctions::parse(completedPart);

					end = end + data[data.size() - 1].length();

					for (size_t i = end; i < code.size(); i++)
					{
						result += code[i];
					}
				}
				else
				{
					throw std::exception("Invalid syntax in html page");
				}
			}
		}
	}
	return result;
}

std::string HelperFunctions::readTemplate(const std::string& templateName)
{
	ifstream read;
	read.open(templateName);
	std::string result="";
	if (read.is_open())
	{
		result.assign((std::istreambuf_iterator<char>(read)), std::istreambuf_iterator<char>());
		read.close();
	}
	else
	{
		throw  std::exception("File not found");
	}
	return result;
}

void HelperFunctions::createHTML(const std::string& html, const std::string& htmlPath)
{
	std::ofstream file(htmlPath);
	if (file.is_open())
	{
		file << html;
		file.close();
	}
	else
	{
		throw std::exception(("Can not open file '" + htmlPath + "' for writting.").c_str());
	}
}

void HelperFunctions::render(const std::string& templatePath, const std::string& htmlPath)
{
	std::string templateHTML = HelperFunctions::readTemplate("index.html");
	std::string completedHTML = HelperFunctions::parse(templateHTML);
	HelperFunctions::createHTML(completedHTML, "Completed.html");
}

void HelperFunctions::forLoop(const std::string& loopBody, std::string& result, const int& numberOfIteration, const bool& increment, const bool& fewer)
{
	if (increment)
	{
		if (fewer)
		{
			for (int i = 0; i < numberOfIteration; i++)
			{
				result += loopBody;
			}
		}
		else
		{
			throw  std::exception("Invalid loop condition");
		}
	}
	if (!increment)
	{
		if (!fewer)
		{
			for (int i = numberOfIteration; i > 0; i--)
			{
				result += loopBody;
			}
		}
		else
		{
			throw  std::exception("Invalid loop condition");
		}
	}
}

string HelperFunctions::findBlock(int& pos, const std::string& code)
{
	std::stack<int> beginPositions;
	std::stack<int> endPositions;
	size_t foundPos = 0;
	size_t begin = 0;
	size_t end = 0;
	do
	{
		// TODO: search for "{% for" or "for %}"
		//
		//		 if ("{% for" is found)
		//		 {
		//			beginPositions.push(foundPos);
		//		 }
		//		 else if ("for %}" is found)
		//		 {
		//			endPositions.push(foundPos);
		//			if (beginPositions.size() < 2)
		//			{
		//				begin = beginPositions.top();
		//				end = endPositions.top();
		//			}
		//			beginPositions.pop();
		//		 }
		//		 
	} while (beginPositions.size() == 0 || foundPos == std::string::npos);
	if (foundPos == std::string::npos || (end + 6) >= code.size())	// 6 -- size of "for %}"
	{
		pos = code.size();
	}
	else
	{
		pos = end + 6;	// 6 -- size of "for %}"
	}
	return std::string(code[begin], code[pos]);
}

void HelperFunctions::findAllBlocks(std::list<std::string>& blocks, const std::string& code)
{
	int pos = 0;
	while (pos <= code.size())
	{
		blocks.push_back(HelperFunctions::findBlock(pos, code));
	}
}
