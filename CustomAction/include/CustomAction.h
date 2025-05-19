#pragma once

#include <Windows.h>
#include <Msi.h>
#include <filesystem>

#include <string>
#include <chrono>
#include <format>
#include <fstream>

#include "LoggerFactory.h"
#include "CleanupFactory.h"

namespace WinLogon::CustomActions
{
    class CustomActions
    {
    public:
        static UINT createInstallationLogFile(MSIHANDLE hInstall)
        {
            auto logger = Logger::LoggerFactory::createLogger(hInstall);
            logger->log(Logger::LogLevel::LOG_INFO, L"Starting installation log file creation...");

            try
            {
                // Execute V3 file cleanup
                auto cleanupManager = Cleanup::CleanupFactory::createV3CleanupManager(hInstall);
                cleanupManager->executeAll( );

                // Get current date and time
                std::wstring currentDateTime = getCurrentDateTime( );

                // Write to log file
                if (writeInstallationLogFile(currentDateTime))
                {
                    // Report success in log
                    logger->log(Logger::LogLevel::LOG_INFO,
                                std::format(L"Installation log file created successfully at C:\\installation_test.txt"));
                    return ERROR_SUCCESS;
                }
                else
                {
                    // Report failure in log
                    logger->log(Logger::LogLevel::LOG_ERROR,
                                std::format(L"Error creating installation log file!"));
                    return ERROR_INSTALL_FAILURE;
                }
            }
            catch (const std::exception& e)
            {
                // Report exception in log
                logger->log(Logger::LogLevel::LOG_ERROR,
                            std::format(L"Exception when creating installation log file: {}",
                                        std::wstring(e.what( ), e.what( ) + strlen(e.what( )))));
                return ERROR_INSTALL_FAILURE;
            }
            catch (...)
            {
                // Report exception in log
                logger->log(Logger::LogLevel::LOG_ERROR,
                            std::format(L"Unknown exception when creating installation log file!"));
                return ERROR_INSTALL_FAILURE;
            }
        }

        static UINT deleteWlconfigFile(MSIHANDLE hInstall)
        {
            // Path of the file to be deleted
            const std::wstring filePath = L"C:\\Windows\\System32\\Wlconfig.cfg";
            auto logger = Logger::LoggerFactory::createLogger(hInstall);

            // Initialize message log
            logger->log(Logger::LogLevel::LOG_INFO,
                        std::format(L"Trying to delete file {}...", filePath));

            try
            {
                // Create a strategy for a single file
                class SingleFileCleanupStrategy : public Cleanup::FileCleanupStrategy
                {
                public:
                    explicit SingleFileCleanupStrategy(const std::filesystem::path& path)
                        : filePath(path)
                    {
                    }

                    bool execute(std::shared_ptr<Logger::ILogger> logger) override
                    {
                        return removeFile(filePath, logger);
                    }

                    std::wstring getName( ) const override
                    {
                        return L"Single File Cleanup Strategy";
                    }

                private:
                    std::filesystem::path filePath;
                };

                auto fileStrategy = std::make_unique<SingleFileCleanupStrategy>(filePath);

                // Try to delete the file
                if (fileStrategy->execute(logger))
                {
                    // Report success in log
                    logger->log(Logger::LogLevel::LOG_INFO,
                                std::format(L"File {} deleted successfully.", filePath));
                    return ERROR_SUCCESS;
                }
                else
                {
                    DWORD errorCode = GetLastError( );
                    logger->log(Logger::LogLevel::LOG_ERROR,
                                std::format(L"Error deleting the file. Error code: {}", errorCode));

                    // If the file doesn't exist, consider it a success
                    if (errorCode == ERROR_FILE_NOT_FOUND)
                    {
                        logger->log(Logger::LogLevel::LOG_INFO,
                                    std::format(L"The file does not exist. Continuing installation."));
                        return ERROR_SUCCESS;
                    }

                    return ERROR_INSTALL_FAILURE;
                }
            }
            catch (...)
            {
                logger->log(Logger::LogLevel::LOG_ERROR,
                            std::format(L"Exception when trying to delete file {}", filePath));
                return ERROR_INSTALL_FAILURE;
            }
        }

        static UINT executeFullCleanup(MSIHANDLE hInstall)
        {
            try
            {
                auto logger = Logger::LoggerFactory::createLogger(hInstall);

                // Create managers individually
                auto v3CleanupManager = Cleanup::CleanupFactory::createV3CleanupManager(hInstall);
                auto v4CleanupManager = Cleanup::CleanupFactory::createV4CleanupManager(hInstall);
                auto registryCleanupManager = Cleanup::CleanupFactory::createRegistryCleanupManager(hInstall);

                // Execute each strategy sequentially
                logger->log(Logger::LogLevel::LOG_INFO, L"Executing V3 files cleanup...");
                bool v3Success = v3CleanupManager->executeAll( );

                logger->log(Logger::LogLevel::LOG_INFO, L"Executing V4 files cleanup...");
                bool v4Success = v4CleanupManager->executeAll( );

                logger->log(Logger::LogLevel::LOG_INFO, L"Executing registry cleanup...");
                bool registrySuccess = registryCleanupManager->executeAll( );

                // Check overall success of all operations
                bool overallSuccess = v3Success && v4Success && registrySuccess;

                return overallSuccess ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
            }
            catch (...)
            {
                auto logger = Logger::LoggerFactory::createLogger(hInstall);
                logger->log(Logger::LogLevel::LOG_ERROR, L"Unknown exception during full cleanup");
                return ERROR_INSTALL_FAILURE;
            }
        }

    private:
        CustomActions( ) = delete;  // Prevents instantiation

        static std::wstring getCurrentDateTime( )
        {
            try
            {
                const auto now = std::chrono::system_clock::now( );
                const auto localTime = std::chrono::current_zone( )->to_local(now);
                return std::format(L"{:%d/%m/%Y %H:%M:%S}", localTime);
            }
            catch (...)
            {
                // In case of any error, return a default message
                return L"Unknown date";
            }
        }

        static bool writeInstallationLogFile(const std::wstring& dateTimeStr)
        {
            try
            {
                std::wofstream logFile(L"C:\\installation_test.txt");
                if (logFile.is_open( ))
                {
                    logFile << std::format(L"Installation performed on: {}", dateTimeStr) << std::endl;
                    logFile.close( );
                    return true;
                }
            }
            catch (...)
            {
                // Failed to write to file
            }

            return false;
        }
    };
}