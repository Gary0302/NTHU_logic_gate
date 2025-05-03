#include "circuit.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept> 
#include <algorithm> 

GateType Circuit::string_to_gate_type(const std::string& type_str) {
    if (type_str == "buf") return GateType::BUF;
    if (type_str == "not") return GateType::NOT;
    if (type_str == "and") return GateType::AND;
    if (type_str == "or") return GateType::OR;
    if (type_str == "nand") return GateType::NAND;
    if (type_str == "nor") return GateType::NOR;
    if (type_str == "xor") return GateType::XOR;
    if (type_str == "xnor") return GateType::XNOR;
    return GateType::UNKNOWN; 
}


bool Circuit::parse_circuit(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Error: Cannot open circuit file: " << filename << std::endl;
        return false;
    }

    std::string line;
    int num_pis = 0, num_pos = 0, num_gates = 0;

    
    if (!std::getline(infile, line)) {
        std::cerr << "Error: Circuit file is empty or cannot read header." << std::endl;
        return false;
    }
    std::istringstream iss_header(line);
    
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


    
    if (num_pis <= 0 || num_pos <= 0 || num_gates < 0) { 
        std::cerr << "Error: Invalid counts in header (PIs>0, POs>0, Gates>=0 required)." << std::endl;
        return false;
    }

    
    primary_inputs.clear();
    pi_set.clear();
    if (!std::getline(infile, line)) {
         std::cerr << "Error: Cannot read PI names line." << std::endl;
         return false;
    }
    std::istringstream iss_pis(line);
    std::string pi_name;
    while (iss_pis >> pi_name) {
         if (pi_name.rfind("//", 0) == 0) break; 
        primary_inputs.push_back(pi_name);
        pi_set.insert(pi_name); 
    }
    if (primary_inputs.size() != static_cast<size_t>(num_pis)) {
        std::cerr << "Error: PI count mismatch. Header=" << num_pis << ", Found=" << primary_inputs.size() << std::endl;
        return false;
    }

    
    primary_outputs.clear();
     if (!std::getline(infile, line)) {
         std::cerr << "Error: Cannot read PO names line." << std::endl;
         return false;
    }
    std::istringstream iss_pos(line);
    std::string po_name;
    while (iss_pos >> po_name) {
        if (po_name.rfind("//", 0) == 0) break;
        primary_outputs.push_back(po_name);
        
    }
    if (primary_outputs.size() != static_cast<size_t>(num_pos)) {
        std::cerr << "Error: PO count mismatch. Header=" << num_pos << ", Found=" << primary_outputs.size() << std::endl;
        return false;
    }

    
    gates.clear();
    gates.reserve(num_gates); 
    output_to_gate_index.clear(); 
    for (int i = 0; i < num_gates; ++i) {
        if (!std::getline(infile, line)) {
            std::cerr << "Error: Unexpected end of file while reading gates. Expected " << num_gates << ", read " << i << "." << std::endl;
            return false;
        }
        
        std::string temp_line_check = line;
        temp_line_check.erase(0, temp_line_check.find_first_not_of(" \t\n\r\f\v")); 
        if (temp_line_check.empty() || temp_line_check.rfind("//", 0) == 0) {
            i--; 
            continue;
        }


        std::istringstream iss_gate(line);
        std::string gate_type_str, output, input1, input2;
        std::vector<std::string> inputs;

        if (!(iss_gate >> gate_type_str)) continue; 
        if (gate_type_str.rfind("//", 0) == 0) { 
             i--; continue;
        }


        GateType type = string_to_gate_type(gate_type_str);
        if (type == GateType::UNKNOWN) {
            std::cerr << "Error: Unknown gate type '" << gate_type_str << "' on line " << (i + 4 + (primary_inputs.empty()?0:1) + (primary_outputs.empty()?0:1)) << std::endl; // 估計行號
            return false;
        }

        if (type == GateType::BUF || type == GateType::NOT) {
            
             if (!(iss_gate >> input1 >> output)) {
                  std::cerr << "Error: Invalid format for " << gate_type_str << " gate (expected 1 input, 1 output) near line " << (i + 4 + (primary_inputs.empty()?0:1) + (primary_outputs.empty()?0:1)) << std::endl;
                 return false;
             }
             
             if(input1.rfind("//",0) == 0 || output.rfind("//",0) == 0) {
                  std::cerr << "Error: Unexpected comment format for " << gate_type_str << " gate near line " << (i + 4 + (primary_inputs.empty()?0:1) + (primary_outputs.empty()?0:1)) << std::endl;
                  return false;
             }
            inputs.push_back(input1);
        } else { 
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

        
        if (output_to_gate_index.count(output)) {
            std::cerr << "Error: Multiple gates driving the same output wire '" << output << "'" << std::endl;
            return false;
        }
        
        if (pi_set.count(output)) {
             std::cerr << "Warning: Gate output '" << output << "' has the same name as a Primary Input." << std::endl;
             
        }


        
        gates.emplace_back(type, output, inputs);
        
        output_to_gate_index[output] = gates.size() - 1;
    }

    
    if (gates.size() != static_cast<size_t>(num_gates)) {
        std::cerr << "Error: Gate count mismatch. Header=" << num_gates << ", Found=" << gates.size() << "." << std::endl;
        return false;
    }

    infile.close();
    std::cout << "Circuit parsing successful: " << primary_inputs.size() << " PIs, "
              << primary_outputs.size() << " POs, " << gates.size() << " Gates." << std::endl;
    return true;
}



