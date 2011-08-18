
/**
 * libemulation
 * Control bus
 * (C) 2010-2011 by Marc S. Ressl (mressl@umich.edu)
 * Released under the GPL
 *
 * Implements a control bus with clock control and reset/IRQ/NMI lines
 */

#include <math.h>

#include "ControlBus.h"

#include "DeviceInterface.h"
#include "AudioInterface.h"
#include "CPUInterface.h"

ControlBus::ControlBus()
{
	clockFrequency = 1E6;
    cpuSpeedMultiplier = 1;
	resetOnPowerOn = false;
	powerState = CONTROLBUS_POWERSTATE_OFF;
    
	device = NULL;
	audio = NULL;
	cpu = NULL;
	cpuSocket = NULL;
	
	resetCount = 0;
	irqCount = 0;
	nmiCount = 0;
    
    clock = 0;
    clockRemainder = 0;
}

bool ControlBus::setValue(string name, string value)
{
	if (name == "clockFrequency")
		clockFrequency = getFloat(value);
	else if (name == "cpuSpeedMultiplier")
		cpuSpeedMultiplier = getFloat(value);
	else if (name == "resetOnPowerOn")
		resetOnPowerOn = getInt(value);
    else if (name == "powerState")
        powerState = (ControlBusPowerState)getInt(value);
	else
		return false;
	
	return true;
}

bool ControlBus::getValue(string name, string &value)
{
    if (name == "powerState")
        value = getString(powerState);
    else
        return false;
    
    return true;
}

bool ControlBus::setRef(string name, OEComponent *ref)
{
	if (name == "device")
	{
		if (device)
		{
            if (powerState == CONTROLBUS_POWERSTATE_ON)
                device->postMessage(this, DEVICE_CLEAR_ACTIVITY, NULL);
			device->removeObserver(this, DEVICE_EVENT_DID_OCCUR);
		}
		device = ref;
		if (device)
		{
            if (powerState == CONTROLBUS_POWERSTATE_ON)
                device->postMessage(this, DEVICE_ASSERT_ACTIVITY, NULL);
			device->addObserver(this, DEVICE_EVENT_DID_OCCUR);
		}
	}
	else if (name == "audio")
	{
        if (audio)
            audio->removeObserver(this, AUDIO_FRAME_IS_RENDERING);
		audio = ref;
        if (audio)
            audio->addObserver(this, AUDIO_FRAME_IS_RENDERING);
	}
	else if (name == "cpu")
		cpu = ref;
	else if (name == "cpuSocket")
		cpuSocket = ref;
	else
		return false;
	
	return true;
}

bool ControlBus::init()
{
	if (!device)
	{
		logMessage("device not connected");
		return false;
	}
	
	if (!audio)
	{
		logMessage("audio not connected");
		return false;
	}
	
    if (!cpu)
    {
        logMessage("cpu not connected");
        return false;
    }
    
    setPowerState(powerState);
    if (powerState == CONTROLBUS_POWERSTATE_ON)
        device->postMessage(this, DEVICE_ASSERT_ACTIVITY, NULL);
    
	return true;
}

bool ControlBus::postMessage(OEComponent *sender, int message, void *data)
{
	switch (message)
	{
		case CONTROLBUS_SET_POWERSTATE:
            setPowerState(*((ControlBusPowerState *)data));
			return true;
            
		case CONTROLBUS_GET_POWERSTATE:
			*((ControlBusPowerState *)data) = powerState;
			return true;
			
		case CONTROLBUS_ASSERT_RESET:
			resetCount++;
			if (resetCount == 1)
				OEComponent::notify(this, CONTROLBUS_RESET_DID_ASSERT, NULL);
			return true;
			
		case CONTROLBUS_CLEAR_RESET:
			resetCount--;
			if (resetCount == 0)
				OEComponent::notify(this, CONTROLBUS_RESET_DID_CLEAR, NULL);
			return true;
			
		case CONTROLBUS_ASSERT_IRQ:
			irqCount++;
			if (irqCount == 1)
				OEComponent::notify(this, CONTROLBUS_IRQ_DID_ASSERT, NULL);
			return true;
			
		case CONTROLBUS_CLEAR_IRQ:
			irqCount--;
			if (irqCount == 0)
				OEComponent::notify(this, CONTROLBUS_IRQ_DID_CLEAR, NULL);
			return true;
			
		case CONTROLBUS_ASSERT_NMI:
			nmiCount++;
			if (nmiCount == 1)
				OEComponent::notify(this, CONTROLBUS_NMI_DID_ASSERT, NULL);
			return true;
			
		case CONTROLBUS_CLEAR_NMI:
			nmiCount--;
			if (nmiCount == 0)
				OEComponent::notify(this, CONTROLBUS_NMI_DID_ASSERT, NULL);
			return true;
			
		case CONTROLBUS_SCHEDULE_TIMER:
            scheduleTimer(sender, *((OEUInt64 *)data));
			return true;
			
		case CONTROLBUS_CLEAR_TIMERS:
            clearTimers(sender);
			return true;
			
		case CONTROLBUS_GET_CLOCK:
            // To-Do
			return true;
			
		case CONTROLBUS_SKIP_CLOCKS:
            // To-Do
			return true;
			
		case CONTROLBUS_GET_AUDIOBUFFERINDEX:
            // To-Do
			return true;
	}
	
	return false;
}

