#include "Parser.h"
#include "../BL/Regex.h"
#include "../BL/LoopStatement.h"
#include "../BL/IfStatement.h"
#include "../BL/HTML.h"
#include "../Config/Config.h"
#include <regex>
#include <algorithm>
#include <queue>

bool Parser::matchString(const std::string& str, const std::string& regexStr)
{
	return std::regex_search(str.begin(), str.end(), std::regex(regexStr));
}

std::string Parser::parseInline(const std::string& code, ContextBase* context)
{
	std::string result("");
	if (!Parser::matchString(code, REGEX::INLINE_TAG_REGEX))
	{
		result = code;
	}
	else
	{
		std::regex expression(REGEX::INLINE_TAG_REGEX);
		std::sregex_iterator begin(code.begin(), code.end(), expression), end;
		size_t pos = 0;
		for (auto it = begin; it != end; it++)
		{
			std::string currentLine(it->str());
			result += std::string(code.begin() + pos, code.begin() + (size_t)it->position());
			pos = it->position() + currentLine.size();
			if (Parser::matchString(currentLine, REGEX::VAR_REGEX))
			{
				if (context)
				{
					std::string contextValue(context->getByKey(std::regex_replace(currentLine, std::regex("\\W+"), "")));
					if (contextValue.size() > 0)
					{
						result += contextValue;
					}
					else
					{
						pos -= currentLine.size();
					}
				}
				else
				{
					pos -= currentLine.size();
				}
			}
			else if (Parser::matchString(currentLine, REGEX::INCLUDE_TAG_REGEX))
			{
				size_t startOffset = currentLine.find("\"") + 1, endOffset = currentLine.find("\"", startOffset);
				std::string snippet = HTML::read(CONFIG::TEMPLATE_DIR + std::string(currentLine.begin() + startOffset, currentLine.begin() + endOffset));
				result += Parser::parseInline(Parser::parseTemplate(snippet, context), context);
			}
			else if (Parser::matchString(currentLine, REGEX::IMAGE_TAG_REGEX))
			{
				size_t k = 0;
				size_t startOffset = currentLine.find("'"), endOffset;
				if (startOffset != std::string::npos)
				{
					endOffset = currentLine.find("'", ++startOffset);
					if (endOffset == std::string::npos)
					{
						throw RenderError("Parser::parseInline(): invalid template syntax.", __FILE__, __LINE__, currentLine);
					}
				}
				else
				{
					throw RenderError("Parser::parseInline(): invalid template syntax.", __FILE__, __LINE__, currentLine);
				}
				std::string imageName(currentLine.begin() + startOffset, currentLine.begin() + endOffset);
				result += CONFIG::MEDIA_DIR;
				if (imageName.find('.') == std::string::npos)
				{
					if (context)
					{
						result += context->getByKey(std::regex_replace(imageName, std::regex("\\W+"), ""));
					}
				}
				else
				{
					result += imageName;
				}
			}
		}
		result += std::string(code.begin() + pos, code.begin() + code.size());
	}
	return result;
}

size_t Parser::findSubstringPosition(const std::string& str, const std::string& regex, bool reverse)
{
	size_t result = 0;
	std::smatch data;
	if (std::regex_search(str, data, std::regex(regex)))
	{
		if (reverse)
		{
			result = std::abs((int)(data.position() - str.size())) - data.str().size();
		}
		else
		{
			result = data.position() + data.str().size();
		}
	}
	else
	{
		throw std::exception();
	}
	return result;
}