int Circuit::get_wire_value(const std::string& wire_name,
                           std::map<std::string, int>& current_values) {

    
    auto it = current_values.find(wire_name);
    if (it != current_values.end()) {
        return it->second; 
    }

    
    
    auto gate_it = output_to_gate_index.find(wire_name);
    if (gate_it == output_to_gate_index.end()) {
       
        if (pi_set.count(wire_name)) {
             
             std::cerr << "Internal Error: PI '" << wire_name << "' value not loaded." << std::endl;
        } else {
             
            std::cerr << "Error: Wire '" << wire_name << "' is not a PI and has no driving gate." << std::endl;
        }
        return -1; 
    }

    
    const Gate& driving_gate = gates[gate_it->second];

    
    std::vector<int> input_vals;
    input_vals.reserve(driving_gate.input_names.size());
    for (const std::string& input_name : driving_gate.input_names) {
        int val = get_wire_value(input_name, current_values); 
        if (val == -1) {
            return -1; 
        }
        input_vals.push_back(val);
    }

    
    int result = -1; 
    int val1 = input_vals[0];
   
    int val2 = (input_vals.size() > 1) ? input_vals[1] : 0;

    switch (driving_gate.type) {
        case GateType::BUF:  result = val1; break;
        case GateType::NOT:  result = !val1; break; 
        case GateType::AND:  result = val1 & val2; break;
        case GateType::OR:   result = val1 | val2; break;
        case GateType::NAND: result = !(val1 & val2); break;
        case GateType::NOR:  result = !(val1 | val2); break;
        case GateType::XOR:  result = val1 ^ val2; break;
        case GateType::XNOR: result = !(val1 ^ val2); break;
        case GateType::UNKNOWN: 
             std::cerr << "Internal Error: Trying to evaluate UNKNOWN gate type." << std::endl;
             return -1;
    }

    
    result = (result != 0);

    
    current_values[wire_name] = result;

    return result;
}
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

        
        size_t comment_pos = pattern_line.find("//");
        if (comment_pos != std::string::npos) {
            pattern_line = pattern_line.substr(0, comment_pos);
        }
        
        pattern_line.erase(pattern_line.find_last_not_of(" \t\n\r\f\v") + 1);
        
         pattern_line.erase(0, pattern_line.find_first_not_of(" \t\n\r\f\v"));

        if (pattern_line.empty()) continue; 


        
        if (pattern_line.length() != primary_inputs.size()) {
            std::cerr << "Error: Pattern #" << pattern_count << " length (" << pattern_line.length()
                      << ") does not match PI count (" << primary_inputs.size() << ")." << std::endl;
            outfile.close(); 
            pat_infile.close();
            return false; 
        }

        
        std::map<std::string, int> current_wire_values;

        
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
                break; 
            }
        }
        if (!pattern_valid) {
             outfile.close();
             pat_infile.close();
             return false; 
        }


        
        std::vector<int> po_values;
        po_values.reserve(primary_outputs.size());
        bool error_occurred = false;
        for (const std::string& po_name : primary_outputs) {
            int po_val = get_wire_value(po_name, current_wire_values);

            if (po_val == -1) { 
                 std::cerr << "Error: Failed to calculate value for PO '" << po_name
                           << "' for pattern #" << pattern_count << "." << std::endl;
                 error_occurred = true;
                 break; 
            }
            po_values.push_back(po_val);
        }

         if (error_occurred) {
            outfile.close();
            pat_infile.close();
             return false;
         }
        for (size_t i = 0; i < po_values.size(); ++i) {
            outfile << po_values[i];
        }
        outfile << std::endl; 
    }

    pat_infile.close();
    outfile.close();
    std::cout << "Simulation of " << pattern_count << " patterns completed successfully." << std::endl;
    return true; 
}