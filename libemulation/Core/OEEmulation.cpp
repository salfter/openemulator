
/**
 * libemulation
 * OEEmulation
 * (C) 2009-2011 by Marc S. Ressl (mressl@umich.edu)
 * Released under the GPL
 *
 * Controls an emulation.
 */

#include <fstream>
#include <sstream>

#include <libxml/parser.h>

#include "OEEmulation.h"
#include "OEDevice.h"

#include "OEComponentFactory.h"

OEEmulation::OEEmulation() : OEEDL()
{
	createCanvas = NULL;
	destroyCanvas = NULL;
	didUpdate = NULL;
	
	activityCount = 0;
	
	addComponent("emulation", this);
}

OEEmulation::~OEEmulation()
{
	close();
	
	didUpdate = NULL;
	
	disconnectEmulation();
	destroyEmulation();
}

void OEEmulation::setResourcePath(string path)
{
	resourcePath = path;
}

void OEEmulation::setCreateCanvas(EmulationCreateCanvas createCanvas)
{
	this->createCanvas = createCanvas;
}

void OEEmulation::setDestroyCanvas(EmulationDestroyCanvas destroyCanvas)
{
	this->destroyCanvas = destroyCanvas;
}

void OEEmulation::setDidUpdate(EmulationDidUpdate didUpdate)
{
	this->didUpdate = didUpdate;
}

void OEEmulation::setUserData(void *userData)
{
	this->userData = userData;
}

bool OEEmulation::open(string path)
{
	if (!OEEDL::open(path))
		return false;
	
	if (createEmulation())
		if (configureEmulation())
			if (initEmulation())
				return true;
	
	close();
	
	return false;
}

bool OEEmulation::addComponent(string id, OEComponent *component)
{
	if (!component)
		return false;
	
	if (componentsMap.count(id))
		return false;
	
	componentsMap[id] = component;
	
	return true;
}

bool OEEmulation::removeComponent(string id)
{
	if (componentsMap.count(id))
		return false;
	
	componentsMap.erase(id);
	
	return true;
}

OEComponent *OEEmulation::getComponent(string id)
{
	if (!componentsMap.count(id))
		return NULL;
	
	return componentsMap[id];
}

string OEEmulation::getId(OEComponent *component)
{
	for (map<string, OEComponent *>::iterator i = componentsMap.begin();
		 i != componentsMap.end();
		 i++)
	{
		if (i->second == component)
			return i->first;
	}
	
	return "";
}

bool OEEmulation::addEDL(string path, OEIdMap connectionsMap)
{
    if (!OEEDL::addEDL(path, connectionsMap))
        return false;
    
	return true;
}

bool OEEmulation::removeDevice(string id)
{
/*	for (OEIds::iterator i = devices.begin();
		 i != devices.end();
		 i++)
	{
		if (*i == id)
		{
			devices.erase(i);
			return true;
		}
	}
	
	return false;
	// Verify device exists
	if (!isDevice(deviceId))
	{
		log("could not find '" + deviceId + "'");
		return false;
	}
	
	// Recursively remove devices connected to this device
	if (!removeConnectedDevices(deviceId))
		return false;
	
	// Remove references to this device
	disconnectDevice(deviceId);
	
	// Remove elements matching this device
	removeElements(deviceId);
	*/
	return false;
}

bool OEEmulation::isActive()
{
	return (activityCount != 0);
}



bool OEEmulation::createEmulation()
{
	if (!doc)
		return false;
	
	xmlNodePtr rootNode = xmlDocGetRootElement(doc);
	
	for(xmlNodePtr node = rootNode->children;
		node;
		node = node->next)
	{
		if (!xmlStrcmp(node->name, BAD_CAST "device"))
		{
			string id = getNodeProperty(node, "id");
			
			if (!createDevice(id))
				return false;
		}
		if (!xmlStrcmp(node->name, BAD_CAST "component"))
		{
			string id = getNodeProperty(node, "id");
			string className = getNodeProperty(node, "class");
			
			if (!createComponent(id, className))
				return false;
		}
	}
	
	return true;
}

bool OEEmulation::createDevice(string id)
{
	if (!getComponent(id))
	{
		OEComponent *component;
		component = new OEDevice(this);
		
		if (component && addComponent(id, component))
			return true;
		else
			logMessage("could not create device '" + id + "'");
	}
	else
		logMessage("redefinition of '" + id + "'");
	
	return false;
}

bool OEEmulation::createComponent(string id, string className)
{
	if (!getComponent(id))
	{
		OEComponent *component;
		component = OEComponentFactory::create(className);
		
		if (component)
			return addComponent(id, component);
		else
			logMessage("could not create '" + id +
					   "', class '" + className + "' undefined");
	}
	else
		logMessage("redefinition of '" + id + "'");
	
	return false;
}

