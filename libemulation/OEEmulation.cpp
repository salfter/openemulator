
/**
 * libemulation
 * Emulation
 * (C) 2009-2010 by Marc S. Ressl (mressl@umich.edu)
 * Released under the GPL
 *
 * Controls an emulation
 */

#include <fstream>
#include <sstream>

#include <libxml/parser.h>

#include "OEEmulation.h"
#include "OEComponentFactory.h"

OEEmulation::~OEEmulation()
{
	close();
}

void OEEmulation::setResourcePath(string path)
{
	resourcePath = path;
}

bool OEEmulation::open(string path)
{
	close();
	
	if (!OEEDL::open(path))
		return false;
	
	if (create())
		if (configure())
			if (init())
				return true;
	
	close();
	
	return false;
}

void OEEmulation::close()
{
	deconfigure();
	destroy();
	
	OEEDL::close();
}

bool OEEmulation::setComponent(string id, OEComponent *component)
{
	if (component)
		components[id] = component;
	else
		components.erase(id);
	
	return true;
}

OEComponent *OEEmulation::getComponent(string id)
{
	if (!components.count(id))
		return NULL;
	
	return components[id];
}

bool OEEmulation::create()
{
	xmlNodePtr rootNode = xmlDocGetRootElement(doc);
	
	for(xmlNodePtr node = rootNode->children;
		node;
		node = node->next)
		if (!xmlStrcmp(node->name, BAD_CAST "component"))
		{
			string id = getNodeProperty(node, "id");
			string className = getNodeProperty(node, "class");
			
			if (!createComponent(id, className))
				return false;
		}
	
	return true;
}

bool OEEmulation::createComponent(string id, string className)
{
	if (!getComponent(id))
	{
		OEComponent *component = OEComponentFactory::create(className);
		if (component)
			return setComponent(id, component);
		else
			log("could not create '" + id + "', class '" + className + "' undefined");
	}
	else
		log("redefinition of '" + id + "'");
	
	return false;
}

bool OEEmulation::configure()
{
	xmlNodePtr rootNode = xmlDocGetRootElement(doc);
	
	for(xmlNodePtr node = rootNode->children;
		node;
		node = node->next)
	{
		if (!xmlStrcmp(node->name, BAD_CAST "component"))
		{
			string id = getNodeProperty(node, "id");
			
			if (!configureComponent(id, node->children))
				return false;
		}
	}
	
	return true;
}

bool OEEmulation::configureComponent(string id, xmlNodePtr children)
{
	OEComponent *component = getComponent(id);
	if (!component)
	{
		log("could not configure '" + id + "', it was not created");
		return false;
	}
	
	OEPropertiesMap propertiesMap;
	propertiesMap["id"] = id;
	propertiesMap["device"] = getDeviceId(id);
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
				value = parseProperties(value, propertiesMap);
				
				if (!component->setValue(name, value))
					log("invalid property '" + name + "' for '" + id + "'");
			}
			else if (hasNodeProperty(node, "ref"))
			{
				string refId = getNodeProperty(node, "ref");
				OEComponent *ref = getComponent(refId);
				if (!component->setRef(name, ref))
					log("invalid property '" + name + "' for '" + id + "'");
			}
			else if (hasNodeProperty(node, "src"))
			{
				string src = getNodeProperty(node, "src");
				
				OEData *data = new OEData;
				string parsedSrc = parseProperties(src, propertiesMap);
				bool dataRead = false;
				if (hasProperty(src, "packagePath"))
				{
					if (package)
						dataRead = package->readFile(parsedSrc, data);
				}
				else
					dataRead = readFile(parsedSrc, data);
				
				if (!dataRead)
				{
					delete data;
					data = NULL;
				}
				
				if (!component->setData(name, data))
					log("invalid property '" + name + "' for '" + id + "'");
			}
			else
			{
				log("invalid property '" + name + "' for '" + id +
					  "', unrecognized type");
			}
		}
	}
	
	return true;
}

bool OEEmulation::init()
{
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
		log("could not init '" + id + "', it was not created");
		return false;
	}
	
	if (component->init())
		return true;
	else
		log("could not init '" + id + "'");
	
	return false;
}

bool OEEmulation::update()
{
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
		log("could not update '" + id + "', it was not created");
		return false;
	}
	
	OEPropertiesMap propertiesMap;
	propertiesMap["id"] = id;
	propertiesMap["device"] = getDeviceId(id);
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
					setNodeProperty(node, name, value);
			}
			else if (hasNodeProperty(node, "src"))
			{
				string src = getNodeProperty(node, "src");
				
				if (hasProperty(src, "resourcePath"))
					continue;
				
				OEData *data;
				string parsedSrc = parseProperties(src, propertiesMap);
				
				if (component->getData(name, &data))
				{
					if (data)
					{
						if (package->writeFile(parsedSrc, data))
							continue;
						else
							log("could not write '" + src + "'");
						return false;
					}
				}
			}
		}
	}
	
	return true;
}

void OEEmulation::deconfigure()
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
			
			deconfigureComponent(id, node->children);
		}
	}
}

void OEEmulation::deconfigureComponent(string id, xmlNodePtr children)
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

void OEEmulation::destroy()
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
	
	setComponent(id, NULL);
}

bool OEEmulation::hasProperty(string value, string propertyName)
{
	return (value.find("${" + propertyName + "}") != string::npos);
}

string OEEmulation::parseProperties(string value,
									OEPropertiesMap &propertiesMap)
{
	int startIndex;
	
	while ((startIndex = value.find("${")) != string::npos)
	{
		int endIndex = value.find("}", startIndex);
		if (endIndex == string::npos)
		{
			value = value.substr(0, startIndex);
			break;
		}
		
		string propertyName = value.substr(startIndex + 2,
											  endIndex - startIndex - 2);
		string replacement;
		
		for (OEPropertiesMap::iterator i = propertiesMap.begin();
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
