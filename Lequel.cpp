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
#include <list>
#include <map>
#include "Lequel.h"

using namespace std;

static inline void extractTrigramsFromLine(const string& line, TrigramProfile& trigrams);

/**
 * @brief Builds a trigram profile from a given text.
 *
 * @param text Vector of lines (Text)
 * @return TrigramProfile The trigram profile
 */
TrigramProfile buildTrigramProfile(const Text &text)
{
    TrigramProfile trigrams;
    
    try
    {
        size_t totalChars = 0;
        for (const auto& line : text)
        {
			totalChars += line.length();
        }

        size_t estimatedTrigrams = min(totalChars, static_cast<size_t>(100000));
        trigrams.reserve(estimatedTrigrams);

        for (const auto& line : text)
        {
            if (!line.empty())
            {
                extractTrigramsFromLine(line, trigrams);
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error processing text: " << e.what() << std::endl;
	}

    return trigrams;
}

/**
 * @brief Normalizes a trigram profile.
 *
 * @param trigramProfile The trigram profile.
 */
void normalizeTrigramProfile(TrigramProfile &trigramProfile)
{
    if (trigramProfile.empty()) return;

    if (trigramProfile.size() == 1) {
        trigramProfile.begin()->second = 1.0f;
        return;
    }

    try
    {
        float norm = 0.0f;
        for (const auto& pair : trigramProfile) {
            norm += pair.second * pair.second;
        }
        norm = sqrtf(norm);

        //cout << "Normalized trigram profile with norm: " << norm << endl;

        if (norm > 0.0f) {
            for (auto& pair : trigramProfile) {
				//cout << "Trigram: " << pair.first << ", Frequency before normalization: " << pair.second << endl;
                pair.second /= norm;
            }
		}
    }
    catch (const std::exception& e)
    {
		std::cerr << "Error normalizing trigram profile: " << e.what() << std::endl;
    }
}

/**
 * @brief Calculates the cosine similarity between two trigram profiles
 *
 * @param textProfile The text trigram profile
 * @param languageProfile The language trigram profile
 * @return float The cosine similarity score
 */
float getCosineSimilarity(const TrigramProfile &textProfile, const TrigramProfile &language)
{
    try 
    {
		float dotProduct = 0.0f;
        for (const auto& textPair : textProfile)
        {
            auto it = language.find(textPair.first);
            if (it != language.end())
            {
                dotProduct += textPair.second * it->second;
            }
        }
		return dotProduct; // Since both profiles are normalized, the cosine similarity is the dot product
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error calculating cosine similarity: " << e.what() << std::endl;
	}
    return -1.0f;
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
    if (text.empty() || languages.empty()) {
        return "unknown";;
    }

    try
    {
        TrigramProfile textTrigrams = buildTrigramProfile(text);

        normalizeTrigramProfile(textTrigrams);

        vector<float> cosineSimilarity(languages.size());

        for (size_t i = 0; i < languages.size(); i++)
        {
            cosineSimilarity[i] = getCosineSimilarity(textTrigrams, languages[i].trigramProfile);
        }

        float maxSimilarity = -1.0f;
        size_t bestIndex = 0;
        for (size_t i = 0; i < cosineSimilarity.size(); i++)
        {
            //cout << "Cosine similarity for " << languages[i].languageCode << ": " << cosineSimilarity[i] << endl;
            if (cosineSimilarity[i] > maxSimilarity)
            {
                maxSimilarity = cosineSimilarity[i];
                bestIndex = i;
            }
        }
        if (maxSimilarity > 0.0f)
            return languages[bestIndex].languageCode;
        else
            return "unknown"; // Fill-in result here
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error calculating cosine similarity: " << e.what() << std::endl;
    }

    return "unknown";
}

static inline void extractTrigramsFromLine(const string& line, TrigramProfile& trigrams)
{
    const size_t length = line.length();
    if (length < 3) return;

    // Remover caracteres de fin de línea
    string cleanLine = line;
    while (!cleanLine.empty() && (cleanLine.back() == '\r' || cleanLine.back() == '\n')) {
        cleanLine.pop_back();
    }

    if (cleanLine.length() < 3) return;
    wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
    wstring wline = converter.from_bytes(cleanLine);

    if (wline.length() < 3) return;

    // Extraer trigramas de caracteres Unicode
    for (size_t i = 0; i <= wline.length() - 3; i++) {
        wstring wtrigram = wline.substr(i, 3);
        string trigram = converter.to_bytes(wtrigram);
        trigrams[trigram]++;
    }
}