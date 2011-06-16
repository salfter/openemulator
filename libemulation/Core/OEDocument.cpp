
/**
 * libemulation
 * OEDocument
 * (C) 2010-2011 by Marc S. Ressl (mressl@umich.edu)
 * Released under the GPL
 *
 * Controls an OpenEmulator XML description
 */

#include "OEDocument.h"

OEDocument::OEDocument()
{
	is_open = false;
	
	package = NULL;
	doc = NULL;
}

OEDocument::~OEDocument()
{
	close();
	
	if (doc)
		xmlFreeDoc(doc);
}

bool OEDocument::open(string path)
{
	close();
	
	OEData data;
	string pathExtension = getPathExtension(path);
	if (pathExtension == OE_FILE_PATH_EXTENSION)
	{
		is_open = readFile(path, &data);
		
		if (!is_open)
			logMessage("could not open '" + path + "'");
	}
	else if (pathExtension == OE_PACKAGE_PATH_EXTENSION)
	{
		package = new OEPackage();
		if (package && package->open(path))
		{
			is_open = package->readFile(OE_PACKAGE_EDL_PATH, &data);
			
			if (!is_open)
				logMessage("could not read '" OE_PACKAGE_EDL_PATH
                           "' in '" + path + "'");
		}
		else
			logMessage("could not open package '" + path + "'");
	}
	else
		logMessage("could not identify type of '" + path + "'");
	
	if (is_open)
	{
		doc = xmlReadMemory(&data[0],
							(int) data.size(),
							OE_PACKAGE_EDL_PATH,
							NULL,
							0);
		
		if (!doc)
		{
			is_open = false;
			logMessage("could not parse document '" + path + "'");
		}
	}
	
    /* To-Do: validate DTD
     if (is_open)
     {
     xmlValidCtxtPtr validCtxt = xmlNewValidCtxt();
     xmlDtdPtr dtd = xmlNewDtd(doc,
     name,
     ExternalID,
     SystemID)
     
     if (!xmlValidateDocumentFinal(validCtxt, doc))
     {
     is_open = false;
     logMessage("could not validate DTD of document '" + path + "'");
     }
     
     xmlFreeValidCtxt(validCtxt);
     }*/
	
	if (is_open && !validateDocument())
	{
		is_open = false;
		logMessage("unknown EDL version");
	}
    
    if (is_open)
        is_open = constructDocument(doc);
	
	if (!is_open)
		close();
	
	return is_open;
}

bool OEDocument::isOpen()
{
	return is_open;
}

bool OEDocument::save(string path)
{
	close();
	
	OEData data;
	string pathExtension = getPathExtension(path);
	if (pathExtension == OE_FILE_PATH_EXTENSION)
	{
		if (updateDocument(doc))
		{
			if (dumpDocument(data))
			{
				is_open = writeFile(path, &data);
				
				if (!is_open)
					logMessage("could not open '" + path + "'");
			}
			else
				logMessage("could not dump document '" + path + "'");
		}
		else
			logMessage("could not update the configuration for '" + path + "'");
	}
	else if (pathExtension == OE_PACKAGE_PATH_EXTENSION)
	{
		package = new OEPackage();
		if (package && package->open(path))
		{
			if (updateDocument(doc))
			{
				if (dumpDocument(data))
				{
					is_open = package->writeFile(OE_PACKAGE_EDL_PATH, &data);
					if (!is_open)
						logMessage("could not write '" OE_PACKAGE_EDL_PATH
								   "' in '" + path + "'");
				}
				else
					logMessage("could not dump document '" + path + "'");
			}
            else
                logMessage("could not update the configuration for '" + path + "'");
			
			delete package;
			package = NULL;
		}
		else
			logMessage("could not open '" + path + "'");
	}
	else
		logMessage("could not identify type of '" + path + "'");
	
	if (!is_open)
		close();
	
	return is_open;
}

void OEDocument::close()
{
	is_open = false;
	
	if (package)
		delete package;
    
	package = NULL;
}



