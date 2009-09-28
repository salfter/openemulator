
/**
 * OpenEmulator
 * Mac OS X Template Chooser Window Controller
 * (C) 2009 by Marc S. Ressl (mressl@umich.edu)
 * Released under the GPL
 *
 * Controls the template chooser window.
 */

#import "TemplateChooserWindowController.h"

@implementation TemplateChooserWindowController

- (id)init:(DocumentController *)theDocumentController
{
	self = [super initWithWindowNibName:@"TemplateChooser"];
	if (self)
		documentController = theDocumentController;
	
	return self;
}

- (id)init
{
	return [self init:nil];
}

- (void)windowDidLoad
{
	// Reset window size, leave window centered on screen
}

- (void)windowWillClose:(NSNotification *)notification
{
	if (documentController)
		[documentController noteTemplateChooserWindowClosed]; 
}

- (void)performChoose:(id)sender
{
	[[self window] performClose:self];
	
	NSURL *url = [NSURL fileURLWithPath:@"/Users/mressl/Apple II.emulation"];
	
	NSError *error;
	if (![documentController openUntitledDocumentFromTemplateURL:url
														 display:YES
														   error:&error])
		[[NSAlert alertWithError:error] runModal];
}

@end
