/**
 * @brief Lequel? language identification based on trigrams
 * @author Marc S. Ressl
 *
 * @copyright Copyright (c) 2022-2023
 *
 * @cite https://towardsdatascience.com/understanding-cosine-similarity-and-its-application-fd42f585296a
 */

#include <cmath>
#include <codecvt>
#include <locale>
#include <iostream>

#include "Lequel.h"

using namespace std;

/**
 * @brief Builds a trigram profile from a given text.
 *
 * @param text Vector of lines (Text)
 * @return TrigramProfile The trigram profile
 */
TrigramProfile buildTrigramProfile(const Text &text)
{
    TrigramProfile trigrams;
    wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;    
    
    for (auto& line : text)
    {
        if ((line.length() > 0) &&
            (line[line.length() - 1] == '\r'))
            line = line.substr(0, line.length() - 1);

        if (line.length() < 3)
        {
            continue;
        }

		wstring wline = converter.from_bytes(line);
        for (int offset = 0; offset < 3; offset++)
        {
            for (int numTrigram = 0; (3 * numTrigram + offset + 3) < wline.length(); numTrigram++)
            {
				string trigram = converter.to_bytes(wline.substr(3 * numTrigram + offset, 3));
				trigrams[trigram] ++;
            }
        }
    }

    for (auto &par : trigrams)
    {
        cout << "\"" << par.first << "\": " << par.second << endl;
    }

    return TrigramProfile(); // Fill-in result here
}

/**
 * @brief Normalizes a trigram profile.
 *
 * @param trigramProfile The trigram profile.
 */
void normalizeTrigramProfile(TrigramProfile &trigramProfile)
{
    // Your code goes here...

    return;
}

/**
 * @brief Calculates the cosine similarity between two trigram profiles
 *
 * @param textProfile The text trigram profile
 * @param languageProfile The language trigram profile
 * @return float The cosine similarity score
 */
float getCosineSimilarity(TrigramProfile &textProfile, TrigramProfile &languageProfile)
{
    // Your code goes here...

    return 0; // Fill-in result here
}

/**
 * @brief Identifies the language of a text.
 *
 * @param text A Text (vector of lines)
 * @param languages A list of Language objects
 * @return string The language code of the most likely language
 */
string identifyLanguage(const Text &text, LanguageProfiles &languages)
{
    // Your code goes here...
    buildTrigramProfile(text);
    return ""; // Fill-in result here
}
