#include "yaml-cpp/parser.h"
#include "yaml-cpp/eventhandler.h"
#include "yaml-cpp/exceptions.h"
#include "directives.h"
#include "scanner.h"
#include "singledocparser.h"
#include "tag.h"
#include "token.h"
#include <sstream>
#include <cstdio>

namespace YAML
{
	Parser::Parser()
	{
	}
	
	Parser::Parser(std::istream& in)
	{
		Load(in);
	}

	Parser::~Parser()
	{
	}

	Parser::operator bool() const
	{
		return m_pScanner.get() && !m_pScanner->empty();
	}

	void Parser::Load(std::istream& in)
	{
		m_pScanner.reset(new Scanner(in));
		m_pDirectives.reset(new Directives);
	}

	// HandleNextDocument
	// . Handles the next document
	// . Throws a ParserException on error.
	// . Returns false if there are no more documents
	bool Parser::HandleNextDocument(EventHandler& eventHandler)
	{
		if(!m_pScanner.get())
			return false;

		ParseDirectives();
		if(m_pScanner->empty())
			return false;
		
		SingleDocParser sdp(*m_pScanner, *m_pDirectives);
		sdp.HandleDocument(eventHandler);
		return true;
	}

	// ParseDirectives
	// . Reads any directives that are next in the queue.
	void Parser::ParseDirectives()
	{
		bool readDirective = false;

		while(1) {
			if(m_pScanner->empty())
				break;

			Token& token = m_pScanner->peek();
			if(token.type != Token::DIRECTIVE)
				break;

			// we keep the directives from the last document if none are specified;
			// but if any directives are specific, then we reset them
			if(!readDirective)
				m_pDirectives.reset(new Directives);

			readDirective = true;
			HandleDirective(token);
			m_pScanner->pop();
		}
	}

	void Parser::HandleDirective(const Token& token)
	{
		if(token.value == "YAML")
			HandleYamlDirective(token);
		else if(token.value == "TAG")
			HandleTagDirective(token);
	}

	// HandleYamlDirective
	// . Should be of the form 'major.minor' (like a version number)
	void Parser::HandleYamlDirective(const Token& token)
	{
		if(token.params.size() != 1)
			throw ParserException(token.mark, ErrorMsg::YAML_DIRECTIVE_ARGS);
		
		if(!m_pDirectives->version.isDefault)
			throw ParserException(token.mark, ErrorMsg::REPEATED_YAML_DIRECTIVE);

		std::stringstream str(token.params[0]);
		str >> m_pDirectives->version.major;
		str.get();
		str >> m_pDirectives->version.minor;
		if(!str || str.peek() != EOF)
			throw ParserException(token.mark, std::string(ErrorMsg::YAML_VERSION) + token.params[0]);

		if(m_pDirectives->version.major > 1)
			throw ParserException(token.mark, ErrorMsg::YAML_MAJOR_VERSION);

		m_pDirectives->version.isDefault = false;
		// TODO: warning on major == 1, minor > 2?
	}

	// HandleTagDirective
	// . Should be of the form 'handle prefix', where 'handle' is converted to 'prefix' in the file.
	void Parser::HandleTagDirective(const Token& token)
	{
		if(token.params.size() != 2)
			throw ParserException(token.mark, ErrorMsg::TAG_DIRECTIVE_ARGS);

		const std::string& handle = token.params[0];
		const std::string& prefix = token.params[1];
		if(m_pDirectives->tags.find(handle) != m_pDirectives->tags.end())
			throw ParserException(token.mark, ErrorMsg::REPEATED_TAG_DIRECTIVE);
		
		m_pDirectives->tags[handle] = prefix;
	}

	void Parser::PrintTokens(std::ostream& out)
	{
		if(!m_pScanner.get())
			return;
		
		while(1) {
			if(m_pScanner->empty())
				break;

			out << m_pScanner->peek() << "\n";
			m_pScanner->pop();
		}
	}
}
