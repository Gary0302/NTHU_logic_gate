#include "circuit.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept> // 可以用於更嚴格的錯誤處理
#include <algorithm> // for std::find

// 將字串形式的閘類型轉換為 GateType 枚舉
GateType Circuit::string_to_gate_type(const std::string& type_str) {
    if (type_str == "buf") return GateType::BUF;
    if (type_str == "not") return GateType::NOT;
    if (type_str == "and") return GateType::AND;
    if (type_str == "or") return GateType::OR;
    if (type_str == "nand") return GateType::NAND;
    if (type_str == "nor") return GateType::NOR;
    if (type_str == "xor") return GateType::XOR;
    if (type_str == "xnor") return GateType::XNOR;
    return GateType::UNKNOWN; // 無效類型
}

// 解析電路描述檔
bool Circuit::parse_circuit(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Error: Cannot open circuit file: " << filename << std::endl;
        return false;
    }

    std::string line;
    int num_pis = 0, num_pos = 0, num_gates = 0;

    // 1. 讀取標頭 (Header): PI, PO, Gate 數量
    if (!std::getline(infile, line)) {
        std::cerr << "Error: Circuit file is empty or cannot read header." << std::endl;
        return false;
    }
    std::istringstream iss_header(line);
    // 忽略可能的註解 (以 // 開頭)
    std::string segment;
    std::vector<int> counts;
    while(iss_header >> segment) {
        if (segment.rfind("//", 0) == 0) break; // Stop at comment
        try {
           counts.push_back(std::stoi(segment));
        } catch (const std::invalid_argument& ia) {
             std::cerr << "Error: Invalid number format in header: " << segment << std::endl;
             return false;
        } catch (const std::out_of_range& oor) {
             std::cerr << "Error: Number out of range in header: " << segment << std::endl;
             return false;
        }
    }
    if (counts.size() != 3) {
        std::cerr << "Error: Header format incorrect. Expected 3 numbers (PIs POs Gates)." << std::endl;
        return false;
    }
    num_pis = counts[0];
    num_pos = counts[1];
    num_gates = counts[2];


    // 基本的數量驗證
    if (num_pis <= 0 || num_pos <= 0 || num_gates < 0) { // 專案說 gates >= 1，但允許 0 閘可能更健壯？ 依照spec >= 1
        std::cerr << "Error: Invalid counts in header (PIs>0, POs>0, Gates>=0 required)." << std::endl;
        return false;
    }

    // 2. 讀取 PI 名稱
    primary_inputs.clear();
    pi_set.clear();
    if (!std::getline(infile, line)) {
         std::cerr << "Error: Cannot read PI names line." << std::endl;
         return false;
    }
    std::istringstream iss_pis(line);
    std::string pi_name;
    while (iss_pis >> pi_name) {
         if (pi_name.rfind("//", 0) == 0) break; // Stop at comment
        primary_inputs.push_back(pi_name);
        pi_set.insert(pi_name); // 加入 set 以便快速查找
    }
    if (primary_inputs.size() != static_cast<size_t>(num_pis)) {
        std::cerr << "Error: PI count mismatch. Header=" << num_pis << ", Found=" << primary_inputs.size() << std::endl;
        return false;
    }

    // 3. 讀取 PO 名稱
    primary_outputs.clear();
     if (!std::getline(infile, line)) {
         std::cerr << "Error: Cannot read PO names line." << std::endl;
         return false;
    }
    std::istringstream iss_pos(line);
    std::string po_name;
    while (iss_pos >> po_name) {
        if (po_name.rfind("//", 0) == 0) break; // Stop at comment
        primary_outputs.push_back(po_name);
        // PO 也可以是 PI，所以不用檢查是否已在 pi_set 中
    }
    if (primary_outputs.size() != static_cast<size_t>(num_pos)) {
        std::cerr << "Error: PO count mismatch. Header=" << num_pos << ", Found=" << primary_outputs.size() << std::endl;
        return false;
    }

    // 4. 讀取 Gates
    gates.clear();
    gates.reserve(num_gates); // 預先分配空間以提高效率
    output_to_gate_index.clear(); // 清空 map
    for (int i = 0; i < num_gates; ++i) {
        if (!std::getline(infile, line)) {
            std::cerr << "Error: Unexpected end of file while reading gates. Expected " << num_gates << ", read " << i << "." << std::endl;
            return false;
        }
        // 跳過空行或純註解行
        std::string temp_line_check = line;
        temp_line_check.erase(0, temp_line_check.find_first_not_of(" \t\n\r\f\v")); // trim leading whitespace
        if (temp_line_check.empty() || temp_line_check.rfind("//", 0) == 0) {
            i--; // 這行不算，需要再讀一行
            continue;
        }


        std::istringstream iss_gate(line);
        std::string gate_type_str, output, input1, input2;
        std::vector<std::string> inputs;

        if (!(iss_gate >> gate_type_str)) continue; // 如果行是空的 (理論上被上面檢查掉了)
        if (gate_type_str.rfind("//", 0) == 0) { // 如果第一個就是註解
             i--; continue;
        }


        GateType type = string_to_gate_type(gate_type_str);
        if (type == GateType::UNKNOWN) {
            std::cerr << "Error: Unknown gate type '" << gate_type_str << "' on line " << (i + 4 + (primary_inputs.empty()?0:1) + (primary_outputs.empty()?0:1)) << std::endl; // 估計行號
            return false;
        }

        if (type == GateType::BUF || type == GateType::NOT) {
            // 讀取 input1, output，並檢查是否有註解或其他多餘內容
             if (!(iss_gate >> input1 >> output)) {
                  std::cerr << "Error: Invalid format for " << gate_type_str << " gate (expected 1 input, 1 output) near line " << (i + 4 + (primary_inputs.empty()?0:1) + (primary_outputs.empty()?0:1)) << std::endl;
                 return false;
             }
             // 檢查是否讀到註解開頭
             if(input1.rfind("//",0) == 0 || output.rfind("//",0) == 0) {
                  std::cerr << "Error: Unexpected comment format for " << gate_type_str << " gate near line " << (i + 4 + (primary_inputs.empty()?0:1) + (primary_outputs.empty()?0:1)) << std::endl;
                  return false;
             }
            inputs.push_back(input1);
        } else { // 雙輸入閘
             if (!(iss_gate >> input1 >> input2 >> output)) {
                 std::cerr << "Error: Invalid format for " << gate_type_str << " gate (expected 2 inputs, 1 output) near line " << (i + 4 + (primary_inputs.empty()?0:1) + (primary_outputs.empty()?0:1)) << std::endl;
                 return false;
             }
             if(input1.rfind("//",0) == 0 || input2.rfind("//",0) == 0 || output.rfind("//",0) == 0) {
                  std::cerr << "Error: Unexpected comment format for " << gate_type_str << " gate near line " << (i + 4 + (primary_inputs.empty()?0:1) + (primary_outputs.empty()?0:1)) << std::endl;
                  return false;
             }
            inputs.push_back(input1);
            inputs.push_back(input2);
        }

        // 檢查輸出線是否已被其他閘驅動 (避免 multiple drivers)
        if (output_to_gate_index.count(output)) {
            std::cerr << "Error: Multiple gates driving the same output wire '" << output << "'" << std::endl;
            return false;
        }
        // 檢查輸出線是否與 PI 同名 (通常不允許，但要看 spec)
        if (pi_set.count(output)) {
             std::cerr << "Warning: Gate output '" << output << "' has the same name as a Primary Input." << std::endl;
             // 根據具體要求，這可能是錯誤或警告
             // return false; // 如果這是錯誤
        }


        // 創建 Gate 物件並加入列表
        gates.emplace_back(type, output, inputs);
        // 記錄這個輸出線是由哪個 gate (其索引) 產生的
        output_to_gate_index[output] = gates.size() - 1;
    }

    // 再次驗證讀取的閘數量是否正確
    if (gates.size() != static_cast<size_t>(num_gates)) {
        std::cerr << "Error: Gate count mismatch. Header=" << num_gates << ", Found=" << gates.size() << "." << std::endl;
        return false;
    }

    infile.close();
    std::cout << "Circuit parsing successful: " << primary_inputs.size() << " PIs, "
              << primary_outputs.size() << " POs, " << gates.size() << " Gates." << std::endl;
    return true;
}


