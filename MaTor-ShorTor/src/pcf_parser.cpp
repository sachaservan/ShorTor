#include "pcf_parser.hpp"

#include <iostream>
#include <map>
#include <vector>
/* Lexer */

using namespace pcfparser;
using nlohmann::json;

typedef std::shared_ptr<std::vector<PCFToken>> tokensPtr;

std::map<std::string, PCFTokenType> pcfparser::keywordsMap = { {";",PCFTokenType::SEMICOLON}, {"?",PCFTokenType::QMARK},{ "(",PCFTokenType::PAREN_L },{ ")",PCFTokenType::PAREN_R },{"NOT",PCFTokenType::NOT},
									{"ANY",PCFTokenType::ANY}, {"AND",PCFTokenType::AND}, {"OR",PCFTokenType::OR},
									{"XOR",PCFTokenType::XOR}, {"==",PCFTokenType::EQUAL}, {"BANDWIDTH",PCFTokenType::BANDWIDTH},{ "AVGBANDWIDTH",PCFTokenType::AVGBANDWIDTH },
									{"FLAGGED",PCFTokenType::FLAGGED}, {"NAMED",PCFTokenType::NAMED}, {"NAME",PCFTokenType::NAME},
									{"PUBLISHED",PCFTokenType::PUBLISHED}, {"FINGERPRINT",PCFTokenType::FINGERPRINT},
									{"VERSION",PCFTokenType::VERSION}, {"COUNTRY",PCFTokenType::COUNTRY}, {"ASNUMBER",PCFTokenType::ASNUMBER},
									{"ASNAME",PCFTokenType::ASNAME}, {"OS",PCFTokenType::OS}, {"LONGTITUDE",PCFTokenType::LONGTITUDE},
									{"LATITUDE",PCFTokenType::LATITUDE}, {"<",PCFTokenType::LT}, {">",PCFTokenType::GT},
									{"AUTHORITY",PCFTokenType::AUTHORITY}, {"BAD_EXIT",PCFTokenType::BADEXIT}, {"EXIT",PCFTokenType::EXIT},
									{"FAST",PCFTokenType::FAST}, {"GUARD",PCFTokenType::GUARD}, {"HS_DIR",PCFTokenType::HSDIR}, {"STABLE",PCFTokenType::STABLE},
									{"RUNNING",PCFTokenType::RUNNING}, {"UNNAMED",PCFTokenType::UNNAMED}, {"VALID",PCFTokenType::VALID},
									{"V2_DIR",PCFTokenType::VTWODIR}, {"SET",PCFTokenType::SET}, {"MUL",PCFTokenType::MUL}, {"ADD",PCFTokenType::ADD} };

std::vector<std::string> pcfparser::keywords = { ";", "?", "(", ")", "NOT",
					 "ANY", "AND","OR",
					 "XOR", "==", "BANDWIDTH", "AVGBANDWIDTH",
					 "FLAGGED", "NAMED", "NAME",
					 "PUBLISHED", "FINGERPRINT",
					 "VERSION", "COUNTRY", "ASNUMBER",
					 "ASNAME", "OS", "LONGTITUDE",
					 "LATITUDE", "<", ">",
					 "AUTHORITY", "BAD_EXIT", "EXIT",
					 "FAST", "GUARD", "HS_DIR", "STABLE",
					 "RUNNING", "UNNAMED", "VALID",
					 "V2_DIR", "SET", "MUL", "ADD" };


int pcfparser::keywordPrefix(const std::string& expression) {
	//sanity check
	if (expression.empty())
		return -1;

	std::string keyword;
	for (int i = 0; i < keywords.size(); i++) {
		keyword = pcfparser::keywords[i];
		if (expression.size() < keyword.size())
			continue;
		else if (expression.substr(0, keyword.size()) == keyword)
			return i;
	}
	return -1;
}

