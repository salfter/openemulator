
/**
 * OpenEmulator
 * Mac OS X Emulation Item
 * (C) 2010-2011 by Marc S. Ressl (mressl@umich.edu)
 * Released under the GPL
 *
 * Implements an emulation item.
 */

#import "EmulationItem.h"
#import "StringConversion.h"

#import "OEEmulation.h"

#import "DeviceInterface.h"
#import "StorageInterface.h"

@implementation EmulationItem

- (id)initRootWithDocument:(Document *)theDocument
{
	if (self = [super init])
	{
		type = EMULATIONITEM_ROOT;
		children = [[NSMutableArray alloc] init];
		
		document = theDocument;
		
		[document lockEmulation];
		
		OEEmulation *emulation = (OEEmulation *)[theDocument emulation];
		OEIds *devices = (OEIds *)emulation->getDevices();
		for (int i = 0; i < devices->size(); i++)
		{
			string deviceId = devices->at(i);
			OEComponent *theDevice = emulation->getComponent(deviceId);
			
			// Create group item
			string value;
			
			theDevice->postMessage(NULL, DEVICE_GET_GROUP, &value);
			NSString *group = getNSString(value);
			EmulationItem *groupItem = [self childWithUID:group];
			if (!groupItem)
			{
				groupItem = [[EmulationItem alloc] initGroup:group];
				[children addObject:groupItem];
				[groupItem release];
			}
			
			// Create device item
			EmulationItem *deviceItem;
			deviceItem = [[EmulationItem alloc] initDevice:theDevice
													   uid:getNSString(deviceId)
												  document:theDocument];
			[[groupItem children] addObject:deviceItem];
			[deviceItem release];
		}
		
		[document unlockEmulation];
	}
	
	return self;
}

- (id)initGroup:(NSString *)theGroup
{
	if (self = [super init])
	{
		type = EMULATIONITEM_GROUP;
		uid = [theGroup copy];
		children = [[NSMutableArray alloc] init];
		
		label = [[NSLocalizedString(theGroup, @"Emulation Item Group Label.")
				  uppercaseString] retain];
	}
	
	return self;
}

- (id)initDevice:(void *)theDevice
			 uid:(NSString *)theUID
		document:(Document *)theDocument
{
	if (self = [super init])
	{
		type = EMULATIONITEM_DEVICE;
		uid = [theUID copy];
		children = [[NSMutableArray alloc] init];
		document = theDocument;
		
		device = (OEComponent *)theDevice;
		string value;
		
		// Read device values
		((OEComponent *)device)->postMessage(NULL, DEVICE_GET_LABEL, &value);
		label = [getNSString(value) retain];
		NSString *resourcePath = [[NSBundle mainBundle] resourcePath];
		((OEComponent *)device)->postMessage(NULL, DEVICE_GET_IMAGEPATH, &value);
		NSString *imagePath = [[resourcePath stringByAppendingPathComponent:
								getNSString(value)] retain];
		image = [[NSImage alloc] initByReferencingFile:imagePath];
		
		((OEComponent *)device)->postMessage(NULL, DEVICE_GET_LOCATIONLABEL, &value);
		locationLabel = [getNSString(value) retain];
		((OEComponent *)device)->postMessage(NULL, DEVICE_GET_STATELABEL, &value);
		stateLabel = [getNSString(value) retain];
		
		// Read settings
		settingsRef = [[NSMutableArray alloc] init];
		settingsName = [[NSMutableArray alloc] init];
		settingsLabel = [[NSMutableArray alloc] init];
		settingsType = [[NSMutableArray alloc] init];
		settingsOptions = [[NSMutableArray alloc] init];
		DeviceSettings settings;
		((OEComponent *)device)->postMessage(NULL, DEVICE_GET_SETTINGS, &settings);
		for (int i = 0; i < settings.size(); i++)
		{
			DeviceSetting setting = settings[i];
			[settingsRef addObject:getNSString(setting.ref)];
			[settingsName addObject:getNSString(setting.name)];
			[settingsLabel addObject:getNSString(setting.label)];
			[settingsType addObject:getNSString(setting.type)];
			[settingsOptions addObject:[getNSString(setting.options)
										componentsSeparatedByString:@","]];
		}
		
		// Read canvases
		canvases = [[NSMutableArray alloc] init];
		OEComponents theCanvases;
		((OEComponent *)device)->postMessage(NULL, DEVICE_GET_CANVASES, &theCanvases);
		for (int i = 0; i < theCanvases.size(); i++)
			[canvases addObject:[NSValue valueWithPointer:theCanvases.at(i)]];
		
		// Update storage device
		((OEComponent *)device)->postMessage(NULL, DEVICE_GET_STORAGE, &storage);
		if (storage)
		{
			((OEComponent *)storage)->postMessage(NULL, STORAGE_GET_MOUNTPATH, &value);
			if (value.size())
			{
				NSString *storageUID;
				storageUID = [NSString stringWithFormat:@"%@.storage", uid];
				
				EmulationItem *storageItem;
				storageItem = [[EmulationItem alloc] initMountWithStorage:storage
																	  uid:storageUID 
															locationLabel:locationLabel
																 document:theDocument];
				[children addObject:storageItem];
				[storageItem release];
			}
		}
	}
	
	return self;
}

- (id)initMountWithStorage:(void *)theStorage
					   uid:(NSString *)theUID
			 locationLabel:(NSString *)theLocationLabel
				  document:(Document *)theDocument
{
	if (self = [super init])
	{
		type = EMULATIONITEM_MOUNT;
		children = [[NSMutableArray alloc] init];
		document = theDocument;
		
		string value;
		((OEComponent *)theStorage)->postMessage(NULL, STORAGE_GET_MOUNTPATH, &value);
		label = [[getNSString(value) lastPathComponent] retain];
		image = [[NSImage imageNamed:@"DiskImage"] retain];
		
		locationLabel = [theLocationLabel copy];
		value = "";
		((OEComponent *)theStorage)->postMessage(NULL, STORAGE_GET_STATELABEL, &value);
		stateLabel = [getNSString(value) retain];
		
		storage = theStorage;
	}
	
	return self;
}

