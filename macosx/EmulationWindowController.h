
/**
 * OpenEmulator
 * Mac OS X Emulation Window Controller
 * (C) 2010-2011 by Marc S. Ressl (mressl@umich.edu)
 * Released under the GPL
 *
 * Controls an emulation window.
 */

#import <Cocoa/Cocoa.h>

@class EmulationOutlineView;
@class EmulationItem;

@interface EmulationWindowController : NSWindowController
<NSWindowDelegate, NSToolbarDelegate, NSOutlineViewDelegate,
NSOutlineViewDataSource, NSComboBoxDataSource>
{
	IBOutlet id fSplitView;
	IBOutlet EmulationOutlineView *fOutlineView;
	
	IBOutlet id fDeviceBox;
	IBOutlet id fDeviceImage;
	IBOutlet id fDeviceLocationLabel;
	IBOutlet id fDeviceStateTitle;
	IBOutlet id fDeviceStateLabel;
	IBOutlet id fDeviceButton;
	IBOutlet id fTableView;
	IBOutlet id fTableKeyColumn;
	IBOutlet id fTableValueColumn;
	
	IBOutlet id fStatusLabelView;
	
	EmulationItem *rootItem;
	
	EmulationItem *selectedItem;
	
	NSButtonCell *checkBoxCell;
	NSPopUpButtonCell *popUpButtonCell;
	NSSliderCell *sliderCell;
}

- (void)updateEmulation:(id)sender;

- (EmulationItem *)itemForSender:(id)sender;
- (BOOL)selectItem:(EmulationItem *)item withUid:(NSString *)uid;

- (IBAction)outlineAction:(id)sender;
- (IBAction)buttonAction:(id)sender;

- (IBAction)openDiskImage:(id)sender;
- (BOOL)mount:(NSString *)path inItem:(EmulationItem *)item;
- (BOOL)doMount:(NSString *)path inItem:(EmulationItem *)item;
- (IBAction)unmount:(id)sender;
- (IBAction)revealInFinder:(id)sender;
- (IBAction)showDevice:(id)sender;

- (BOOL)connect:(NSString *)thePath
          label:(NSString *)theLabel
         toItem:(EmulationItem *)theItem;
- (BOOL)doConnect:(NSString *)thePath
           toItem:(EmulationItem *)theItem;

@end
