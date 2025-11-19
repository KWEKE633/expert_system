#include "KnowledgeBase.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

std::string Rule::to_string() const {
    return antecedent->to_string() + " => " + consequent->to_string();
}

// FactExpression の評価
FactState FactExpression::evaluate(KnowledgeBase& kb) {
    FactState state = kb.isFactTrue(factSymbol);
    
    if (!isNegated) {
        return state;
    }
    
    // 否定されている場合 (!X)
    if (state == FactState::TRUE) return FactState::FALSE;
    if (state == FactState::FALSE) return FactState::TRUE;
    return FactState::UNDETERMINED; // 未決定の否定は未決定
}

FactState BinaryOperation::evaluate(KnowledgeBase& kb) {
    FactState leftState = left->evaluate(kb);
    FactState rightState = right->evaluate(kb);
    
    if (op == Operator::AND) {
        if (leftState == FactState::FALSE || rightState == FactState::FALSE) return FactState::FALSE;
        if (leftState == FactState::TRUE && rightState == FactState::TRUE) return FactState::TRUE;
        return FactState::UNDETERMINED; // T+U, U+T, U+U
    }
    
    if (op == Operator::OR) {
        if (leftState == FactState::TRUE || rightState == FactState::TRUE) return FactState::TRUE;
        if (leftState == FactState::FALSE && rightState == FactState::FALSE) return FactState::FALSE;
        return FactState::UNDETERMINED; // F|U, U|F, U|U
    }
    
    if (op == Operator::XOR) {
        // 未決定を含む場合は原則 UNDETERMINED
        if (leftState == FactState::UNDETERMINED || rightState == FactState::UNDETERMINED) {
            return FactState::UNDETERMINED;
        }

        // 確定している場合
        bool is_left_true = (leftState == FactState::TRUE);
        bool is_right_true = (rightState == FactState::TRUE);

        if (is_left_true != is_right_true) { // どちらか一方のみ真
            return FactState::TRUE;
        }
        return FactState::FALSE; // 両方真 or 両方偽
    }
    
    return FactState::FALSE; 
}

void KnowledgeBase::skipWhitespace() {
    while (current_pos < input_str.length() && 
           (input_str[current_pos] == ' ' || input_str[current_pos] == '\t')) {
        current_pos++;
    }
}

char KnowledgeBase::peek() const { 
    return (current_pos < input_str.length()) ? input_str[current_pos] : '\0'; 
}

void KnowledgeBase::consume() { 
    if (current_pos < input_str.length()) current_pos++; 
}

void KnowledgeBase::syntaxError(const std::string& message) {
    throw std::runtime_error("Syntax Error at position " + std::to_string(current_pos) + " ('" + input_str + "'): " + message);
}


// --- KnowledgeBase 論理式パーサー (再帰下降) ---

std::unique_ptr<Expression> KnowledgeBase::parse_Factor() {
    skipWhitespace();
    char current_char = peek();

    if (current_char == '(') {
        consume(); // '(' を消費
        std::unique_ptr<Expression> expr = parse_XOR(); 
        skipWhitespace();
        if (peek() != ')') {
            syntaxError("Expected ')'");
        }
        consume(); // ')' を消費
        return expr;
    } 
    else if (current_char >= 'A' && current_char <= 'Z') {
        consume(); // 事実文字を消費
        return std::make_unique<FactExpression>(current_char, false); 
    } 
    else {
        syntaxError("Expected a fact (A-Z) or '('");
        return nullptr;
    }
}

std::unique_ptr<Expression> KnowledgeBase::parse_NOT() {
    skipWhitespace();
    
    if (peek() == '!') {
        consume(); // '!' を消費
        // NOT の右側が FactExpression の場合、その否定フラグを立てる
        std::unique_ptr<Expression> operand = parse_Factor();
        if (FactExpression* fact_expr = dynamic_cast<FactExpression*>(operand.get())) {
            fact_expr->isNegated = !fact_expr->isNegated; // NOTのネストを考慮 (!!A -> A)
            return operand;
        }
        // TODO: !(A+B) のような複雑な否定は、UnaryOperatorノードが必要だが、ここでは簡単化
        // 課題の例 "not B" (!B) のみをサポート
        syntaxError("Complex negation like !(A+B) is not fully supported.");
        return operand;
    }
    
    return parse_Factor();
}

std::unique_ptr<Expression> KnowledgeBase::parse_AND() {
    std::unique_ptr<Expression> left = parse_NOT(); 

    while (true) {
        skipWhitespace();
        if (peek() == '+') {
            consume(); 
            std::unique_ptr<Expression> right = parse_NOT();
            left = std::make_unique<BinaryOperation>(
                BinaryOperation::Operator::AND, std::move(left), std::move(right)
            );
        } else {
            break;
        }
    }
    return left;
}

