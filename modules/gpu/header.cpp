#include "header.h"
#include <stdexcept>

namespace eokas
{
    void PipelineLayout::add(const PipelineLayoutEntry& entry)
    {
        if (const PipelineLayoutEntry* existing = findByName(entry.type, entry.name))
        {
            if (existing->slot != entry.slot)
            {
                throw std::runtime_error("PipelineLayout: name maps to different slots.");
            }
            return;
        }
        entries.push_back(entry);
    }

    const PipelineLayoutEntry* PipelineLayout::findBySlot(PipelineResourceType type, uint32_t slot) const
    {
        for (const auto& entry : entries)
        {
            if (entry.type == type && entry.slot == slot) return &entry;
        }
        return nullptr;
    }

    const PipelineLayoutEntry* PipelineLayout::findByName(PipelineResourceType type, const std::string& name) const
    {
        for (const auto& entry : entries)
        {
            if (entry.type == type && entry.name == name) return &entry;
        }
        return nullptr;
    }

    bool PipelineLayout::compatibleWith(const PipelineLayout& other) const
    {
        for (const auto& entry : other.entries)
        {
            const PipelineLayoutEntry* found = findBySlot(entry.type, entry.slot);
            if (!found || found->name != entry.name || found->count != entry.count)
            {
                return false;
            }
        }
        return true;
    }
}