std::shared_ptr<std::vector<PCFToken>> PCFParser::tokenize(std::string expression) {
	std::shared_ptr<std::vector<PCFToken>> tokens(new std::vector<PCFToken>);
	int position = 0;

	while (!expression.empty()) {
		PCFToken newToken;
		int arraypos = -1;

		//skip whitespace between tokens
		int spaces = 0;
		for (char c : expression) {
			if (!isspace(c))
				break;
			position++;
			spaces++;
		}
		expression = expression.substr(spaces, expression.size() - spaces);

		size_t remainingLength = expression.size();
		if (remainingLength == 0)
			break;

		//try to lex a keyword
		if ((arraypos = keywordPrefix(expression)) != -1) {
			std::string keyword = pcfparser::keywords[arraypos];

			newToken.type = pcfparser::keywordsMap[keyword];
			newToken.position = position;


			tokens->push_back(newToken);
			position += keyword.size();
			expression = expression.substr(keyword.size(), remainingLength - keyword.size());
			continue;
		}

		//lex string literal
		if (expression[0] == '\'') {
			int eos = 0;
			for (char c : expression.substr(1, remainingLength - 1)) {
				if (c == '\'')
					break;
				eos++;
			}

			if (eos == remainingLength - 1) {
				throw_exception(pcf_parse_exception, pcf_parse_exception::LEXER_STRING_UNCLOSED, position);
			}
			if (eos == 0) {
				throw_exception(pcf_parse_exception, pcf_parse_exception::LEXER_STRING_EMPTY, position);
			}

			std::string literalValue = expression.substr(1, eos);

			newToken.type = PCFTokenType::LITERAL_STRING;
			newToken.position = position;
			newToken.stringLiteralValue = literalValue;


			tokens->push_back(newToken);
			position += literalValue.size() + 2;
			expression = expression.substr(literalValue.size() + 2, remainingLength - (literalValue.size() + 2));
			continue;
		}

		// lex float or int literal
		if (isdigit(expression[0])) {
			int length = 0;
			bool decimalPoint = false;
			double floatLiteralValue = 0.0f;
			long intLiteralValue = 0;

			//determine length of literal and look for a decimal point
			for (char c : expression) {
				if (!isdigit(c)) {
					if (c == '.' && !decimalPoint)
						decimalPoint = true;
					else
						break;
				}
				length++;
			}

			//if there was a decimal point, try lexing a floating point literal
			if (decimalPoint) {
				newToken.floatLiteralValue = std::stod(expression.substr(0, length), NULL);
				newToken.type = PCFTokenType::LITERAL_FLOAT;
			} else {
				newToken.intLiteralValue = std::stoi(expression.substr(0, length), NULL);
				newToken.type = PCFTokenType::LITERAL_INT;
			}

			newToken.position = position;
			newToken.stringLiteralValue = expression.substr(0, length);

			tokens->push_back(newToken);
			position += length;
			expression = expression.substr(length, remainingLength - length);

			continue;
		}

		// if we get this far, what we are trying to lex is not a valid token
		throw_exception(pcf_parse_exception, pcf_parse_exception::LEXER_INVALID_TOKEN, position);
		break;
	}

	return tokens;
}


/* Parser */
void PCFParser::nextToken() {
	PCFParser::currentToken++;
}