bool OEEmulation::configureEmulation()
{
	if (!doc)
		return false;
	
	xmlNodePtr rootNode = xmlDocGetRootElement(doc);
	
	for(xmlNodePtr node = rootNode->children;
		node;
		node = node->next)
	{
		if (!xmlStrcmp(node->name, BAD_CAST "device"))
		{
			string id = getNodeProperty(node, "id");
			string label = getNodeProperty(node, "label");
			string image = getNodeProperty(node, "image");
			string group = getNodeProperty(node, "group");
			
			if (!configureDevice(id, label, image, group, node->children))
				return false;
		}
		if (!xmlStrcmp(node->name, BAD_CAST "component"))
		{
			string id = getNodeProperty(node, "id");
			
			if (!configureComponent(id, node->children))
				return false;
		}
	}
	
	return true;
}

bool OEEmulation::configureDevice(string id,
								  string label, string image, string group, 
								  xmlNodePtr children)
{
	OEComponent *device = getComponent(id);
	string locationLabel = getLocationLabel(id);
	
	// Parse settings
	DeviceSettings settings;
	for(xmlNodePtr node = children;
		node;
		node = node->next)
	{
		if (!xmlStrcmp(node->name, BAD_CAST "setting"))
		{
			DeviceSetting setting;
			
			setting.ref = getNodeProperty(node, "ref");
			setting.name = getNodeProperty(node, "name");
			setting.type = getNodeProperty(node, "type");
			setting.options = getNodeProperty(node, "options");
			setting.label = getNodeProperty(node, "label");
			
			settings.push_back(setting);
		}
	}
	
	device->postMessage(this, DEVICE_SET_LABEL, &label);
	device->postMessage(this, DEVICE_SET_IMAGEPATH, &image);
	device->postMessage(this, DEVICE_SET_GROUP, &group);
	device->postMessage(this, DEVICE_SET_LOCATIONLABEL, &locationLabel);
	device->postMessage(this, DEVICE_SET_SETTINGS, &settings);
	
	return true;
}

bool OEEmulation::configureComponent(string id, xmlNodePtr children)
{
	OEComponent *component = getComponent(id);
	if (!component)
	{
		logMessage("could not configure '" + id + "', component is not created");
		return false;
	}
	
	map<string, string> propertiesMap;
	propertiesMap["id"] = id;
	propertiesMap["deviceId"] = getDeviceId(id);
	propertiesMap["resourcePath"] = resourcePath;
	
	for(xmlNodePtr node = children;
		node;
		node = node->next)
	{
		if (!xmlStrcmp(node->name, BAD_CAST "property"))
		{
			string name = getNodeProperty(node, "name");
			
			if (hasNodeProperty(node, "value"))
			{
				string value = getNodeProperty(node, "value");
				value = parseValueProperties(value, propertiesMap);
				
				if (!component->setValue(name, value))
					logMessage("'" + id + "': invalid property '" + name + "'");
			}
			else if (hasNodeProperty(node, "ref"))
			{
				string refId = getNodeProperty(node, "ref");
				OEComponent *ref = getComponent(refId);
				if (!component->setRef(name, ref))
					logMessage("'" + id + "': invalid property '" + name + "'");
			}
			else if (hasNodeProperty(node, "data"))
			{
				string dataSrc = getNodeProperty(node, "data");
				
				OEData data;
				string parsedSrc = parseValueProperties(dataSrc, propertiesMap);
				bool dataRead = false;
				if (hasValueProperty(dataSrc, "packagePath"))
				{
					if (package)
						dataRead = package->readFile(parsedSrc, &data);
				}
				else
					dataRead = readFile(parsedSrc, &data);
				
				if (dataRead)
				{
					if (!component->setData(name, &data))
						logMessage("'" + id + "': invalid property '" + name + "'");
				}
			}
			else
			{
				logMessage("'" + id + "': invalid property '" + name + "', "
						   "unrecognized type");
			}
		}
	}
	
	return true;
}

bool OEEmulation::initEmulation()
{
	if (!doc)
		return false;
	
	xmlNodePtr rootNode = xmlDocGetRootElement(doc);
	
	for(xmlNodePtr node = rootNode->children;
		node;
		node = node->next)
		if (!xmlStrcmp(node->name, BAD_CAST "component"))
		{
			string id = getNodeProperty(node, "id");
			
			if (!initComponent(id))
				return false;
		}
	
	return true;
}

