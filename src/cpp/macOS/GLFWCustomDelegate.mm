#import "GLFWCustomDelegate.h"
#import <objc/runtime.h>
#import <string>

namespace global {  // part of Monochrome
	extern void add_file_to_load(const std::string& file);
}

@implementation GLFWCustomDelegate

+ (void)load{
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		Class cls = objc_getClass("GLFWApplicationDelegate");
	
		[GLFWCustomDelegate swizzle:cls src:@selector(application:openFile:) tgt:@selector(swz_application:openFile:)];
		[GLFWCustomDelegate swizzle:cls src:@selector(application:openFiles:) tgt:@selector(swz_application:openFiles:)];
	});
}

+ (void) swizzle:(Class) original_c src:(SEL)original_s tgt:(SEL)target_s{
	Class target_c = [GLFWCustomDelegate class];
	Method originalMethod = class_getInstanceMethod(original_c, original_s);
	Method swizzledMethod = class_getInstanceMethod(target_c, target_s);

	BOOL didAddMethod =
	class_addMethod(original_c,
					original_s,
					method_getImplementation(swizzledMethod),
					method_getTypeEncoding(swizzledMethod));

	if (didAddMethod) {
		class_replaceMethod(original_c,
							target_s,
							method_getImplementation(originalMethod),
							method_getTypeEncoding(originalMethod));
	} else {
		method_exchangeImplementations(originalMethod, swizzledMethod);
	}
}

- (BOOL)swz_application:(NSApplication *)sender openFile:(NSString *)filename{
	global::add_file_to_load(filename.UTF8String);
    return YES;
}

- (void)swz_application:(NSApplication *)sender openFiles:(NSArray<NSString *> *)filenames{
	for(NSString *filename in filenames){
		global::add_file_to_load(filename.UTF8String);
	}
}

@end