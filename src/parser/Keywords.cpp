#include "Token.h"
#include <algorithm>
#include <cctype>

namespace minisql {
namespace parser {

std::unordered_map<std::string, TokenType> Keywords::keywords_;
bool Keywords::initialized_ = false;

void Keywords::initKeywords() {
    if (initialized_) return;

    // DDL 关键字
    keywords_["CREATE"] = TokenType::KEYWORD;
    keywords_["ALTER"] = TokenType::KEYWORD;
    keywords_["DROP"] = TokenType::KEYWORD;
    keywords_["TRUNCATE"] = TokenType::KEYWORD;

    // DML 关键字
    keywords_["INSERT"] = TokenType::KEYWORD;
    keywords_["UPDATE"] = TokenType::KEYWORD;
    keywords_["DELETE"] = TokenType::KEYWORD;
    keywords_["SELECT"] = TokenType::KEYWORD;

    // DCL 关键字
    keywords_["GRANT"] = TokenType::KEYWORD;
    keywords_["REVOKE"] = TokenType::KEYWORD;

    // 查询相关
    keywords_["FROM"] = TokenType::KEYWORD;
    keywords_["WHERE"] = TokenType::KEYWORD;
    keywords_["AND"] = TokenType::KEYWORD;
    keywords_["OR"] = TokenType::KEYWORD;
    keywords_["NOT"] = TokenType::KEYWORD;
    keywords_["IN"] = TokenType::KEYWORD;
    keywords_["LIKE"] = TokenType::KEYWORD;
    keywords_["BETWEEN"] = TokenType::KEYWORD;
    keywords_["IS"] = TokenType::KEYWORD;
    keywords_["NULL"] = TokenType::KEYWORD;
    keywords_["AS"] = TokenType::KEYWORD;
    keywords_["DISTINCT"] = TokenType::KEYWORD;

    // 排序和分组
    keywords_["ORDER"] = TokenType::KEYWORD;
    keywords_["BY"] = TokenType::KEYWORD;
    keywords_["ASC"] = TokenType::KEYWORD;
    keywords_["DESC"] = TokenType::KEYWORD;
    keywords_["GROUP"] = TokenType::KEYWORD;
    keywords_["HAVING"] = TokenType::KEYWORD;
    keywords_["LIMIT"] = TokenType::KEYWORD;
    keywords_["OFFSET"] = TokenType::KEYWORD;

    // 连接
    keywords_["JOIN"] = TokenType::KEYWORD;
    keywords_["INNER"] = TokenType::KEYWORD;
    keywords_["LEFT"] = TokenType::KEYWORD;
    keywords_["RIGHT"] = TokenType::KEYWORD;
    keywords_["FULL"] = TokenType::KEYWORD;
    keywords_["OUTER"] = TokenType::KEYWORD;
    keywords_["CROSS"] = TokenType::KEYWORD;
    keywords_["ON"] = TokenType::KEYWORD;

    // 表和数据库
    keywords_["TABLE"] = TokenType::KEYWORD;
    keywords_["DATABASE"] = TokenType::KEYWORD;
    keywords_["INDEX"] = TokenType::KEYWORD;
    keywords_["VIEW"] = TokenType::KEYWORD;
    keywords_["PROCEDURE"] = TokenType::KEYWORD;
    keywords_["TRIGGER"] = TokenType::KEYWORD;

    // 数据类型
    keywords_["INT"] = TokenType::KEYWORD;
    keywords_["INTEGER"] = TokenType::KEYWORD;
    keywords_["BIGINT"] = TokenType::KEYWORD;
    keywords_["SMALLINT"] = TokenType::KEYWORD;
    keywords_["TINYINT"] = TokenType::KEYWORD;
    keywords_["FLOAT"] = TokenType::KEYWORD;
    keywords_["DOUBLE"] = TokenType::KEYWORD;
    keywords_["DECIMAL"] = TokenType::KEYWORD;
    keywords_["CHAR"] = TokenType::KEYWORD;
    keywords_["VARCHAR"] = TokenType::KEYWORD;
    keywords_["TEXT"] = TokenType::KEYWORD;
    keywords_["DATE"] = TokenType::KEYWORD;
    keywords_["TIME"] = TokenType::KEYWORD;
    keywords_["DATETIME"] = TokenType::KEYWORD;
    keywords_["TIMESTAMP"] = TokenType::KEYWORD;
    keywords_["BOOLEAN"] = TokenType::KEYWORD;
    keywords_["BOOL"] = TokenType::KEYWORD;

    // 约束
    keywords_["PRIMARY"] = TokenType::KEYWORD;
    keywords_["KEY"] = TokenType::KEYWORD;
    keywords_["UNIQUE"] = TokenType::KEYWORD;
    keywords_["FOREIGN"] = TokenType::KEYWORD;
    keywords_["REFERENCES"] = TokenType::KEYWORD;
    keywords_["CONSTRAINT"] = TokenType::KEYWORD;
    keywords_["DEFAULT"] = TokenType::KEYWORD;
    keywords_["AUTO_INCREMENT"] = TokenType::KEYWORD;
    keywords_["NOT"] = TokenType::KEYWORD;

    // 其他
    keywords_["SET"] = TokenType::KEYWORD;
    keywords_["VALUES"] = TokenType::KEYWORD;
    keywords_["INTO"] = TokenType::KEYWORD;
    keywords_["USE"] = TokenType::KEYWORD;
    keywords_["SHOW"] = TokenType::KEYWORD;
    keywords_["DATABASES"] = TokenType::KEYWORD;
    keywords_["TABLES"] = TokenType::KEYWORD;
    keywords_["COLUMNS"] = TokenType::KEYWORD;
    keywords_["IF"] = TokenType::KEYWORD;
    keywords_["EXISTS"] = TokenType::KEYWORD;
    keywords_["ENGINE"] = TokenType::KEYWORD;
    keywords_["COMMENT"] = TokenType::KEYWORD;
    keywords_["TRUE"] = TokenType::KEYWORD;
    keywords_["FALSE"] = TokenType::KEYWORD;

    initialized_ = true;
}

bool Keywords::isKeyword(const std::string& str) {
    initKeywords();
    std::string upper = str;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    return keywords_.find(upper) != keywords_.end();
}

TokenType Keywords::getKeywordType(const std::string& str) {
    initKeywords();
    std::string upper = str;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    auto it = keywords_.find(upper);
    return it != keywords_.end() ? it->second : TokenType::UNKNOWN;
}

}  // namespace parser
}  // namespace minisql
