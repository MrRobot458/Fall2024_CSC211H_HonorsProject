/*
Name: Emmanuel Rivas
ID: 15310887
Class: Fall 2024, CSC 211H
Date: 02/01/2025
Instructor: Dr. Azhar
Honors Project: Hangman
*/

#include "Display.h"
#include "Database.h"
#include "Hangman.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <iomanip>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

// Initialize static members
const std::string Display::GAME_ART_PATH = "Data/Resources/gameArt.txt";
const std::string Display::ART_SEPARATOR = "$$$$$$$$$$";

std::vector<std::string> Display::gameLogo;
std::vector<std::string> Display::baseAndPole;
std::vector<std::string> Display::head;
std::vector<std::string> Display::torso;
std::vector<std::string> Display::leftUpperArm;
std::vector<std::string> Display::leftLowerArm;
std::vector<std::string> Display::rightUpperArm;
std::vector<std::string> Display::rightLowerArm;
std::vector<std::string> Display::leftUpperLeg;
std::vector<std::string> Display::leftLowerLeg;
std::vector<std::string> Display::rightUpperLeg;
std::vector<std::string> Display::rightLowerLeg;

void Display::initializeConsole() {
    try {
        loadGameArt();

        #ifdef _WIN32
            // Enable Unicode and UTF-8 support
            SetConsoleOutputCP(CP_UTF8);  // For output
            SetConsoleCP(CP_UTF8);  // For input
            
            // Enable ANSI escape sequences and Unicode
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD dwMode = 0;

            GetConsoleMode(hOut, &dwMode);
            SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        #endif
    }
    catch (const std::exception& err) {
        throw std::runtime_error("Console initialization failed: " + std::string(err.what()));
    }
}

void Display::loadGameArt() {
    try {
        // Open the game art file
        std::ifstream file(GAME_ART_PATH);

        if (!file) throw std::runtime_error("Could not open game art file: " + GAME_ART_PATH);

        // Read each item into its respective vector
        readArtItem(file, gameLogo);
        readArtItem(file, baseAndPole);
        readArtItem(file, head);
        readArtItem(file, torso);
        readArtItem(file, leftUpperArm);
        readArtItem(file, leftLowerArm);
        readArtItem(file, rightUpperArm);
        readArtItem(file, rightLowerArm);
        readArtItem(file, leftUpperLeg);
        readArtItem(file, leftLowerLeg);
        readArtItem(file, rightUpperLeg);
        readArtItem(file, rightLowerLeg);

        file.close();
    }
    catch (const std::exception& err) {
        throw std::runtime_error("Failed to load game art data into vectors: " + std::string(err.what()));
    }
}

void Display::readArtItem(std::ifstream& file, std::vector<std::string>& artVector) {
    std::string line;
    artVector.clear();

    while (std::getline(file, line)) {
        if (line == ART_SEPARATOR) break;
        artVector.push_back(line);
    }
}

void Display::clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void Display::setTextColor(const std::string& color) {
    #ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

        if (color == "green") SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        else if (color == "red") SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
        else if (color == "yellow") SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        else if (color == "orange") SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
    #else
        if (color == "green") std::cout << "\033[32;1m";
        else if (color == "red") std::cout << "\033[31;1m";
        else if (color == "yellow") std::cout << "\033[33;1m";
        else if (color == "orange") std::cout << "\033[38;5;208m";
    #endif
}

void Display::resetTextColor() {
    #ifdef _WIN32
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
    #else
        std::cout << "\033[0m";
    #endif
}

void Display::showWelcome() {

    clearScreen();
    showInfo("Welcome to...");
    std::this_thread::sleep_for(std::chrono::seconds(3));  // Wait 3 seconds before displaying logo
    setTextColor("orange");

    for (const std::string& line : gameLogo) std::cout << line << std::endl;

    resetTextColor();
    showInfo("\n\n\n\nNote: This game is best played in full screen mode!");
}

void Display::showMainMenu() {
    clearScreen();
    setTextColor("orange");

    for (const std::string& line : gameLogo) std::cout << line << std::endl;

    resetTextColor();

    std::cout << "\n\nMain Menu:\n"
        << "1. Play Game\n"
        << "2. About\n"
        << "3. High Scores\n"
        << "4. Quit\n\n"
        << "Enter your choice: ";
}

void Display::showModeMenu() {
    clearScreen();
    std::cout << "Select Game Mode:\n"
        << "1. Classic Mode\n"
        << "2. Test Mode\n\n"
        << "Enter your choice (or 'back' to return): ";
}

void Display::showCategoryMenu() {
    clearScreen();
    std::cout << "Select Question Category:\n"
        << "1. CSC 111 - Introductory Programming\n"
        << "2. CSC 211 - OOP and Advanced Programming\n"
        << "3. CSC 231 - Discrete Structures\n\n"
        << "Enter your choice (or 'back' to return): ";
}

