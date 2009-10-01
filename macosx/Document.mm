
/**
 * OpenEmulator
 * Mac OS X Document
 * (C) 2009 by Marc S. Ressl (mressl@umich.edu)
 * Released under the GPL
 *
 * Controls an emulation document.
 */

#import "Document.h"
#import "DocumentWindowController.h"

#import "OEEmulation.h"
#import "OEInfo.h"

#define TEMPLATE_FOLDER @"~/Library/Application Support/Open Emulator/Templates"

@implementation Document

- (id)init
{
	if (self = [super init])
	{
		emulation = nil;
		
		pasteboard = [NSPasteboard generalPasteboard];
		pasteboardTypes = [[NSArray alloc] initWithObjects:NSStringPboardType, nil];
		
		[self setPower:false];
		[self setLabel:nil];
		[self setDescription:nil];
		[self setModificationDate:nil];
		[self setRunTime:nil];
		[self setImage:nil];
		
		expansions = [[NSMutableArray alloc] init];
		diskDrives = [[NSMutableArray alloc] init];
		peripherals = [[NSMutableArray alloc] init];
		
		// To-Do: [self setVideoPreset:x];
		[self setVolume:nanf("")];
	}
	
	return self;
}

- (id)initFromTemplateURL:(NSURL *)absoluteURL
					error:(NSError **)outError
{
//	printf("initFromTemplateURL\n");
	if ([self init])
	{
		if ([self readFromURL:absoluteURL
					   ofType:nil
						error:outError])
			return self;
	}
	
	*outError = [NSError errorWithDomain:NSCocoaErrorDomain
									code:NSFileReadUnknownError
								userInfo:nil];
	return nil;
}

- (void)dealloc
{
//	printf("dealloc\n");
	[pasteboardTypes release];
	
	if (emulation)
		delete (OEEmulation *) emulation;
	
	[super dealloc];
}

- (void) setDMLProperty:(NSString *)key value:(NSString *)value
{
	if (!emulation)
		return;
	
	xmlDocPtr dml = ((OEEmulation *) emulation)->getDML();
	
	xmlNodePtr rootNode = xmlDocGetRootElement(dml);
	
	xmlSetProp(rootNode, BAD_CAST [key UTF8String], BAD_CAST [value UTF8String]);
}

- (NSString *) getDMLProperty:(NSString *)key
{
	if (!emulation)
		return nil;
	
	xmlDocPtr dml = ((OEEmulation *) emulation)->getDML();
	
	xmlNodePtr rootNode = xmlDocGetRootElement(dml);
	
	xmlChar *valuec = xmlGetProp(rootNode, BAD_CAST [key UTF8String]);
	NSString *value = [NSString stringWithUTF8String:(const char *) valuec];
	xmlFree(valuec);
	
	return value;
}

- (NSImage *) getResourceImage:(NSString *)imagePath
{
	NSString *resourcePath = [[NSBundle mainBundle] resourcePath];
	NSString *path = [[resourcePath
					   stringByAppendingString:@"/images/"]
					  stringByAppendingString:imagePath];
	NSImage *theImage = [[NSImage alloc] initWithContentsOfFile:path];
	if (theImage)
		[theImage autorelease];
	
	return theImage;
}

- (BOOL)readFromURL:(NSURL *)absoluteURL
			 ofType:(NSString *)typeName
			  error:(NSError **)outError
{
//	printf("readFromURL\n");
	const char *emulationPath = [[absoluteURL path] UTF8String];
	const char *resourcePath = [[[NSBundle mainBundle] resourcePath] UTF8String];
	
	if (emulation)
		delete (OEEmulation *) emulation;
	
	emulation = (void *) new OEEmulation(emulationPath, resourcePath);
	
	if (emulation)
	{
		if (((OEEmulation *) emulation)->isOpen())
		{
			[self setLabel:[self getDMLProperty:@"label"]];
			[self setDescription:[self getDMLProperty:@"description"]];
			[self setImage:[self getResourceImage:[self getDMLProperty:@"image"]]];
			
//			[self updateRuntime];
//			[self updateDevices];
			
			return YES;
		}
		
		delete (OEEmulation *) emulation;
		emulation = NULL;
	}
	
	*outError = [NSError errorWithDomain:NSCocoaErrorDomain
									code:NSFileReadUnknownError
								userInfo:nil];
	return NO;
}