OEHeaderInfo OEDocument::getHeaderInfo()
{
	OEHeaderInfo headerInfo;
	
	if (doc)
	{
		xmlNodePtr rootNode = xmlDocGetRootElement(doc);
        
		headerInfo.label = getNodeProperty(rootNode, "label");
		headerInfo.image = getNodeProperty(rootNode, "image");
		headerInfo.description = getNodeProperty(rootNode, "description");
	}
	
	return headerInfo;
}

OEPortsInfo OEDocument::getFreePortsInfo()
{
	OEPortsInfo freePortsInfo;
	
	if (doc)
    {
        xmlNodePtr rootNode = xmlDocGetRootElement(doc);
        
        for(xmlNodePtr node = rootNode->children;
            node;
            node = node->next)
        {
            if (getNodeName(node) == "port")
            {
                string ref = getNodeProperty(node, "ref");
                
                if (ref == "")
                {
                    OEPortInfo portInfo;
                    portInfo.id = getNodeProperty(node, "id");
                    portInfo.type = getNodeProperty(node, "type");
                    portInfo.label = getNodeProperty(node, "label");
                    portInfo.image = getNodeProperty(node, "image");
                    
                    freePortsInfo.push_back(portInfo);
                }
            }
        }
    }
    
	return freePortsInfo;
}

OEConnectorsInfo OEDocument::getFreeConnectorsInfo()
{
    OEIds portRefs;
	OEConnectorsInfo freeConnectorsInfo;
	
	if (doc)
    {
        xmlNodePtr rootNode = xmlDocGetRootElement(doc);
        
        for(xmlNodePtr node = rootNode->children;
            node;
            node = node->next)
        {
            if (getNodeName(node) == "port")
                portRefs.push_back(getNodeProperty(node, "ref"));
            else if (getNodeName(node) == "connector")
            {
                string id = getNodeProperty(node, "id");
                string type = getNodeProperty(node, "type");
                
                OEConnectorInfo connectorInfo;
                connectorInfo.id = id;
                connectorInfo.type = type;
                freeConnectorsInfo.push_back(connectorInfo);
            }
        }
    }
	
    for (OEIds::iterator i = portRefs.begin();
         i != portRefs.end();
         i++)
    {
        for (OEConnectorsInfo::iterator j = freeConnectorsInfo.begin();
             j != freeConnectorsInfo.end();
             j++)
        {
            if (*i == j->id)
            {
                freeConnectorsInfo.erase(j);
                break;
            }
        }
    }
    
	return freeConnectorsInfo;
}

bool OEDocument::addDocument(string path, OEIdMap connections)
{
	if (!doc)
		return false;
	
	OEDocument document;
	if (!document.open(path))
		return false;
	
    // Remap ids
    OEIds deviceIds = document.getDeviceIds();
	OEIdMap deviceIdMap = makeIdMap(deviceIds);
    
	remapConnections(deviceIdMap, connections);
    document.remapDocument(deviceIdMap);
    
    // Connect ports and connector inlets
    OEInletMap portInletMap = getInlets(doc, connections, "port");
    OEInletMap connectorInletMap = getInlets(document.getXMLDoc(), connections, "connector");
    
    connectPorts(doc, connections);
    connectInlets(doc, portInletMap);
    connectInlets(document.getXMLDoc(), connectorInletMap);
    
    // Insert document
    string firstPortId = connections.begin()->first;
    xmlNodePtr node = getInsertionNode(firstPortId);
    if (!node)
    {
        logMessage("could not find insertion node for '" + path + "'");
        
        return false;
    }
    
    if (!document.insertInto(node))
    {
        logMessage("could not insert '" + path + "'");
        
        return false;
    }
    
    // Construct new document
    constructDocument(document.getXMLDoc());
    
    // Init port inlets
    configureInlets(portInletMap);
    
    return true;
}

bool OEDocument::isDeviceTerminal(string deviceId)
{
    return false;
}

bool OEDocument::removeDevice(string deviceId)
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

OEIds OEDocument::getDeviceIds()
{
    OEIds deviceIds;
    
	if (doc)
    {
        xmlNodePtr rootNode = xmlDocGetRootElement(doc);
        
        for(xmlNodePtr node = rootNode->children;
            node;
            node = node->next)
        {
            if (getNodeName(node) == "device")
            {
                string deviceId = getNodeProperty(node, "id");
                
                deviceIds.push_back(deviceId);
            }
        }
    }
    
	return deviceIds;
}

