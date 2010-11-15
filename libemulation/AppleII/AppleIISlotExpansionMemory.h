
/**
 * libemulator
 * Apple II Slot Expansion Memory
 * (C) 2010 by Marc S. Ressl (mressl@umich.edu)
 * Released under the GPL
 *
 * Controls Apple II's slot expansion memory range ($C800-$CFFF).
 */

#include "OEComponent.h"

// Events
enum
{
	APPLEIISLOTEXPANSIONMEMORY_SET_SLOT,
};

class AppleIISlotExpansionMemory : public OEComponent
{
public:
	bool setRef(string name, OEComponent *ref);
	
	OEUInt8 read(OEAddress address);
	void write(OEAddress address, OEUInt8 value);
	
private:
	OEComponent *floatingBus;
	OEComponent *slot;
};
