/*
Name: Emmanuel Rivas
ID: 15310887
Class: Fall 2024, CSC 211H
Date: 02/01/2025
Instructor: Dr. Azhar
Honors Project: Hangman
*/

#ifndef DATABASE_H
#define DATABASE_H

#include "Hangman.h"
#include "sqlite3.h"
#include <string>
#include <vector>
#include <memory>

// SQLite forward declarations for SQL commands, statements, queries and transactions
struct sqlite3;
struct sqlite3_stmt;

// Forward declarations for question categories and question-answer pairs
enum class Category;
struct QuestionAnswer;

// Custom exceptions for database operations
class DatabaseException : public std::runtime_error {
public:
    explicit DatabaseException(const std::string& message) : std::runtime_error(message) { }
};

// Database class declaration
class Database {
public:
    // Constructor
    Database();

    // Delete copy constructor and assignment operator overloading
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Database initialization
    void initialize(const std::string& dbPath);
    bool databaseExists(const std::string& dbPath) const;
    void createTables();

    // Connection management
    void open(const std::string& dbPath);
    void close();
    bool isOpen() const;

    // Data loading operations
    void loadQuestionsFromTSV(const std::string& filePath, const std::string& category);
    std::vector<QuestionAnswer> getQuestions(Category category);

    // Player operations
    int addPlayer(const std::string& playerName);
    bool playerExists(const std::string& playerName);
    int getPlayerId(const std::string& playerName);

    // Score operations
    void saveScore(int playerId, const std::string& category, const std::string& mode, double score);
    void updateHighScores(int sessionId, int categoryId, int modeId);
    double getHighScore(int playerId, const std::string& category, const std::string& mode);
    std::vector<std::string> getHighScores(const std::string& category, const std::string& mode);
    std::vector<std::pair<std::string, double>> getTopScores(const std::string& category, const std::string& mode, int limit = 10);

    // Destructor
    ~Database();

private:
    const std::string RESOURCES_FOLDER = "Data/Resources";
    const std::vector<std::string> CATEGORY_FILES = {
        "CSC_111.tsv",
        "CSC_211.tsv",
        "CSC_231.tsv"
    };

    sqlite3* db;
    std::string currentDbPath;

    // Helper functions
    int executeQuery(const std::string& query);
    bool tableExists(const std::string& tableName);
    void beginTransaction();
    void commitTransaction();
    void rollbackTransaction();

    // Statement preparation
    using StmtPtr = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;
    StmtPtr prepareStatement(const std::string& query);

    // Error handling
    void checkError(int result, const std::string& operation);
    std::string getLastError() const;

    // Utility functions
    void bindText(sqlite3_stmt* stmt, int index, const std::string& value);
    void bindInt(sqlite3_stmt* stmt, int index, int value);
    void bindDouble(sqlite3_stmt* stmt, int index, double value);
};

#endif  // DATABASE_H