void Display::showClassicGameState(const Hangman& game) {  // FIX
    clearScreen();
    std::cout << generateHangmanStage(game.getIncorrectRounds()) << std::endl;

    std::cout << "\nRound: " << game.getCurrentRound() << "/10\n"
        << "Score: " << std::fixed << std::setprecision(2) << game.getCurrentScore() << "\n"
        << "Chances Left: " << game.getRemainingChances() << "\n\n"
        << "Question: " << game.getCurrentQuestion().question << "\n\n"
        << "Enter your guess (or 'quit' to exit): ";
}

void Display::showTestGameState(const Hangman& game) {
    clearScreen();

    std::cout << "\nRound: " << game.getCurrentRound() << "/10\n"
        << "Time Remaining: " << game.getRemainingTime().count() << " seconds\n\n"
        << "Question: " << game.getCurrentQuestion().question << "\n\n"
        << "Enter your answer (or 'quit' to exit): ";
}

void Display::showGuessResult(bool correct) {
    if (correct) {
        showSuccess("Correct!");
        pauseScreen();
    }
    else {
        showError("Incorrect!");
    }
}

void Display::showGameOver(const Hangman& game) {
    clearScreen();
    std::cout << generateHangmanStage(game.getIncorrectRounds()) << std::endl;

    if (game.getHasPlayerWon()) {
        setTextColor("green");
        std::cout << "\nCongratulations! You've won!\n";
    }
    else {
        setTextColor("red");

        if ((game.getGameMode() == GameMode::TEST) && (game.getRemainingTime().count() <= 0)) std::cout << "Time's up!\n";

        std::cout << "\nGame Over!\n";
    }

    resetTextColor();

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\nFinal Score: " << game.getCurrentScore() << "\n";

    if (game.isNewHighScore()) {
        setTextColor("yellow");
        std::cout << "\nNEW HIGH SCORE!\n";
        resetTextColor();
    }

    pauseScreen();
}

void Display::showPlayerNamePrompt() {
    clearScreen();
    std::cout << "Enter your player name to save your gameplay (or press Enter to play without saving): ";
}

void Display::showQuitConfirmation() {
    showWarning("Are you sure you want to quit? Your progress won't be saved (Y/N): ");
}

void Display::showPlayAgainPrompt() {
    std::cout << "\nWould you like to play again? (Y/N): ";
}

void Display::showError(const std::string& message) {
    setTextColor("red");
    std::cout << "Error: " << message << std::endl;
    resetTextColor();

    pauseScreen();
}

void Display::showSuccess(const std::string& message) {
    setTextColor("green");
    std::cout << message << std::endl;
    resetTextColor();
}

void Display::showWarning(const std::string& message) {
    setTextColor("yellow");
    std::cout << message;
    resetTextColor();
}

void Display::showInfo(const std::string& message) {
    std::cout << message << std::endl;
}

