#include <stdio.h>
#include <iostream>
#include <windows.h>
#include <fstream>
#include <ctime>
#include <string>
#include <Msi.h>

#include <ConsoleLogger.h>
#include "LoggerFactory.h"
#include "CleanupFactory.h"

static const bool IsRunAsAdmin( )
{
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup))
    {
        if (!CheckTokenMembership(nullptr, adminGroup, &isAdmin))
        {
            isAdmin = FALSE;
        }
        FreeSid(adminGroup);
    }
    return isAdmin != 0;
}

class ConsoleProgressDisplay
{
public:
    ConsoleProgressDisplay( )
    {
        std::cout << "=========================================" << std::endl;
        std::cout << "           Uninstaller Tool              " << std::endl;
        std::cout << "=========================================" << std::endl;
    }

    void onTaskCompleted(const std::wstring& taskName, bool success)
    {
        std::wcout << (success ? L"[SUCCESS] " : L"[FAILED] ") << taskName << std::endl << std::endl;
    }

    ~ConsoleProgressDisplay( )
    {
        std::cout << "=========================================" << std::endl;
        std::cout << "           Cleanup Complete              " << std::endl;
        std::cout << "=========================================" << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
    }
};


// Função principal
int main( )
{

using namespace WinLogon::CustomActions;

    if (!IsRunAsAdmin( ))
    {
        std::cout << "[ERROR] " << "This program requires administrator privileges." << std::endl;
        std::cout << "[ERROR] " << "Please run as administrator and try again." << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get( );
        return 1;
    }

    // Initialize the MSI handle with a null pointer, since we're not using it in the EXE application
    MSIHANDLE hInstall{};

    // Criar um display de progresso para o console
    ConsoleProgressDisplay progressDisplay;

    // Criar o logger
    auto logger = Logger::LoggerFactory::createLogger(hInstall);

    try
    {
        // Executar limpeza de arquivos V3
        logger->log(Logger::LogLevel::LOG_INFO, L"Executing V3 files cleanup...");
        auto v3CleanupManager = Cleanup::CleanupFactory::createV3CleanupManager(hInstall);
        bool v3Success = v3CleanupManager->executeAll( );
        progressDisplay.onTaskCompleted(L"V3 Files Cleanup", v3Success);

        // Executar limpeza de arquivos V4
        logger->log(Logger::LogLevel::LOG_INFO, L"Executing V4 files cleanup...");
        auto v4CleanupManager = Cleanup::CleanupFactory::createV4CleanupManager(hInstall);
        bool v4Success = v4CleanupManager->executeAll( );
        progressDisplay.onTaskCompleted(L"V4 Files Cleanup", v4Success);

        // Executar limpeza de registros
        logger->log(Logger::LogLevel::LOG_INFO, L"Executing registry cleanup...");
        auto registryCleanupManager = Cleanup::CleanupFactory::createRegistryCleanupManager(hInstall);
        bool registrySuccess = registryCleanupManager->executeAll( );
        progressDisplay.onTaskCompleted(L"Registry Cleanup", registrySuccess);

        // Alternativa: usar o gerenciador para cada estratégia sequencialmente
        /*
        logger->log(Logger::LogLevel::LOG_INFO, L"Executing all cleanup strategies...");

        // Criar os gerenciadores
        auto v3CleanupManager = Cleanup::CleanupFactory::createV3CleanupManager(hInstall);
        auto v4CleanupManager = Cleanup::CleanupFactory::createV4CleanupManager(hInstall);
        auto registryCleanupManager = Cleanup::CleanupFactory::createRegistryCleanupManager(hInstall);

        // Executar cada estratégia
        bool v3Success = v3CleanupManager->executeAll();
        bool v4Success = v4CleanupManager->executeAll();
        bool registrySuccess = registryCleanupManager->executeAll();

        // Verificar sucesso geral
        bool overallSuccess = v3Success && v4Success && registrySuccess;

        progressDisplay.onTaskCompleted(L"Complete System Cleanup", overallSuccess);
        */
    }
    catch (const std::exception& e)
    {
        logger->log(Logger::LogLevel::LOG_ERROR,
                    L"Exception occurred: " + std::wstring(e.what( ), e.what( ) + strlen(e.what( ))));
        std::cerr << "Error: " << e.what( ) << std::endl;
    }
    catch (...)
    {
        logger->log(Logger::LogLevel::LOG_ERROR, L"Unknown exception occurred");
        std::cerr << "Unknown error occurred" << std::endl;
    }

    std::cin.get( );
    return 0;
}