- (BOOL)writeToURL:(NSURL *)absoluteURL
			ofType:(NSString *)typeName
			 error:(NSError **)outError
{
//	printf("writeToURL\n");
	const char *emulationPath = [[[absoluteURL path] stringByAppendingString:@"/"]
								 UTF8String];
	if (emulation)
	{
		if (((OEEmulation *) emulation)->save(string(emulationPath)))
			return YES;
	}
	
	*outError = [NSError errorWithDomain:NSCocoaErrorDomain
									code:NSFileWriteUnknownError
								userInfo:nil];
	return NO;
}

- (void)setFileModificationDate:(NSDate *)date
{
	[super setFileModificationDate:date];

	// Update modificationDate
	NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
	[dateFormatter setDateStyle:NSDateFormatterMediumStyle];
	[dateFormatter setTimeStyle:NSDateFormatterMediumStyle];
	NSString *value;
	value = [dateFormatter stringFromDate:date];
	[dateFormatter release];
	
	[self setModificationDate:value];
}

- (IBAction)saveDocumentAsTemplate:(id)sender
{
	NSString *path = [TEMPLATE_FOLDER stringByExpandingTildeInPath];
	
	NSFileManager *fileManager = [NSFileManager defaultManager];
	if (![fileManager fileExistsAtPath:path])
		[fileManager createDirectoryAtPath:path
			   withIntermediateDirectories:YES
								attributes:nil
									 error:nil];
	
	NSSavePanel *panel = [[NSSavePanel alloc] init];
	[panel setRequiredFileType:@"emulation"];
	[panel beginSheetForDirectory:path
							 file:nil
				   modalForWindow:[self windowForSheet]
					modalDelegate:self
				   didEndSelector:@selector(saveDocumentAsTemplateDidEnd:
											returnCode:contextInfo:)
					  contextInfo:nil];
}

- (void)saveDocumentAsTemplateDidEnd:(NSSavePanel *)panel
						  returnCode:(int)returnCode
						 contextInfo:(void *)contextInfo
{
	if (returnCode != NSOKButton)
		return;
	
	NSError *error;
	if (![self writeToURL:[panel URL]
				   ofType:nil
					error:&error])
		[[NSAlert alertWithError:error] runModal];
}

- (void)makeWindowControllers
{
	NSWindowController *windowController;
	windowController = [[DocumentWindowController alloc]
						initWithWindowNibName:@"Document"];
	
	[self addWindowController:windowController];
	[windowController release];
}

- (BOOL)validateUserInterfaceItem:(id)item
{
	if ([item action] == @selector(copy:))
		return [self isCopyValid];
	else if ([item action] == @selector(paste:))
		return [self isPasteValid];
	else if ([item action] == @selector(startSpeaking:))
		return [self isCopyValid];
	
	return YES;
}

- (BOOL)isCopyValid
{
	return YES; // To-Do: libemulation
}

- (BOOL)isPasteValid
{
	return [pasteboard availableTypeFromArray:pasteboardTypes] != nil;
}

- (void)powerButtonPressedAndReleased:(id)sender
{
	[self powerButtonPressed:sender];
	[self powerButtonReleased:sender];
}

- (void)powerButtonPressed:(id)sender
{
	// To-Do: libemulation
	[self setPower:![self power]];
}

- (void)powerButtonReleased:(id)sender
{
	// To-Do: libemulation
}

- (void)resetButtonPressedAndReleased:(id)sender
{
	[self resetButtonPressed:sender];
	[self resetButtonReleased:sender];
}

- (void)resetButtonPressed:(id)sender
{
	// To-Do: libemulation
}

- (void)resetButtonReleased:(id)sender
{
	// To-Do: libemulation
}

- (void)pauseButtonPressed:(id)sender
{
	// To-Do: libemulation
}

- (NSString *)getDocumentText
{
	// To-Do: libemulation
	return @"This is a meticulously designed test of the speech synthesizing system.";  
}