std::unique_ptr<Expression> KnowledgeBase::parse_OR() {
    std::unique_ptr<Expression> left = parse_AND(); 

    while (true) {
        skipWhitespace();
        if (peek() == '|') {
            consume(); 
            std::unique_ptr<Expression> right = parse_AND();
            left = std::make_unique<BinaryOperation>(
                BinaryOperation::Operator::OR, std::move(left), std::move(right)
            );
        } else {
            break;
        }
    }
    return left;
}

std::unique_ptr<Expression> KnowledgeBase::parse_XOR() {
    std::unique_ptr<Expression> left = parse_OR(); 

    while (true) {
        skipWhitespace();
        if (peek() == '^') {
            consume(); 
            std::unique_ptr<Expression> right = parse_OR();
            left = std::make_unique<BinaryOperation>(
                BinaryOperation::Operator::XOR, std::move(left), std::move(right)
            );
        } else {
            break;
        }
    }
    return left;
}

std::unique_ptr<Expression> KnowledgeBase::parseExpression(const std::string& str) {
    this->input_str = str;
    this->current_pos = 0;
    
    std::unique_ptr<Expression> ast_root = parse_XOR(); 

    skipWhitespace();
    if (peek() != '\0') {
        syntaxError("Unexpected token at end of expression");
    }
    
    return ast_root;
}


// --- KnowledgeBase 推論エンジン ---

FactState KnowledgeBase::isFactTrue(char symbol) {
    // 知識ベースに Fact が存在しない場合、作成し、デフォルトの FALSE で初期化
    if (facts.find(symbol) == facts.end()) {
        facts[symbol].symbol = symbol;
    }
    Fact& fact = facts[symbol]; 

    // 1. 基本ケース (キャッシュと無限ループ検出)
    if (fact.currentState != FactState::FALSE && fact.currentState != FactState::UNDETERMINED) {
        return fact.currentState;
    }
    if (fact.isProcessing) {
        // 循環参照を検出
        return FactState::FALSE; // 証明不可能と見なす
    }

    // 2. 推論の開始と状態フラグの設定
    fact.isProcessing = true;
    
    bool isProvenByAnyRule = false;
    bool isUndeterminedPossible = false;
    
    // 推論前の状態を FALSE として、推論後に確定できなかった場合に備える
    fact.currentState = FactState::FALSE; 
    fact.true_reasons.clear(); // 新しい推論サイクルのためクリア

    // 3. すべての関連するルールを試行 (後向き連鎖)
    for (const auto& rule : rules) {
        // 結論部に symbol が含まれているかをチェック (結論がAND分解されている場合は単一のFact)
        bool concerns_symbol = false;
        if (FactExpression* fe = dynamic_cast<FactExpression*>(rule.consequent.get())) {
            concerns_symbol = (fe->factSymbol == symbol && !fe->isNegated);
        } else if (rule.consequent->isOrXor() || dynamic_cast<BinaryOperation*>(rule.consequent.get())) {
            // OR/XOR または複雑な結論の場合、含まれる事実をチェック
            for(char c : rule.consequent->getFacts()) {
                if(c == symbol) concerns_symbol = true;
            }
        }

        if (concerns_symbol) { 
            FactState premiseState = rule.antecedent->evaluate(*this); 

            if (premiseState == FactState::TRUE) {
                isProvenByAnyRule = true;
                // 記録
                fact.true_reasons.push_back("Derived TRUE from Rule: " + rule.to_string() + " (Premise was TRUE)");
            }
            if (premiseState == FactState::UNDETERMINED) {
                isUndeterminedPossible = true;
            }
        }
    }

    // 4. 結論の決定と状態の更新
    fact.isProcessing = false;

    if (isProvenByAnyRule) {
        fact.currentState = FactState::TRUE;
    } else if (isUndeterminedPossible) {
        fact.currentState = FactState::UNDETERMINED;
        fact.final_state_reason = "Fact is UNDETERMINED. Premise of a relevant rule was UNDETERMINED.";
    } else {
        fact.currentState = FactState::FALSE;
        fact.final_state_reason = "Fact is FALSE (by default/not proven by any rule).";
    }

    return fact.currentState;
}

// --- KnowledgeBase OR/XOR 伝播ロジック (前方連鎖的) ---

