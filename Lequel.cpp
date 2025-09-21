/**
 * @brief Lequel? language identification based on trigrams (Optimized)
 * @author Dylan Frigerio, Micaela Dinsen, Marc S. Ressl
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
#include <string_view>
#include "Lequel.h"

using namespace std;

namespace {
    // Thread-local converter to avoid constant recreation
    thread_local std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    /**
     * @brief Extracts trigrams from a single line of text and adds them to the profile.
     *
     * @param line The input text line to process
     * @param trigrams Reference to the trigram profile to update
     */
    inline void extractTrigramsFromLine(const string& line, TrigramProfile& trigrams) {
        if (line.length() < 3) return;

        string cleanLine(line);

        // Remove line ending characters
        while (!cleanLine.empty() && (cleanLine.back() == '\r' || cleanLine.back() == '\n')) {
            cleanLine.pop_back();
        }

        if (cleanLine.length() < 3) return;

        try {
            // Unicode conversion for proper character handling
            wstring wline = converter.from_bytes(cleanLine);

            if (wline.length() < 3) return;

            for (size_t i = 0; i <= wline.length() - 3; ++i) {
                std::wstring_view wtrigram_view(wline.data() + i, 3);
                const string trigram = converter.to_bytes(std::wstring(wtrigram_view));
                ++trigrams[trigram];
            }
        }
        catch (const std::exception&) {
            // Fallback to byte-by-byte processing on conversion error
            for (size_t i = 0; i <= cleanLine.length() - 3; ++i) {
                std::string_view trigram_view(cleanLine.data() + i, 3);
                ++trigrams[std::string(trigram_view)];
            }
        }
    }

    /**
     * @brief Calculates the Euclidean norm of a trigram profile.
     *
     * @param profile The trigram profile
     * @return float The calculated norm
     */
    inline float calculateNorm(const TrigramProfile& profile) noexcept {
        float norm_sq = 0.0f;
        for (const auto& pair : profile) {
            norm_sq += pair.second * pair.second;
        }
        return std::sqrt(norm_sq);
    }
}

/**
 * @brief Builds a trigram profile from a given text.
 *
 * @param text Vector of lines (Text)
 * @return TrigramProfile The trigram profile
 */
TrigramProfile buildTrigramProfile(const Text& text) {
    if (text.empty())
        return {};

    TrigramProfile trigrams;

    // Pre-reserve memory to avoid multiple rehashing operations
    trigrams.reserve(80000);

    // Limit processed lines for performance optimization
    const size_t MAX_LINES_TO_PROCESS = 10000;
    size_t linesProcessed = 0;

    for (const auto& line : text) {
        if (linesProcessed >= MAX_LINES_TO_PROCESS) {
            break;
        }
        if (!line.empty()) {
            extractTrigramsFromLine(line, trigrams);
        }
        linesProcessed++;
    }

    return trigrams;
}

/**
 * @brief Normalizes a trigram profile.
 *
 * @param trigramProfile The trigram profile.
 */
void normalizeTrigramProfile(TrigramProfile& trigramProfile) {
    if (trigramProfile.empty())
        return;
    const float norm = calculateNorm(trigramProfile);
    if (norm > 0.0f) {
        const float invNorm = 1.0f / norm;
        for (auto& pair : trigramProfile) {
            pair.second *= invNorm;
        }
    }
}

/**
 * @brief Calculates the cosine similarity between two trigram profiles
 *
 * @param textProfile The text trigram profile
 * @param languageProfile The language trigram profile
 * @return float The cosine similarity score
 */
float getCosineSimilarity(const TrigramProfile& textProfile, const TrigramProfile& languageProfile) {
    if (textProfile.empty() || languageProfile.empty())
        return 0.0f;

    float dotProduct = 0.0f;

    // Iterate over the smaller profile for better performance
    const auto& smallerProfile = textProfile.size() < languageProfile.size() ? textProfile : languageProfile;
    const auto& largerProfile = textProfile.size() < languageProfile.size() ? languageProfile : textProfile;

    for (const auto& pair : smallerProfile) {
        auto it = largerProfile.find(pair.first);
        if (it != largerProfile.end()) {
            dotProduct += pair.second * it->second;
        }
    }

    return dotProduct;
}

/**
 * @brief Identifies the language of a text.
 *
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

    size_t totalTrigrams = 0;
    for (const auto& pair : textTrigrams) {
        totalTrigrams += pair.second;
    }

    float maxSimilarity = -1.0f;
    float secondMaxSimilarity = -1.0f;
    const string* bestLanguageCode = nullptr;

    for (const auto& langProfile : languages) {
        const float similarity = getCosineSimilarity(textTrigrams, langProfile.trigramProfile);

        if (similarity > maxSimilarity) {
            secondMaxSimilarity = maxSimilarity;
            maxSimilarity = similarity;
            bestLanguageCode = &langProfile.languageCode;
        }
        else if (similarity > secondMaxSimilarity) {
            secondMaxSimilarity = similarity;
        }
    }

    const float SIMILARITY_THRESHOLD = 0.05f;
    // Confidence margin - the best match should be significantly better than the second best
    const float CONFIDENCE_MARGIN = 0.02f;

    bool isConfidentMatch = maxSimilarity > SIMILARITY_THRESHOLD &&
        bestLanguageCode != nullptr &&
        (maxSimilarity - secondMaxSimilarity) > CONFIDENCE_MARGIN;

    if (isConfidentMatch) {
        return *bestLanguageCode;
    }
    else {
        return "unknown";
    }
}