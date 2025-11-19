#ifndef FACT_H
#define FACT_H

#include <string>
#include <vector>

class Rule;

// 事実の真偽の状態を定義
enum class FactState {
    TRUE,
    FALSE,
    UNDETERMINED,
    PROCESSING // 無限ループ検出用：現在推論中の状態
};

class Fact {
    public:
        char symbol = '\0';
        FactState currentState = FactState::FALSE; // デフォルトは偽
        bool isProcessing = false; // 推論中のフラグ

        // ボーナス: 推論の可視化のための履歴
        std::vector<std::string> true_reasons; 
        std::string final_state_reason;
};

#endif