void KnowledgeBase::propagate_Undetermined() {
    // 状態が変化しなくなるまで推論を繰り返す
    bool changed;
    int max_iterations = 100; // 安全のための制限
    do {
        changed = false;
        max_iterations--;

        for (Rule& rule : rules) {
            // OR/XOR 結論を持つルール、かつ前提が真のもののみチェック
            if (rule.consequent->isOrXor() && rule.antecedent->evaluate(*this) == FactState::TRUE) {
                
                std::vector<char> conclusions = rule.consequent->getFacts();
                int undetermined_or_false_count = 0;
                char last_undetermined_fact = '\0';
                
                for (char c : conclusions) {
                    FactState state = isFactTrue(c); // 推論を呼び出して最新の状態を取得
                    if (state == FactState::UNDETERMINED || state == FactState::FALSE) {
                        undetermined_or_false_count++;
                        last_undetermined_fact = c;
                    }
                }
                
                // 結論確定ロジック: 1つだけ UNDETERMINED/FALSE が残っている場合
                if (undetermined_or_false_count == 1) {
                    if (facts[last_undetermined_fact].currentState != FactState::TRUE) {
                        // 残った 1 つの事実が TRUE に確定する！
                        facts[last_undetermined_fact].currentState = FactState::TRUE;
                        
                        // 記録
                        std::string reason = "Derived TRUE by elimination from Rule: " + rule.to_string();
                        reason += " (All other conclusions were determined to be FALSE or resolved)";
                        facts[last_undetermined_fact].true_reasons.push_back(reason);

                        changed = true; // 状態が変化
                    }
                }
            }
        }
    } while (changed && max_iterations > 0);
}

// --- KnowledgeBase I/O パーサー ---

void KnowledgeBase::addImpliesRule(const std::string& antecedent_str, const std::string& consequent_str) {
    // 結論部の AND 分解
    std::string temp_conc_str = consequent_str;
    temp_conc_str.erase(std::remove(temp_conc_str.begin(), temp_conc_str.end(), ' '), temp_conc_str.end());

    // 結論が AND 分解可能かチェック (例: B+C)
    size_t plus_pos = temp_conc_str.find('+');

    if (plus_pos != std::string::npos && !temp_conc_str.empty()) {
        std::stringstream ss(temp_conc_str);
        std::string segment;

        while (std::getline(ss, segment, '+')) {
            if (segment.empty()) continue;
            // AND分解された各部分を個別のルールとして追加
            rules.emplace_back(Rule{
                parseExpression(antecedent_str),
                parseExpression(segment) 
            });
        }
    } else {
        // 通常のルール、または OR/XOR 結論 (分解しない)
        rules.emplace_back(Rule{
            parseExpression(antecedent_str),
            parseExpression(consequent_str) 
        });
    }
}

void KnowledgeBase::parseRule(const std::string& rule_str) {
    // 1. <=> の検出と分解 (ボーナス)
    size_t biconditional_pos = rule_str.find("<=>");
    if (biconditional_pos != std::string::npos) {
        std::string left_side = rule_str.substr(0, biconditional_pos);
        std::string right_side = rule_str.substr(biconditional_pos + 3);

        addImpliesRule(left_side, right_side); 
        addImpliesRule(right_side, left_side); 
        return;
    }

    // 2. => の検出 (必須)
    size_t implies_pos = rule_str.find("=>");
    if (implies_pos != std::string::npos) {
        std::string antecedent_str = rule_str.substr(0, implies_pos);
        std::string consequent_str = rule_str.substr(implies_pos + 2);
        
        addImpliesRule(antecedent_str, consequent_str); 
        return;
    }
    
    // エラー処理
    syntaxError("Invalid rule format: expected '=>' or '<=>'");
}

void KnowledgeBase::parseInitialFacts(const std::string& fact_str, bool interactive) {
    std::string cleaned_str = fact_str;
    cleaned_str.erase(std::remove_if(cleaned_str.begin(), cleaned_str.end(), ::isspace), cleaned_str.end());

    // リセットモードでない場合、既存の初期事実を FALSE に設定
    if (!interactive) {
        for (auto& pair : facts) {
            if (pair.second.currentState == FactState::TRUE) {
                 pair.second.currentState = FactState::FALSE;
            }
        }
    }

    for (char c : cleaned_str) {
        if (c >= 'A' && c <= 'Z') {
            facts[c].symbol = c;
            facts[c].currentState = FactState::TRUE;
        }
    }
}

void KnowledgeBase::parseQueries(const std::string& query_str) {
    std::string cleaned_str = query_str;
    cleaned_str.erase(std::remove_if(cleaned_str.begin(), cleaned_str.end(), ::isspace), cleaned_str.end());

    for (char c : cleaned_str) {
        if (c >= 'A' && c <= 'Z') {
            queries.push_back(c);
        }
    }
}

