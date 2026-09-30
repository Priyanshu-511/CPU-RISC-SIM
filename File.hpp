#pragma once

#include <fstream>
#include <iostream>
#include <string>

class File {
    std::string name;
    std::fstream file;

    public:
        File() = default;

        explicit File(const std::string& name) : name(name) {}

        void openFile() {
            if (name.empty()) {
                std::cout << "Error opening file: no file name provided" << std::endl;
                return;
            }
            if (file.is_open()) {
                return;
            }

            file.open(name, std::ios::in | std::ios::out);
            if (!file.is_open()) {
                std::ofstream createFile(name);
                createFile.close();
                file.clear();
                file.open(name, std::ios::in | std::ios::out);
            }
            if (!file.is_open()) {
                std::cout << "Error opening file" << std::endl;
                return;
            }
            std::cout << "File opened" << std::endl;
        }

        void closeFile() {
            if (file.is_open()) {
                file.close();
            }
        }

        void editFile() {
            std::string content;
            std::cout << "Enter file content: ";
            std::getline(std::cin, content);
            editFile(content);
        }

        void editFile(const std::string& content) {
            if (name.empty()) {
                std::cout << "Error editing file: no file name provided" << std::endl;
                return;
            }

            closeFile();
            std::ofstream output(name, std::ios::out | std::ios::trunc);
            if (!output.is_open()) {
                std::cout << "Error editing file" << std::endl;
                return;
            }
            output << content;
            output.close();
            openFile();
        }

        void displayContent() {
            if (!file.is_open()) {
                openFile();
            }
            if (!file.is_open()) {
                return;
            }

            file.clear();
            file.seekg(0, std::ios::beg);
            std::string line;
            while (std::getline(file, line)) {
                std::cout << line << std::endl;
            }
            file.clear();
        }
};