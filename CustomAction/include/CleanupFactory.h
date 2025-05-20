#pragma once

#include <Windows.h>
#include <msi.h>
#include <memory>

#include "CleanupManager.h"
#include "V3FilesCleanupStrategy.h"
#include "V4FilesCleanupStrategy.h"
#include "RegistryEntriesCleanupStrategy.h"
#include "AuthPointRegistryCleanupStrategy.h"

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

        static std::unique_ptr<CleanupManager> createAuthPointRegistryCleanupManager(MSIHANDLE handle)
        {
            return createManager<Strategies::AuthPointRegistryCleanupStrategy>(handle);
        }

        static std::unique_ptr<CleanupManager> createFullCleanupManager(MSIHANDLE handle)
        {
            return createManager<
                Strategies::V3FilesCleanupStrategy,
                Strategies::V4FilesCleanupStrategy,
                Strategies::RegistryEntriesCleanupStrategy,
                Strategies::AuthPointRegistryCleanupStrategy
            >(handle);
        }

    private:
        CleanupFactory( ) = delete;  // Prevent initialization

        template<typename StrategyType>
        static void addStrategyToManager(std::unique_ptr<CleanupManager> const& manager)
        {
            manager->addStrategy(std::make_unique<StrategyType>( ));
        }
    };
}