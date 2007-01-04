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

- (IBAction)installUninstall:(id)sender
{
}

@end
