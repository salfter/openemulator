
/**
 * libemulator
 * Generic RAM
 * (C) 2010 by Marc S. Ressl (mressl@umich.edu)
 * Released under the GPL
 *
 * Controls a generic RAM segment
 */

#include "RAM.h"

RAM::RAM()
{
	size = 1;
	mask = 0;
	
	memory.resize(size);
	resetPattern.resize(size);
	resetPattern[0] = 0;
}

bool RAM::setProperty(string name, string &value)
{
	if (name == "map")
		mappedRange = value;
	else if (name == "size")
	{
		size = getLowerPowerOf2(getInt(value));
		if (size < 1)
			size = 1;
		memory.resize(size);
		mask = size - 1;
	}
	else if (name == "resetPattern")
		resetPattern = getCharVector(value);
	else
		return false;
	
	return true;
}

bool RAM::setData(string name, OEData &data)
{
	if (name == "image")
	{
		memory = data;
		memory.resize(size);
	}
	else
		return false;
	
	return true;
}

bool RAM::getData(string name, OEData &data)
{
	if (name == "image")
		data = memory;
	else
		return false;
	
	return true;
}

bool RAM::connect(string name, OEComponent *component)
{
	return false;
}

void RAM::notify(int notification, OEComponent *component)
{
	/*			OENotification *notification = (OENotification *) data;
	 if (notification->message == HOSTSYSTEM_RESET)
	 {
	 for (int i = 0; i < memory.size(); i++)
	 memory[i] = resetPattern[i % resetPattern.size()];
	 }
	 */			
}

int RAM::read(int address)
{
	return memory[address & mask];
}

void RAM::write(int address, int value)
{
	memory[address & mask] = value;
}
