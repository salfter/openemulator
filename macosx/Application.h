
/**
 * OpenEmulator
 * Mac OS X Application
 * (C) 2009-2010 by Marc S. Ressl (mressl@umich.edu)
 * Released under the GPL
 *
 * Intercepts app events.
 */

#import <Cocoa/Cocoa.h>

@interface Application : NSApplication
{
}

- (void) sendEvent:(NSEvent *) theEvent;

@end
