#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <vector>
#include <string>
#include <map>
#include <set>

#include "gate.h"

class Circuit {
public:
    bool parse_circuit(const std::string& filename);
    bool simulate_all_patterns(const std::string& pattern_file, const std::string& output_file);

private:
    std::vector<std::string> primary_inputs;
    std::vector<std::string> primary_outputs;
    std::vector<Gate> gates;

    std::map<std::string, size_t> output_to_gate_index;
    std::set<std::string> pi_set;

    int get_wire_value(const std::string& wire_name,
                       std::map<std::string, int>& current_values);

    GateType string_to_gate_type(const std::string& type_str);
};

#endif // CIRCUIT_H