void OEDocument::setDeviceId(string& id, string deviceId)
{
	int dotIndex = (int) id.find_first_of('.');
	
	if (dotIndex == string::npos)
		id = deviceId;
	else
		id = deviceId + "." + id.substr(dotIndex + 1);
}

string OEDocument::getDeviceId(string id)
{
	int dotIndex = (int) id.find_first_of('.');
	
	if (dotIndex == string::npos)
		return id;
	
	return id.substr(0, dotIndex);
}




bool OEDocument::validateDocument()
{
	xmlNodePtr rootNode = xmlDocGetRootElement(doc);
	
	return (getNodeProperty(rootNode, "version") == "1.0");
}

bool OEDocument::dumpDocument(OEData& data)
{
	if (!doc)
		return false;
	
	xmlChar *xmlDump;
	int xmlDumpSize;
	
	xmlDocDumpMemory(doc, &xmlDump, &xmlDumpSize);
	if (xmlDump)
	{
		data.resize(xmlDumpSize);
		memcpy(&data.front(), xmlDump, xmlDumpSize);
		
		xmlFree(xmlDump);
		
		return true;
	}
	
	return false;
}

bool OEDocument::constructDocument(xmlDocPtr doc)
{
    return true;
}

bool OEDocument::configureInlets(OEInletMap& inletMap)
{
    return true;
}

bool OEDocument::updateDocument(xmlDocPtr doc)
{
    return true;
}



xmlDocPtr OEDocument::getXMLDoc()
{
    return doc;
}

// Make an id map so a new document, when inserted in the
// current document, has unique names
OEIdMap OEDocument::makeIdMap(OEIds& deviceIds)
{
    OEIds ourDeviceIds = getDeviceIds();
    
	OEIdMap idMap;
	
	for (OEIds::iterator i = deviceIds.begin();
		 i != deviceIds.end();
		 i++)
	{
		string deviceId = *i;
        
		int newDeviceIdCount = 1;
		string newDeviceId = deviceId;
		while (find(ourDeviceIds.begin(), ourDeviceIds.end(), newDeviceId) !=
               ourDeviceIds.end())
		{
			newDeviceIdCount++;
			newDeviceId = deviceId + getString(newDeviceIdCount);
		}
		
		idMap[deviceId] = newDeviceId;
	}
	
	return idMap;
}

// Remap a node property
void OEDocument::remapNodeProperty(OEIdMap& deviceIdMap, xmlNodePtr node, string property)
{
	string nodeProperty;
	
	if (hasNodeProperty(node, property))
	{
		string id = getNodeProperty(node, property);
		string deviceId = getDeviceId(id);
        
		if (deviceIdMap.count(deviceId))
		{
			setDeviceId(id, deviceIdMap[deviceId]);
			setNodeProperty(node, property, id);
		}
	}
}

// Remap document ids
void OEDocument::remapDocument(OEIdMap& deviceIdMap)
{
	xmlNodePtr rootNode = xmlDocGetRootElement(doc);
	
	for(xmlNodePtr node = rootNode->children;
		node;
		node = node->next)
	{
		remapNodeProperty(deviceIdMap, node, "id");
		remapNodeProperty(deviceIdMap, node, "ref");
		
		if (node->children)
		{
			for(xmlNodePtr propertyNode = node->children;
				propertyNode;
				propertyNode = propertyNode->next)
			{
				remapNodeProperty(deviceIdMap, propertyNode, "id");
				remapNodeProperty(deviceIdMap, propertyNode, "ref");
			}
		}
	}
}

// Remap connections
void OEDocument::remapConnections(OEIdMap& deviceIdMap, OEIdMap& connections)
{
	for (OEIdMap::iterator i = connections.begin();
		 i != connections.end();
		 i++)
	{
		string srcId = i->first;
		string destId = i->second;
		
		string deviceId = getDeviceId(destId);
        if (deviceIdMap.count(deviceId))
        {
            setDeviceId(destId, deviceIdMap[deviceId]);
            connections[srcId] = destId;
        }
    }
}

