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

            // Pre-reserva de memoria del mapa:
            // En lugar de hacer rehash en cada línea, lo hacemos una vez en buildTrigramProfile.
            // Esto evita cálculos repetitivos dentro de este bucle.

            // Uso de wstring_view para evitar crear strings:
            // Extraer trigramas de caracteres Unicode
            for (size_t i = 0; i <= wline.length() - 3; ++i) {
                // Creamos una "vista" del trigrama. Esto es casi instantáneo y no asigna nueva memoria.
                std::wstring_view wtrigram_view(wline.data() + i, 3);

                // Convertimos a string solo en el último momento, para usarlo como clave.
                // Esto reduce a la mitad las asignaciones de memoria dentro del bucle.
                const string trigram = converter.to_bytes(std::wstring(wtrigram_view));
                ++trigrams[trigram];
            }
        }
        catch (const std::exception&) {
            // En caso de error de conversión, fallback a procesamiento byte por byte
            // Uso de string_view en el fallback:
            for (size_t i = 0; i <= cleanLine.length() - 3; ++i) {
                // Aquí también usamos una vista para ser más eficientes.
                std::string_view trigram_view(cleanLine.data() + i, 3);
                // Creamos el string solo al final, para usarlo como clave del mapa.
                ++trigrams[std::string(trigram_view)];
            }
        }
    }

    // Función optimizada para calcular la norma
    inline float calculateNorm(const TrigramProfile& profile) noexcept {
        float norm_sq = 0.0f; // 
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

    // Reserva de memoria centralizada:
    // Pre-reservamos memoria para el mapa una sola vez al inicio.
    // Esto evita que el mapa tenga que redimensionarse múltiples veces, mejorando el rendimiento.
    // 80000 es una estimación generosa y segura para la cantidad de trigramas únicos.
    trigrams.reserve(80000);

    // Implementamos un límite en la cantidad de líneas procesadas para mejorar el rendimiento
    const size_t MAX_LINES_TO_PROCESS = 10000;
    size_t linesProcessed = 0;

    for (const auto& line : text) {
        if (linesProcessed >= MAX_LINES_TO_PROCESS) {
            break; // Salimos del bucle al llegar al límite.
        }
        if (!line.empty()) { // Añadimos una comprobación para no procesar líneas vacías.
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
        // La multiplicación es más rápida que la división.
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

    normalizeTrigramProfile(textTrigrams);

    float maxSimilarity = -1.0f;
    
    // Guardar un puntero al string del código de idioma es ligeramente más directo
    // que guardar un índice y luego volver a acceder al vector.
    const string* bestLanguageCode = nullptr;

    for (const auto& langProfile : languages) {
        const float similarity = getCosineSimilarity(textTrigrams, langProfile.trigramProfile);
        if (similarity > maxSimilarity) {
            maxSimilarity = similarity;
            bestLanguageCode = &langProfile.languageCode;
        }
    }

    // Un umbral de confianza es importante para evitar falsos positivos.
    const float SIMILARITY_THRESHOLD = 0.01f;
    if (maxSimilarity > SIMILARITY_THRESHOLD && bestLanguageCode != nullptr) {
        return *bestLanguageCode;
    } else {
        return "unknown";
    }
}