json PCFParser::parseCondition() {
	json o;

	if (currentToken->type == PCFTokenType::ANY) {
		parsedPredicate = pcfPredicate([](const Relay&) { return true; });
		o["condtype"] = "ANY";
		nextToken();
		return o;
	}

	if (currentToken->type == PCFTokenType::BANDWIDTH || currentToken->type == PCFTokenType::AVGBANDWIDTH) {
		PCFTokenType firstToken = currentToken->type;
		o["condtype"] = firstToken == PCFTokenType::BANDWIDTH ? "BANDWIDTH" : "AVGBANDWIDTH";
		//try parse operator
		nextToken();
		PCFTokenType opType = currentToken->type;
		if (!(opType == PCFTokenType::EQUAL
			|| opType == PCFTokenType::LT
			|| opType == PCFTokenType::GT))
			throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_INVALID_OPERATOR, currentToken->position);

		nextToken();
		if (!(currentToken->type == PCFTokenType::LITERAL_INT))
			throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_EXPECTED_INT, currentToken->position);

		long literalValue = currentToken->intLiteralValue;
		o["value"] = literalValue;
		switch (opType) {
		case PCFTokenType::EQUAL:
			o["op"] = "EQUAL";
			if (firstToken == PCFTokenType::BANDWIDTH)
				parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getBandwidth() == literalValue;});
			else
				parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getAveragedBandwidth() == literalValue;});
			break;

		case PCFTokenType::LT:
			o["op"] = "LT";
			if (firstToken == PCFTokenType::BANDWIDTH)
				parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getBandwidth() < literalValue;});
			else
				parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getAveragedBandwidth() < literalValue;});
			break;

		case PCFTokenType::GT:
			o["op"] = "GT";
			if (firstToken == PCFTokenType::BANDWIDTH)
				parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getBandwidth() > literalValue;});
			else
				parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getAveragedBandwidth() > literalValue;});
			break;

		default: throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_GENERAL, currentToken->position);
		}
		nextToken();
		return o;
	}
	/*
	if (currentToken->type == PCFTokenType::COUNTRY) {
		std::cout << "Parsing country" << std::endl;
		o["condtype"] = "COUNTRY";
		//try parse operator
		nextToken();
		PCFTokenType opType = currentToken->type;
		if (!(opType == PCFTokenType::EQUAL))
			throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_INVALID_OPERATOR, currentToken->position);
		std::cout << "Equality worked out " << std::endl;

		nextToken();
		if (!(currentToken->type == PCFTokenType::LITERAL_STRING))
			throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_EXPECTED_STRING, currentToken->position);
		std::cout << "Next token indeed was a string" << std::endl;

		std::string literalValue = currentToken->stringLiteralValue;
		std::cout << "The value of the string is " << literalValue << std::endl;
		o["value"] = literalValue;
		o["op"] = "EQUAL";
		parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getCountry() == literalValue;});
		std::cout << "I set the predicate " << literalValue << std::endl;
		nextToken();
		std::cout << "I (for some reason) have called nextToken() " << literalValue << std::endl;
		return o;
	}
	*/

	if (currentToken->type == PCFTokenType::FLAGGED) {
		o["condtype"] = "FLAGGED";
		nextToken();
		int testFlag;
		switch (currentToken->type) {
		case PCFTokenType::AUTHORITY:
			o["flag"] = "AUTHORITY";
			testFlag = AUTHORITY;
			break;
		case PCFTokenType::BADEXIT:
			o["flag"] = "BAD_EXIT";
			testFlag = BAD_EXIT;
			break;
		case PCFTokenType::EXIT:
			o["flag"] = "EXIT";
			testFlag = EXIT;
			break;
		case PCFTokenType::FAST:
			o["flag"] = "FAST";
			testFlag = FAST;
			break;
		case PCFTokenType::GUARD:
			o["flag"] = "GUARD";
			testFlag = GUARD;
			break;
		case PCFTokenType::HSDIR:
			o["flag"] = "HS_DIR";
			testFlag = HS_DIR;
			break;
		case PCFTokenType::NAMED:
			o["flag"] = "NAMED";
			testFlag = NAMED;
			break;
		case PCFTokenType::STABLE:
			o["flag"] = "STABLE";
			testFlag = STABLE;
			break;
		case PCFTokenType::RUNNING:
			o["flag"] = "RUNNING";
			testFlag = RUNNING;
			break;
		case PCFTokenType::UNNAMED:
			o["flag"] = "UNNAMED";
			testFlag = UNNAMED;
			break;
		case PCFTokenType::VALID:
			o["flag"] = "VALID";
			testFlag = VALID;
			break;
		case PCFTokenType::VTWODIR:
			o["flag"] = "V2_DIR";
			testFlag = V2_DIR;
			break;
		default: throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_INVALID_FLAG, currentToken->position);
		}
		parsedPredicate = pcfPredicate([testFlag](const Relay& r) {return r.hasFlags(testFlag);});
		nextToken();
		return o;
	}

	if (currentToken->type == PCFTokenType::LATITUDE
		|| currentToken->type == PCFTokenType::LONGTITUDE) {
		bool isLatitude = currentToken->type == PCFTokenType::LATITUDE ? true : false;

		nextToken();
		PCFTokenType opType = currentToken->type;
		if (!(opType == PCFTokenType::EQUAL
			|| opType == PCFTokenType::LT
			|| opType == PCFTokenType::GT))
			throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_INVALID_OPERATOR, currentToken->position);

		nextToken();
		if (!(currentToken->type == PCFTokenType::LITERAL_FLOAT))
			throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_EXPECTED_FLOAT, currentToken->position);

		long literalValue = currentToken->floatLiteralValue;
		o["value"] = literalValue;
		nextToken();

		if (isLatitude) {
			o["condtype"] = "LATITUDE";
			switch (opType) {
			case PCFTokenType::EQUAL:
				o["op"] = "EQUAL";
				parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getLatitude() == literalValue;});
				break;

			case PCFTokenType::LT:
				o["op"] = "LT";
				parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getLatitude() < literalValue;});
				break;

			case PCFTokenType::GT:
				o["op"] = "GT";
				parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getLatitude() > literalValue;});
				break;

			default: throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_GENERAL, currentToken->position);
			}
		} else {
			o["condtype"] = "LONGTITUDE";
			switch (opType) {
			case PCFTokenType::EQUAL:
				o["op"] = "EQUAL";
				parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getLongitude() == literalValue;});
				break;

			case PCFTokenType::LT:
				o["op"] = "LT";
				parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getLongitude() < literalValue;});
				break;

			case PCFTokenType::GT:
				o["op"] = "GT";
				parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getLongitude() > literalValue;});
				break;

			default: throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_GENERAL, currentToken->position);
			}

		}
		return o;
	}

	if (currentToken->type == PCFTokenType::NAME
		|| currentToken->type == PCFTokenType::PUBLISHED
		|| currentToken->type == PCFTokenType::FINGERPRINT
		|| currentToken->type == PCFTokenType::VERSION
		|| currentToken->type == PCFTokenType::COUNTRY
		|| currentToken->type == PCFTokenType::ASNUMBER
		|| currentToken->type == PCFTokenType::ASNAME
		|| currentToken->type == PCFTokenType::OS) {
		//save property
		PCFTokenType propType = currentToken->type;

		nextToken();
		if (!(currentToken->type == PCFTokenType::EQUAL))
			throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_EXPECTED_EQUAL, currentToken->position);

		nextToken();
		if (!(currentToken->type == PCFTokenType::LITERAL_STRING))
			throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_EXPECTED_STRING, currentToken->position);

		std::string literalValue = currentToken->stringLiteralValue;
		o["value"] = literalValue;
		o["op"] = "EQUAL";
		nextToken();

		switch (propType) {
		case PCFTokenType::NAME:
			o["condtype"] = "NAME";
			parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getName() == literalValue;});
			break;

		case PCFTokenType::PUBLISHED:
			o["condtype"] = "PUBLISHED";
			parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getPublishedDate() == literalValue;});
			break;

		case PCFTokenType::FINGERPRINT:
			o["condtype"] = "FINGERPRINT";
			parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getFingerprint() == literalValue;});
			break;

		case PCFTokenType::VERSION:
			o["condtype"] = "VERSION";
			parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getVersion() == literalValue;});
			break;

		case PCFTokenType::COUNTRY:
			//std::cout << "COUNTRY found!" << std::endl;
			o["condtype"] = "COUNTRY";
			parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getCountry() == literalValue;});
			//std::cout << "Literal value=" << literalValue << std::endl;
			return o;
			break;

		case PCFTokenType::ASNUMBER:
			o["condtype"] = "ASNUMBER";
			parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getASNumber() == literalValue;});
			break;

		case PCFTokenType::ASNAME:
			o["condtype"] = "ASNAME";
			parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getASName() == literalValue;});
			break;

		case PCFTokenType::OS:
			o["condtype"] = "OS";
			parsedPredicate = pcfPredicate([literalValue](const Relay& r) {return r.getPlatform() == literalValue;});
			break;

		default: throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_GENERAL, currentToken->position);
		}
		return o;
	}
	throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_INVALID_CONDITION, currentToken->position);
}

