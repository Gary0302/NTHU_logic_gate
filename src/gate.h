#ifndef GATE_H
#define GATE_H

#include <string>
#include <vector>
#include <utility> // for std::move

// 定義支援的邏輯閘類型
enum class GateType {
    BUF, NOT,
    AND, OR, NAND, NOR, XOR, XNOR,
    UNKNOWN // 用於錯誤處理或未定義狀態
};

// 代表一個邏輯閘
struct Gate {
    GateType type;                // 閘的類型
    std::string output_name;      // 輸出線路的名稱
    std::vector<std::string> input_names; // 輸入線路的名稱列表 (1 或 2 個)

    // 建構子，方便創建 Gate 物件
    Gate(GateType t, std::string out, std::vector<std::string> ins)
        : type(t), output_name(std::move(out)), input_names(std::move(ins)) {}
};

#endif // GATE_H