void ControlBus::notify(OEComponent *sender, int notification, void *data)
{
    if (sender == audio)
    {
        AudioBuffer *buffer = (AudioBuffer *)data;
        
        scheduleTimer(NULL, clockFrequency * buffer->frameNum / buffer->sampleRate);
        
        while (1)
        {
            float accurateCPUClocks = clockRemainder + events.front().clocks * cpuSpeedMultiplier;
            OEUInt64 cpuClocks = accurateCPUClocks;
            clockRemainder = accurateCPUClocks - cpuClocks;
            cpu->postMessage(this, CPU_RUN, &cpuClocks);
            
            ControlBusEvent *event = &events.front();
            clock += event->clocks;
            OEComponent *component = event->component;
            events.pop_front();
            
            if (!component)
                break;
            
            component->notify(this, CONTROLBUS_TIMER_DID_FIRE, NULL);
        }
    }
    else if (sender == device)
    {
        switch (*((int *)data))
        {
            case DEVICE_POWERDOWN:
                setPowerState(CONTROLBUS_POWERSTATE_OFF);
                break;
                
            case DEVICE_SLEEP:
                if (powerState != CONTROLBUS_POWERSTATE_OFF)
                    setPowerState(CONTROLBUS_POWERSTATE_PAUSE);
                break;
                
            case DEVICE_WAKEUP:
                setPowerState(CONTROLBUS_POWERSTATE_ON);
                break;
                
            case DEVICE_COLDRESTART:
                setPowerState(CONTROLBUS_POWERSTATE_OFF);
                setPowerState(CONTROLBUS_POWERSTATE_ON);
                break;
                
            case DEVICE_WARMRESTART:
                postMessage(this, CONTROLBUS_ASSERT_RESET, NULL);
                postMessage(this, CONTROLBUS_CLEAR_RESET, NULL);
                break;
                
            case DEVICE_DEBUGGERBREAK:
                postMessage(this, CONTROLBUS_ASSERT_NMI, NULL);
                postMessage(this, CONTROLBUS_CLEAR_NMI, NULL);
                break;
        }
    }
}

void ControlBus::setPowerState(ControlBusPowerState powerState)
{
    ControlBusPowerState lastPowerState = this->powerState;
    
    this->powerState = powerState;
    
    string stateLabel;
    switch (powerState)
    {
        case CONTROLBUS_POWERSTATE_OFF:
            stateLabel = "Powered Off";
            break;
            
        case CONTROLBUS_POWERSTATE_HIBERNATE:
            stateLabel = "Hibernated";
            break;
            
        case CONTROLBUS_POWERSTATE_SLEEP:
            stateLabel = "Sleeping";
            break;
            
        case CONTROLBUS_POWERSTATE_STANDBY:
            stateLabel = "Stand By";
            break;
            
        case CONTROLBUS_POWERSTATE_PAUSE:
            stateLabel = "Paused";
            break;
            
        case CONTROLBUS_POWERSTATE_ON:
            stateLabel = "Powered On";
            break;
            
        default:
            stateLabel = "Unknown";
            break;
    }
    device->postMessage(this, DEVICE_SET_STATELABEL, &stateLabel);
    device->postMessage(this, DEVICE_UPDATE, NULL);
    
    if ((lastPowerState == CONTROLBUS_POWERSTATE_ON) &&
        (powerState != CONTROLBUS_POWERSTATE_ON))
        device->postMessage(this, DEVICE_CLEAR_ACTIVITY, NULL);
    else if ((lastPowerState != CONTROLBUS_POWERSTATE_ON) &&
            (powerState == CONTROLBUS_POWERSTATE_ON))
        device->postMessage(this, DEVICE_ASSERT_ACTIVITY, NULL);
    
    if (lastPowerState != powerState)
        OEComponent::notify(this, CONTROLBUS_POWERSTATE_DID_CHANGE, &powerState);
    
    if (resetOnPowerOn &&
        (lastPowerState != CONTROLBUS_POWERSTATE_ON) &&
        (powerState == CONTROLBUS_POWERSTATE_ON))
    {
        postMessage(this, CONTROLBUS_ASSERT_RESET, NULL);
        postMessage(this, CONTROLBUS_CLEAR_RESET, NULL);
    }
}

void ControlBus::scheduleTimer(OEComponent *component, OEUInt64 clocks)
{
    OEUInt64 cpuClocks;
    cpu->postMessage(this, CPU_GET_CLOCKS, &cpuClocks);
    events.front().clocks -= cpuClocks / cpuSpeedMultiplier;
    
    list<ControlBusEvent>::iterator i;
    for (i = events.begin();
         i != events.end();
         i++)
    {
        if (clocks < i->clocks)
        {
            i->clocks -= clocks;
            
            if (i == events.begin())
            {
                cpuClocks = clocks * cpuSpeedMultiplier;
                cpu->postMessage(this, CPU_SET_CLOCKS, &cpuClocks);
            }
            
            break;
        }
        
        clocks -= i->clocks;
    }
    
    ControlBusEvent event;
    
    event.clocks = clocks;
    event.component = component;
    
    events.insert(i, event);
}

void ControlBus::clearTimers(OEComponent *component)
{
    list<ControlBusEvent>::iterator i;
    for (i = events.begin();
         i != events.end();
         i++)
    {
        if (i->component == component)
            i = events.erase(i);
    }
}