json PCFParser::parseUnaryConditionalExpression() {
	json o;
	if (currentToken->type == PCFTokenType::NOT) {
		o["op"] = "NOT";
		nextToken();
		o["child"] = parseUnaryConditionalExpression();
		pcfPredicate accumulatedPredicate = parsedPredicate;
		parsedPredicate = pcfPredicate([accumulatedPredicate](const Relay& r) { return !accumulatedPredicate(r);});
		return o;
	} else if (currentToken->type == PCFTokenType::PAREN_L) {
		nextToken();
		o = parseConditionalExpression();
		if (currentToken->type != PCFTokenType::PAREN_R) {
			throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_GENERAL, currentToken->position);
		}
		nextToken();
		return o;
	} else {
		return parseCondition();
	}
}

json PCFParser::parseConditionalExpression() {
	json o, leftJSON;
	pcfPredicate leftPredicate;

	// parse first
	leftJSON = parseUnaryConditionalExpression();
	leftPredicate = parsedPredicate;

	// parse operators
	auto opAhead = [&]() {
		return currentToken->type == PCFTokenType::AND
			|| currentToken->type == PCFTokenType::XOR
			|| currentToken->type == PCFTokenType::OR;
	};
	std::function<std::pair<pcfPredicate, json>(pcfPredicate, json, PCFTokenType)> parseInfixOperator = [&](pcfPredicate lhsPredicate, json lhsJson, PCFTokenType minPrecedence) {
		json rhsJson;
		pcfPredicate rhsPredicate;
		PCFTokenType leftOp = currentToken->type;
		while (opAhead() && (leftOp = currentToken->type) >= minPrecedence) {
			PCFTokenType opType = currentToken->type;
			nextToken();
			rhsJson = parseUnaryConditionalExpression();
			rhsPredicate = parsedPredicate;
			PCFTokenType rightOp = currentToken->type;
			while (opAhead() && rightOp > leftOp) {
				std::pair<pcfPredicate, json> p = parseInfixOperator(rhsPredicate, rhsJson, rightOp);
				rhsPredicate = p.first;
				rhsJson = p.second;
			}

			// Operator precendence means we should combine lhs and rhs now...
			json o;
			o["left"] = lhsJson;
			o["right"] = rhsJson;
			pcfPredicate left = lhsPredicate;
			pcfPredicate right = rhsPredicate;
			switch (opType) {
			case PCFTokenType::AND:
				o["op"] = "AND";
				lhsPredicate = pcfPredicate([left, right](const Relay& r) { return left(r) && right(r); });
				break;
			case PCFTokenType::OR:
				o["op"] = "OR";
				lhsPredicate = pcfPredicate([left, right](const Relay& r) { return left(r) || right(r); });
				break;
			case PCFTokenType::XOR:
				o["op"] = "XOR";
				lhsPredicate = pcfPredicate([left, right](const Relay& r) { return !left(r) != !right(r); });
				break;
			default: throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_GENERAL, currentToken->position);
			}
			lhsJson = o;
		}
		return std::pair<pcfPredicate, json>(lhsPredicate, lhsJson);
	};
	auto result = parseInfixOperator(leftPredicate, leftJSON, PCFTokenType::SEMICOLON);
	parsedPredicate = result.first;
	return result.second;
}

