#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "Fact.h"
#include <memory>
#include <vector>
#include <string>

class KnowledgeBase;

class Expression {
    public:
        virtual FactState evaluate(KnowledgeBase& kb) = 0;
        virtual std::vector<char> getFacts() const = 0; // 式に含まれる事実を収集
        virtual bool isOrXor() const { return false; } // 結論部のOR/XOR判定用
        virtual std::string to_string() const = 0; // デバッグ/可視化用
        virtual ~Expression() = default;
};

// 葉ノード：事実（A, B, Cなど）
class FactExpression : public Expression {
    public:
        char factSymbol;
        bool isNegated; // 否定(!)されているか

        FactExpression(char symbol, bool negated) : factSymbol(symbol), isNegated(negated) {}

        FactState evaluate(KnowledgeBase& kb) override;
        
        std::vector<char> getFacts() const override {
            return {factSymbol};
        }
        
        std::string to_string() const override {
            return (isNegated ? "!" : "") + std::string(1, factSymbol);
        }
};

// 中間ノード：二項演算子 (+, |, ^)
class BinaryOperation : public Expression {
    public:
        enum class Operator { AND, OR, XOR };
        Operator op;
        std::unique_ptr<Expression> left;
        std::unique_ptr<Expression> right;

        BinaryOperation(Operator op, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
            : op(op), left(std::move(left)), right(std::move(right)) {}

        FactState evaluate(KnowledgeBase& kb) override;

        std::vector<char> getFacts() const override {
            std::vector<char> facts = left->getFacts();
            std::vector<char> right_facts = right->getFacts();
            facts.insert(facts.end(), right_facts.begin(), right_facts.end());
            return facts;
        }

        bool isOrXor() const override { 
            return op == Operator::OR || op == Operator::XOR; 
        }

        std::string to_string() const override {
            std::string op_str;
            if (op == Operator::AND) op_str = "+";
            else if (op == Operator::OR) op_str = "|";
            else if (op == Operator::XOR) op_str = "^";
            return "(" + left->to_string() + op_str + right->to_string() + ")";
        }
};

#endif