- (void)dealloc
{
	[uid release];
	[children release];
	
	[label release];
	[image release];
	
	[locationLabel release];
	[stateLabel release];
	
	[settingsRef release];
	[settingsName release];
	[settingsLabel release];
	[settingsType release];
	[settingsOptions release];
	
	[canvases release];
	
	[super dealloc];
}



- (BOOL)isGroup
{
	return (type == EMULATIONITEM_GROUP);
}

- (NSString *)uid
{
	return [[uid copy] autorelease];
}

- (NSMutableArray *)children
{
	return children;
}

- (EmulationItem *)childWithUID:(NSString *)theUID
{
	for (EmulationItem *item in children)
	{
		if ([[item uid] compare:theUID] == NSOrderedSame)
			return item;
	}
	
	return nil;
}



- (NSString *)label
{
	return label;
}

- (NSImage *)image
{
	return image;
}



- (NSString *)locationLabel
{
	return locationLabel;
}

- (NSString *)stateLabel
{
	return stateLabel;
}



- (NSInteger)numberOfSettings
{
	return [settingsRef count];
}

- (NSString *)labelForSettingAtIndex:(NSInteger)index
{
	return [settingsLabel objectAtIndex:index];
}

- (NSString *)typeForSettingAtIndex:(NSInteger)index
{
	return [settingsType objectAtIndex:index];
}

- (NSArray *)optionsForSettingAtIndex:(NSInteger)index
{
	return [settingsOptions objectAtIndex:index];
}

- (void)setValue:(NSString *)value forSettingAtIndex:(NSInteger)index;
{
	NSString *settingRef = [settingsRef objectAtIndex:index];
	NSString *settingName = [settingsName objectAtIndex:index];
	NSString *settingType = [settingsType objectAtIndex:index];
	if ([settingType compare:@"select"] == NSOrderedSame)
	{
		NSArray *settingOptions = [settingsOptions objectAtIndex:index];
		value = [settingOptions objectAtIndex:[value integerValue]];
	}
	
	[document lockEmulation];
	OEEmulation *emulation = (OEEmulation *)[document emulation];
	OEComponent *component = emulation->getComponent(getCPPString(settingRef));
	if (component)
	{
		if (component->setValue(getCPPString(settingName), getCPPString(value)))
			component->update();
	}
	[document unlockEmulation];
}

- (NSString *)valueForSettingAtIndex:(NSInteger)index
{
	NSString *settingRef = [settingsRef objectAtIndex:index];
	NSString *settingName = [settingsName objectAtIndex:index];
	NSString *value = @"";
	
	[document lockEmulation];
	OEEmulation *emulation = (OEEmulation *)[document emulation];
	OEComponent *component = emulation->getComponent(getCPPString(settingRef));
	if (component)
	{
		string theValue;
		component->getValue(getCPPString(settingName), theValue);
		value = getNSString(theValue);
	}
	[document unlockEmulation];
	
	NSString *settingType = [settingsType objectAtIndex:index];
	if ([settingType compare:@"select"] == NSOrderedSame)
	{
		NSArray *options = [settingsOptions objectAtIndex:index];
		value = [NSString stringWithFormat:@"%d", [options indexOfObject:value]];
	}
	
	return value;
}



- (BOOL)hasCanvases
{
	if (canvases)
		return [canvases count];
	
	return false;
}

- (void)showCanvases
{
	if (canvases)
		for (int i = 0; i < [canvases count]; i++)
			[document showCanvas:[[canvases objectAtIndex:i] pointerValue]];
}



- (BOOL)isStorageDevice
{
	return (type == EMULATIONITEM_DEVICE) && (storage != nil);
}

- (BOOL)mount:(NSString *)path
{
	if (!storage)
		return NO;
	
	string value = getCPPString(path);
	bool success;
	[document lockEmulation];
	success = ((OEComponent *)storage)->postMessage(NULL, STORAGE_MOUNT, &value);
	[document unlockEmulation];
	
	return success;
}

- (BOOL)testMount:(NSString *)path
{
	if (!storage)
		return NO;
	
	string value = getCPPString(path);
	bool success;
	[document lockEmulation];
	success = ((OEComponent *)storage)->postMessage(NULL, STORAGE_TESTMOUNT, &value);
	[document unlockEmulation];
	
	return success;
}



- (BOOL)isMount
{
	return (type == EMULATIONITEM_MOUNT);
}

- (void)revealInFinder
{
	if (!storage)
		return;
	
	string value;
	[document lockEmulation];
	((OEComponent *)storage)->postMessage(NULL, STORAGE_GET_MOUNTPATH, &value);
	[document unlockEmulation];
	
	[[NSWorkspace sharedWorkspace] selectFile:getNSString(value)
					 inFileViewerRootedAtPath:@""];
}

- (BOOL)isLocked
{
	if (!storage)
		return NO;
	
	BOOL success;
	[document lockEmulation];
	success = ((OEComponent *)storage)->postMessage(NULL, STORAGE_IS_LOCKED, NULL);
	[document unlockEmulation];
	
	return success;
}

- (void)unmount
{
	if (!storage)
		return;
	
	[document lockEmulation];
	((OEComponent *)storage)->postMessage(NULL, STORAGE_UNMOUNT, NULL);
	[document unlockEmulation];
	
	return;
}

@end