// 遞迴計算線路值 (核心模擬邏輯)
int Circuit::get_wire_value(const std::string& wire_name,
                           std::map<std::string, int>& current_values) {

    // 1. 檢查是否已計算過 (Memoization)
    auto it = current_values.find(wire_name);
    if (it != current_values.end()) {
        return it->second; // 直接返回快取的值
    }

    // 2. 檢查是否是 PI (Primary Input)
    // PI 的值應該在 simulate_all_patterns 中被預先載入 current_values
    // 如果執行到這裡，一個 PI 卻不在 current_values 中，表示有邏輯錯誤
    // 我們在下面找不到驅動閘時，可以間接處理這種情況

    // 3. 查找驅動此線路的閘 (Gate)
    auto gate_it = output_to_gate_index.find(wire_name);
    if (gate_it == output_to_gate_index.end()) {
        // 如果找不到驅動閘，檢查它是否是 PI
        if (pi_set.count(wire_name)) {
             // 這是個 PI，但它的值沒有被載入 current_values (可能在 simulate_all_patterns 漏了)
             std::cerr << "Internal Error: PI '" << wire_name << "' value not loaded." << std::endl;
        } else {
             // 這條線既不是 PI，也不是任何已知閘的輸出，這是一個懸空輸入 (dangling input) 或錯誤的線路名
            std::cerr << "Error: Wire '" << wire_name << "' is not a PI and has no driving gate." << std::endl;
        }
        return -1; // 返回錯誤碼
    }

    // 獲取驅動這個線路的閘
    const Gate& driving_gate = gates[gate_it->second];

    // 4. 遞迴計算此閘所有輸入線路的值
    std::vector<int> input_vals;
    input_vals.reserve(driving_gate.input_names.size());
    for (const std::string& input_name : driving_gate.input_names) {
        int val = get_wire_value(input_name, current_values); // 遞迴調用
        if (val == -1) {
            // 如果任何一個輸入無法計算，則此閘的輸出也無法計算
            // 錯誤信息已在下層遞迴中打印
            return -1; // 向上傳播錯誤
        }
        input_vals.push_back(val);
    }

    // 5. 根據閘類型和輸入值計算輸出值
    int result = -1; // 預設為錯誤
    int val1 = input_vals[0];
    // 對於單輸入閘，val2 用不到，但為避免未初始化，給個值 (e.g., 0)
    int val2 = (input_vals.size() > 1) ? input_vals[1] : 0;

    switch (driving_gate.type) {
        case GateType::BUF:  result = val1; break;
        case GateType::NOT:  result = !val1; break; // NOT 0 = 1, NOT 1 = 0
        case GateType::AND:  result = val1 & val2; break;
        case GateType::OR:   result = val1 | val2; break;
        case GateType::NAND: result = !(val1 & val2); break;
        case GateType::NOR:  result = !(val1 | val2); break;
        case GateType::XOR:  result = val1 ^ val2; break;
        case GateType::XNOR: result = !(val1 ^ val2); break;
        case GateType::UNKNOWN: // 理論上不應發生
             std::cerr << "Internal Error: Trying to evaluate UNKNOWN gate type." << std::endl;
             return -1;
    }

    // 將 C++ 的布林運算結果 (可能是非 0/1 的 int) 轉換為嚴格的 0 或 1
    result = (result != 0);

    // 6. 將計算結果存入快取 (Memoization)
    current_values[wire_name] = result;

    return result;
}

