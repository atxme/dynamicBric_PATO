////////////////////////////////////////////////////////////
//  log test file
//  implements test cases for log functions
//
// Written : 15/01/2025
////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <thread>
#include <regex>

extern "C" {
    #include "xLog/xLog.h"
}

class LogTest : public ::testing::Test {
protected:
    t_logCtx config;
    const char* TEST_LOG_FILE = "test.log";

    void SetUp() override {
        memset(&config, 0, sizeof(t_logCtx));
        config.t_bLogToConsole = true;
        config.t_bLogToFile = true;
        strncpy(config.t_cLogPath, TEST_LOG_FILE, XOS_LOG_PATH_SIZE);
        
        // Supprimer l'ancien fichier de log s'il existe
        std::remove(TEST_LOG_FILE);
    }

    void TearDown() override {
        xLogClose();
        std::remove(TEST_LOG_FILE);
    }

    // Helper pour lire le contenu du fichier log
    std::string readLogFile() {
        std::ifstream file(TEST_LOG_FILE);
        return std::string(std::istreambuf_iterator<char>(file),
                          std::istreambuf_iterator<char>());
    }

    bool logContainsPattern(const std::string& pattern) {
        std::string content = readLogFile();
        std::regex regex(pattern);
        return std::regex_search(content, regex);
    }
};

// Test initialisation basique
TEST_F(LogTest, BasicInitialization) {
    EXPECT_EQ(xLogInit(&config), XOS_LOG_OK);
}

// Test initialisation avec paramètre invalide
TEST_F(LogTest, InvalidInit) {
    EXPECT_DEATH(xLogInit(nullptr), ".*");
}

// Test écriture simple
TEST_F(LogTest, BasicWrite) {
    EXPECT_EQ(xLogInit(&config), XOS_LOG_OK);
    EXPECT_EQ(X_LOG_TRACE("Test message"), XOS_LOG_OK);
    EXPECT_TRUE(logContainsPattern("TRACE.*Test message"));
}

// Test écriture avec formatage
TEST_F(LogTest, FormattedWrite) {
    EXPECT_EQ(xLogInit(&config), XOS_LOG_OK);
    EXPECT_EQ(X_LOG_TRACE("Test %d %s", 42, "formatted"), XOS_LOG_OK);
    EXPECT_TRUE(logContainsPattern("TRACE.*Test 42 formatted"));
}

// Test écriture concurrente
TEST_F(LogTest, ConcurrentWrites) {
    EXPECT_EQ(xLogInit(&config), XOS_LOG_OK);
    
    std::vector<std::thread> threads;
    const int NUM_THREADS = 10;
    const int WRITES_PER_THREAD = 100;
    
    for(int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([this, i] {
            for(int j = 0; j < WRITES_PER_THREAD; j++) {
                EXPECT_EQ(X_LOG_TRACE("Thread %d Write %d", i, j), XOS_LOG_OK);
            }
        });
    }
    
    for(auto& thread : threads) {
        thread.join();
    }
}

// Test limite de taille du message
TEST_F(LogTest, MessageSizeLimit) {
    EXPECT_EQ(xLogInit(&config), XOS_LOG_OK);
    
    std::string longMessage(XOS_LOG_MSG_SIZE + 10, 'A');
    EXPECT_EQ(X_LOG_TRACE("%s", longMessage.c_str()), XOS_LOG_OK);
}

// Test écriture sans initialisation
TEST_F(LogTest, WriteWithoutInit) {
    EXPECT_EQ(X_LOG_TRACE("Test message"), XOS_LOG_NOT_INIT);
}

// Test configuration console uniquement
TEST_F(LogTest, ConsoleOnlyConfig) {
    config.t_bLogToFile = false;
    EXPECT_EQ(xLogInit(&config), XOS_LOG_OK);
    EXPECT_EQ(X_LOG_TRACE("Console only"), XOS_LOG_OK);
}

// Test configuration fichier uniquement
TEST_F(LogTest, FileOnlyConfig) {
    config.t_bLogToConsole = false;
    EXPECT_EQ(xLogInit(&config), XOS_LOG_OK);
    EXPECT_EQ(X_LOG_TRACE("File only"), XOS_LOG_OK);
    EXPECT_TRUE(logContainsPattern("File only"));
}

// Test fermeture et réinitialisation
TEST_F(LogTest, CloseAndReinit) {
    EXPECT_EQ(xLogInit(&config), XOS_LOG_OK);
    EXPECT_EQ(xLogClose(), XOS_LOG_OK);
    EXPECT_EQ(xLogInit(&config), XOS_LOG_OK);
}

// Test différents types de logs
TEST_F(LogTest, DifferentLogTypes) {
    EXPECT_EQ(xLogInit(&config), XOS_LOG_OK);
    EXPECT_EQ(X_LOG_TRACE("Trace message"), XOS_LOG_OK);
    EXPECT_EQ(X_LOG_ASSERT("Assert message"), XOS_LOG_OK);
    
    std::string content = readLogFile();
    EXPECT_TRUE(logContainsPattern("TRACE.*Trace message"));
    EXPECT_TRUE(logContainsPattern("ASSERT.*Assert message"));
}
