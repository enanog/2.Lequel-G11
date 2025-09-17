/**
 * @brief Lequel? language identification based on trigrams (Optimized)
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
#include <algorithm>
#include <string_view>
#include "Lequel.h"

using namespace std;

namespace {
    // Cache para evitar recrear el converter constantemente
    thread_local std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    inline void extractTrigramsFromLine(const string& line, TrigramProfile& trigrams) {
        if (line.length() < 3) return;

        // Crear string para manipulación (evitamos modificar string_view)
        string cleanLine(line);

        // Remover caracteres de fin de línea más eficientemente
        while (!cleanLine.empty() && (cleanLine.back() == '\r' || cleanLine.back() == '\n')) {
            cleanLine.pop_back();
        }

        if (cleanLine.length() < 3) return;

        try {
            // Conversión a wstring para manejo correcto de caracteres Unicode
            wstring wline = converter.from_bytes(cleanLine);

            if (wline.length() < 3) return;

            // Reservar espacio aproximado para evitar realocaciones
            const size_t expectedTrigrams = wline.length() - 2;
            if (trigrams.bucket_count() < expectedTrigrams) {
                trigrams.rehash(expectedTrigrams * 1.5);
            }

            wstring wtrigram;
            wtrigram.reserve(3);

            // Extraer trigramas de caracteres Unicode
            for (size_t i = 0; i <= wline.length() - 3; ++i) {
                wtrigram.clear();
                wtrigram.push_back(wline[i]);
                wtrigram.push_back(wline[i + 1]);
                wtrigram.push_back(wline[i + 2]);

                const string trigram = converter.to_bytes(wtrigram);
                ++trigrams[trigram];
            }
        }
        catch (const std::exception&) {
            string trigram;
            trigram.reserve(3);
            // En caso de error de conversión, fallback a procesamiento byte por byte
            for (size_t i = 0; i <= cleanLine.length() - 3; ++i) {
                trigram.clear();
                trigram.push_back(cleanLine[i]);
                trigram.push_back(cleanLine[i + 1]);
                trigram.push_back(cleanLine[i + 2]);

                ++trigrams[trigram];
            }
        }
    }

    // Función optimizada para calcular la norma
    inline float calculateNorm(const TrigramProfile& profile) noexcept {
        float norm = 0.0f;
        for (const auto& pair : profile) {
            norm += pair.second * pair.second;
        }
        return std::sqrt(norm);
    }
}

/**
 * @brief Builds a trigram profile from a given text.
 *
 * @param text Vector of lines (Text)
 * @return TrigramProfile The trigram profile
 */
TrigramProfile buildTrigramProfile(const Text& text) {
    if (text.empty()) return {};

    TrigramProfile trigrams;

    // Estimación más precisa del tamaño del mapa
    size_t totalChars = 0;
    for (const auto& line : text) {
        totalChars += line.length();
    }

    // Reservar espacio basado en una estimación más realista
    size_t estimatedTrigrams = std::min(totalChars * 0.8, 100000.0);
    trigrams.reserve(estimatedTrigrams);

    // Procesar líneas
    for (const auto& line : text) {
        if (!line.empty()) {
            extractTrigramsFromLine(line, trigrams);
        }
    }

    return trigrams;
}

/**
 * @brief Normalizes a trigram profile.
 *
 * @param trigramProfile The trigram profile.
 */
void normalizeTrigramProfile(TrigramProfile& trigramProfile) {
    if (trigramProfile.empty()) return;

    if (trigramProfile.size() == 1) {
        trigramProfile.begin()->second = 1.0f;
        return;
    }

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
float getCosineSimilarity(const TrigramProfile& textProfile, const TrigramProfile& language) {
    if (textProfile.empty() || language.empty()) return 0.0f;

    float dotProduct = 0.0f;

    // Iterar sobre el perfil más pequeño para mejor rendimiento
    const auto& profiles = (textProfile.size() < language.size())
        ? std::make_pair(std::cref(textProfile), std::cref(language))
        : std::make_pair(std::cref(language), std::cref(textProfile));

    for (const auto& pair : profiles.first) {
		auto it = profiles.second.find(pair.first);
        if (it != profiles.second.end()) {
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

    // Construir y normalizar el perfil del texto
    TrigramProfile textTrigrams = buildTrigramProfile(text);
    normalizeTrigramProfile(textTrigrams);

    // Encontrar la mejor coincidencia usando algoritmo más eficiente
    float maxSimilarity = -1.0f;
    size_t bestIndex = 0;

    for (size_t i = 0; i < languages.size(); ++i) {
        const float similarity = getCosineSimilarity(textTrigrams, languages[i].trigramProfile);

        if (similarity > maxSimilarity) {
            maxSimilarity = similarity;
            bestIndex = i;
        }
    }

    return (maxSimilarity > 0.0f) ? languages[bestIndex].languageCode : "unknown";
}