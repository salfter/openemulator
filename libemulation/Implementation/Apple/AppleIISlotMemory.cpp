
/**
 * libemulator
 * Apple II Slot Memory
 * (C) 2010-2012 by Marc S. Ressl (mressl@umich.edu)
 * Released under the GPL
 *
 * Controls Apple II slot memory ($C100-$C7FF)
 */

#include "AppleIISlotMemory.h"

#include "MemoryInterface.h"

#include "AppleIIInterface.h"

AppleIISlotMemory::AppleIISlotMemory()
{
    en = false;
    
    slotMemory = NULL;
    slotExpansionMemory = NULL;
}

bool AppleIISlotMemory::setValue(string name, string value)
{
	if (name == "en")
		en = getOEInt(value);
	else
		return false;
    
	return true;
}

bool AppleIISlotMemory::getValue(string name, string& value)
{
	if (name == "en")
        value = getString(en);
	else
		return false;
    
    return true;
}

bool AppleIISlotMemory::setRef(string name, OEComponent *ref)
{
	if (name == "mmu")
    {
        if (mmu)
            mmu->removeObserver(this, APPLEII_SLOTEXPANSIONMEMORY_WILL_UNMAP);
		mmu = ref;
        if (mmu)
            mmu->addObserver(this, APPLEII_SLOTEXPANSIONMEMORY_WILL_UNMAP);
    }
	else if (name == "slotMemory")
		slotMemory = ref;
	else if (name == "slotExpansionMemory")
		slotExpansionMemory = ref;
	else
		return false;
	
	return true;
}

bool AppleIISlotMemory::init()
{
    if (!slotMemory)
    {
        logMessage("slotMemory not connected");
        
        return false;
    }
    
    if (en)
    {
    }
    
    return true;
}

void AppleIISlotMemory::notify(OEComponent *sender, int notification, void *data)
{
    if ((sender == mmu) && en)
    {
        en = false;
        
        mapMemory(APPLEII_UNMAP_SLOTMEMORYMAPS);
    }
}

OEChar AppleIISlotMemory::read(OEAddress address)
{
    if (!en)
    {
        en = true;
        
        mapMemory(APPLEII_MAP_SLOTMEMORYMAPS);
    }
    
    return slotMemory->read(address);
}

void AppleIISlotMemory::write(OEAddress address, OEChar value)
{
    if (!en)
    {
        en = true;
        
        mapMemory(APPLEII_MAP_SLOTMEMORYMAPS);
    }
    
    slotMemory->write(address, value);
}

void AppleIISlotMemory::mapMemory(int message)
{
    MemoryMap memoryMap;
    
    memoryMap.component = slotExpansionMemory;
    memoryMap.startAddress = 0xc800;
    memoryMap.endAddress = 0xcfff;
    memoryMap.read = true;
    memoryMap.write = true;
    
    mmu->postMessage(this, message, &memoryMap);
}
