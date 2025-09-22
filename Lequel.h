/**
 * @brief Lequel? language identification based on trigrams
 * @author Marc S. Ressl
 *
 * @copyright Copyright (c) 2022-2023
 *
 * @cite https://towardsdatascience.com/understanding-cosine-similarity-and-its-application-fd42f585296a
 */

#ifndef LEQUEL_H
#define LEQUEL_H

#include <vector>
#include <unordered_map>
#include <string>

#include "Text.h"

 // TrigramProfile: unorderedmap of trigram (as uint32_t) -> frequency
typedef std::unordered_map<uint64_t, float> TrigramProfile;

// TrigramList: vector of trigrams (as uint32_t)
typedef std::vector<uint64_t> TrigramList;

struct LanguageProfile
{
    std::string languageCode;
    TrigramProfile trigramProfile;
};

typedef std::vector<LanguageProfile> LanguageProfiles;

// Helper functions for uint32_t trigrams
uint64_t wcharTrigramToInt(const wchar_t* data);
std::wstring intToWcharTrigram(uint64_t trigram);
uint64_t stringTrigramToInt(const std::string& trigram);

// Functions
TrigramProfile buildTrigramProfile(const Text& text);
void normalizeTrigramProfile(TrigramProfile& trigramProfile);
float getCosineSimilarity(const TrigramProfile& textProfile, const TrigramProfile& language);
std::string identifyLanguage(const Text& text, const LanguageProfiles& languages);

#endif
