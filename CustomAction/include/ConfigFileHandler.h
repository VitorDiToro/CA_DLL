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

        static UINT CopyConfigFileToDestination(MSIHANDLE hInstall, const std::map<std::wstring, std::wstring>& params)
        {
            auto logger = Logger::LoggerFactory::createLogger(hInstall);
            logger->log(Logger::LogLevel::LOG_INFO, L"Starting config file copy operation...");

            try
            {
                std::wstring message = L"Parameters:";
                for (const auto& [key, value] : params)
                {
                    message += std::format(L"\n\tKey: {}, Value: {}", key, value);
                }
                logger->log(Logger::LogLevel::LOG_INFO, message);

                // Extract parameters
                std::optional<std::wstring> configInCurrentFolder;
                std::optional<std::wstring> customConfigPath;
                std::optional<std::wstring> customConfigContent;

                auto it = params.find(L"configInCurrentFolder");
                if (it != params.end( ) && !it->second.empty( ))
                {

                    configInCurrentFolder = it->second;
                }

                it = params.find(L"customParameterConfigPath");
                if (it != params.end( ) && !it->second.empty( ))
                    customConfigPath = it->second;

                it = params.find(L"customParameterConfigContent");
                if (it != params.end( ) && !it->second.empty( ))
                    customConfigContent = it->second;

                // Decision logic.
                // Note: Keep this specific order. It's important to check for content first, then path, and finally current folder.
                if (customConfigContent.has_value( ))
                {
                    message = std::format(L"customConfigContent: {}", customConfigContent.value( ));
                    logger->log(Logger::LogLevel::LOG_INFO, message);

                    if (!CreateConfigFromContent(customConfigContent.value( ), logger))
                        return ERROR_INSTALL_FAILURE;
                }
                else if (customConfigPath.has_value( ))
                {
                    message = std::format(L"customConfigPath: {}", customConfigPath.value( ));
                    logger->log(Logger::LogLevel::LOG_INFO, message);

                    if (!SaveConfigFrom(customConfigPath.value( ), logger))
                        return ERROR_INSTALL_FAILURE;
                }
                else if (configInCurrentFolder.has_value( ))
                {
                    message = std::format(L"configInCurrentFolder: {}", configInCurrentFolder.value( ));
                    logger->log(Logger::LogLevel::LOG_INFO, message);

                    if (!SaveConfigFrom(configInCurrentFolder.value( ), logger))
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
                            std::format(L"Exception in CopyConfigFileToDestination: {}",
                                        std::wstring(e.what( ), e.what( ) + strlen(e.what( )))));
                return ERROR_INSTALL_FAILURE;
            }
            catch (...)
            {
                logger->log(Logger::LogLevel::LOG_ERROR, L"Unknown exception in CopyConfigFileToDestination");
                return ERROR_INSTALL_FAILURE;
            }
        }


        // Opens a file dialog to select a configuration file
        static UINT OpenFileChooser(MSIHANDLE hInstall, const std::wstring& propertyName)
        {
            auto logger = Logger::LoggerFactory::createLogger(hInstall);
            logger->log(Logger::LogLevel::LOG_INFO, L"Starting OpenFileChooser operation...");

            try
            {
                std::wstring_view initialDir = getMsiDir(hInstall, logger);

                OPENFILENAMEW ofn;
                createSearchFileDialog(ofn, initialDir);


                if (GetOpenFileNameW(&ofn))
                {
                    // Successfully selected a file, set the MSI property
                    logger->log(Logger::LogLevel::LOG_INFO,
                                std::format(L"Selected file: {}", ofn.lpstrFile));


                    return setMsiProperty(hInstall, propertyName, ofn, logger);
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
                            std::format(L"Exception in OpenFileChooser: {}",
                                        std::wstring(e.what( ), e.what( ) + strlen(e.what( )))));
                return ERROR_INSTALL_FAILURE;
            }
            catch (...)
            {
                logger->log(Logger::LogLevel::LOG_ERROR, L"Unknown exception in OpenFileChooser");
                return ERROR_INSTALL_FAILURE;
            }
        }


        static UINT CopyConfigFiles(MSIHANDLE hInstall)
        {
            auto logger = Logger::LoggerFactory::createLogger(hInstall);
            logger->log(Logger::LogLevel::LOG_INFO, L"Starting to copy configuration files to temporary location...");

            try
            {
                const auto& system32Path = Constants::ConfigConstants::SYSTEM32_CONFIG_PATH;
                const auto& driversEtcPath = Constants::ConfigConstants::DRIVERS_ETC_CONFIG_PATH;
                const auto tempDir = Constants::ConfigConstants::GetTempConfigDir( );

                // Create temp directory if it doesn't exist
                EnsureDirectoryExists(tempDir, logger);

                // Define destination paths
                auto tempInstallConfigPath = tempDir / Constants::ConfigConstants::TEMP_INSTALL_CONFIG_NAME;
                auto tempLocalConfigPath = tempDir / Constants::ConfigConstants::TEMP_LOCAL_CONFIG_NAME;

                bool anySuccess = false;

                // Copy files to temporary location
                bool system32CopySuccess = CopyConfigFile(system32Path,
                                                          tempInstallConfigPath,
                                                          L"System32",
                                                          logger);

                bool driversEtcCopySuccess = CopyConfigFile(driversEtcPath,
                                                            tempLocalConfigPath,
                                                            L"Drivers/etc",
                                                            logger);

                // Return success if at least one file was copied
                if (system32CopySuccess || driversEtcCopySuccess)
                {
                    logger->log(Logger::LogLevel::LOG_INFO, L"Configuration files successfully copied to temporary location.");
                    return ERROR_SUCCESS;
                }
                else
                {
                    logger->log(Logger::LogLevel::LOG_WARNING, L"No configuration files were found to copy.");
                    return ERROR_SUCCESS; // Still return success as this is not a critical error
                }
            }
            catch (const std::exception& e)
            {
                logger->log(Logger::LogLevel::LOG_ERROR,
                            std::format(L"Exception in CopyConfigFiles: {}",
                                        std::wstring(e.what( ), e.what( ) + strlen(e.what( )))));
                return ERROR_INSTALL_FAILURE;
            }
            catch (...)
            {
                logger->log(Logger::LogLevel::LOG_ERROR, L"Unknown exception in CopyConfigFiles");
                return ERROR_INSTALL_FAILURE;
            }
        }


        static UINT RestoreConfigFiles(MSIHANDLE hInstall)
        {
            auto logger = Logger::LoggerFactory::createLogger(hInstall);
            logger->log(Logger::LogLevel::LOG_INFO, L"Starting to restore configuration files from temporary location...");

            try
            {
                const auto tempDir = Constants::ConfigConstants::GetTempConfigDir( );
                const auto tempInstallConfigPath = tempDir / Constants::ConfigConstants::TEMP_INSTALL_CONFIG_NAME;
                const auto tempLocalConfigPath = tempDir / Constants::ConfigConstants::TEMP_LOCAL_CONFIG_NAME;

                const auto& programFilesDir = Constants::ConfigConstants::CONFIG_PROGRAM_FILES_DESTINATION;
                const auto& programDataDir = Constants::ConfigConstants::CONFIG_PROGRAM_DATA_DESTINATION;
                const auto& defaultFileName = Constants::ConfigConstants::DEFAULT_CONFIG_FILE_NAME;


                // Create directories if they don't exist
                EnsureDirectoryExists(programFilesDir, logger);
                EnsureDirectoryExists(programDataDir, logger);

                // Restore files
                bool programFileSuccess = RestoreConfigFile(tempInstallConfigPath, programFilesDir, defaultFileName, L"Installation", logger);
                bool programDataSuccess = RestoreConfigFile(tempLocalConfigPath, programDataDir, defaultFileName, L"Local", logger);

                // Clean up temporary directory
                if (std::filesystem::exists(tempDir))
                {
                    logger->log(Logger::LogLevel::LOG_INFO,
                                std::format(L"Cleaning up temporary directory: {}", tempDir.wstring( )));

                    try
                    {
                        std::filesystem::remove_all(tempDir);
                        logger->log(Logger::LogLevel::LOG_INFO, L"Temporary directory cleaned up successfully.");
                    }
                    catch (const std::exception& e)
                    {
                        logger->log(Logger::LogLevel::LOG_WARNING,
                                    std::format(L"Failed to clean up temporary directory: {}",
                                                std::wstring(e.what( ), e.what( ) + strlen(e.what( )))));
                        // Continue execution as this is not critical
                    }
                }

                if (programFileSuccess || programDataSuccess)
                {
                    logger->log(Logger::LogLevel::LOG_INFO, L"Configuration files successfully restored from temporary location.");
                    return ERROR_SUCCESS;
                }
                else
                {
                    logger->log(Logger::LogLevel::LOG_WARNING, L"No configuration files were found to restore.");
                    return ERROR_SUCCESS; // Still return success as this is not a critical error
                }
            }
            catch (const std::exception& e)
            {
                logger->log(Logger::LogLevel::LOG_ERROR,
                            std::format(L"Exception in RestoreConfigFiles: {}",
                                        std::wstring(e.what( ), e.what( ) + strlen(e.what( )))));
                return ERROR_INSTALL_FAILURE;
            }
            catch (...)
            {
                logger->log(Logger::LogLevel::LOG_ERROR, L"Unknown exception in RestoreConfigFiles");
                return ERROR_INSTALL_FAILURE;
            }
        }


    private:

        ConfigFileHandler( ) = delete;  // Prevents instantiation

        static bool CopyConfigFile(const std::filesystem::path& sourcePath,
                                   const std::filesystem::path& destPath,
                                   const std::wstring& fileDescription,
                                   std::shared_ptr<Logger::ILogger> logger)
        {

            if (!std::filesystem::exists(sourcePath))
            {
                logger->log(Logger::LogLevel::LOG_INFO,
                            std::format(L"{} configuration file doesn't exist: {}", fileDescription, sourcePath.wstring( )));
                return false;
            }

            logger->log(Logger::LogLevel::LOG_INFO,
                        std::format(L"Copying {} to {}", sourcePath.wstring( ), destPath.wstring( )));

            try
            {
                std::filesystem::copy_file(sourcePath, destPath,
                                           std::filesystem::copy_options::overwrite_existing);

                logger->log(Logger::LogLevel::LOG_INFO,
                            std::format(L"{} configuration file copied successfully.", fileDescription));
                return true;
            }
            catch (const std::exception& e)
            {
                logger->log(Logger::LogLevel::LOG_ERROR,
                            std::format(L"Failed to copy {} config file: {}",
                                        fileDescription,
                                        std::wstring(e.what( ), e.what( ) + strlen(e.what( )))));
                return false;
            }
        }


        static void EnsureDirectoryExists(const std::filesystem::path& directory, std::shared_ptr<Logger::ILogger> logger)
        {
            if (!std::filesystem::exists(directory))
            {
                logger->log(Logger::LogLevel::LOG_INFO,
                            std::format(L"Creating directory: {}", directory.wstring( )));
                std::filesystem::create_directories(directory);
            }
        }


        static bool RestoreConfigFile(const std::filesystem::path& sourcePath,
                                      const std::filesystem::path& destDir,
                                      const std::wstring_view& destFileName,
                                      const std::wstring& fileDescription,
                                      std::shared_ptr<Logger::ILogger> logger)
        {
            if (!std::filesystem::exists(sourcePath))
            {
                return false;
            }

            const auto destPath = destDir / destFileName;
            logger->log(Logger::LogLevel::LOG_INFO,
                        std::format(L"Restoring {} to {}", sourcePath.wstring( ), destPath.wstring( )));

            try
            {
                std::filesystem::copy_file(sourcePath, destPath,
                                           std::filesystem::copy_options::overwrite_existing);

                logger->log(Logger::LogLevel::LOG_INFO,
                            std::format(L"{} configuration file restored successfully.", fileDescription));
                return true;
            }
            catch (const std::exception& e)
            {
                logger->log(Logger::LogLevel::LOG_ERROR,
                            std::format(L"Failed to restore {} config file: {}",
                                        fileDescription,
                                        std::wstring(e.what( ), e.what( ) + strlen(e.what( )))));
                return false;
            }
        }


        static bool CreateConfigFromContent(const std::wstring& content, std::shared_ptr<Logger::ILogger> logger)
        {
            try
            {
                logger->log(Logger::LogLevel::LOG_INFO,
                            std::format(L"Creating {} file...", Constants::ConfigConstants::DEFAULT_CONFIG_FILE_NAME));

                // Create directory if it doesn't exist
                auto destDir = Constants::ConfigConstants::CONFIG_PROGRAM_FILES_DESTINATION;
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
                                std::format(L"Failed to open file for writing: {}", destPath.wstring( )));
                    return false;
                }

                // Convert wstring to UTF-8
                std::string utf8Content;
                int size_needed = WideCharToMultiByte(CP_UTF8, 0, content.data( ), -1, NULL, 0, NULL, NULL);
                utf8Content.resize(size_needed);
                WideCharToMultiByte(CP_UTF8, 0, content.data( ), -1, &utf8Content[0], size_needed, NULL, NULL);

                // Write to file
                outFile.write(utf8Content.data( ), utf8Content.size( ) - 1);  // -1 to exclude null terminator
                outFile.flush( );
                outFile.close( );

                logger->log(Logger::LogLevel::LOG_INFO,
                            std::format(L"{} created successfully.", Constants::ConfigConstants::DEFAULT_CONFIG_FILE_NAME));
                return true;
            }
            catch (const std::exception& e)
            {
                logger->log(Logger::LogLevel::LOG_ERROR,
                            std::format(L"Exception in CreateConfigFromContent: {}",
                                        std::wstring(e.what( ), e.what( ) + strlen(e.what( )))));
                return false;
            }
        }


        static bool SaveConfigFrom(const std::wstring& configPath, std::shared_ptr<Logger::ILogger> logger)
        {
            try
            {
                // Normalize source path
                std::wstring sourcePath = NormalizeSourcePath(configPath, logger);

                // Create destination directory if it doesn't exist
                auto destDir = Constants::ConfigConstants::CONFIG_PROGRAM_FILES_DESTINATION;
                if (!std::filesystem::exists(destDir))
                {
                    logger->log(Logger::LogLevel::LOG_INFO, L"Creating destination directory...");
                    std::filesystem::create_directories(destDir);
                }

                // Create destination path
                auto destPath = destDir / Constants::ConfigConstants::DEFAULT_CONFIG_FILE_NAME;

                logger->log(Logger::LogLevel::LOG_INFO,
                            std::format(L"Copying configuration file to destination.\n      Source: {}\n      Destination: {}",
                                        sourcePath, destPath.wstring( )));

                // Copy file
                std::filesystem::copy_file(sourcePath, destPath, std::filesystem::copy_options::overwrite_existing);

                logger->log(Logger::LogLevel::LOG_INFO,
                            std::format(L"{} copied successfully.", Constants::ConfigConstants::DEFAULT_CONFIG_FILE_NAME));
                return true;
            }
            catch (const std::exception& e)
            {
                logger->log(Logger::LogLevel::LOG_ERROR,
                            std::format(L"Exception in SaveConfigFrom: {}",
                                        std::wstring(e.what( ), e.what( ) + strlen(e.what( )))));
                return false;
            }
        }


        static std::wstring NormalizeSourcePath(const std::wstring& configPath, std::shared_ptr<Logger::ILogger> logger)
        {
            try
            {
                // First, normalize the path to an absolute path
                std::filesystem::path path(configPath);
                path = std::filesystem::absolute(path);

                // Dispatch to appropriate handler based on path existence
                if (std::filesystem::exists(path))
                {
                    return NormalizeExistingPath(path, logger);
                }
                else
                {
                    logger->log(Logger::LogLevel::LOG_INFO,
                                std::format(L"Path {} doesn't exist.", path.wstring( )));
                }
            }
            catch (const std::exception& e)
            {
                // In case of error, log and return the original path
                logger->log(Logger::LogLevel::LOG_ERROR,
                            std::format(L"Error in NormalizeSourcePath: {}",
                                        std::wstring(e.what( ), e.what( ) + strlen(e.what( )))));

                return configPath;
            }
        }


        static std::wstring NormalizeExistingPath(const std::filesystem::path& path, std::shared_ptr<Logger::ILogger> logger)
        {
            // If the path exists, check if it's a directory
            if (std::filesystem::is_directory(path))
            {
                // It's a directory, so append the default file name
                auto updatedPath = path / Constants::ConfigConstants::DEFAULT_CONFIG_FILE_NAME;

                logger->log(Logger::LogLevel::LOG_INFO,
                            std::format(L"Path is an existing directory. Updating CONFIG_PATH.\n      From: {}\n      To: {}",
                                        path.wstring( ), updatedPath.wstring( )));

                return updatedPath.wstring( );
            }
            else
            {
                // It's a file, return without modification
                logger->log(Logger::LogLevel::LOG_INFO,
                            std::format(L"Path is an existing file. Using as is: {}", path.wstring( )));

                return path.wstring( );
            }
        }


        static UINT setMsiProperty(MSIHANDLE hInstall, const std::wstring& propertyName,
                                   OPENFILENAMEW& ofn, std::shared_ptr<WinLogon::CustomActions::Logger::ILogger>& logger)
        {
            std::wstring filePath(ofn.lpstrFile);

            delete[] ofn.lpstrFile;


            if (UINT result = MsiSetPropertyW(hInstall, propertyName.c_str( ), filePath.c_str( ));
                result != ERROR_SUCCESS)
            {
                logger->log(Logger::LogLevel::LOG_ERROR,
                            std::format(L"Failed to set MSI property: {}", propertyName));
                return ERROR_INSTALL_FAILURE;
            }

            logger->log(Logger::LogLevel::LOG_INFO,
                        std::format(L"Set MSI property {} successfully with value: {}", propertyName, filePath));

            return ERROR_SUCCESS;
        }


        static void createSearchFileDialog(OPENFILENAMEW& ofn, std::wstring_view const& initialDir)
        {
            wchar_t* szFile = new wchar_t[MAX_PATH];
            ZeroMemory(szFile, MAX_PATH * sizeof(wchar_t));

            // Initialize OPENFILENAME
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = NULL;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = MAX_PATH; // Number of chars, not bytes.
            ofn.lpstrFilter = L"Configuration Files (*.cfg)\0*.cfg\0All Files (*.*)\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = initialDir.empty( ) ? NULL : initialDir.data( );
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        }


        static std::wstring_view getMsiDir(MSIHANDLE hInstall, std::shared_ptr<WinLogon::CustomActions::Logger::ILogger>& logger)
        {
            std::wstring initialDir{ };

            // Get the MSI path
            WCHAR msiPath[MAX_PATH] = { 0 };
            DWORD msiPathSize = sizeof(msiPath) / sizeof(msiPath[0]);
            UINT result = MsiGetPropertyW(hInstall, L"OriginalDatabase", msiPath, &msiPathSize);

            if (result == ERROR_SUCCESS && msiPath[0] != L'\0')
            {
                std::filesystem::path msiFilePath(msiPath);
                initialDir = msiFilePath.parent_path( ).wstring( );
                logger->log(Logger::LogLevel::LOG_INFO,
                            std::format(L"Using MSI directory as initial dir: {}", initialDir));
            }

            return initialDir.data();
        }
    };
}