// Find last node belonging to a device
xmlNodePtr OEDocument::getLastNode(string deviceId)
{
    xmlNodePtr lastNode = NULL;
    xmlNodePtr lastDocNode = NULL;
    
    xmlNodePtr rootNode = xmlDocGetRootElement(doc);
    
    for(xmlNodePtr node = rootNode->children;
        node;
        node = node->next)
    {
        string id = getNodeProperty(node, "id");
        
        if (id == "")
            continue;
        
        if (getDeviceId(id) == deviceId)
            lastNode = node;
        
        lastDocNode = node;
    }
    
    if (!lastNode)
        lastNode = lastDocNode;
    
    return lastNode;
}

// Follow device chain
string OEDocument::followDeviceChain(string deviceId, vector<string>& visitedIds)
{
	// Avoid circularity
	if (find(visitedIds.begin(), visitedIds.end(), deviceId) != visitedIds.end())
		return deviceId;
	visitedIds.push_back(deviceId);
	
    string lastDeviceId = deviceId;
    
    xmlNodePtr rootNode = xmlDocGetRootElement(doc);
    
    for(xmlNodePtr node = rootNode->children;
        node;
        node = node->next)
    {
		if (getNodeName(node) == "port")
		{
			string id = getNodeProperty(node, "id");
			
			if (getDeviceId(id) == deviceId)
            {
                string ref = getNodeProperty(node, "ref");
                
                if (ref != "")
                    lastDeviceId = followDeviceChain(getDeviceId(ref), visitedIds);
            }
		}
    }
    
    return lastDeviceId;
}

// Find node for inserting another document
xmlNodePtr OEDocument::getInsertionNode(string portId)
{
    string previousDeviceId;
    
    // Find the previous connected device to this port
    xmlNodePtr rootNode = xmlDocGetRootElement(doc);
    
    for(xmlNodePtr node = rootNode->children;
        node;
        node = node->next)
    {
        if (getNodeName(node) == "port")
        {
            string id = getNodeProperty(node, "id");
            
            if (id == portId)
                break;
            
            if (getDeviceId(id) == getDeviceId(portId))
            {
                string ref = getNodeProperty(node, "ref");
                
                if (ref != "")
                    previousDeviceId = getDeviceId(ref);
            }
        }
    }
    
    string insertDeviceId;
    if (previousDeviceId != "")
    {
        vector<string> visitedIds;
        insertDeviceId = followDeviceChain(previousDeviceId, visitedIds);
    }
    else
        insertDeviceId = getDeviceId(portId);
    
    // Return last node of the insertion point
    return getLastNode(insertDeviceId);
}

// Insert this document into dest
bool OEDocument::insertInto(xmlNodePtr dest)
{
    if (!dest)
        return false;
    
	xmlNodePtr rootNode = xmlDocGetRootElement(doc);
    
	for(xmlNodePtr node = rootNode->children;
		node;
		node = node->next)
	{
        // Don't add last character data
        if ((node->type != XML_TEXT_NODE) || node->next)
        {
            if (!xmlAddNextSibling(dest, xmlCopyNode(node, 1)))
                return false;
        }
        
        dest = dest->next;
	}
    
    return true;
}

// Scan inlets
void OEDocument::addInlets(OEInletMap& inletMap, string deviceId, xmlNodePtr children)
{
    for(xmlNodePtr node = children;
        node;
        node = node->next)
    {
        if (getNodeName(node) == "inlet")
        {
            string ref = getNodeProperty(node, "ref");
            string property = getNodeProperty(node, "property");
            string outletRef = deviceId + "." + getNodeProperty(node, "outletRef");
            
            inletMap[ref][property] = outletRef;
        }
    }
}

// Scan ports and connectors for inlets
OEInletMap OEDocument::getInlets(xmlDocPtr doc, OEIdMap& connections, string nodeType)
{
    OEInletMap inletMap;
    
	xmlNodePtr rootNode = xmlDocGetRootElement(doc);
    
	for(xmlNodePtr node = rootNode->children;
		node;
		node = node->next)
	{
        if (getNodeName(node) == nodeType)
        {
            string id = getNodeProperty(node, "id");
            
            for (OEIdMap::iterator i = connections.begin();
                 i != connections.end();
                 i++)
            {
                string portId = i->first;
                string connectorId = i->second;
                
                if (id == portId)
                    addInlets(inletMap, getDeviceId(connectorId), node->children);
                else if (id == connectorId)
                    addInlets(inletMap, getDeviceId(portId), node->children);
            }
        }
    }
    
    return inletMap;
}

