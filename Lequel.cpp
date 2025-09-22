/**
 * @brief Lequel? language identification based on trigrams (Unicode Optimized)
 * @author Marc S. Ressl
 * @modified Dylan Frigerio, Micaela Dinsen
 * 
 * @copyright Copyright (c) 2022-2023
 *
 * @cite https://towardsdatascience.com/understanding-cosine-similarity-and-its-application-fd42f585296a
 */

#include <cmath>
#include <codecvt>
#include <locale>
#include <iostream>
#include <algorithm>
#include "Lequel.h"

using namespace std;

namespace {
    // Thread-local UTF-8 to UTF-16 converter (avoids recreating converter objects per call)
    thread_local std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    /**
     * @brief Extracts and counts trigrams from a single line of UTF-8 text.
     * @param line Input string (UTF-8 encoded)
     * @param trigrams Destination trigram profile
     */
    inline void extractTrigramsFromLine(const string& line, TrigramProfile& trigrams) {
        if (line.length() < 3) return;

        string cleanLine(line);

        // Remove trailing newline characters if present
        while (!cleanLine.empty() && (cleanLine.back() == '\r' || cleanLine.back() == '\n')) {
            cleanLine.pop_back();
        }

        if (cleanLine.length() < 3) return;

        try {
            // Convert to UTF-16 to correctly handle Unicode characters
            wstring wline = converter.from_bytes(cleanLine);

            if (wline.length() < 3) return;

            // Slide a 3-character window across the line
            for (size_t i = 0; i <= wline.length() - 3; ++i) {
                uint64_t trigram = wcharTrigramToInt(wline.data() + i);

                // Ignore invalid/null trigrams
                if (trigram != 0 &&
                   (trigram & 0xFFFF) != 0 &&
                   ((trigram >> 16) & 0xFFFF) != 0 &&
                   ((trigram >> 32) & 0xFFFF) != 0) {
                    ++trigrams[trigram];
                }
            }
        }
        catch (const std::exception&) {
            // Fallback: byte-based trigram extraction for corrupted UTF-8 sequences
            if (cleanLine.length() >= 3) {
                for (size_t i = 0; i <= cleanLine.length() - 3; ++i) {
                    uint64_t trigram = (uint64_t(static_cast<unsigned char>(cleanLine[i])) << 32) |
                                       (uint64_t(static_cast<unsigned char>(cleanLine[i + 1])) << 16) |
                                        uint64_t(static_cast<unsigned char>(cleanLine[i + 2]));

                    if (trigram != 0) {
                        ++trigrams[trigram];
                    }
                }
            }
        }
    }

    /**
     * @brief Computes Euclidean norm of a trigram profile (for normalization).
     */
    inline float calculateNorm(const TrigramProfile& profile) noexcept {
        float norm_sq = 0.0f;
        for (const auto& [key, value] : profile) {
            norm_sq += value * value;
        }
        return sqrt(norm_sq);
    }
}

/**
 * @brief Builds a trigram profile from full text.
 * @param text Vector of text lines
 */
TrigramProfile buildTrigramProfile(const Text& text) {
    if (text.empty())
        return {};

    TrigramProfile trigrams;

    // Preallocate to reduce reallocations
    trigrams.reserve(200000);

    size_t totalChars = 0;

    for (const auto& line : text) {
        if (line.length() >= 3) {
            extractTrigramsFromLine(line, trigrams);
        }
    }

    return trigrams;
}

/**
 * @brief Normalizes a trigram profile.
 * @param trigramProfile The trigram profile.
 */
void normalizeTrigramProfile(TrigramProfile& trigramProfile) {
    if (trigramProfile.empty())
        return;

    const float norm = calculateNorm(trigramProfile);
    if (norm > 0.0f) {
        const float invNorm = 1.0f / norm;
        for (auto& [key, value] : trigramProfile) {
            value *= invNorm;
        }
    }
}

/**
 * @brief Calculates the cosine similarity between two trigram profiles
 * @param textProfile The text trigram profile
 * @param languageProfile The language trigram profile
 * @return float The cosine similarity score
 */
float getCosineSimilarity(const TrigramProfile& textProfile, const TrigramProfile& languageProfile) {
    if (textProfile.empty() || languageProfile.empty())
        return 0.0f;

    float dotProduct = 0.0f;

    // Iterate over the smaller profile for efficiency
    const auto& smallerProfile = textProfile.size() < languageProfile.size() ? textProfile : languageProfile;
    const auto& largerProfile = textProfile.size() < languageProfile.size() ? languageProfile : textProfile;

    for (const auto& [key, value] : smallerProfile) {
        auto it = largerProfile.find(key);
        if (it != largerProfile.end()) {
            dotProduct += value * it->second;
        }
    }

    return dotProduct;
}

/**
 * @brief Identifies the language of a text.
 * @param text A Text (vector of lines)
 * @param languages A list of Language objects
 * @return string The language code of the most likely language
 */
string identifyLanguage(const Text& text, const LanguageProfiles& languages) {
    if (text.empty() || languages.empty()) {
        return "unknown";
    }

    TrigramProfile textTrigrams = buildTrigramProfile(text);
    if (textTrigrams.empty()) {
        return "unknown";
    }

    normalizeTrigramProfile(textTrigrams);

    float maxSimilarity = -1.0f;
    const string* bestLanguageCode = nullptr;

    for (const auto& langProfile : languages) {
        const float similarity = getCosineSimilarity(textTrigrams, langProfile.trigramProfile);

        if (similarity > maxSimilarity) {
            maxSimilarity = similarity;
            bestLanguageCode = &langProfile.languageCode;
        }
    }
	const float similarityThreshold = 0.01f; // Minimum similarity to consider a match
    bool isConfidentMatch = (maxSimilarity > similarityThreshold) &&
                            bestLanguageCode != nullptr;

    if (maxSimilarity > similarityThreshold && bestLanguageCode != nullptr) {
        return *bestLanguageCode;
    }
    return "unknown";
}

// --- Helper functions ---

uint64_t wcharTrigramToInt(const wchar_t* data) {
    // Packs three UTF-16 code units into one 64-bit integer
    return  (uint64_t(data[0]) << 32) |
            (uint64_t(data[1]) << 16) |
             uint64_t(data[2]);
}

std::wstring intToWcharTrigram(uint64_t trigram) {
    // Unpacks a 64-bit integer back into a 3-character wide string
    wstring result(3, L'\0');
    result[0] = static_cast<wchar_t>((trigram >> 32) & 0xFFFF);
    result[1] = static_cast<wchar_t>((trigram >> 16) & 0xFFFF);
    result[2] = static_cast<wchar_t>(trigram & 0xFFFF);
    return result;
}

uint64_t stringTrigramToInt(const std::string& trigram) {
    if (trigram.empty()) return 0;

    try {
        thread_local std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
        wstring wtrigram = conv.from_bytes(trigram);
        if (wtrigram.length() >= 3) {
            return wcharTrigramToInt(wtrigram.data());
        }
    }
    catch (const std::exception&) {
        // Fallback: interpret bytes as characters if UTF-8 decoding fails
        if (trigram.length() >= 3) {
            return (uint64_t(static_cast<unsigned char>(trigram[0])) << 32) |
                (uint64_t(static_cast<unsigned char>(trigram[1])) << 16) |
                uint64_t(static_cast<unsigned char>(trigram[2]));
        }
    }
    return 0;
}