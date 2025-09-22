/**
 * @brief Lequel? main module (MODIFICACIONES PARA uint64_t Unicode)
 * @author Dylan Frigerio, Micaela Dinsen, Marc S. Ressl
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

enum class AppState
{
    WAITING,
    PROCESSING,
    RESULT_READY
};

/**
 * @brief Loads trigram data.
 *
 * @param languageCodeNames Map of language code vs. language name (in i18n locale).
 * @param languages The trigram profiles.
 * @return true Succeeded
 * @return false Failed
 */
bool loadLanguagesData(map<string, string>& languageCodeNames, LanguageProfiles& languages)
{
    cout << "Reading language codes..." << endl;

    CSVData languageCodesCSVData;
    if (!readCSV(LANGUAGECODE_NAMES_FILE, languageCodesCSVData))
        return false;

    // Load trigram profile for each language code
    for (auto& fields : languageCodesCSVData)
    {
        if (fields.size() != 2)
            continue;

        string languageCode = fields[0];
        string languageName = fields[1];

        languageCodeNames[languageCode] = languageName;

        cout << "Reading trigram profile for language code \"" << languageCode << "\"..." << endl;

        CSVData languageCSVData;
        if (!readCSV(TRIGRAMS_PATH + languageCode + ".csv", languageCSVData))
            return false;

        languages.push_back(LanguageProfile());
        LanguageProfile& language = languages.back();

        language.languageCode = languageCode;

        // CAMBIO PRINCIPAL: Convertir trigramas string a uint64_t con soporte Unicode
        for (auto& fields : languageCSVData)
        {
            if (fields.size() != 2)
                continue;

            string trigramString = fields[0];
            float frequency = (float)stoi(fields[1]);

            // Convertir el trigrama de string a uint64_t (con soporte Unicode)
            uint64_t trigramInt = stringTrigramToInt(trigramString);

            // Solo agregar trigramas válidos (no null)
            if (trigramInt != 0) {
                language.trigramProfile[trigramInt] = frequency;
            }
        }

        normalizeTrigramProfile(language.trigramProfile);

        cout << "Loaded " << language.trigramProfile.size() << " trigrams for " << languageCode << endl;
    }

    cout << "Loaded " << languages.size() << " language profiles total." << endl;
    return true;
}

int main(int, char* [])
{
    map<string, string> languageCodeNames;
    LanguageProfiles languages;

    if (!loadLanguagesData(languageCodeNames, languages))
    {
        cout << "Could not load trigram data." << endl;
        return 1;
    }

    int screenWidth = 800;
    int screenHeight = 450;
    InitWindow(screenWidth, screenHeight, "Lequel?");

    SetTargetFPS(60);

    // Application state variables
    AppState currentState = AppState::WAITING;
    string languageCode = "---";
    double processingTimeMs = 0.0;
    high_resolution_clock::time_point startTime;
    bool processing = false;
    bool loadedText = false;
    string pendingClipboard = "";
    string pendingFilePath = "";
    bool isFromFile = false;

    while (!WindowShouldClose())
    {
        // Handle clipboard input (Ctrl+V or Cmd+V)
        if (IsKeyPressed(KEY_V) &&
            (IsKeyDown(KEY_LEFT_CONTROL) ||
                IsKeyDown(KEY_RIGHT_CONTROL) ||
                IsKeyDown(KEY_LEFT_SUPER) ||
                IsKeyDown(KEY_RIGHT_SUPER)))
        {
            processing = true;
            currentState = AppState::PROCESSING;
            loadedText = true;
            pendingClipboard = GetClipboardText();
            isFromFile = false;
        }

        // Handle dropped files
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

        // Process text after showing "Processing..." message
        if (loadedText && !processing && currentState == AppState::PROCESSING)
        {
            startTime = high_resolution_clock::now();

            Text text;
            bool success = false;

            if (isFromFile)
            {
                success = getTextFromFile(pendingFilePath, text);
                pendingFilePath = "";
                if (success) {
                    cout << "Processing file with " << text.size() << " lines..." << endl;
                }
            }
            else
            {
                success = getTextFromString(pendingClipboard, text);
                pendingClipboard = "";
                if (success) {
                    cout << "Processing clipboard text with " << text.size() << " lines..." << endl;
                }
            }

            if (success)
            {
                languageCode = identifyLanguage(text, languages);
                cout << "Identified language: " << languageCode << endl;
            }
            else
            {
                languageCode = "error";
                cout << "Error processing text." << endl;
            }

            auto endTime = high_resolution_clock::now();
            auto duration = duration_cast<microseconds>(endTime - startTime);
            processingTimeMs = duration.count() / 1000.0;

            cout << "Processing completed in " << processingTimeMs << " ms" << endl;

            currentState = AppState::RESULT_READY;
        }

        BeginDrawing();
        ClearBackground(BEIGE);

        DrawText("Lequel?", 80, 80, 128, BROWN);
        DrawText("Copy and paste with Ctrl+V, or drag a file...", 80, 220, 24, BROWN);

        switch (currentState)
        {
        case AppState::WAITING:
            break;

        case AppState::PROCESSING:
        {
            string processingText = "Processing...";
            int processingWidth = MeasureText(processingText.c_str(), 48);
            DrawText(processingText.c_str(), (screenWidth - processingWidth) / 2, 315, 48, DARKBROWN);
            processing = false;
            break;
        }

        case AppState::RESULT_READY:
        {
            string languageString = "";
            if (languageCode == "error")
            {
                languageString = "Processing error";
            }
            else if (languageCode != "---")
            {
                if (languageCodeNames.find(languageCode) != languageCodeNames.end())
                    languageString = languageCodeNames[languageCode];
                else
                    languageString = "Unknown";
            }

            if (!languageString.empty())
            {
                int languageStringWidth = MeasureText(languageString.c_str(), 48);
                DrawText(languageString.c_str(), (screenWidth - languageStringWidth) / 2, 315, 48, DARKBROWN);

                // Display processing time
                string timeText;
                if (processingTimeMs < 1000)
                {
                    timeText = "Processing time: " + to_string(processingTimeMs) + " ms";
                }
                else
                {
                    timeText = "Processing time: " + to_string(processingTimeMs / 1000.0) + " s";
                }
                int timeWidth = MeasureText(timeText.c_str(), 20);
                DrawText(timeText.c_str(), (screenWidth - timeWidth) / 2, 375, 20, DARKBROWN);
            }

            loadedText = false;
            break;
        }
        }

        // Reset state for new input
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