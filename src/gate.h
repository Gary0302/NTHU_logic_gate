#ifndef GATE_H
#define GATE_H

#include <string>
#include <vector>
#include <utility>

enum class GateType {
    BUF, NOT,
    AND, OR, NAND, NOR, XOR, XNOR,
    UNKNOWN
};

struct Gate {
    GateType type;
    std::string output_name;
    std::vector<std::string> input_names;

    Gate(GateType t, std::string out, std::vector<std::string> ins)
        : type(t), output_name(std::move(out)), input_names(std::move(ins)) {}
};

#endif // GATE_H