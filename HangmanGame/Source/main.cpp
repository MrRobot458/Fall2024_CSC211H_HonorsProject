/*
Name: Emmanuel Rivas
ID: 15310887
Class: Fall 2024, CSC 211H
Date: 02/01/2025
Instructor: Dr. Azhar
Honors Project: Hangman
*/

#include "Database.h"
#include "Hangman.h"
#include "Display.h"
#include <iostream>
#include <filesystem>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

// Anonymous namespace for encapsulation
namespace {
    // Constants
    const std::string DB_NAME = "Hangman_DB.db";
    const std::string DB_FOLDER = "Data/SQLite_DB";
    const std::string RESOURCES_FOLDER = "Data/Resources";
    const std::vector<std::string> CATEGORY_FILES = {
        "CSC_111.tsv",
        "CSC_211.tsv",
        "CSC_231.tsv"
    };

    // Helper functions
    bool ensureDirectoryExists(const std::string& path) {
        bool directoryExists = false;

        if (std::filesystem::exists(path) == true) directoryExists = true;

        return directoryExists;
    }

    std::string getDBPath() {
        std::string path = "";
        path = DB_FOLDER + "/" + DB_NAME;

        return path;
    }

    void initializeDatabase(Database& db) {
        try {
            // Ensure database directory exists
            if (ensureDirectoryExists(DB_FOLDER) == false) std::filesystem::create_directories(DB_FOLDER);

            // Initialize database
            db.initialize(getDBPath());
        }
        catch (const std::exception& err) {
            throw std::runtime_error("Database initialization failed: " + std::string(err.what()));
        }

        Display::pauseScreen();
    }

    Category stringToCategory(const std::string& str) {
        Category cat;

        if (str == "CSC_111") cat = Category::CSC_111;
        if (str == "CSC_211") cat = Category::CSC_211;
        if (str == "CSC_231") cat = Category::CSC_231;
        else throw std::invalid_argument("Invalid category string");

        return cat;
    }

    void handleGameplay(Hangman& game, Database& db, const std::string& playerName) {
        try {
            int playerId = -1;

            if (!playerName.empty()) {
                try {
                    db.open(getDBPath());
                    playerId = db.addPlayer(playerName);

                    if (playerId != -1) game.setCurrentPlayerId(playerId);
                }
                catch (const DatabaseException& err) {
                    Display::showError("Database error during player addition: " + std::string(err.what()));
                }
                catch (const std::exception& err) {
                    Display::showError("Unknown error during player addition: " + std::string(err.what()));
                }

                db.close();
            }

            while (true) {
                Category category;
                std::string categoryStr = "";
                GameMode mode;
                std::string modeStr = "";

                // Get game mode selection
                Display::showModeMenu();
                std::getline(std::cin, modeStr);

                if (modeStr == "1") mode = GameMode::CLASSIC;
                else if (modeStr == "2") mode = GameMode::TEST;
                else throw std::invalid_argument("Invalid mode selection");

                // Get category selection
                Display::showCategoryMenu();
                std::getline(std::cin, categoryStr);

                if (categoryStr == "1") category = Category::CSC_111;
                else if (categoryStr == "2") category = Category::CSC_211;
                else if (categoryStr == "3") category = Category::CSC_231;
                else throw std::invalid_argument("Invalid category selection");

                // Set up game
                game.setGameMode(mode);
                game.setCategory(category);

                // Load questions
                db.open(getDBPath());

                std::vector<QuestionAnswer> questions;
                questions = db.getQuestions(category);

                db.close();

                if (questions.empty()) throw std::runtime_error("No questions available for selected category");

                game.loadQuestions(questions);

                // Start game
                if (!game.startGame()) throw std::runtime_error("Failed to start game");

                // Main game loop
                while (!game.isGameOver()) {
                    const QuestionAnswer& currentQ = game.getCurrentQuestion();

                    // Display game state
                    if (mode == GameMode::CLASSIC) Display::showClassicGameState(game);
                    else Display::showTestGameState(game);

                    // Get player's guess
                    std::string guess;
                    std::getline(std::cin, guess);

                    // Check for quit
                    if ((guess == "quit") || (guess == "exit")) {
                        Display::showQuitConfirmation();

                        std::string confirm;
                        std::getline(std::cin, confirm);

                        if ((confirm == "y") || (confirm == "Y")) {
                            game.resetGame();
                            return;
                        }

                        continue;
                    }

                    // Process guess
                    try {
                        bool correct = false;
                        correct = game.makeGuess(guess);

                        if (mode == GameMode::CLASSIC) Display::showGuessResult(correct);
                    }
                    catch (const std::exception& err) {
                        Display::showError("Invalid guess: " + std::string(err.what()));
                        continue;
                    }
                }

                // Game over - save score if player provided name
                if (playerId != -1) {
                    try {
                        db.open(getDBPath());
                        db.saveScore(playerId, Hangman::categoryToString(category), Hangman::gameModeToString(mode), game.getCurrentScore());
                        db.close();
                    }
                    catch (const DatabaseException& err) {
                        Display::showError("Failed to save score: " + std::string(err.what()));
                    }
                    catch (const std::exception& err) {
                        Display::showError("Unexpected error while saving score: " + std::string(err.what()));
                    }
                }

                // Show final results
                Display::showGameOver(game);

                // Ask to play again
                Display::showPlayAgainPrompt();

                std::string playAgain;
                std::getline(std::cin, playAgain);

                if ((playAgain != "y") && (playAgain != "Y")) break;
            }
        }
        catch (const std::exception& err) {
            Display::showError("Game error: " + std::string(err.what()));
        }
    }
}

int main() {
    int exitStatus = 0;

    try {
        // Initialize game components
        Database db;
        Hangman game;

        // Set up console and display welcome
        Display::initializeConsole();
        Display::showWelcome();

        // Initialize database
        initializeDatabase(db);
        game.setCurrentDbPath(getDBPath());

        while (true) {
            std::string choice;

            // Show main menu and get player input
            Display::showMainMenu();
            std::getline(std::cin, choice);

            if (choice == "1") {  // Play game
                Display::showPlayerNamePrompt();

                std::string playerName = "";
                std::getline(std::cin, playerName);

                handleGameplay(game, db, playerName);
            }
            else if (choice == "2") {  // About section
                Display::showAbout();
            }
            else if (choice == "3") {  // High Scores
                db.open(getDBPath());
                Display::showHighScores(db);
                db.close();
            }
            else if (choice == "4") {  // Quit game
                break;
            }
            else {  // Invalid input
                Display::showError("Invalid choice. Please try again.");
            }
        }

        exitStatus = EXIT_SUCCESS;
    }
    catch (const std::exception& err) {
        Display::showError("Fatal error: " + std::string(err.what()));

        exitStatus = EXIT_FAILURE;
    }

    return exitStatus;
}

// MAIN_CPP
