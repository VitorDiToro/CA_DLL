#pragma once

#include <Windows.h>
#include <Msi.h>
#include <filesystem>

#include <string>
#include <chrono>
#include <format>
#include <fstream>
#include <map>

#include "LoggerFactory.h"
#include "CleanupFactory.h"
#include "ConfigFileHandler.h"

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
                auto authPointRegistryCleanupManager = Cleanup::CleanupFactory::createAuthPointRegistryCleanupManager(hInstall);

                // Execute each strategy sequentially
                logger->log(Logger::LogLevel::LOG_INFO, L"Executing V3 files cleanup...");
                bool v3Success = v3CleanupManager->executeAll( );

                logger->log(Logger::LogLevel::LOG_INFO, L"Executing V4 files cleanup...");
                bool v4Success = v4CleanupManager->executeAll( );

                logger->log(Logger::LogLevel::LOG_INFO, L"Executing registry cleanup...");
                bool registrySuccess = registryCleanupManager->executeAll( );

                logger->log(Logger::LogLevel::LOG_INFO, L"Executing AuthPoint registry cleanup...");
                bool authPointSuccess = authPointRegistryCleanupManager->executeAll( );

                // Check overall success of all operations
                bool overallSuccess = v3Success && v4Success && registrySuccess && authPointSuccess;

                return overallSuccess ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
            }
            catch (...)
            {
                auto logger = Logger::LoggerFactory::createLogger(hInstall);
                logger->log(Logger::LogLevel::LOG_ERROR, L"Unknown exception during full cleanup");
                return ERROR_INSTALL_FAILURE;
            }
        }

        static UINT cleanAuthPointRegistries(MSIHANDLE hInstall)
        {
            try
            {
                auto logger = Logger::LoggerFactory::createLogger(hInstall);
                logger->log(Logger::LogLevel::LOG_INFO, L"Starting AuthPoint registry cleanup...");

                auto authPointCleanupManager = Cleanup::CleanupFactory::createAuthPointRegistryCleanupManager(hInstall);
                bool success = authPointCleanupManager->executeAll( );

                if (success)
                {
                    logger->log(Logger::LogLevel::LOG_INFO, L"AuthPoint registry cleanup completed successfully.");
                    return ERROR_SUCCESS;
                }
                else
                {
                    logger->log(Logger::LogLevel::LOG_WARNING, L"AuthPoint registry cleanup completed with warnings.");
                    return ERROR_INSTALL_FAILURE;
                }
            }
            catch (...)
            {
                auto logger = Logger::LoggerFactory::createLogger(hInstall);
                logger->log(Logger::LogLevel::LOG_ERROR, L"Exception during AuthPoint registry cleanup");
                return ERROR_INSTALL_FAILURE;
            }
        }


        static UINT copyConfigFileToDestination(MSIHANDLE hInstall)
        {
            try
            {
                auto logger = Logger::LoggerFactory::createLogger(hInstall);
                logger->log(Logger::LogLevel::LOG_INFO, L"Starting copy config file operation...");

                // Get parameters from CustomActionData
                WCHAR szCustomActionData[4096] = { 0 };
                DWORD cchCustomActionData = sizeof(szCustomActionData) / sizeof(szCustomActionData[0]);
                UINT result = MsiGetPropertyW(hInstall, L"CustomActionData", szCustomActionData, &cchCustomActionData);
                if (result != ERROR_SUCCESS)
                {
                    logger->log(Logger::LogLevel::LOG_ERROR, L"Failed to get CustomActionData property");
                    return ERROR_INSTALL_FAILURE;
                }

                // Parse CustomActionData (format: key=value;key=value;...)
                std::map<std::wstring, std::wstring> params;
                std::wstring customActionData = szCustomActionData;
                size_t pos = 0;
                while ((pos = customActionData.find(L";")) != std::wstring::npos)
                {
                    std::wstring token = customActionData.substr(0, pos);
                    size_t equalPos = token.find(L"=");
                    if (equalPos != std::wstring::npos)
                    {
                        std::wstring key = token.substr(0, equalPos);
                        std::wstring value = token.substr(equalPos + 1);
                        params[key] = value;
                    }
                    customActionData.erase(0, pos + 1);
                }
                // Handle the last token
                if (!customActionData.empty( ))
                {
                    size_t equalPos = customActionData.find(L"=");
                    if (equalPos != std::wstring::npos)
                    {
                        std::wstring key = customActionData.substr(0, equalPos);
                        std::wstring value = customActionData.substr(equalPos + 1);
                        params[key] = value;
                    }
                }

                return Config::ConfigFileHandler::CopyConfigFileToDestination(hInstall, params);
            }
            catch (const std::exception& e)
            {
                auto logger = Logger::LoggerFactory::createLogger(hInstall);
                logger->log(Logger::LogLevel::LOG_ERROR,
                            std::wstring(L"Exception in copyConfigFileToDestination: ") +
                            std::wstring(e.what( ), e.what( ) + strlen(e.what( ))));
                return ERROR_INSTALL_FAILURE;
            }
            catch (...)
            {
                auto logger = Logger::LoggerFactory::createLogger(hInstall);
                logger->log(Logger::LogLevel::LOG_ERROR, L"Unknown exception in copyConfigFileToDestination");
                return ERROR_INSTALL_FAILURE;
            }
        }

        static UINT openFileChooser(MSIHANDLE hInstall)
        {
            try
            {
                auto logger = Logger::LoggerFactory::createLogger(hInstall);
                logger->log(Logger::LogLevel::LOG_INFO, L"Starting open file chooser operation...");

                // The property to set with the selected file path
                std::wstring propertyName = L"CONFIG_PATH";

                // Also set CONFIG_PATH_UI for UI display
                UINT result = Config::ConfigFileHandler::OpenFileChooser(hInstall, propertyName);
                if (result == ERROR_SUCCESS)
                {
                    // Get the value we just set
                    WCHAR szConfigPath[MAX_PATH] = { 0 };
                    DWORD cchConfigPath = sizeof(szConfigPath) / sizeof(szConfigPath[0]);
                    UINT propResult = MsiGetPropertyW(hInstall, propertyName.c_str( ), szConfigPath, &cchConfigPath);

                    if (propResult == ERROR_SUCCESS && szConfigPath[0] != L'\0')
                    {
                        // Also set CONFIG_PATH_UI
                        MsiSetPropertyW(hInstall, L"CONFIG_PATH_UI", szConfigPath);
                    }
                }

                return result;
            }
            catch (const std::exception& e)
            {
                auto logger = Logger::LoggerFactory::createLogger(hInstall);
                logger->log(Logger::LogLevel::LOG_ERROR,
                            std::wstring(L"Exception in openFileChooser: ") +
                            std::wstring(e.what( ), e.what( ) + strlen(e.what( ))));
                return ERROR_INSTALL_FAILURE;
            }
            catch (...)
            {
                auto logger = Logger::LoggerFactory::createLogger(hInstall);
                logger->log(Logger::LogLevel::LOG_ERROR, L"Unknown exception in openFileChooser");
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