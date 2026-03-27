#include "../src/executor/ExpressionEvaluator.h"
#include "../src/parser/AST.h"
#include "../src/common/Value.h"
#include <cassert>
#include <iostream>

using namespace minisql;

void testLiteralExpression() {
    std::cout << "Running testLiteralExpression..." << std::endl;

    // 创建整数字面量
    auto intLiteral = std::make_shared<parser::LiteralExpr>(Value(42));
    executor::ExpressionEvaluator evaluator;
    auto result = evaluator.evaluate(intLiteral);

    assert(result.isSuccess());
    assert(result.getValue()->getInt() == 42);
    assert(result.getValue()->getType() == DataType::INT);

    std::cout << "testLiteralExpression: PASSED" << std::endl;
}

void testDoubleLiteral() {
    std::cout << "Running testDoubleLiteral..." << std::endl;

    auto doubleLiteral = std::make_shared<parser::LiteralExpr>(Value(3.14));
    executor::ExpressionEvaluator evaluator;
    auto result = evaluator.evaluate(doubleLiteral);

    assert(result.isSuccess());
    assert(result.getValue()->getDouble() > 3.13 && result.getValue()->getDouble() < 3.15);
    assert(result.getValue()->getType() == DataType::DOUBLE);

    std::cout << "testDoubleLiteral: PASSED" << std::endl;
}

void testStringLiteral() {
    std::cout << "Running testStringLiteral..." << std::endl;

    auto stringLiteral = std::make_shared<parser::LiteralExpr>(Value("hello"));
    executor::ExpressionEvaluator evaluator;
    auto result = evaluator.evaluate(stringLiteral);

    assert(result.isSuccess());
    assert(result.getValue()->getString() == "hello");

    std::cout << "testStringLiteral: PASSED" << std::endl;
}

void testColumnRef() {
    std::cout << "Running testColumnRef..." << std::endl;

    auto columnRef = std::make_shared<parser::ColumnRef>("", "age");
    executor::ExpressionEvaluator evaluator;

    // 设置行上下文
    executor::RowContext context;
    context["age"] = Value(25);
    evaluator.setRowContext(context);

    auto result = evaluator.evaluate(columnRef);
    assert(result.isSuccess());
    assert(result.getValue()->getInt() == 25);

    std::cout << "testColumnRef: PASSED" << std::endl;
}

void testBinaryExpr() {
    std::cout << "Running testBinaryExpr..." << std::endl;

    auto left = std::make_shared<parser::LiteralExpr>(Value(10));
    auto right = std::make_shared<parser::LiteralExpr>(Value(5));
    auto addExpr = std::make_shared<parser::BinaryExpr>(left, "+", right);

    executor::ExpressionEvaluator evaluator;

    // 测试加法
    {
        auto result = evaluator.evaluate(addExpr);
        assert(result.isSuccess());
        assert(result.getValue()->getInt() == 15);
    }

    // 测试减法
    {
        auto subExpr = std::make_shared<parser::BinaryExpr>(left, "-", right);
        auto result = evaluator.evaluate(subExpr);
        assert(result.isSuccess());
        assert(result.getValue()->getInt() == 5);
    }

    // 测试乘法
    {
        auto mulExpr = std::make_shared<parser::BinaryExpr>(left, "*", right);
        auto result = evaluator.evaluate(mulExpr);
        assert(result.isSuccess());
        assert(result.getValue()->getInt() == 50);
    }

    // 测试除法
    {
        auto divExpr = std::make_shared<parser::BinaryExpr>(left, "/", right);
        auto result = evaluator.evaluate(divExpr);
        assert(result.isSuccess());
        assert(result.getValue()->getInt() == 2);
    }

    std::cout << "testBinaryExpr: PASSED" << std::endl;
}

void testComparisonExpr() {
    std::cout << "Running testComparisonExpr..." << std::endl;

    auto left = std::make_shared<parser::LiteralExpr>(Value(10));
    auto right = std::make_shared<parser::LiteralExpr>(Value(5));

    executor::ExpressionEvaluator evaluator;

    // 测试等于
    {
        auto eqExpr = std::make_shared<parser::BinaryExpr>(left, "=", right);
        auto result = evaluator.evaluate(eqExpr);
        assert(result.isSuccess());
        assert(result.getValue()->getBool() == false);
    }

    // 测试大于
    {
        auto gtExpr = std::make_shared<parser::BinaryExpr>(left, ">", right);
        auto result = evaluator.evaluate(gtExpr);
        assert(result.isSuccess());
        assert(result.getValue()->getBool() == true);
    }

    // 测试不等于
    {
        auto neqExpr = std::make_shared<parser::BinaryExpr>(left, "!=", right);
        auto result = evaluator.evaluate(neqExpr);
        assert(result.isSuccess());
        assert(result.getValue()->getBool() == true);
    }

    // 测试小于等于
    {
        auto leExpr = std::make_shared<parser::BinaryExpr>(left, "<=", right);
        auto result = evaluator.evaluate(leExpr);
        assert(result.isSuccess());
        assert(result.getValue()->getBool() == false);
    }

    std::cout << "testComparisonExpr: PASSED" << std::endl;
}

