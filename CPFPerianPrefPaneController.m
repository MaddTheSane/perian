#import "CPFPerianPrefPaneController.h"

@implementation CPFPerianPrefPaneController

- (id)initWithBundle:(NSBundle *)bundle
{
    if ( ( self = [super initWithBundle:bundle] ) != nil ) {
    }
    
    return self;
}

- (void)mainViewDidLoad
{
	/* Read prefs here and display */
}

- (void)didUnselect
{
	/* Write prefs here */
}

//General Pane
- (IBAction)installUninstall:(id)sender
{
}
- (IBAction)updateCheck:(id)sender
{
}

- (IBAction)setAutoUpdateCheck:(id)sender
{
}


//AC3
- (IBAction)setAC3DynamicRange:(id)sender
{
}

- (IBAction)setAC3StereoOverDolby:(id)sender
{
}

//About
- (IBAction)launchWebsite:(id)sender
{
}

- (IBAction)launchDonate:(id)sender
{
}

- (IBAction)launchForum:(id)sender
{
}


@end