block Parser::findBlock(size_t& pos, const std::string& code)
{
	size_t foundPos = 0;
	block result;
	std::regex expression(REGEX::BLOCK_TAG_REGEX);
	std::smatch data;
	if (std::regex_search(code.begin() + pos, code.end(), data, expression))
	{
		foundPos = data.position() + pos;
		result.before = std::string(code.begin() + pos, code.begin() + foundPos);
		std::queue<size_t> endPositions, beginPositions;
		size_t begin = 0, end = code.size();
		blockParams blockParameters(foundPos, 0, false, false, REGEX::BEGIN_TAG_REGEX, REGEX::END_TAG_REGEX);
		do
		{
			Parser::findTag(code, blockParameters);
			if (blockParameters.first)
			{
				beginPositions.push(blockParameters.foundPos);
			}
			else if (blockParameters.second)
			{
				endPositions.push(blockParameters.foundPos + blockParameters.offset);
			}
			if (beginPositions.size() < endPositions.size())
			{
				size_t endPos = endPositions.back();
				std::string err(code.begin() + code.rfind("{", endPos), code.begin() + endPos);
				std::string errorLine = "tag '" + err + "' has invalid opening tag or it is missed.";
				throw RenderError("Parser::findBlock(): invalid template syntax.", __FILE__, __LINE__, errorLine);
			}
			if (beginPositions.size() == endPositions.size())
			{
				begin = beginPositions.front();
				end = endPositions.back();
				break;
			}
		} while (beginPositions.size() != 0);
		result.code += std::string(code.begin() + begin, code.begin() + end);
		if (std::regex_search(code.begin() + end, code.end(), data, expression))
		{
			foundPos = data.position() + end;
		}
		else
		{
			foundPos = code.size();
		}
		result.after += std::string(code.begin() + end, code.begin() + foundPos);
		pos = foundPos;
	}
	else
	{
		result.after = code;
		pos = code.size();
	}
	return result;
}

void Parser::findAllBlocks(std::vector<block>& blocks, const std::string& code)
{
	size_t pos = 0;
	while (pos < code.size())
	{
		blocks.push_back(Parser::findBlock(pos, code));
	}
}

void Parser::findTag(const std::string& str, blockParams& params)
{
	std::regex expression("(" + params.regexFirst + ")|(" + params.regexSecond + ")");
	std::smatch data;
	if (std::regex_search(str.begin() + params.foundPos + params.offset, str.end(), data, expression))
	{
		if (Parser::matchString(data.str(), params.regexFirst))
		{
			params.first = true;
			params.second = false;
		}
		else if (Parser::matchString(data.str(), params.regexSecond))
		{
			params.first = false;
			params.second = true;
		}
		params.foundPos += data.position() + params.offset;
		params.offset = data.str().size();
	}
	else
	{
		size_t begin = params.foundPos + params.offset;
		std::string errorLine(str.begin() + begin, str.begin() + str.find("}", begin) + 1);
		throw RenderError("Parser::findTag(): incorrect tag.", __FILE__, __LINE__, errorLine);
	}
}

codeType Parser::getCodeType(const std::string& code)
{
	codeType result;
	size_t endPos = 0;
	try
	{
		endPos = Parser::findSubstringPosition(code, "%\\s*\\}");
	}
	catch (const std::exception&)
	{
		throw RenderError("Parser::codeType(): invalid template syntax.", __FILE__, __LINE__);
	}
	std::string statement(code.begin(), code.begin() + endPos);
	if (Parser::matchString(statement, REGEX::FOR_REGEX))
	{
		result = codeType::forLoop;
	}
	else if (Parser::matchString(statement, REGEX::IF_REGEX))
	{
		result = codeType::ifStatament;
	}
	else if (Parser::matchString(statement, REGEX::BEGIN_COMMENT_REGEX))
	{
		result = codeType::comment;
	}
	else
	{
		throw RenderError("Parser::codeType(): invalid code type.", __FILE__, __LINE__);
	}
	return result;
}

std::string Parser::parseTemplate(const std::string& code, ContextBase* context)
{
	std::string result("");
	std::vector<block> blocks;
	Parser::findAllBlocks(blocks, code);
	for (std::vector<block>::iterator block = blocks.begin(); block != blocks.end(); block++)
	{
		if ((*block).before.size() == 0 && (*block).code.size() == 0)
		{
			result += (*block).after;
			return result;
		}
		result += (*block).before;
		result += Parser::parseTemplate(Parser::executeCode((*block).code, context), context);
		result += (*block).after;
	}
	return result;
}

std::string Parser::executeCode(const std::string& code, ContextBase* context)
{
	forLoopParams forParameters;
	ifParams ifParameters;
	std::string body(""), result("");
	switch (Parser::getCodeType(code))
	{
	case codeType::forLoop:
		body = LoopStatement::parse(code, forParameters);
		result = LoopStatement::execute(body, forParameters);
		break;
	case codeType::ifStatament:
		body = IfStatement::parse(code, ifParameters);
		result = IfStatement::execute(body, ifParameters, context);
		break;
	case codeType::comment:
		result = "";
		break;
	default:
		throw RenderError("Parser::executeCode(): incorrect type of code.", __FILE__, __LINE__);
	}
	return result;
}
