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

// TrigramProfile: unorderedmap of trigram -> frequency
typedef std::unordered_map<std::string, float> TrigramProfile;

// TrigramList: vector of trigrams
typedef std::vector<std::string> TrigramList;

struct LanguageProfile
{
    std::string languageCode;
    TrigramProfile trigramProfile;
};

typedef std::vector<LanguageProfile> LanguageProfiles;

// Functions
TrigramProfile buildTrigramProfile(const Text &text);
void normalizeTrigramProfile(TrigramProfile &trigramProfile);
float getCosineSimilarity(const TrigramProfile &textProfile, const TrigramProfile& language);
std::string identifyLanguage(const Text &text, const LanguageProfiles &languages);

#endif
