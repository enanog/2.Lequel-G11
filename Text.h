/**
 * @brief Reads text files
 * @author Marc S. Ressl
 * @modified Dylan Frigerio, Micaela Dinsen
 * 
 * @copyright Copyright (c) 2022-2023
 */

#ifndef TEXT_H
#define TEXT_H

#include <vector>
#include <string>

// Text: list of strings
typedef std::vector<std::wstring> Text;

// Functions
bool getTextFromString(const std::string &s, Text &text);
bool getTextFromFile(const std::string path, Text &text);

#endif