void testLogicalExpr() {
    std::cout << "Running testLogicalExpr..." << std::endl;

    auto trueVal = std::make_shared<parser::LiteralExpr>(Value(true));
    auto falseVal = std::make_shared<parser::LiteralExpr>(Value(false));

    executor::ExpressionEvaluator evaluator;

    // 测试 AND
    {
        auto andExpr = std::make_shared<parser::BinaryExpr>(trueVal, "AND", trueVal);
        auto result = evaluator.evaluate(andExpr);
        assert(result.isSuccess());
        assert(result.getValue()->getBool() == true);
    }

    {
        auto andExpr2 = std::make_shared<parser::BinaryExpr>(trueVal, "AND", falseVal);
        auto result = evaluator.evaluate(andExpr2);
        assert(result.isSuccess());
        assert(result.getValue()->getBool() == false);
    }

    // 测试 OR
    {
        auto orExpr = std::make_shared<parser::BinaryExpr>(falseVal, "OR", falseVal);
        auto result = evaluator.evaluate(orExpr);
        assert(result.isSuccess());
        assert(result.getValue()->getBool() == false);
    }

    {
        auto orExpr2 = std::make_shared<parser::BinaryExpr>(trueVal, "OR", falseVal);
        auto result = evaluator.evaluate(orExpr2);
        assert(result.isSuccess());
        assert(result.getValue()->getBool() == true);
    }

    std::cout << "testLogicalExpr: PASSED" << std::endl;
}

void testUnaryExpr() {
    std::cout << "Running testUnaryExpr..." << std::endl;

    executor::ExpressionEvaluator evaluator;

    // 测试 NOT
    {
        auto trueVal = std::make_shared<parser::LiteralExpr>(Value(true));
        auto notExpr = std::make_shared<parser::UnaryExpr>("NOT", trueVal);
        auto result = evaluator.evaluate(notExpr);
        assert(result.isSuccess());
        assert(result.getValue()->getBool() == false);
    }

    // 测试负号
    {
        auto intVal = std::make_shared<parser::LiteralExpr>(Value(10));
        auto negExpr = std::make_shared<parser::UnaryExpr>("-", intVal);
        auto result = evaluator.evaluate(negExpr);
        assert(result.isSuccess());
        assert(result.getValue()->getInt() == -10);
    }

    std::cout << "testUnaryExpr: PASSED" << std::endl;
}

void testScalarFunction() {
    std::cout << "Running testScalarFunction..." << std::endl;

    executor::ExpressionEvaluator evaluator;

    // 测试 UPPER
    {
        auto strVal = std::make_shared<parser::LiteralExpr>(Value("hello"));
        std::vector<parser::ExprPtr> args1 = {strVal};
        auto upperExpr = std::make_shared<parser::FunctionCallExpr>("UPPER", args1);
        auto result = evaluator.evaluate(upperExpr);
        assert(result.isSuccess());
        assert(result.getValue()->getString() == "HELLO");
    }

    // 测试 LOWER
    {
        auto strVal2 = std::make_shared<parser::LiteralExpr>(Value("WORLD"));
        std::vector<parser::ExprPtr> args2 = {strVal2};
        auto lowerExpr = std::make_shared<parser::FunctionCallExpr>("LOWER", args2);
        auto result = evaluator.evaluate(lowerExpr);
        assert(result.isSuccess());
        assert(result.getValue()->getString() == "world");
    }

    // 测试 CONCAT
    {
        auto strVal3 = std::make_shared<parser::LiteralExpr>(Value("Hello"));
        auto strVal4 = std::make_shared<parser::LiteralExpr>(Value(" World"));
        std::vector<parser::ExprPtr> args3 = {strVal3, strVal4};
        auto concatExpr = std::make_shared<parser::FunctionCallExpr>("CONCAT", args3);
        auto result = evaluator.evaluate(concatExpr);
        assert(result.isSuccess());
        assert(result.getValue()->getString() == "Hello World");
    }

    // 测试 LENGTH
    {
        auto strVal = std::make_shared<parser::LiteralExpr>(Value("hello"));
        std::vector<parser::ExprPtr> args = {strVal};
        auto lenExpr = std::make_shared<parser::FunctionCallExpr>("LENGTH", args);
        auto result = evaluator.evaluate(lenExpr);
        assert(result.isSuccess());
        assert(result.getValue()->getInt() == 5);
    }

    std::cout << "testScalarFunction: PASSED" << std::endl;
}

int main() {
    std::cout << "=== ExpressionEvaluator Tests ===" << std::endl;

    testLiteralExpression();
    testDoubleLiteral();
    testStringLiteral();
    testColumnRef();
    testBinaryExpr();
    testComparisonExpr();
    testLogicalExpr();
    testUnaryExpr();
    testScalarFunction();

    std::cout << "\n=== All tests PASSED ===" << std::endl;
    return 0;
}
