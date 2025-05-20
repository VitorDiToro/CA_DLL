#pragma once

#include <Windows.h>
#include <Msi.h>

#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <ShlObj.h>
#include <optional>
#include <commdlg.h>
#include <filesystem>

#include "LoggerFactory.h"
#include "ConfigConstants.h"

namespace WinLogon::CustomActions::Config
{
    class ConfigFileHandler
    {
    public:
        /**
         * @brief Copy a configuration file to the destination directory
         * @param hInstall MSI handle
         * @param params A map of parameters for the operation
         * @return TRUE if successful, FALSE otherwise
         */
        static UINT CopyConfigFileToDestination(MSIHANDLE hInstall, const std::map<std::wstring, std::wstring>& params)
        {
            auto logger = Logger::LoggerFactory::createLogger(hInstall);
            logger->log(Logger::LogLevel::LOG_INFO, L"Starting config file copy operation...");

            try
            {
                // Log parameters
                std::wstring message = L"Parameters:";
                for (const auto& [key, value] : params)
                {
                    message += L"\n\tKey: " + key + L", Value: " + value;
                }
                logger->log(Logger::LogLevel::LOG_INFO, message);

                // Extract parameters
                std::optional<std::wstring> configInCurrentFolder;
                std::optional<std::wstring> customConfigPath;
                std::optional<std::wstring> customConfigContent;

                auto it = params.find(L"configInCurrentFolder");
                if (it != params.end() && !it->second.empty())
                    configInCurrentFolder = it->second;

                it = params.find(L"customParameterConfigPath");
                if (it != params.end() && !it->second.empty())
                    customConfigPath = it->second;

                it = params.find(L"customParameterConfigContent");
                if (it != params.end() && !it->second.empty())
                    customConfigContent = it->second;

                // Decision logic
                if (customConfigContent.has_value())
                {
                    if (!CreateConfigFromContent(logger, customConfigContent.value()))
                        return ERROR_INSTALL_FAILURE;
                }
                else if (customConfigPath.has_value())
                {
                    if (!SaveConfigFrom(logger, customConfigPath.value()))
                        return ERROR_INSTALL_FAILURE;
                }
                else if (configInCurrentFolder.has_value())
                {
                    if (!SaveConfigFrom(logger, configInCurrentFolder.value()))
                        return ERROR_INSTALL_FAILURE;
                }
                else
                {
                    logger->log(Logger::LogLevel::LOG_WARNING, L"No valid configuration source provided.");
                    return ERROR_INSTALL_FAILURE;
                }

                return ERROR_SUCCESS;
            }
            catch (const std::exception& e)
            {
                logger->log(Logger::LogLevel::LOG_ERROR,
                            std::wstring(L"Exception in CopyConfigFileToDestination: ") +
                            std::wstring(e.what(), e.what() + strlen(e.what())));
                return ERROR_INSTALL_FAILURE;
            }
            catch (...)
            {
                logger->log(Logger::LogLevel::LOG_ERROR, L"Unknown exception in CopyConfigFileToDestination");
                return ERROR_INSTALL_FAILURE;
            }
        }

        /**
         * @brief Opens a file dialog to select a configuration file
         * @param hInstall MSI handle
         * @param propertyName The MSI property to set with the selected file path
         * @return TRUE if successful, FALSE otherwise
         */
        static UINT OpenFileChooser(MSIHANDLE hInstall, const std::wstring& propertyName)
        {
            auto logger = Logger::LoggerFactory::createLogger(hInstall);
            logger->log(Logger::LogLevel::LOG_INFO, L"Starting OpenFileChooser operation...");

            try
            {
                OPENFILENAMEW ofn;
                wchar_t szFile[MAX_PATH] = { 0 };

                // Initialize OPENFILENAME
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = NULL;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = sizeof(szFile);
                ofn.lpstrFilter = L"Configuration Files (*.cfg)\0*.cfg\0All Files (*.*)\0*.*\0";
                ofn.nFilterIndex = 1;
                ofn.lpstrFileTitle = NULL;
                ofn.nMaxFileTitle = 0;
                ofn.lpstrInitialDir = NULL;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

                if (GetOpenFileNameW(&ofn))
                {
                    // Successfully selected a file, set the MSI property
                    logger->log(Logger::LogLevel::LOG_INFO,
                              std::wstring(L"Selected file: ") + ofn.lpstrFile);

                    UINT result = MsiSetPropertyW(hInstall, propertyName.c_str(), ofn.lpstrFile);
                    if (result != ERROR_SUCCESS)
                    {
                        logger->log(Logger::LogLevel::LOG_ERROR,
                                  std::wstring(L"Failed to set MSI property: ") + propertyName);
                        return ERROR_INSTALL_FAILURE;
                    }

                    logger->log(Logger::LogLevel::LOG_INFO,
                              std::wstring(L"Set MSI property ") + propertyName + L" successfully.");
                    return ERROR_SUCCESS;
                }
                else
                {
                    // User canceled or error occurred
                    logger->log(Logger::LogLevel::LOG_INFO, L"File selection canceled or failed.");
                    return ERROR_SUCCESS;  // Not a failure, user just canceled
                }
            }
            catch (const std::exception& e)
            {
                logger->log(Logger::LogLevel::LOG_ERROR,
                          std::wstring(L"Exception in OpenFileChooser: ") +
                          std::wstring(e.what(), e.what() + strlen(e.what())));
                return ERROR_INSTALL_FAILURE;
            }
            catch (...)
            {
                logger->log(Logger::LogLevel::LOG_ERROR, L"Unknown exception in OpenFileChooser");
                return ERROR_INSTALL_FAILURE;
            }
        }