void KnowledgeBase::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Error: Could not open file " + filename);
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t non_comment_start = line.find_first_not_of(" \t");
        if (non_comment_start == std::string::npos || line[non_comment_start] == '#') {
            continue;
        }

        std::string cleaned_line = line.substr(non_comment_start);
        
        size_t inline_comment_pos = cleaned_line.find('#');
        if (inline_comment_pos != std::string::npos) {
            cleaned_line = cleaned_line.substr(0, inline_comment_pos);
        }
        size_t last_char = cleaned_line.find_last_not_of(" \t\n\r");
        if (last_char == std::string::npos) continue; // 行が空になった場合
        cleaned_line = cleaned_line.substr(0, last_char + 1);

        if (cleaned_line.empty()) continue; //コメント削除後に空になった場合

        if (cleaned_line.front() == '?') {
            parseQueries(cleaned_line.substr(1));
        } else if (cleaned_line.front() == '=') {
            parseInitialFacts(cleaned_line.substr(1));
        } else if (cleaned_line.find("=>") != std::string::npos || 
                   cleaned_line.find("<=>") != std::string::npos) {
            parseRule(cleaned_line);
        }
    }

    saveInitialState(); // インタラクティブモード用に初期状態を保存
}

// --- KnowledgeBase 実行と出力 ---

void KnowledgeBase::runQueries(bool verbose) {
    // 1. 全ての状態をリセット (インタラクティブモードからの呼び出しに備える)
    resetFacts();

    // 2. OR/XOR伝播を繰り返す (ボーナス)
    propagate_Undetermined(); 

    // 3. クエリを実行し、結果を出力
    for (char query_fact : queries) {
        FactState result = isFactTrue(query_fact);
        std::string result_str;
        
        if (result == FactState::TRUE) {
            result_str = "is True";
        } else if (result == FactState::FALSE) {
            result_str = "is False";
        } else {
            result_str = "is Undetermined";
        }
        
        std::cout << query_fact << " " << result_str << std::endl;

        if (verbose) {
            // 推論の可視化 (ボーナス)
            std::cout << "--- Reasoning for " << query_fact << " ---" << std::endl;
            if (result == FactState::TRUE) {
                for (const auto& reason : facts[query_fact].true_reasons) {
                    std::cout << "  - " << reason << std::endl;
                }
            } else {
                std::cout << "  " << facts[query_fact].final_state_reason << std::endl;
            }
            std::cout << "--------------------------" << std::endl;
        }
    }
}

void KnowledgeBase::saveInitialState() {
    initial_fact_states.clear();
    for (const auto& pair : facts) {
        initial_fact_states[pair.first] = pair.second.currentState;
    }
}

void KnowledgeBase::resetFacts() {
    // 全ての事実の状態と推論フラグをリセット
    for (auto& pair : facts) {
        pair.second.currentState = FactState::FALSE;
        pair.second.isProcessing = false;
        pair.second.true_reasons.clear();
        pair.second.final_state_reason.clear();
    }
    
    // 初期状態を復元 (入力ファイルやインタラクティブ設定で TRUE になったもの)
    for (const auto& pair : initial_fact_states) {
        if (pair.second == FactState::TRUE) {
             facts[pair.first].currentState = FactState::TRUE;
        }
    }
}

void KnowledgeBase::runInteractiveMode() {
    std::cout << "\n--- Interactive Fact Validation Mode ---" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  ? <Facts> : Run queries (e.g., ?GVX)" << std::endl;
    std::cout << "  = <Facts> : Set facts to TRUE (e.g., =A B)" << std::endl;
    std::cout << "  ! <Facts> : Set facts to FALSE (e.g., !C)" << std::endl;
    std::cout << "  log       : Toggle verbose output (Reasoning Visualization)" << std::endl;
    std::cout << "  exit      : Exit interactive mode" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    std::string command;
    bool verbose = true; // 可視化をデフォルトでオン

    while (std::cout << "KB> " && std::getline(std::cin, command)) {
        if (command == "exit") break;
        if (command == "log") {
            verbose = !verbose;
            std::cout << "Verbose output is " << (verbose ? "ON" : "OFF") << "." << std::endl;
            continue;
        }

        // 1. 推論結果をリセットし、初期状態を復元
        resetFacts();
        
        if (command.front() == '?') {
            queries.clear();
            parseQueries(command.substr(1));
            runQueries(verbose);
        } else if (command.front() == '=') {
            // 現在の知識ベースの状態に上書き
            std::string fact_str = command.substr(1);
            fact_str.erase(std::remove_if(fact_str.begin(), fact_str.end(), ::isspace), fact_str.end());
            for (char c : fact_str) {
                if (c >= 'A' && c <= 'Z') initial_fact_states[c] = FactState::TRUE;
            }
            std::cout << "Facts set to TRUE. Run query with '?'" << std::endl;
        } else if (command.front() == '!') {
            // 現在の知識ベースの状態に上書き
            std::string fact_str = command.substr(1);
            fact_str.erase(std::remove_if(fact_str.begin(), fact_str.end(), ::isspace), fact_str.end());
            for (char c : fact_str) {
                if (c >= 'A' && c <= 'Z') initial_fact_states[c] = FactState::FALSE;
            }
            std::cout << "Facts set to FALSE. Run query with '?'" << std::endl;
        } else {
            std::cout << "Unknown command." << std::endl;
        }
    }
}
