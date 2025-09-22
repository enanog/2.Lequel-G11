/**
 * @brief Lequel? language identification based on trigrams (Unicode Optimized)
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
    // Thread-local converter for UTF-8 to wide string conversion - avoids recreating converter objects
    thread_local std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    /**
     * @brief Extracts trigrams from a single line of text and adds them to the profile.
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
            // Unicode conversion for proper character handling - essential for non-Latin scripts
            wstring wline = converter.from_bytes(cleanLine);

			cout << "Processing line wline" << endl;
            if (wline.length() < 3) return;

            // Extract trigrams as uint64_t - much faster than string comparison and hashing
            for (size_t i = 0; i <= wline.length() - 3; ++i) {
                uint64_t trigram = wcharTrigramToInt(wline.data() + i);

                // Skip trigrams with null characters - they don't provide meaningful language information
                if (trigram != 0 &&
                   (trigram & 0xFFFF) != 0 &&
                   ((trigram >> 16) & 0xFFFF) != 0 &&
                   ((trigram >> 32) & 0xFFFF) != 0) {
                    ++trigrams[trigram];
                }
            }
        }
        catch (const std::exception&) {
            // Fallback to byte-by-byte processing on conversion error - handles corrupted UTF-8
			cout << "Processing line exception" << endl;
            if (cleanLine.length() >= 3) {
                for (size_t i = 0; i <= cleanLine.length() - 3; ++i) {
                    // Pack bytes as if they were wchar_t for consistency with Unicode processing
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
     * @brief Calculates the Euclidean norm of a trigram profile.
     * @param profile The trigram profile
     * @return float The calculated norm
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
 * @brief Builds a trigram profile from a given text.
 * @param text Vector of lines (Text)
 * @return TrigramProfile The trigram profile
 */
TrigramProfile buildTrigramProfile(const Text& text) {
    if (text.empty())
        return {};

    TrigramProfile trigrams;

    // Pre-reserve space for Unicode trigrams - prevents multiple memory reallocations
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
        // Normalize frequencies to unit vector - enables cosine similarity calculation
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

    // Iterate over the smaller profile for better performance - O(min(n,m)) instead of O(max(n,m))
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

	cout << "Building trigram profile for input text..." << endl;

    TrigramProfile textTrigrams = buildTrigramProfile(text);
    if (textTrigrams.empty()) {
        return "unknown";
    }

	cout << "Normalizing trigram profile..." << endl;

    normalizeTrigramProfile(textTrigrams);

    // Calculate total trigrams for information
    size_t totalTrigrams = 0;
    for (const auto& [key, value] : textTrigrams) {
        totalTrigrams += static_cast<size_t>(value);
    }

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

    if (isConfidentMatch) {
        cout << "Best match: " << *bestLanguageCode
            << " (similarity: " << maxSimilarity << ")" << endl;
        return *bestLanguageCode;
    }
    else {
        cout << "No confident match found (best similarity: "
            << maxSimilarity << ")" << endl;
        return "unknown";
    }
}

// Global helper function implementations
uint64_t wcharTrigramToInt(const wchar_t* data) {
    // Pack three wide characters into a single 64-bit integer for fast comparison
    return  (uint64_t(data[0]) << 32) |
            (uint64_t(data[1]) << 16) |
             uint64_t(data[2]);
}

std::wstring intToWcharTrigram(uint64_t trigram) {
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
        // Fallback for invalid UTF-8 - treats bytes as characters
        if (trigram.length() >= 3) {
            return (uint64_t(static_cast<unsigned char>(trigram[0])) << 32) |
                   (uint64_t(static_cast<unsigned char>(trigram[1])) << 16) |
                    uint64_t(static_cast<unsigned char>(trigram[2]));
        }
    }
    return 0;
}