#ifndef EXCEPTIONS_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define EXCEPTIONS_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) || (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "yaml-cpp/mark.h"
#include "yaml-cpp/traits.h"
#include <stdexcept>
#include <string>
#include <sstream>

namespace YAML
{
	// error messages
	namespace ErrorMsg
	{
		const char * const YAML_DIRECTIVE_ARGS    = "YAML directives must have exactly one argument";
		const char * const YAML_VERSION           = "bad YAML version: ";
		const char * const YAML_MAJOR_VERSION     = "YAML major version too large";
		const char * const REPEATED_YAML_DIRECTIVE= "repeated YAML directive";
		const char * const TAG_DIRECTIVE_ARGS     = "TAG directives must have exactly two arguments";
		const char * const REPEATED_TAG_DIRECTIVE = "repeated TAG directive";
		const char * const CHAR_IN_TAG_HANDLE     = "illegal character found while scanning tag handle";
		const char * const TAG_WITH_NO_SUFFIX     = "tag handle with no suffix";
		const char * const END_OF_VERBATIM_TAG    = "end of verbatim tag not found";
		const char * const END_OF_MAP             = "end of map not found";
		const char * const END_OF_MAP_FLOW        = "end of map flow not found";
		const char * const END_OF_SEQ             = "end of sequence not found";
		const char * const END_OF_SEQ_FLOW        = "end of sequence flow not found";
		const char * const MULTIPLE_TAGS          = "cannot assign multiple tags to the same node";
		const char * const MULTIPLE_ANCHORS       = "cannot assign multiple anchors to the same node";
		const char * const MULTIPLE_ALIASES       = "cannot assign multiple aliases to the same node";
		const char * const ALIAS_CONTENT          = "aliases can't have any content, *including* tags";
		const char * const INVALID_HEX            = "bad character found while scanning hex number";
		const char * const INVALID_UNICODE        = "invalid unicode: ";
		const char * const INVALID_ESCAPE         = "unknown escape character: ";
		const char * const UNKNOWN_TOKEN          = "unknown token";
		const char * const DOC_IN_SCALAR          = "illegal document indicator in scalar";
		const char * const EOF_IN_SCALAR          = "illegal EOF in scalar";
		const char * const CHAR_IN_SCALAR         = "illegal character in scalar";
		const char * const TAB_IN_INDENTATION     = "illegal tab when looking for indentation";
		const char * const FLOW_END               = "illegal flow end";
		const char * const BLOCK_ENTRY            = "illegal block entry";
		const char * const MAP_KEY                = "illegal map key";
		const char * const MAP_VALUE              = "illegal map value";
		const char * const ALIAS_NOT_FOUND        = "alias not found after *";
		const char * const ANCHOR_NOT_FOUND       = "anchor not found after &";
		const char * const CHAR_IN_ALIAS          = "illegal character found while scanning alias";
		const char * const CHAR_IN_ANCHOR         = "illegal character found while scanning anchor";
		const char * const ZERO_INDENT_IN_BLOCK   = "cannot set zero indentation for a block scalar";
		const char * const CHAR_IN_BLOCK          = "unexpected character in block scalar";
		const char * const AMBIGUOUS_ANCHOR       = "cannot assign the same alias to multiple nodes";
		const char * const UNKNOWN_ANCHOR         = "the referenced anchor is not defined";

        const char * const INVALID_NODE           = "invalid node; this may result from using a map iterator as a sequence iterator, or vice-versa";
		const char * const INVALID_SCALAR         = "invalid scalar";
		const char * const KEY_NOT_FOUND          = "key not found";
        const char * const BAD_CONVERSION         = "bad conversion";
		const char * const BAD_DEREFERENCE        = "bad dereference";
        const char * const BAD_SUBSCRIPT          = "operator[] call on a scalar";
        const char * const BAD_PUSHBACK           = "appending to a non-sequence";
        const char * const BAD_INSERT             = "inserting in a non-convertible-to-map";
		
		const char * const UNMATCHED_GROUP_TAG    = "unmatched group tag";
		const char * const UNEXPECTED_END_SEQ     = "unexpected end sequence token";
		const char * const UNEXPECTED_END_MAP     = "unexpected end map token";
		const char * const SINGLE_QUOTED_CHAR     = "invalid character in single-quoted string";
		const char * const INVALID_ANCHOR         = "invalid anchor";
		const char * const INVALID_ALIAS          = "invalid alias";
		const char * const INVALID_TAG            = "invalid tag";
        const char * const BAD_FILE               = "bad file";

