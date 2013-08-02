#import <AppKit/NSApplication.h>
extern void ASKInitialize();

int main(int argc, const char *argv[])
{
    ASKInitialize();
    return NSApplicationMain(argc, argv);
}
