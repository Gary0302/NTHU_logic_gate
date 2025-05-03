#include <iostream>
#include <string>
#include <vector>
#include <chrono> 

#include "circuit.h" 

int main(int argc, char* argv[]) {
    

    
    if (argc != 4) {
        
        std::cerr << "Usage: " << argv[0] << " <circuit_file.in> <pattern_file.txt> <output_file.out>" << std::endl;
        return 1; 
    }

    
    std::string circuit_filename = argv[1];
    std::string pattern_filename = argv[2];
    std::string output_filename = argv[3];

    Circuit circuit;

   
    std::cout << "Parsing circuit file: " << circuit_filename << "..." << std::endl;
    if (!circuit.parse_circuit(circuit_filename)) {
        std::cerr << "Failed to parse circuit file. Exiting." << std::endl;
        return 1;
    }
    


    
    std::cout << "Simulating patterns from: " << pattern_filename << "..." << std::endl;
    if (!circuit.simulate_all_patterns(pattern_filename, output_filename)) {
         std::cerr << "Simulation failed. Exiting." << std::endl;
         return 1; 
    }
    


    std::cout << "Program finished successfully. Output written to: " << output_filename << std::endl;
    return 0; 
}