void Display::showAbout() { // FIX THIS
    clearScreen();

    std::cout << "About The Game:\n\n";
    std::cout << "Hangman v1.0.0\n\n"
        << "This is an engaging educational game designed to help computer science students and enthusiasts test and improve their\n"
        << "knowledge across different computer science/programming courses and topics. The game has different modes and course categories\n"
        << "that allow for casual playing, or testing your skills and knowledge under pressure in a variety of computer science topics.\n"
        << "Whether you're a beginner or looking to reinforce your understanding, this game offers a fun and interactive way to learn!\n\n"
        << "Game Modes:\n"
        << "1. Classic Mode\n"
        << "\t1. You get multiple chances to guess the correct answer\n"
        << "\t2. Earn points for correct answers\n"
        << "\t3. Lose points for incorrect attempts\n"
        << "\t4. Complete 10 rounds of questions\n"
        << "2. Test Mode\n"
        << "\t1. Time-limited challenge\n"
        << "\t2. Complete the 10 rounds before time runs out\n"
        << "\t3. Each correct answer earns points\n"
        << "\t4. Test your knowledge under pressure\n\n"
        << "Categories:\n"
        << "1. CSC 111 - Introductory Programming\n"
        << "2. CSC 211 - Object Oriented and Advanced Programming\n"
        << "3. CSC 231 - Discrete Structures\n\n"
        << "How to Play:\n"
        << "1. Enter a player name, if you want to save the gameplay\n"
        << "2. Select a game mode (Classic or Test)\n"
        << "3. Choose a course category\n"
        << "4. For each question, enter your answer\n"
        << "   In Classic Mode:\n"
        << "\t1. You get 5 chances each round to guess correctly\n"
        << "\t2. Each correct guess earns you 10 points\n"
        << "\t3. Each incorrect guess costs you 2 points\n"
        << "\t4. 5 incorrect attempts in a row move you to the next round and adds a body part to the hangman\n"
        << "\t5. The hangman has 10 body parts (1 per round)\n"
        << "\t5. Press Enter when you're ready to continue\n"
        << "\t6. Your current score and chances left will be displayed each round\n"
        << "   In Test Mode:\n"
        << "\t1. You have 20 minutes to answer all questions\n"
        << "\t2. The timer doesn't stop counting down once it starts: Speed, accuracy and correctness are key\n"
        << "\t3. The hangman is only displayed at the end - Focus! Don't run out of time!\n"
        << "\t4. The game moves onto the next round once you enter your guess\n"
        << "\t5. You won't be shown if you answered correct or not, so be sure of your answer\n"
        << "\t6. The time remaining will be displayed each round\n\n"
        << "5. The final result will be shown in the end after the last round\n"
        << "6. Enter your response (Y/y/N/n) when asked to play again\n"
        << "7. If you gave a player name, check the high scores section after the game to see if you made it into the top 10 for\n"
        << "   the category/mode you played\n\n"
        << "High Scores section important details :\n"
        << "1. If there is a tie in the high scores, the most recent score will take the higher ranking position\n"
        << "2. If you score the same, with the same player name, and for the same category - mode combination, your new score will\n"
        << "   replace your last score\n\n"
        << "Aim to score as high as possible!\n\n"
        << "Tips:\n"
        << "1. Read questions carefully\n"
        << "2. Think through your answers\n"
        << "3. Check your spelling! The game is very strict with spelling\n"
        << "4. Learn from each round\n"
        << "5. Challenge yourself to improve your score and reach the top 10!\n\n"
        << "                               \\O/\n"
        << "                                |\n"
        << "Good luck and happy learning!  / \\\n\n"
        << "Legal disclaimer: The game's logo was created at patorjk.com/software/taag/#p=display&f=Graffiti&t=Type%20Something%20\n"
        << "For more information regarding this game and/or support, please visit:\n"
        << "https://sites.google.com/d/1Gi6kN-hNAyjqnlh1jcknQ5J-j1ZC2A-3/p/11nPlMVktieiS7bwFFc6LwSO93391A70p/edit\n";

    pauseScreen();
}

void Display::showHighScores(Database& db) {
    clearScreen();
    std::cout << "High Scores:\n\n";

    const std::vector<std::string> categories = { "CSC_111", "CSC_211", "CSC_231" };
    const std::vector<std::string> modes = { "Classic", "Test" };

    for (const std::string& category : categories) {
        std::cout << "\n" << category << ":\n";

        for (const std::string& mode : modes) {
            std::cout << "\n" << mode << " Mode:\n";

            std::vector<std::string> scores;
            scores = db.getHighScores(category, mode);

            if (scores.empty()) std::cout << "No scores recorded yet\n";
            else for (const std::string& score : scores) std::cout << score << "\n";
        }
    }

    pauseScreen();
}

void Display::pauseScreen(const std::string& message) {
    std::cout << "\n" << message;
    std::cin.ignore(1000000, '\n');
}

std::string Display::generateHangmanStage(int incorrectRounds) {
    std::string result;

    // Add body parts based on number of incorrect guesses
    switch (incorrectRounds) {
    case 0:  // Base and pole added
        for (const std::string& line : baseAndPole) result += (line + "\n");
        break;
    case 1:  // Head added
        for (const std::string& line : head) result += (line + "\n");
        break;
    case 2:  // Torso added
        for (const std::string& line : torso) result += (line + "\n");
        break;
    case 3:  // Left upper arm added
        for (const std::string& line : leftUpperArm) result += (line + "\n");
        break;
    case 4:  // Left lower arm added
        for (const std::string& line : leftLowerArm) result += (line + "\n");
        break;
    case 5:  // Right upper arm added
        for (const std::string& line : rightUpperArm) result += (line + "\n");
        break;
    case 6:  // Right lower arm added
        for (const std::string& line : rightLowerArm) result += (line + "\n");
        break;
    case 7:  // Left upper leg added
        for (const std::string& line : leftUpperLeg) result += (line + "\n");
        break;
    case 8:  // Left lower leg added
        for (const std::string& line : leftLowerLeg) result += (line + "\n");
        break;
    case 9:  // Right upper leg added
        for (const std::string& line : rightUpperLeg) result += (line + "\n");
        break;
    case 10:  // Right lower leg added
        for (const std::string& line : rightLowerLeg) result += (line + "\n");
        break;
    default:  // Invalid hangman stage index
        break;
    }

    return result;
}

// DISPLAY_CPP
