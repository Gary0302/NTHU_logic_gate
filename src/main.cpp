#include <iostream>
#include <string>
#include <vector>
#include <chrono> // 用於計時 (可選)

#include "circuit.h" // 包含我們定義的 Circuit 類別

int main(int argc, char* argv[]) {
    // 記錄開始時間 (可選)
    // auto start_time = std::chrono::high_resolution_clock::now();

    // 1. 檢查命令列參數數量是否正確
    if (argc != 4) {
        // argv[0] 是程式名稱
        std::cerr << "Usage: " << argv[0] << " <circuit_file.in> <pattern_file.txt> <output_file.out>" << std::endl;
        return 1; // 返回非 0 值表示錯誤
    }

    // 2. 從命令列參數獲取檔案名稱
    std::string circuit_filename = argv[1];
    std::string pattern_filename = argv[2];
    std::string output_filename = argv[3];

    // 3. 創建 Circuit 物件
    Circuit circuit;

    // 4. 解析電路檔案
    std::cout << "Parsing circuit file: " << circuit_filename << "..." << std::endl;
    if (!circuit.parse_circuit(circuit_filename)) {
        std::cerr << "Failed to parse circuit file. Exiting." << std::endl;
        return 1; // 解析失敗，返回錯誤碼
    }
    // 解析成功信息已在 parse_circuit 內部打印


    // 5. 執行模擬
    std::cout << "Simulating patterns from: " << pattern_filename << "..." << std::endl;
    if (!circuit.simulate_all_patterns(pattern_filename, output_filename)) {
         std::cerr << "Simulation failed. Exiting." << std::endl;
         return 1; // 模擬失敗，返回錯誤碼
    }
    // 模擬成功信息已在 simulate_all_patterns 內部打印

    // 記錄結束時間並計算耗時 (可選)
    // auto end_time = std::chrono::high_resolution_clock::now();
    // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    // std::cout << "Total execution time: " << duration.count() << " ms" << std::endl;


    std::cout << "Program finished successfully. Output written to: " << output_filename << std::endl;
    return 0; // 返回 0 表示成功
}