
#import <TargetConditionals.h>

#if TARGET_OS_IPHONE

#import <UIKit/UIKit.h>
#import "AppDelegate.h"

int main(int argc, char * argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}

#else

#import <Cocoa/Cocoa.h>

int main(int argc, const char * argv[]) {
    return NSApplicationMain(argc, argv);
}

#endif