/*json PCFParser::parseConditionalExpression() {
	pcfPredicate accumulatedPredicate, leftOperand;
	json o, leftJSON, rightJSON;

	// ANY rule
	if (currentToken->type == PCFTokenType::ANY) {
		parsedPredicate = pcfPredicate([](const Relay&) { return true; });
		o["op"] = "ANY";
		nextToken();
		return o;
	}

	// NOT <conditional-expression> 
	if (currentToken->type == PCFTokenType::NOT) {
		json childJSON;
		o["op"] = "NOT";
		nextToken();
		childJSON = parseConditionalExpression();
		o["child"] = childJSON;
		accumulatedPredicate = parsedPredicate; //save old predicate

		//the new predicate is the negation of the old one
		parsedPredicate = pcfPredicate([accumulatedPredicate](const Relay& r) { return !accumulatedPredicate(r);});
		return o;
	}

	//<condition> <binop-cond> <conditional-expression>
	leftJSON = parseCondition();
	//while operator parse next conditional expression
	//nextToken();

	if (!(currentToken->type == PCFTokenType::AND
		|| currentToken->type == PCFTokenType::XOR
		|| currentToken->type == PCFTokenType::OR)) {
		return leftJSON;
	}

	while (currentToken->type == PCFTokenType::AND
		|| currentToken->type == PCFTokenType::XOR
		|| currentToken->type == PCFTokenType::OR) {
		o["left"] = leftJSON;
		leftOperand = parsedPredicate;
		PCFTokenType opType = currentToken->type;

		nextToken();
		rightJSON = parseConditionalExpression();
		o["right"] = rightJSON;

		accumulatedPredicate = parsedPredicate;
		switch (opType) {
		case PCFTokenType::AND:
			o["op"] = "AND";
			parsedPredicate = pcfPredicate([leftOperand, accumulatedPredicate](const Relay& r) { return leftOperand(r) && accumulatedPredicate(r); });
			break;
		case PCFTokenType::OR:
			o["op"] = "OR";
			parsedPredicate = pcfPredicate([leftOperand, accumulatedPredicate](const Relay& r) { return leftOperand(r) || accumulatedPredicate(r); });
			break;
		case PCFTokenType::XOR:
			o["op"] = "XOR";
			parsedPredicate = pcfPredicate([leftOperand, accumulatedPredicate](const Relay& r) { return !leftOperand(r) != !accumulatedPredicate(r); });
			break;
		default: throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_GENERAL, currentToken->position);
		}
	}
	return o;
}*/

