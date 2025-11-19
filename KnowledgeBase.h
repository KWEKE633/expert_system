#ifndef KNOWLEDGEBASE_H
#define KNOWLEDGEBASE_H

#include "Fact.h"
#include "Expression.h"
#include <map>
#include <vector>
#include <string>
#include <memory>

class Rule {
    public:
        std::unique_ptr<Expression> antecedent; // 前提部 (AST)
        std::unique_ptr<Expression> consequent; // 結論部 (AST)

        std::string to_string() const;
};

class KnowledgeBase {
    public:
        std::map<char, Fact> facts;
        std::vector<Rule> rules;
        std::vector<char> queries;

        // ボーナス: インタラクティブモード用の初期状態
        std::map<char, FactState> initial_fact_states; 

        // I/O & 初期化
        void loadFromFile(const std::string& filename);
        void runQueries(bool verbose = false);
        void runInteractiveMode();

        // 推論エンジン
        FactState isFactTrue(char symbol); 

    private:
        // 推論ヘルパー
        void resetFacts();
        void saveInitialState();
        void propagate_Undetermined(); // ボーナス: OR/XOR結論からの伝播

        // パーサーの状態とメソッド
        std::string input_str;
        size_t current_pos = 0;
        
        void skipWhitespace();
        char peek() const;
        void consume();
        void syntaxError(const std::string& message);

        // I/O パーサー
        void parseRule(const std::string& rule_str);
        void parseInitialFacts(const std::string& fact_str, bool interactive = false);
        void parseQueries(const std::string& query_str);
        void addImpliesRule(const std::string& antecedent_str, const std::string& consequent_str);
        
        // 論理式パーサー (AST構築)
        std::unique_ptr<Expression> parseExpression(const std::string& str);
        std::unique_ptr<Expression> parse_XOR(); 
        std::unique_ptr<Expression> parse_OR(); 
        std::unique_ptr<Expression> parse_AND(); 
        std::unique_ptr<Expression> parse_NOT(); 
        std::unique_ptr<Expression> parse_Factor();
};

#endif
