#include "KnowledgeBase.h"
#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file.txt>" << std::endl;
        return 1;
    }

    KnowledgeBase kb;
    std::string filename = argv[1];

    try {
        kb.loadFromFile(filename);
        kb.runInteractiveMode();

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "An unknown error occurred." << std::endl;
        return 1;
    }

    return 0;
}
