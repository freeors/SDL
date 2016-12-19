#import <Foundation/Foundation.h>
#include "../../SDL_internal.h"

#if SDL_IPHONE_QQ
@interface QQAccess : NSObject

+ (QQAccess *)defaultAccess;

- (void)login;

- (void)logout;

+ (BOOL)HandleOpenURL:(NSURL *)url;

@end
#endif