// Connects ports
void OEDocument::connectPorts(xmlDocPtr doc, OEIdMap& connections)
{
	xmlNodePtr rootNode = xmlDocGetRootElement(doc);
    
	for(xmlNodePtr node = rootNode->children;
		node;
		node = node->next)
	{
        if ((getNodeName(node) == "port"))
        {
            string id = getNodeProperty(node, "id");
            
            if (connections.count(id))
                setNodeProperty(node, "ref", connections[id]);
        }
    }
}

// Connect inlets
void OEDocument::connectInlets(xmlDocPtr doc, OEInletMap& inletMap)
{
	xmlNodePtr rootNode = xmlDocGetRootElement(doc);
    
	for(xmlNodePtr node = rootNode->children;
		node;
		node = node->next)
	{
        if (getNodeName(node) == "component")
        {
            string id = getNodeProperty(node, "id");
            
            if (inletMap.count(id))
                connectInlet(inletMap[id], node->children);
        }
    }
}

void OEDocument::connectInlet(OEIdMap& propertyMap, xmlNodePtr children)
{
	for(xmlNodePtr node = children;
		node;
		node = node->next)
	{
        if (getNodeName(node) == "property")
        {
            string name = getNodeProperty(node, "name");
            
            if (propertyMap.count(name))
                setNodeProperty(node, "ref", propertyMap[name]);
        }
    }
}

string OEDocument::getLocationLabel(string deviceId, vector<string>& visitedIds)
{
	if (!doc)
		return "";
    
	// Avoid circularity
	if (find(visitedIds.begin(), visitedIds.end(), deviceId) != visitedIds.end())
		return "*";
	visitedIds.push_back(deviceId);
	
	// Follow connection chain
	xmlNodePtr rootNode = xmlDocGetRootElement(doc);
	
	for(xmlNodePtr node = rootNode->children;
		node;
		node = node->next)
	{
		if (getNodeName(node) == "port")
		{
			string portId = getNodeProperty(node, "id");
			string ref = getNodeProperty(node, "ref");
			string label = getNodeProperty(node, "label");
			
			if (getDeviceId(ref) == deviceId)
				return (getLocationLabel(getDeviceId(portId), visitedIds) +
						" " + label);
		}
	}
	
	// Find device
	for(xmlNodePtr node = rootNode->children;
		node;
		node = node->next)
	{
		if (getNodeName(node) == "device")
		{
			string theDeviceId = getNodeProperty(node, "id");
			string label = getNodeProperty(node, "label");
			
			if (theDeviceId == deviceId)
				return label;
		}
	}
	
	// Device was not found
	return "?";
}

string OEDocument::getLocationLabel(string id)
{
	if (!doc)
		return "";
	
	vector<string> visitedIds;
	string location = getLocationLabel(id, visitedIds);
	int depth = (int) visitedIds.size();
	
	if (depth == 1)
		return "";
	else
		return location;
}

string OEDocument::getNodeName(xmlNodePtr node)
{
    string name;
    
    if (node->name)
        name = (char *) node->name;
    
	return name;
}

string OEDocument::getNodeProperty(xmlNodePtr node, string name)
{
	char *value = (char *) xmlGetProp(node, BAD_CAST name.c_str());
	string valueString = value ? value : "";
	xmlFree(value);
	
	return valueString;
}

bool OEDocument::hasNodeProperty(xmlNodePtr node, string name)
{
	char *value = (char *) xmlGetProp(node, BAD_CAST name.c_str());
	xmlFree(value);
	
	return (value != NULL);
}

void OEDocument::setNodeProperty(xmlNodePtr node, string name, string value)
{
	xmlSetProp(node, BAD_CAST name.c_str(), BAD_CAST value.c_str());
}
