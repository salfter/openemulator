
/**
 * OpenEmulator
 * Mac OS X Device Chooser View Controller
 * (C) 2009 by Marc S. Ressl (mressl@umich.edu)
 * Released under the GPL
 *
 * Controls a device chooser view.
 */

#import "DeviceChooserViewController.h"

#import "OEInfo.h"

#import "ChooserItem.h"
#import "Document.h"

@interface DeviceInfo : NSObject
{
	OEInfo *info;
	NSString *path;
}

- (id) initWithPath:(NSString *) path;
- (OEInfo *) info;
- (NSString *) path;

@end

@implementation DeviceInfo

- initWithPath:(NSString *) thePath;
{
	if (self = [super init])
	{
		info = new OEInfo(string([thePath UTF8String]));
		path = [thePath copy];
	}
	
	return self;
}

- (void) dealloc
{
	[super dealloc];
	
	delete info;
	[path release];
}

- (OEInfo *) info
{
	return info;
}

- (NSString *) path
{
	return path;
}

@end

@implementation DeviceChooserViewController

- (id) init
{
	self = [super init];
	
	if (self)
		deviceInfos = [[NSMutableArray alloc] init];
	
	return self;
}

- (void) dealloc
{
	[super dealloc];
	
	[deviceInfos release];
}

- (void) awakeFromNib
{
	[super awakeFromNib];
	
	NSString *resourcePath = [[NSBundle mainBundle] resourcePath];
	NSString *templatesPath = [resourcePath
							   stringByAppendingPathComponent:@"devices"];
	NSArray *deviceFilenames = [[NSFileManager defaultManager]
								contentsOfDirectoryAtPath:templatesPath
								error:nil];
	
	int deviceFilenamesCount = [deviceFilenames count];
	for (int i = 0; i < deviceFilenamesCount; i++)
	{
		NSString *deviceFilename = [deviceFilenames objectAtIndex:i];
		NSString *devicePath = [templatesPath
								stringByAppendingPathComponent:deviceFilename];
		DeviceInfo *deviceInfo = [[DeviceInfo alloc] initWithPath:devicePath];
		if (deviceInfo)
		{
			[deviceInfo autorelease];
			[deviceInfos addObject:deviceInfo];
		}
	}
}

- (void) updateForInlets:(NSMutableArray *)freeInlets
{
	NSString *resourcePath = [[NSBundle mainBundle] resourcePath];
	NSString *imagesPath = [resourcePath
							stringByAppendingPathComponent:@"images"];
	
	[groups removeAllObjects];
	[groupNames removeAllObjects];
	
	for (int i = 0; i < [deviceInfos count]; i++)
	{
		DeviceInfo *deviceInfo = [deviceInfos objectAtIndex:i];
		OEInfo *info = [deviceInfo info];
		
		// Verify if device is connectable
		NSMutableArray *inlets = [NSMutableArray arrayWithArray:freeInlets];
		OEPorts *outlets = info->getOutlets();
		BOOL isDeviceConnectable = YES;
		for (OEPorts::iterator o = outlets->begin();
			 o != outlets->end();
			 o++)
		{
			if (o->connectedPort)
				continue;
			
			NSString *outletType = [NSString stringWithUTF8String:o->type.c_str()];
			BOOL isInletFound = NO;
			for (int j = 0; j < [inlets count]; j++)
			{
				NSString *type = [inlets objectAtIndex:j];
				if ([type compare:outletType] == NSOrderedSame)
				{
					[inlets removeObjectAtIndex:j];
					isInletFound = YES;
					break;
				}
			}
			
			if (!isInletFound)
			{
				isDeviceConnectable = NO;
				break;
			}
		}
		if (!isDeviceConnectable)
			continue;
		
		// Add device
		NSString *label = [NSString stringWithUTF8String:info->getLabel().c_str()];
		NSString *imageName = [NSString stringWithUTF8String:info->getImage().c_str()];
		NSString *description = [NSString stringWithUTF8String:info->getDescription().c_str()];
		NSString *groupName = [NSString stringWithUTF8String:info->getGroup().c_str()];
		
		if (![groups objectForKey:groupName])
		{
			NSMutableArray *group = [[[NSMutableArray alloc] init] autorelease];
			[groups setObject:group forKey:groupName];
		}
		NSString *imagePath = [imagesPath stringByAppendingPathComponent:imageName];
		ChooserItem *item = [[ChooserItem alloc] initWithItem:[deviceInfo path]
														label:label
													imagePath:imagePath
												  description:description];
		if (!item)
			continue;
		
		[item autorelease];
		[[groups objectForKey:groupName] addObject:item];
	}
	
	[groupNames addObjectsFromArray:[[groups allKeys]
									 sortedArrayUsingSelector:@selector(compare:)]];
	
	[self selectItemWithPath:nil];
}

@end
