/*
Name: Emmanuel Rivas
ID: 15310887
Class: Fall 2024, CSC 211H
Date: 02/01/2025
Instructor: Dr. Azhar
Honors Project: Hangman
*/

#ifndef HANGMAN_H
#define HANGMAN_H

#include "Game.h"
#include "Database.h"
#include <string>
#include <vector>
#include <random>
#include <chrono>

// Enums for game states and modes
enum class GameState { MENU, PLAYING, PAUSED, GAME_OVER };
enum class GameMode { CLASSIC, TEST };
enum class Category { CSC_111, CSC_211, CSC_231 };

// Struct to hold a question-answer pair
struct QuestionAnswer {
    std::string question;
    std::string answer;
    bool used;  // Track if question-answer pair has been used in current game

    QuestionAnswer(std::string q, std::string a) : question(std::move(q)), answer(std::move(a)), used(false) { }
};

// Custom exceptions for Hangman game
class HangmanException : public GameException {
public:
    explicit HangmanException(const std::string& message) : GameException(message) { }
};

// Derived class for Hangman game
class Hangman : public Game {
public:
    // Constructor
    Hangman();

    // Game interface implementation
    bool startGame() override;
    bool isGameOver() const override;
    void endGame() override;
    void resetGame() override;

    // Hangman specific functions
    // Getters
    GameState getGameState() const { return currentState; }
    GameMode getGameMode() const { return currentMode; }
    Category getCategory() const { return currentCategory; }
    const QuestionAnswer& getCurrentQuestion() const;

    int getCurrentRound() const { return currentRound; }
    double getCurrentScore() const { return currentScore; }
    int getIncorrectRounds() const { return incorrectRounds; }
    int getIncorrectGuesses() const { return incorrectGuesses; }
    int getRemainingChances() const;
    std::chrono::seconds getRemainingTime() const;
    bool getHasPlayerWon() const;
    bool isNewHighScore() const;


    // Setters
    void setGameMode(GameMode mode);
    void setCategory(Category cat);
    void setCurrentDbPath(const std::string& path) { currentDbPath = path; }
    void setCurrentPlayerId(int id) { currentPlayerId = id; }
    
    // Hangman operations
    bool loadQuestions(const std::vector<QuestionAnswer>& questions);
    bool makeGuess(const std::string& guess);

    // Static helper functions
    static std::string categoryToString(Category cat);
    static std::string gameModeToString(GameMode mode);

    // Destructor
    ~Hangman();

private:
    static constexpr int TOTAL_ROUNDS = 10;
    static constexpr double MAX_SCORE = 100.0;
    static constexpr int MAX_CLASSIC_QUESTION_CHANCES = 5;  // 5 attempts per round in Classic mode
    static constexpr int SECONDS_PER_TEST_QUESTION = 120;   // 2 minutes per round in Test mode
    static constexpr double POINTS_PER_ROUND = MAX_SCORE / TOTAL_ROUNDS;  // 10.00 points per round
    static constexpr double POINTS_PER_ATTEMPT = POINTS_PER_ROUND / MAX_CLASSIC_QUESTION_CHANCES;  // 2.00 points per attempt in Classic mode rounds

    GameState currentState;
    GameMode currentMode;
    Category currentCategory;
    std::vector<QuestionAnswer> fullQuestionBank;   // All questions for current category
    std::vector<QuestionAnswer> questionSet;        // Current game's question set
    std::string currentDbPath;

    int currentRound;
    double currentScore;
    int incorrectRounds;
    int incorrectGuesses;
    int remainingChances;
    std::chrono::seconds timeLimit;
    std::chrono::steady_clock::time_point gameStartTime;
    int currentPlayerId;
    bool hasPlayerWon;

    // Private member functions
    void initializeGame();
    void selectRandomQuestions();
    bool validateGuess(const std::string& guess) const;
    void updateScore(bool correct);
    bool isTimeExpired() const;
};

#endif  // HANGMAN_H