		template <typename T>
		inline const std::string KEY_NOT_FOUND_WITH_KEY(const T&, typename disable_if<is_numeric<T> >::type * = 0) {
			return KEY_NOT_FOUND;
		}

		inline const std::string KEY_NOT_FOUND_WITH_KEY(const std::string& key) {
			std::stringstream stream;
			stream << KEY_NOT_FOUND << ": " << key;
			return stream.str();
		}
		
		template <typename T>
		inline const std::string KEY_NOT_FOUND_WITH_KEY(const T& key, typename enable_if<is_numeric<T> >::type * = 0) {
			std::stringstream stream;
			stream << KEY_NOT_FOUND << ": " << key;
			return stream.str();
		}
	}

	class Exception: public std::runtime_error {
	public:
		Exception(const Mark& mark_, const std::string& msg_)
		: std::runtime_error(build_what(mark_, msg_)), mark(mark_), msg(msg_) {}
		virtual ~Exception() throw() {}

		Mark mark;
		std::string msg;

	private:
		static const std::string build_what(const Mark& mark, const std::string& msg) {
			std::stringstream output;
			output << "yaml-cpp: error at line " << mark.line+1 << ", column " << mark.column+1 << ": " << msg;
			return output.str();
		}
	};

	class ParserException: public Exception {
	public:
		ParserException(const Mark& mark_, const std::string& msg_)
			: Exception(mark_, msg_) {}
	};

	class RepresentationException: public Exception {
	public:
		RepresentationException(const Mark& mark_, const std::string& msg_)
			: Exception(mark_, msg_) {}
	};

	// representation exceptions
	class InvalidScalar: public RepresentationException {
	public:
		InvalidScalar(const Mark& mark_)
			: RepresentationException(mark_, ErrorMsg::INVALID_SCALAR) {}
	};

	class KeyNotFound: public RepresentationException {
	public:
		template <typename T>
		KeyNotFound(const Mark& mark_, const T& key_)
			: RepresentationException(mark_, ErrorMsg::KEY_NOT_FOUND_WITH_KEY(key_)) {}
	};
	
	template <typename T>
	class TypedKeyNotFound: public KeyNotFound {
	public:
		TypedKeyNotFound(const Mark& mark_, const T& key_)
			: KeyNotFound(mark_, key_), key(key_) {}
		virtual ~TypedKeyNotFound() throw() {}

		T key;
	};

	template <typename T>
	inline TypedKeyNotFound <T> MakeTypedKeyNotFound(const Mark& mark, const T& key) {
		return TypedKeyNotFound <T> (mark, key);
	}

	class InvalidNode: public RepresentationException {
	public:
		InvalidNode()
        : RepresentationException(Mark::null_mark(), ErrorMsg::INVALID_NODE) {}
	};

    class BadConversion: public RepresentationException {
	public:
		BadConversion()
        : RepresentationException(Mark::null_mark(), ErrorMsg::BAD_CONVERSION) {}
	};

    template<typename T>
	class TypedBadConversion: public BadConversion {
	public:
		TypedBadConversion()
        : BadConversion() {}
	};

	class BadDereference: public RepresentationException {
	public:
		BadDereference()
		: RepresentationException(Mark::null_mark(), ErrorMsg::BAD_DEREFERENCE) {}
	};

	class BadSubscript: public RepresentationException {
	public:
		BadSubscript()
		: RepresentationException(Mark::null_mark(), ErrorMsg::BAD_SUBSCRIPT) {}
	};

	class BadPushback: public RepresentationException {
	public:
		BadPushback()
		: RepresentationException(Mark::null_mark(), ErrorMsg::BAD_PUSHBACK) {}
	};
	
	class BadInsert: public RepresentationException {
	public:
		BadInsert()
		: RepresentationException(Mark::null_mark(), ErrorMsg::BAD_INSERT) {}
	};

	class EmitterException: public Exception {
	public:
		EmitterException(const std::string& msg_)
		: Exception(Mark::null_mark(), msg_) {}
	};
    
    class BadFile: public Exception {
    public:
        BadFile(): Exception(Mark::null_mark(), ErrorMsg::BAD_FILE) {}
    };
}

#endif // EXCEPTIONS_H_62B23520_7C8E_11DE_8A39_0800200C9A66
