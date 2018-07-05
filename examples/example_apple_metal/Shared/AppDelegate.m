
#import "AppDelegate.h"

@implementation AppDelegate

#if TARGET_OS_OSX
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}
#endif

@end
