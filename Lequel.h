/**
 * @brief Lequel? language identification based on trigrams
 * @author Marc S. Ressl
 * @modified Dylan Frigerio, Micaela Dinsen
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

 // TrigramProfile: maps packed Unicode trigram (uint64_t) to its normalized frequency
typedef std::unordered_map<uint64_t, float> TrigramProfile;

// TrigramList: holds a sequence of trigrams, stored as 64-bit integers
typedef std::vector<uint64_t> TrigramList;

// Stores a language code (ISO string) and its corresponding trigram profile
struct LanguageProfile
{
    std::string languageCode;
    TrigramProfile trigramProfile;
};

typedef std::vector<LanguageProfile> LanguageProfiles;

// --- Helper functions ---
// Packs three wide characters into a single 64-bit integer
uint64_t wcharTrigramToInt(const wchar_t* data);

// Converts a packed trigram back into a wstring (3 UTF-16 characters)
std::wstring intToWcharTrigram(uint64_t trigram);

// Converts a UTF-8 trigram string to a packed uint64_t representation
uint64_t stringTrigramToInt(const std::string& trigram);

// Functions
TrigramProfile buildTrigramProfile(const Text& text);
void normalizeTrigramProfile(TrigramProfile& trigramProfile);
float getCosineSimilarity(const TrigramProfile& textProfile, const TrigramProfile& language);
std::string identifyLanguage(const Text& text, const LanguageProfiles& languages);

#endif
