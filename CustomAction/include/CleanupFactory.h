#pragma once

#include <Windows.h>
#include <msi.h>
#include <memory>

#include "CleanupManager.h"
#include "V3FilesCleanupStrategy.h"
#include "V4FilesCleanupStrategy.h"
#include "RegistryEntriesCleanupStrategy.h"

namespace WinLogon::CustomActions::Cleanup
{
    class CleanupFactory
    {
    public:

        template<typename... StrategyTypes>
        static std::unique_ptr<CleanupManager> createManager(MSIHANDLE handle)
        {
            auto manager = std::make_unique<CleanupManager>(handle);
            (addStrategyToManager<StrategyTypes>(manager), ...);
            return manager;
        }

        static std::unique_ptr<CleanupManager> createV3CleanupManager(MSIHANDLE handle)
        {
            return createManager<Strategies::V3FilesCleanupStrategy>(handle);
        }

        static std::unique_ptr<CleanupManager> createV4CleanupManager(MSIHANDLE handle)
        {
            return createManager<Strategies::V4FilesCleanupStrategy>(handle);
        }

        static std::unique_ptr<CleanupManager> createRegistryCleanupManager(MSIHANDLE handle)
        {
            return createManager<Strategies::RegistryEntriesCleanupStrategy>(handle);
        }

        // Novo método que permite criar um manager com múltiplas estratégias
        static std::unique_ptr<CleanupManager> createFullCleanupManager(MSIHANDLE handle)
        {
            return createManager<
                Strategies::V3FilesCleanupStrategy,
                Strategies::V4FilesCleanupStrategy,
                Strategies::RegistryEntriesCleanupStrategy
            > (handle);
        }

    private:
        CleanupFactory( ) = delete;  // Previne instanciação

        template<typename StrategyType>
        static void addStrategyToManager(std::unique_ptr<CleanupManager> const& manager)
        {
            manager->addStrategy(std::make_unique<StrategyType>( ));
        }
    };
}