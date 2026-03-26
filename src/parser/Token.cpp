#include "Token.h"
#include <sstream>

namespace minisql {
namespace parser {

std::string Token::toString() const {
    std::ostringstream oss;
    oss << "Token(";
    switch (type) {
        case TokenType::KEYWORD: oss << "KEYWORD"; break;
        case TokenType::IDENTIFIER: oss << "IDENTIFIER"; break;
        case TokenType::STRING_LITERAL: oss << "STRING_LITERAL"; break;
        case TokenType::INT_LITERAL: oss << "INT_LITERAL"; break;
        case TokenType::FLOAT_LITERAL: oss << "FLOAT_LITERAL"; break;
        case TokenType::BOOL_LITERAL: oss << "BOOL_LITERAL"; break;
        case TokenType::NULL_LITERAL: oss << "NULL_LITERAL"; break;
        case TokenType::OPERATOR: oss << "OPERATOR"; break;
        case TokenType::DELIMITER: oss << "DELIMITER"; break;
        case TokenType::DOT: oss << "DOT"; break;
        case TokenType::COMMA: oss << "COMMA"; break;
        case TokenType::SEMICOLON: oss << "SEMICOLON"; break;
        case TokenType::LEFT_PAREN: oss << "LEFT_PAREN"; break;
        case TokenType::RIGHT_PAREN: oss << "RIGHT_PAREN"; break;
        case TokenType::EOF_TOKEN: oss << "EOF_TOKEN"; break;
        default: oss << "UNKNOWN"; break;
    }
    oss << ", \"" << value << "\", line=" << line << ", col=" << column << ")";
    return oss.str();
}

}  // namespace parser
}  // namespace minisql