json PCFParser::parseEffect() {
	json o;
	//<effect-type> <float-literal>
	PCFTokenType effectType = currentToken->type;
	if (!(effectType == PCFTokenType::SET || effectType == PCFTokenType::MUL || effectType == PCFTokenType::ADD ||effectType == PCFTokenType::BW))
		throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_EXPECTED_EFFECT, currentToken->position);

	nextToken();
	double value = 0.0;
	bool useValue = true;
	if (currentToken->type == PCFTokenType::LITERAL_FLOAT) {
		value = currentToken->floatLiteralValue;
	} else if (currentToken->type == PCFTokenType::LITERAL_INT) {
		value = currentToken->intLiteralValue;
	} else if (currentToken->type == PCFTokenType::BANDWIDTH || currentToken->type == PCFTokenType::AVGBANDWIDTH){
		useValue = false;
	} else {
		throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_EXPECTED_FLOAT, currentToken->position);
	}

	if (useValue)
		o["value"] = value;

	switch (effectType) {
	case PCFTokenType::SET:
		o["effect"] = "SET";
		if (useValue)
			parsedEffect = pcfEffect([value](const Relay& r, double c) {return value;});
		else if (currentToken->type == PCFTokenType::BANDWIDTH)
			parsedEffect = pcfEffect([](const Relay& r, double c) {return r.getBandwidth();});
		else
			parsedEffect = pcfEffect([](const Relay& r, double c) {return r.getAveragedBandwidth();});
		break;

	case PCFTokenType::MUL:
		o["effect"] = "MUL";
		if (useValue)
			parsedEffect = pcfEffect([value](const Relay& r, double c) {return value * c;});
		else if (currentToken->type == PCFTokenType::BANDWIDTH)
			parsedEffect = pcfEffect([](const Relay& r, double c) {return r.getBandwidth() * c;});
		else
			parsedEffect = pcfEffect([](const Relay& r, double c) {return r.getAveragedBandwidth() * c;});
		break;

	case PCFTokenType::ADD:
		o["effect"] = "ADD";
		if (useValue)
			parsedEffect = pcfEffect([value](const Relay& r, double c) {return value + c;});
		else if (currentToken->type == PCFTokenType::BANDWIDTH)
			parsedEffect = pcfEffect([](const Relay& r, double c) {return r.getBandwidth() + c;});
		else
			parsedEffect = pcfEffect([](const Relay& r, double c) {return r.getAveragedBandwidth() + c;});
		break;

	default: throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_INVALID_EFFECT, currentToken->position);
	}
	nextToken();
	return o;
}

json PCFParser::parsePCF() {
	json o;
	//<conditional-expression> ? <effect>
	o["condition"] = parseConditionalExpression();

	if (!(currentToken->type == PCFTokenType::QMARK))
		throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_EXPECTED_QMARK, currentToken->position);
	nextToken();

	o["effect"] = parseEffect();
	return o;
}

json PCFParser::parsePCFList() {
	parsedPCFs.clear();
	json o;

	int pcfCount = 1;
	o["pcf" + std::to_string(pcfCount)] = parsePCF();
	pcfCount++;

	parsedPCFs.push_back(std::make_pair(parsedPredicate, parsedEffect));
	// <pcf> rule
	if (currentToken == lastToken)
		return o;

	// <pcf> ; <pcflist> rule
	while (currentToken != lastToken && currentToken->type == PCFTokenType::SEMICOLON) {
		nextToken();
		o["pcf" + std::to_string(pcfCount)] = parsePCF();
		pcfCount++;
		parsedPCFs.push_back(std::make_pair(parsedPredicate, parsedEffect));
	}
	
	if (currentToken != lastToken)
		throw_exception(pcf_parse_exception, pcf_parse_exception::PARSER_EXPECTED_SEMICOLON, currentToken->position);

	return o;
}

void PCFParser::parse(tokensPtr tokens) {
	currentToken = tokens->cbegin();
	lastToken = tokens->cend();

	parsedJSON = parsePCFList();
}

