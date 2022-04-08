#ifndef PCF_PARSER_HPP
#define PCF_PARSER_HPP

/** @file */

#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <sstream>

#include "types/general_exception.hpp"
#include "relay.hpp"
#include "json.hpp"


typedef std::function<double(const Relay&, double)> pcfEffect;
typedef std::function<bool(const Relay&)> pcfPredicate;

/** This namespace provides functions to lex and parse
 * strings according to the PCF language specification.
 * If parsing is successful, the parser will also
 * generate the predicate and effect functions corresponding
 * to the parsed PCF expression, as well as a JSON representation
 * of the PCF.*/
namespace pcfparser
{
	class pcf_parse_exception : public general_exception
	{
	public:
		enum reason
		{
			LEXER_INVALID_TOKEN,
			LEXER_STRING_UNCLOSED,
			LEXER_STRING_EMPTY,

			PARSER_GENERAL,
			PARSER_EXPECTED_EFFECT,
			PARSER_EXPECTED_FLOAT,
			PARSER_EXPECTED_INT,
			PARSER_EXPECTED_STRING,
			PARSER_EXPECTED_QMARK,
			PARSER_EXPECTED_SEMICOLON,
			PARSER_EXPECTED_EQUAL,
			PARSER_INVALID_EFFECT,
			PARSER_INVALID_CONDITION,
			PARSER_INVALID_FLAG,
			PARSER_INVALID_OPERATOR
		};

		pcf_parse_exception(reason why, int position, const char* file, int line) : general_exception("", file, line) {
			this->reasonWhy = why;
			this->position = position;
			std::ostringstream positionStream;
			positionStream << position;
			switch (why) {
				//FIXME nicer messages
			case LEXER_INVALID_TOKEN: this->message = "Invalid token at position " + positionStream.str() + "."; break;
			case LEXER_STRING_EMPTY: this->message = "Empty string literal at position " + positionStream.str() + "."; break;
			case LEXER_STRING_UNCLOSED: this->message = "String literal starting at position " + positionStream.str() + " is not closed."; break;
			case PARSER_GENERAL: this->message = "Should-not-happen behavior happened at position " + positionStream.str() + "."; break;
			case PARSER_EXPECTED_EFFECT: this->message = "Expected effect type at position " + positionStream.str() + "."; break;
			case PARSER_EXPECTED_FLOAT: this->message = "Expected numerical value(float) at position " + positionStream.str() + "."; break;
			case PARSER_EXPECTED_INT: this->message = "Expected numerical value(int) at position " + positionStream.str() + "."; break;
			case PARSER_EXPECTED_STRING: this->message = "Expected string at position " + positionStream.str() + "."; break;
			case PARSER_EXPECTED_SEMICOLON: this->message = "Expected PCF-List separator \';\' at position " + positionStream.str() + "."; break;
			case PARSER_EXPECTED_QMARK: this->message = "Expected PCF separator \'?\' at position " + positionStream.str() + "."; break;
			case PARSER_EXPECTED_EQUAL: this->message = "String properites can only be tested on equality (position: " + positionStream.str() + "."; break;
			case PARSER_INVALID_EFFECT: this->message = "Invalid effect type parsed at " + positionStream.str() + "."; break;
			case PARSER_INVALID_CONDITION: this->message = "Invalid condition parsed at " + positionStream.str() + "."; break;
			case PARSER_INVALID_OPERATOR: this->message = "Invalid logic operator parsed at " + positionStream.str() + "."; break;
			case PARSER_INVALID_FLAG: this->message = "Invalid relay flag parsed at " + positionStream.str() + "."; break;
			default: this->message = "Unknown reason";
			}

			commit_message();
		}

		virtual ~pcf_parse_exception() throw () {}

		virtual int why() const { return reasonWhy; }

	protected:
		reason reasonWhy;
		int position;

		virtual const std::string& getTag() const {
			const static std::string tag = "pcf_parsing_exception";
			return tag;
		}
	};