- (void)copy:(id)sender
{
	if ([self isCopyValid])
	{
		[pasteboard declareTypes:pasteboardTypes owner:self];
		[pasteboard setString:[self getDocumentText] forType:NSStringPboardType];
	}
}

- (void)paste:(id)sender
{
	if ([self isPasteValid])
	{
		NSString *text = [pasteboard stringForType:NSStringPboardType];
		
		// To-do: Send to libemulator
		// (using [text UTF8String])
	}
}

- (void)startSpeaking:(id)sender
{
	NSTextView *dummy = [[NSTextView alloc] init];
	[dummy insertText:[self getDocumentText]];
	[dummy startSpeaking:self];
	[dummy release];
}

- (BOOL)power
{
	return power;
}

- (void)setPower:(BOOL)value
{
	if (power != value)
		power = value;
}

- (NSString *)label
{
	return [[label retain] autorelease];
}

- (void)setLabel:(NSString *)value
{
    if (label != value)
	{
        [label release];
        label = [value copy];
    }
}

- (NSString *)description
{
	return [[description retain] autorelease];
}

- (void)setDescription:(NSString *)value
{
    if (description != value)
	{
		if (description && value)
			[self updateChangeCount:NSChangeDone];
		
		[self setDMLProperty:@"description" value:value];
		
        [description release];
        description = [value copy];
    }
}

- (NSString *)modificationDate
{
	return modificationDate;
}

- (void)setModificationDate:(NSString *)value
{
    if (modificationDate != value)
	{
        [modificationDate release];
        modificationDate = [value copy];
    }
}

- (NSString *)runTime
{
	return runTime;
}

- (void)setRunTime:(NSString *)value
{
    if (runTime != value)
	{
        [runTime release];
        runTime = [value copy];
    }
}

- (NSImage *)image
{
	return [[image retain] autorelease];
}

- (void)setImage:(NSImage *)value
{
    if (image != value)
	{
        [image release];
        image = [value copy];
    }
}

- (NSMutableArray *)expansions
{
	return [[expansions retain] autorelease];
}

- (void)insertObject:(id)value inExpansionsAtIndex:(NSUInteger)index
{
    [expansions insertObject:value atIndex:index];
}

- (void)removeObjectFromExpansionsAtIndex:(NSUInteger)index
{
    [expansions removeObjectAtIndex:index];
}

- (NSMutableArray *)diskDrives
{
	return [[diskDrives retain] autorelease];
}

- (void)insertObject:(id)value inDiskDrivesAtIndex:(NSUInteger)index
{
    [diskDrives insertObject:value atIndex:index];
}

- (void)removeObjectFromDiskDrivesAtIndex:(NSUInteger)index
{
    [diskDrives removeObjectAtIndex:index];
}

- (NSMutableArray *)peripherals
{
	return [[peripherals retain] autorelease];
}

- (void)insertObject:(id)value inPeripheralsAtIndex:(NSUInteger)index
{
    [peripherals insertObject:value atIndex:index];
}

- (void)removeObjectFromPeripheralsAtIndex:(NSUInteger)index
{
    [peripherals removeObjectAtIndex:index];
}

- (float)brightness
{
	return brightness;
}

- (void)setBrightness:(float)value
{
    if (brightness != value)
		brightness = value;
}

- (float)contrast
{
	return contrast;
}

- (void)setContrast:(float)value
{
    if (contrast != value)
		contrast = value;
}

- (float)saturation
{
	return saturation;
}

- (void)setSaturation:(float)value
{
    if (saturation != value)
		saturation = value;
}

- (float)sharpness
{
	return sharpness;
}

- (void)setSharpness:(float)value
{
    if (sharpness != value)
		sharpness = value;
}

- (float)temperature
{
	return temperature;
}

- (void)setTemperature:(float)value
{
    if (temperature != value)
		temperature = value;
}

- (float)tint
{
	return tint;
}

- (void)setTint:(float)value
{
    if (tint != value)
		tint = value;
}

- (float)volume
{
	return volume;
}

- (void)setVolume:(float)value
{
    if (volume != value)
	{
		if (isnan(volume) && value)
			[self updateChangeCount:NSChangeDone];
		
        volume = value;
	}
}

@end
