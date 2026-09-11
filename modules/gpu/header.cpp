#include "header.h"
#include <stdexcept>

namespace eokas
{
    void ProgramParameterMap::add(const ProgramParameterEntry& entry)
    {
        if (const ProgramParameterEntry* existing = findByName(entry.type, entry.name))
        {
            if (existing->slot != entry.slot)
            {
                throw std::runtime_error("ProgramParameterMap: name maps to different slots.");
            }
            return;
        }
        entries.push_back(entry);
    }

    const ProgramParameterEntry* ProgramParameterMap::findBySlot(ProgramParameterType type, uint32_t slot) const
    {
        for (const auto& entry : entries)
        {
            if (entry.type == type && entry.slot == slot) return &entry;
        }
        return nullptr;
    }

    const ProgramParameterEntry* ProgramParameterMap::findByName(ProgramParameterType type, const std::string& name) const
    {
        for (const auto& entry : entries)
        {
            if (entry.type == type && entry.name == name) return &entry;
        }
        return nullptr;
    }
}
