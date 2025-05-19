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
        static std::unique_ptr<CleanupManager> createV3CleanupManager(MSIHANDLE handle)
        {
            auto manager = std::make_unique<CleanupManager>(handle);
            manager->addStrategy(std::make_unique<Strategies::V3FilesCleanupStrategy>());
            return manager;
        }

        static std::unique_ptr<CleanupManager> createV4CleanupManager(MSIHANDLE handle)
        {
            auto manager = std::make_unique<CleanupManager>(handle);
            manager->addStrategy(std::make_unique<Strategies::V4FilesCleanupStrategy>());
            return manager;
        }

        static std::unique_ptr<CleanupManager> createRegistryCleanupManager(MSIHANDLE handle)
        {
            auto manager = std::make_unique<CleanupManager>(handle);
            manager->addStrategy(std::make_unique<Strategies::RegistryEntriesCleanupStrategy>());
            return manager;
        }

    private:
        CleanupFactory() = delete;  // Previne instanciação
    };
}