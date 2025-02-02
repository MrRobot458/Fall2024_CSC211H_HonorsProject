/*
Name: Emmanuel Rivas
ID: 15310887
Class: Fall 2024, CSC 211H
Date: 02/01/2025
Instructor: Dr. Azhar
Honors Project: Hangman
*/

#ifndef DISPLAY_H
#define DISPLAY_H

#include <iostream>
#include <string>
#include <vector>

// Forward declarations of classes used
class Database;
class Hangman;

class Display {
public:
    // Initialize and setup
    static void initializeConsole();
    static void showWelcome();

    // Menu displays
    static void showMainMenu();
    static void showModeMenu();
    static void showCategoryMenu();
    static void showAbout();
    static void showHighScores(Database& db);

    // Game state displays
    static void showClassicGameState(const Hangman& game);
    static void showTestGameState(const Hangman& game);
    static void showGuessResult(bool correct);
    static void showGameOver(const Hangman& game);

    // Prompts and confirmations
    static void showPlayerNamePrompt();
    static void showQuitConfirmation();
    static void showPlayAgainPrompt();

    // Utility displays
    static void showError(const std::string& message);
    static void showInfo(const std::string& message);
    static void showSuccess(const std::string& message);
    static void showWarning(const std::string& message);
    static void pauseScreen(const std::string& message = "Press Enter to continue...");

private:
    // File information
    static const std::string GAME_ART_PATH;
    static const std::string ART_SEPARATOR;  // Separator between items in the game art file

    // Store logo and hangman stages as string vectors
    static std::vector<std::string> gameLogo;
    static std::vector<std::string> baseAndPole;
    static std::vector<std::string> head;
    static std::vector<std::string> torso;
    static std::vector<std::string> leftUpperArm;
    static std::vector<std::string> leftLowerArm;
    static std::vector<std::string> rightUpperArm;
    static std::vector<std::string> rightLowerArm;
    static std::vector<std::string> leftUpperLeg;
    static std::vector<std::string> leftLowerLeg;
    static std::vector<std::string> rightUpperLeg;
    static std::vector<std::string> rightLowerLeg;

    // Private helper methods
    static void loadGameArt();
    static void readArtItem(std::ifstream& file, std::vector<std::string>& artVector);
    static void clearScreen();
    static void setTextColor(const std::string& color);
    static void resetTextColor();
    static std::string generateHangmanStage(int incorrectGuesses);
};

#endif  // DISPLAY_H