    private:

        ConfigFileHandler( ) = delete;  // Prevents instantiation


        /**
         * @brief Create a configuration file from content
         * @param logger Logger to use
         * @param content The content to write to the file
         * @return TRUE if successful, FALSE otherwise
         */
        static bool CreateConfigFromContent(std::shared_ptr<Logger::ILogger> logger, const std::wstring& content)
        {
            try
            {
                logger->log(Logger::LogLevel::LOG_INFO,
                          std::wstring(L"Creating ") + Constants::ConfigConstants::DEFAULT_CONFIG_FILE_NAME + L" file...");

                // Create directory if it doesn't exist
                auto destDir = Constants::ConfigConstants::CONFIG_FILE_DESTINATION_DIR;
                if (!std::filesystem::exists(destDir))
                {
                    logger->log(Logger::LogLevel::LOG_INFO, L"Creating destination directory...");
                    std::filesystem::create_directories(destDir);
                }

                // Create destination path
                auto destPath = destDir / Constants::ConfigConstants::DEFAULT_CONFIG_FILE_NAME;

                // Write content to file
                std::ofstream outFile(destPath, std::ios::out | std::ios::binary);
                if (!outFile)
                {
                    logger->log(Logger::LogLevel::LOG_ERROR,
                              std::wstring(L"Failed to open file for writing: ") + destPath.wstring());
                    return false;
                }

                // Convert wstring to UTF-8
                std::string utf8Content;
                int size_needed = WideCharToMultiByte(CP_UTF8, 0, content.c_str(), -1, NULL, 0, NULL, NULL);
                utf8Content.resize(size_needed);
                WideCharToMultiByte(CP_UTF8, 0, content.c_str(), -1, &utf8Content[0], size_needed, NULL, NULL);

                // Write to file
                outFile.write(utf8Content.c_str(), utf8Content.size() - 1);  // -1 to exclude null terminator
                outFile.flush();
                outFile.close();

                logger->log(Logger::LogLevel::LOG_INFO,
                          Constants::ConfigConstants::DEFAULT_CONFIG_FILE_NAME + L" created successfully.");
                return true;
            }
            catch (const std::exception& e)
            {
                logger->log(Logger::LogLevel::LOG_ERROR,
                          std::wstring(L"Exception in CreateConfigFromContent: ") +
                          std::wstring(e.what(), e.what() + strlen(e.what())));
                return false;
            }
        }

        /**
         * @brief Copy a configuration file from a source path
         * @param logger Logger to use
         * @param configPath The source path of the configuration file
         * @return TRUE if successful, FALSE otherwise
         */
        static bool SaveConfigFrom(std::shared_ptr<Logger::ILogger> logger, const std::wstring& configPath)
        {
            try
            {
                // Normalize source path
                std::wstring sourcePath = NormalizeSourcePath(logger, configPath);

                // Create destination directory if it doesn't exist
                auto destDir = Constants::ConfigConstants::CONFIG_FILE_DESTINATION_DIR;
                if (!std::filesystem::exists(destDir))
                {
                    logger->log(Logger::LogLevel::LOG_INFO, L"Creating destination directory...");
                    std::filesystem::create_directories(destDir);
                }

                // Create destination path
                auto destPath = destDir / Constants::ConfigConstants::DEFAULT_CONFIG_FILE_NAME;

                logger->log(Logger::LogLevel::LOG_INFO,
                          std::wstring(L"Copying configuration file to destination.\n") +
                          L"      Source: " + sourcePath + L"\n" +
                          L"      Destination: " + destPath.wstring());

                // Copy file
                std::filesystem::copy_file(sourcePath, destPath, std::filesystem::copy_options::overwrite_existing);

                logger->log(Logger::LogLevel::LOG_INFO,
                          Constants::ConfigConstants::DEFAULT_CONFIG_FILE_NAME + L" copied successfully.");
                return true;
            }
            catch (const std::exception& e)
            {
                logger->log(Logger::LogLevel::LOG_ERROR,
                          std::wstring(L"Exception in SaveConfigFrom: ") +
                          std::wstring(e.what(), e.what() + strlen(e.what())));
                return false;
            }
        }

        /**
         * @brief Normalize a source path to ensure it points to a configuration file
         * @param logger Logger to use
         * @param configPath The path to normalize
         * @return The normalized path
         */
        static std::wstring NormalizeSourcePath(std::shared_ptr<Logger::ILogger> logger, const std::wstring& configPath)
        {
            std::wstring normalizedPath = configPath;

            // If path doesn't end with .cfg, assume it's a directory and append the default filename
            if (normalizedPath.find(L".cfg") == std::wstring::npos)
            {
                std::filesystem::path path(normalizedPath);
                path = std::filesystem::absolute(path);

                // Ensure it's a directory
                if (!path.string().empty() && path.string().back() != '\\' && path.string().back() != '/')
                    path /= L"\\";  // Add trailing backslash if missing

                auto updatedPath = path / Constants::ConfigConstants::DEFAULT_CONFIG_FILE_NAME;

                logger->log(Logger::LogLevel::LOG_INFO, 
                          std::wstring(L"Updating CONFIG_PATH.\n") +
                          L"      From: " + configPath + L"\n" +
                          L"      To: " + updatedPath.wstring());

                normalizedPath = updatedPath.wstring();
            }

            return normalizedPath;
        }
    };
}