// 模擬所有輸入模式
bool Circuit::simulate_all_patterns(const std::string& pattern_file, const std::string& output_file) {
    std::ifstream pat_infile(pattern_file);
    if (!pat_infile.is_open()) {
        std::cerr << "Error: Cannot open pattern file: " << pattern_file << std::endl;
        return false;
    }

    std::ofstream outfile(output_file);
    if (!outfile.is_open()) {
        std::cerr << "Error: Cannot open output file: " << output_file << std::endl;
        pat_infile.close();
        return false;
    }

    std::string pattern_line;
    int pattern_count = 0;
    while (std::getline(pat_infile, pattern_line)) {
        pattern_count++;

        // 移除可能的行尾註解
        size_t comment_pos = pattern_line.find("//");
        if (comment_pos != std::string::npos) {
            pattern_line = pattern_line.substr(0, comment_pos);
        }
        // 移除尾部空白
        pattern_line.erase(pattern_line.find_last_not_of(" \t\n\r\f\v") + 1);
        // 移除頭部空白
         pattern_line.erase(0, pattern_line.find_first_not_of(" \t\n\r\f\v"));

        if (pattern_line.empty()) continue; // 跳過空行


        // 驗證模式長度是否與 PI 數量匹配
        if (pattern_line.length() != primary_inputs.size()) {
            std::cerr << "Error: Pattern #" << pattern_count << " length (" << pattern_line.length()
                      << ") does not match PI count (" << primary_inputs.size() << ")." << std::endl;
            outfile.close(); // 出錯時關閉文件
            pat_infile.close();
            return false; // 中斷模擬
        }

        // --- 對於單個模式的模擬 ---
        // 創建一個新的 map 來儲存這個模式下的線路值
        std::map<std::string, int> current_wire_values;

        // 1. 從模式字串載入 PI 的值到 map
        bool pattern_valid = true;
        for (size_t i = 0; i < primary_inputs.size(); ++i) {
            char bit = pattern_line[i];
            if (bit == '0') {
                current_wire_values[primary_inputs[i]] = 0;
            } else if (bit == '1') {
                current_wire_values[primary_inputs[i]] = 1;
            } else {
                std::cerr << "Error: Invalid character '" << bit << "' in pattern #"
                          << pattern_count << " at position " << i << "." << std::endl;
                pattern_valid = false;
                break; // 無需繼續處理此模式
            }
        }
        if (!pattern_valid) {
             outfile.close();
             pat_infile.close();
             return false; // 中斷模擬
        }


        // 2. 計算所有 PO (Primary Output) 的值
        std::vector<int> po_values;
        po_values.reserve(primary_outputs.size());
        bool error_occurred = false;
        for (const std::string& po_name : primary_outputs) {
            // 調用遞迴函數計算每個 PO 的值
            // 注意: current_wire_values 會在 get_wire_value 內部被填充
            int po_val = get_wire_value(po_name, current_wire_values);

            if (po_val == -1) { // 檢查是否有計算錯誤
                 // 錯誤信息已在 get_wire_value 中打印
                 std::cerr << "Error: Failed to calculate value for PO '" << po_name
                           << "' for pattern #" << pattern_count << "." << std::endl;
                 error_occurred = true;
                 break; // 停止計算此模式的其他 PO
            }
            po_values.push_back(po_val);
        }

         if (error_occurred) {
            // 如果計算 PO 時出錯，則停止整個模擬過程
            outfile.close();
            pat_infile.close();
             return false;
         }

        // 3. 將計算出的 PO 值寫入輸出文件
        // 格式要求：將所有 PO 的值連在一起，沒有空格
        for (size_t i = 0; i < po_values.size(); ++i) {
            outfile << po_values[i];
        }
        outfile << std::endl; // 每個模式的輸出佔一行
    }

    pat_infile.close();
    outfile.close();
    std::cout << "Simulation of " << pattern_count << " patterns completed successfully." << std::endl;
    return true; // 所有模式都成功模擬
}