void PCFParser::parseJSON(json obj) {
	std::cerr << "Not yet implemented: PCFParser::parseJSON()." << std::endl;
}

pcfEffect PCFParser::effectFromJSON(json obj) {
	std::cerr << "Not yet implemented: PCFParser::effectFromJSON()." << std::endl;
	exit(3);
}

pcfPredicate PCFParser::predicateFromJSON(json obj) {
	std::cerr << "Not yet implemented: PCFParser::predicateFromJSON()." << std::endl;
	exit(3);
}

void PCFParser::printTokens(std::shared_ptr<std::vector<PCFToken>> tokens) {
	for (PCFToken t : *(tokens)) {
		switch (t.type) {
		case PCFTokenType::SEMICOLON:     std::cout << ";"; break;
		case PCFTokenType::QMARK:     std::cout << "?"; break;
		case PCFTokenType::NOT:     std::cout << "NOT"; break;
		case PCFTokenType::ANY:     std::cout << "ANY"; break;
		case PCFTokenType::AND:     std::cout << "AND"; break;
		case PCFTokenType::OR:     std::cout << "OR"; break;
		case PCFTokenType::XOR:     std::cout << "XOR"; break;
		case PCFTokenType::EQUAL:     std::cout << "=="; break;
		case PCFTokenType::LT:     std::cout << "<"; break;
		case PCFTokenType::GT:     std::cout << ">"; break;
		case PCFTokenType::BANDWIDTH:     std::cout << "BANDWIDTH"; break;
		case PCFTokenType::FLAGGED:     std::cout << "FLAGGED"; break;
		case PCFTokenType::NAME:     std::cout << "NAME"; break;
		case PCFTokenType::PUBLISHED:     std::cout << "PUBLISHED"; break;
		case PCFTokenType::FINGERPRINT:     std::cout << "FINGERPRINT"; break;
		case PCFTokenType::VERSION:     std::cout << "VERSION"; break;
		case PCFTokenType::COUNTRY:     std::cout << "COUNTRY"; break;
		case PCFTokenType::ASNUMBER:     std::cout << "ASNUMBER"; break;
		case PCFTokenType::ASNAME:     std::cout << "ASNAME"; break;
		case PCFTokenType::OS:     std::cout << "OS"; break;
		case PCFTokenType::LONGTITUDE:     std::cout << "LONGTITUDE"; break;
		case PCFTokenType::LATITUDE:     std::cout << "LATITUDE"; break;
		case PCFTokenType::AUTHORITY:     std::cout << "AUTHORITY"; break;
		case PCFTokenType::BADEXIT:     std::cout << "BADEXIT"; break;
		case PCFTokenType::EXIT:     std::cout << "EXIT"; break;
		case PCFTokenType::FAST:     std::cout << "FAST"; break;
		case PCFTokenType::GUARD:     std::cout << "GUARD"; break;
		case PCFTokenType::HSDIR:     std::cout << "HS_DIR"; break;
		case PCFTokenType::NAMED:     std::cout << "NAMED"; break;
		case PCFTokenType::STABLE:     std::cout << "STABLE"; break;
		case PCFTokenType::RUNNING:     std::cout << "RUNNING"; break;
		case PCFTokenType::UNNAMED:     std::cout << "UNNAMED"; break;
		case PCFTokenType::VALID:     std::cout << "VALID"; break;
		case PCFTokenType::VTWODIR:     std::cout << "V2_DIR"; break;
		case PCFTokenType::SET:     std::cout << "SET"; break;
		case PCFTokenType::MUL:     std::cout << "MUL"; break;
		case PCFTokenType::ADD:     std::cout << "ADD"; break;
		case PCFTokenType::BW:     std::cout << "BW"; break;
		case PCFTokenType::LITERAL_FLOAT: std::cout << t.floatLiteralValue; break;
		case PCFTokenType::LITERAL_INT: std::cout << t.intLiteralValue; break;
		case PCFTokenType::LITERAL_STRING: std::cout << '"' << t.stringLiteralValue << '"'; break;
		default: std::cout << "Not a valid token type.";
		}
		std::cout << " ";
	}
	std::cout << std::endl;
}


PCFParser::PCFParser(std::string& input) {
	parse(tokenize(input));
}

PCFParser::PCFParser(std::shared_ptr<std::vector<PCFToken>> tokens) {
	parse(tokens);
}

PCFParser::PCFParser(nlohmann::json obj) {
	parseJSON(obj);
}
