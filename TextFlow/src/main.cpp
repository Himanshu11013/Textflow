#include "better_ui.h"
#include "text_editor.h"
#include <iostream>
#include <memory>
#include <signal.h>
#include <unistd.h>

using namespace TextFlow;

int main(int argc, char* argv[]) {
    std::cout << "TextFlow - Advanced Text Editor" << std::endl;
    std::cout << "Initializing..." << std::endl;
    
    try {
        // Create better UI instance
        auto ui = std::make_unique<BetterUI>();
        
        // Create text editor
        auto editor = std::make_shared<TextEditor>();
        ui->setTextEditor(editor);
        
        // Handle command line arguments
        if (argc > 1) {
            std::string filename = argv[1];
            if (editor->openDocument(filename)) {
                std::cout << "Opened file: " << filename << std::endl;
            } else {
                std::cout << "Failed to open file: " << filename << std::endl;
            }
        }
        
        // Run the UI
        ui->run();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "TextFlow exited successfully" << std::endl;
    return 0;
}