	/** @enum Possible tokens in PCF strings. */
	enum class PCFTokenType
	{
		SEMICOLON, QMARK, PAREN_L, PAREN_R, 
		OR = 6,
		XOR = 7,
		AND = 8,
		NOT, ANY,
		EQUAL, LT, GT, BANDWIDTH, AVGBANDWIDTH, FLAGGED,
		NAME, PUBLISHED, FINGERPRINT, VERSION, COUNTRY, ASNUMBER, ASNAME, OS, LONGTITUDE,
		LATITUDE, AUTHORITY, BADEXIT, EXIT, FAST, GUARD, HSDIR, NAMED, STABLE, RUNNING,
		UNNAMED, VALID, VTWODIR, SET, MUL, ADD, BW, LITERAL_FLOAT, LITERAL_INT, LITERAL_STRING
	};

	/** Structure which collects information about tokens. */
	struct PCFToken
	{
		PCFTokenType type; /**< The token type. @see PCFTokenType */
		int position; /**< Position of the token in the lexed text. */
		std::string stringLiteralValue; /**< In case the token is a string literal, this holds the string value of the token. */
		long intLiteralValue; /**< In case the token is an integer literal, this holds the numerical value (long) of the token. */
		double floatLiteralValue; /**< In case the token is a floating point literal, this holds the numerical value (double) of the token. */
	};


	extern std::map<std::string, PCFTokenType> keywordsMap; /**< This map stores the relationship between keyword strings and their token type. Used for lexing.*/
	extern std::vector<std::string> keywords; /**< Vector of all keywords in PCF language. */

	/** Checks whether a string has a keyword as prefix.
	 * @param expression This string is checked for a keyword prefix.
	 * @return If the string has a keyword prefix, return the position
	 *          of the keyword in the kewords vector, else return -1.*/
	extern int keywordPrefix(const std::string& expression);



	class PCFParser {
	public:
		/** Lexes a PCF string into a vector of tokens.
		* @param expression PCF string to lex.*/
		static std::shared_ptr<std::vector<PCFToken>> tokenize(std::string expression);

		/** Print the given token stream.
		* @param tokens token stream to be printed.*/
		static void printTokens(std::shared_ptr<std::vector<PCFToken>> tokens);

		PCFParser(std::string& input);
		PCFParser(std::shared_ptr<std::vector<PCFToken>> tokens);
		PCFParser(nlohmann::json obj);

		std::vector<std::pair<pcfPredicate, pcfEffect>> getPCFs() {
			return parsedPCFs;
		}

		nlohmann::json getJSON() {
			return parsedJSON;
		}


	private:
		/** @return the next token in the tokenstream indexed by currentToken. */
		void nextToken();

		/** Recursive descent: parse Condition rules
		 * @return JSON object describing the condition. */
		nlohmann::json parseCondition();

		nlohmann::json parseUnaryConditionalExpression();

		/** Recursive descent: parse Conditional expression rules
		 * @return JSON object describing the conditional expression. */
		nlohmann::json parseConditionalExpression();

		/** Recursive descent: parse Effect rules
		 * @return JSON object describing the effect. */
		nlohmann::json parseEffect();

		/** Recursive descent: parse PCF rules
		 * @return JSON object describing the PCf. */
		nlohmann::json parsePCF();

		/** Recursive descent: parse PCF List rules
		 * @return JSON object describing the PCF List. */
		nlohmann::json parsePCFList();

		/** Parses a token vector according to the PCF language specification.
		 * This also constructs and sets the parsedEffect and parsedPredicate functions
		 * and builds a JSON representation of the PCF.
		 * @param tokens token vector to parse. */
		void parse(const std::shared_ptr<std::vector<PCFToken>> tokens);

		void parseJSON(nlohmann::json obj);
		pcfEffect effectFromJSON(nlohmann::json obj);
		pcfPredicate predicateFromJSON(nlohmann::json obj);

		std::vector<PCFToken>::const_iterator currentToken; /**< Token stream index used for parsing. */
		std::vector<PCFToken>::const_iterator lastToken; /**< Token stream end pointer used for parsing. */

		std::vector<std::pair<pcfPredicate, pcfEffect>> parsedPCFs; /**< List of PCFs parsed from the expression. */
		pcfEffect parsedEffect; /**< The effect function which is constructed while parsing a PCF expression. */
		pcfPredicate parsedPredicate; /**< The predicate function which is constructed while parsing a PCF expression. */
		nlohmann::json parsedJSON; /**< The JSON representation of the parsed PCF expression. */
	};
};

#endif
