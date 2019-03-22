#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import "Renderer.h"

#if TARGET_OS_IPHONE

#import <UIKit/UIKit.h>

@interface ViewController : UIViewController
@end

#else

#import <Cocoa/Cocoa.h>

@interface ViewController : NSViewController
@end

#endif
