/**
 * @brief Lequel? main module (optimized for uint64_t Unicode trigrams)
 * @author Marc S. Ressl
 * @modified Dylan Frigerio, Micaela Dinsen
 * 
 * @copyright Copyright (c) 2022-2023
 */

#include <iostream>
#include <map>
#include <string>
#include <chrono>

#include "raylib.h"
#include "CSVData.h"
#include "Lequel.h"

using namespace std;
using namespace std::chrono;

const string LANGUAGECODE_NAMES_FILE = "resources/languagecode_names_es.csv";
const string TRIGRAMS_PATH = "resources/trigrams/";

enum class AppState { WAITING, PROCESSING, RESULT_READY };

/**
 * @brief Loads all language profiles from CSV files.
 * @param languageCodeNames Map of ISO code → language name
 * @param languages Output vector of language profiles
 */
bool loadLanguagesData(map<string, string>& languageCodeNames, LanguageProfiles& languages)
{
    CSVData languageCodesCSVData;
    if (!readCSV(LANGUAGECODE_NAMES_FILE, languageCodesCSVData))
        return false;

    // Iterate through CSV rows (code, name)
    for (auto& fields : languageCodesCSVData)
    {
        if (fields.size() != 2)
            continue;

        string languageCode = fields[0];
        string languageName = fields[1];
        languageCodeNames[languageCode] = languageName;

        CSVData languageCSVData;
        if (!readCSV(TRIGRAMS_PATH + languageCode + ".csv", languageCSVData))
            return false;

        languages.push_back(LanguageProfile());
        LanguageProfile& language = languages.back();
        language.languageCode = languageCode;

        // Convert each trigram string to uint64_t
        for (auto& fields : languageCSVData)
        {
            if (fields.size() != 2)
                continue;

            string trigramString = fields[0];
            float frequency = (float)stoi(fields[1]);

            uint64_t trigramInt = stringTrigramToInt(trigramString);
            if (trigramInt != 0) {
                language.trigramProfile[trigramInt] = frequency;
            }
        }

        normalizeTrigramProfile(language.trigramProfile);
    }
    return true;
}

int main(int, char* [])
{
    map<string, string> languageCodeNames;
    LanguageProfiles languages;

    if (!loadLanguagesData(languageCodeNames, languages))
        return 1;

    int screenWidth = 800, screenHeight = 450;
    InitWindow(screenWidth, screenHeight, "Lequel?");
    SetTargetFPS(60);

    AppState currentState = AppState::WAITING;
    string languageCode = "---";
    double processingTimeMs = 0.0;
    high_resolution_clock::time_point startTime;
    bool processing = false;
    bool loadedText = false;
    string pendingClipboard, pendingFilePath;
    bool isFromFile = false;

    while (!WindowShouldClose())
    {
        // Handle clipboard paste
        if (IsKeyPressed(KEY_V) &&
            (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) ||
                IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER)))
        {
            processing = true;
            currentState = AppState::PROCESSING;
            loadedText = true;
            pendingClipboard = GetClipboardText();
            isFromFile = false;
        }

        // Handle file drag & drop
        if (IsFileDropped())
        {
            FilePathList droppedFiles = LoadDroppedFiles();
            if (droppedFiles.count == 1)
            {
                processing = true;
                currentState = AppState::PROCESSING;
                loadedText = true;
                pendingFilePath = droppedFiles.paths[0];
                isFromFile = true;
            }
            UnloadDroppedFiles(droppedFiles);
        }

        // Once input is loaded, process text
        if (loadedText && !processing && currentState == AppState::PROCESSING)
        {
            startTime = high_resolution_clock::now();

            Text text;
            bool success = false;

            if (isFromFile) {
                success = getTextFromFile(pendingFilePath, text);
                pendingFilePath.clear();
            }
            else {
                success = getTextFromString(pendingClipboard, text);
                pendingClipboard.clear();
            }

            if (success) {
                languageCode = identifyLanguage(text, languages);
            }
            else {
                languageCode = "error";
            }

            auto endTime = high_resolution_clock::now();
            auto duration = duration_cast<microseconds>(endTime - startTime);
            processingTimeMs = duration.count() / 1000.0;

            currentState = AppState::RESULT_READY;
        }

        // --- Rendering ---
        BeginDrawing();
        ClearBackground(BEIGE);

        DrawText("Lequel?", 80, 80, 128, BROWN);
        DrawText("Copy and paste with Ctrl+V, or drag a file...", 80, 220, 24, BROWN);

        switch (currentState)
        {
        case AppState::WAITING:
            break;

        case AppState::PROCESSING:
            DrawText("Processing...", (screenWidth - MeasureText("Processing...", 48)) / 2, 315, 48, DARKBROWN);
            processing = false;
            break;

        case AppState::RESULT_READY:
        {
            string languageString;
            if (languageCode == "error") {
                languageString = "Processing error";
            }
            else if (languageCode != "---") {
                if (languageCodeNames.find(languageCode) != languageCodeNames.end())
                    languageString = languageCodeNames[languageCode];
                else
                    languageString = "Unknown";
            }

            if (!languageString.empty()) {
                DrawText(languageString.c_str(),
                    (screenWidth - MeasureText(languageString.c_str(), 48)) / 2,
                    315, 48, DARKBROWN);

                // Show processing time
                string timeText = (processingTimeMs < 1000)
                    ? "Processing time: " + to_string(processingTimeMs) + " ms"
                    : "Processing time: " + to_string(processingTimeMs / 1000.0) + " s";

                DrawText(timeText.c_str(),
                    (screenWidth - MeasureText(timeText.c_str(), 20)) / 2,
                    375, 20, DARKBROWN);
            }

            loadedText = false;
            break;
        }
        }

        // Reset state on new input
        if (currentState == AppState::RESULT_READY &&
            (IsKeyPressed(KEY_V) || IsFileDropped() || IsKeyPressed(KEY_SPACE)))
        {
            currentState = AppState::WAITING;
            languageCode = "---";
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
