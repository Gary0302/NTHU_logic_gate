#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <vector>
#include <string>
#include <map>
#include <set> // 可以用來檢查PI/PO/Wire名稱是否有效

#include "gate.h" // 包含 Gate 的定義

// 代表整個邏輯電路
class Circuit {
public:
    // 解析電路檔 (.in)
    // 成功返回 true，失敗返回 false
    bool parse_circuit(const std::string& filename);

    // 模擬模式檔 (.txt) 中的所有輸入模式，並將結果寫入輸出檔 (.out)
    // 成功返回 true，失敗返回 false
    bool simulate_all_patterns(const std::string& pattern_file, const std::string& output_file);

private:
    // --- 電路結構 ---
    std::vector<std::string> primary_inputs;  // 主要輸入 (PI) 的名稱列表 (有序)
    std::vector<std::string> primary_outputs; // 主要輸出 (PO) 的名稱列表 (有序)
    std::vector<Gate> gates;                  // 電路中所有邏輯閘的列表 (解析順序)

    // --- 輔助資料結構 ---
    // 為了快速查找某條線是由哪個閘驅動的
    // Key: 輸出線路的名稱
    // Value: gates 向量中的索引 (index)
    std::map<std::string, size_t> output_to_gate_index;

    // 用於快速檢查一個名稱是否是 PI
    std::set<std::string> pi_set;


    // --- 模擬核心 ---
    // 遞迴函數，計算指定線路在當前輸入模式下的值 (0 或 1)
    // 使用 map (current_values) 來儲存已計算的值 (Memoization)，避免重複計算
    // 返回值: 0 或 1，如果計算出錯則返回 -1
    int get_wire_value(const std::string& wire_name,
                       std::map<std::string, int>& current_values);

    // --- 解析輔助 ---
    // 將字串轉換為 GateType 枚舉值
    GateType string_to_gate_type(const std::string& type_str);
};

#endif // CIRCUIT_H