bool OEEmulation::initComponent(string id)
{
	OEComponent *component = getComponent(id);
	if (!component)
	{
		logMessage("could not init '" + id + "', component is not created");
		
		return false;
	}
	
	if (!component->init())
	{
		logMessage("could not init '" + id + "'");
		
		return false;
	}
	
	component->update();
	
	return true;
}

bool OEEmulation::updateEmulation()
{
	if (!doc)
		return false;
	
	xmlNodePtr rootNode = xmlDocGetRootElement(doc);
	
	for(xmlNodePtr node = rootNode->children;
		node;
		node = node->next)
		if (!xmlStrcmp(node->name, BAD_CAST "component"))
		{
			string id = getNodeProperty(node, "id");
			
			if (!updateComponent(id, node->children))
				return false;
		}
	
	return true;
}

bool OEEmulation::updateComponent(string id, xmlNodePtr children)
{
	OEComponent *component = getComponent(id);
	if (!component)
	{
		logMessage("could not update '" + id + "', component is not created");
		return false;
	}
	
	map<string, string> propertiesMap;
	propertiesMap["id"] = id;
	propertiesMap["deviceId"] = getDeviceId(id);
	propertiesMap["resourcePath"] = resourcePath;
	
	for(xmlNodePtr node = children;
		node;
		node = node->next)
	{
		if (!xmlStrcmp(node->name, BAD_CAST "property"))
		{
			string name = getNodeProperty(node, "name");
			
			if (hasNodeProperty(node, "value"))
			{
				string value;
				
				if (component->getValue(name, value))
					setNodeProperty(node, "value", value);
			}
			else if (hasNodeProperty(node, "data"))
			{
				string dataSrc = getNodeProperty(node, "data");
				
				if (!hasValueProperty(dataSrc, "packagePath") || !package)
					continue;
				
				OEData *data = NULL;
				string parsedSrc = parseValueProperties(dataSrc, propertiesMap);
				
				if (component->getData(name, &data) && data)
				{
					if (!package->writeFile(parsedSrc, data))
						logMessage("could not write '" + dataSrc + "'");
				}
			}
		}
	}
	
	return true;
}

void OEEmulation::disconnectEmulation()
{
	if (!doc)
		return;
	
	xmlNodePtr rootNode = xmlDocGetRootElement(doc);
	
	for(xmlNodePtr node = rootNode->children;
		node;
		node = node->next)
	{
		if (!xmlStrcmp(node->name, BAD_CAST "component"))
		{
			string id = getNodeProperty(node, "id");
			
			disconnectComponent(id, node->children);
		}
	}
}

void OEEmulation::disconnectComponent(string id, xmlNodePtr children)
{
	OEComponent *component = getComponent(id);
	if (!component)
		return;
	
	for(xmlNodePtr node = children;
		node;
		node = node->next)
	{
		if (!xmlStrcmp(node->name, BAD_CAST "property"))
		{
			string name = getNodeProperty(node, "name");
			
			if (hasNodeProperty(node, "ref"))
				component->setRef(name, NULL);
		}
	}
}

void OEEmulation::destroyEmulation()
{
	if (!doc)
		return;
	
	xmlNodePtr rootNode = xmlDocGetRootElement(doc);
	
	for(xmlNodePtr node = rootNode->children;
		node;
		node = node->next)
	{
		if (!xmlStrcmp(node->name, BAD_CAST "component"))
		{
			string id = getNodeProperty(node, "id");
			
			destroyComponent(id, node->children);
		}
	}
}

void OEEmulation::destroyComponent(string id, xmlNodePtr children)
{
	OEComponent *component = getComponent(id);
	if (!component)
		return;
	
	delete component;
	
	removeComponent(id);
}

bool OEEmulation::hasValueProperty(string value, string propertyName)
{
	return (value.find("${" + propertyName + "}") != string::npos);
}

string OEEmulation::parseValueProperties(string value, map<string, string>& propertiesMap)
{
	int startIndex;
	
	while ((startIndex = (int) value.find("${")) != string::npos)
	{
		int endIndex = (int) value.find("}", startIndex);
		if (endIndex == string::npos)
		{
			value = value.substr(0, startIndex);
			break;
		}
		
		string propertyName = value.substr(startIndex + 2,
										   endIndex - startIndex - 2);
		string replacement;
		
		for (map<string, string>::iterator i = propertiesMap.begin();
			 i != propertiesMap.end();
			 i++)
		{
			if (i->first == propertyName)
				replacement = i->second;
		}
		
		value = value.replace(startIndex,
							  endIndex - startIndex + 1,
							  replacement);
	}
	
	return value;
}
