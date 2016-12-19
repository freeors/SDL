#include "../../SDL_internal.h"
#include "SDL_video.h"
#include "SDL_third_c.h"
#include "SDL_uikitthird.h"

#if SDL_IPHONE_QQ
#import <TencentOAuth.h>
#import <QQAPIInterface.h>
#endif

#if SDL_IPHONE_WX
#import "WXApi.h"
#endif

SDL_LoginCallback login_callback = NULL;
void* login_param;
NSString* login_secretkey1 = @"";
NSString* login_secretkey2 = @""; // to WeChat, it is appid.

SDL_SendCallback send_callback = NULL;
void* send_param;

extern NSString* UIKit_GetURLScheme(NSString* identifier);

#if SDL_IPHONE_QQ
@interface QQAccess()<TencentSessionDelegate>

@end

@implementation QQAccess {
    TencentOAuth * _tencentOAuth;
    // void(^_result)(BOOL, id);
    // NSMutableDictionary * _resultDic;
}

+ (QQAccess *)defaultAccess
{
    static QQAccess * __access = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        __access = [[QQAccess alloc] init];
    });
    return __access;
}

- (id)init {
    self = [super init];
    if (self) {
        // NSString* appid = @"1105163902";
        NSString* scheme = UIKit_GetURLScheme(@"qqlogin");
        if (scheme.length < 8 || ![scheme hasPrefix:@"tencent"]) {
            return self;
        }
        NSString* appid = [scheme substringFromIndex:7];
        _tencentOAuth = [[TencentOAuth alloc] initWithAppId:appid andDelegate:self];
    }
    return self;
}

- (void)login {
    //    [self logout]; //qq sdk 2.8.1 有bug,登录前注销一下会导致检测不到qq app的安装
    
    // _result = result;
    
/*
    NSArray* _permissions = [NSArray arrayWithObjects:@"get_user_info", nil];
    NSArray* _permissions = [NSArray arrayWithObjects:@"get_simple_userinfo", @"add_t", nil];
    NSArray* _permissions = [NSArray arrayWithObjects:
                            kOPEN_PERMISSION_GET_USER_INFO,
                            kOPEN_PERMISSION_GET_SIMPLE_USER_INFO,
                            kOPEN_PERMISSION_ADD_ALBUM,
                            kOPEN_PERMISSION_ADD_ONE_BLOG,
                            kOPEN_PERMISSION_ADD_SHARE,
                            kOPEN_PERMISSION_ADD_TOPIC,
                            kOPEN_PERMISSION_CHECK_PAGE_FANS,
                            kOPEN_PERMISSION_GET_INFO,
                            kOPEN_PERMISSION_GET_OTHER_INFO,
                            kOPEN_PERMISSION_LIST_ALBUM,
                            kOPEN_PERMISSION_UPLOAD_PIC,
                            kOPEN_PERMISSION_GET_VIP_INFO,
                            kOPEN_PERMISSION_GET_VIP_RICH_INFO,
                            nil];
*/
    NSArray * _permissions = [NSArray arrayWithObjects:@"all", nil];
    [_tencentOAuth authorize:_permissions inSafari:NO];
}

- (void)logout {
    [_tencentOAuth logout:self];
}

- (void)tencentDidLogin {
    if ([_tencentOAuth accessToken] && [[_tencentOAuth accessToken] length] != 0) {
        if ([_tencentOAuth getUserInfo]) {
            NSLog(@"get user info success");
        }
    } else {
        // _result(NO,nil);
        NSLog(@"login failed");
    }
}

- (void)getUserInfoResponse:(APIResponse *)response {
/*
    _resultDic = [NSMutableDictionary dictionaryWithDictionary:
                  [NSJSONSerialization JSONObjectWithData:[[response message] dataUsingEncoding:NSUTF8StringEncoding]
                                                  options:kNilOptions error:nil]];
    [_resultDic setObject:[_tencentOAuth accessToken] forKey:TENCENT_ACCESS_TOKEN];
    [_resultDic setObject:[_tencentOAuth openId] forKey:TENCENT_OPEN_ID];
    [_resultDic setObject:[_tencentOAuth expirationDate] forKey:TENCENT_EXPIRATION_DATE];
    _result(YES, _resultDic);
*/
    NSString* openid = [_tencentOAuth openId];
    NSString* nick = [[response jsonResponse] objectForKey:@"nickname"]; // nick
    NSString* avatarurl = [[response jsonResponse] objectForKey:@"figureurl_qq_2"]; // avatar url
    login_callback(SDL_APP_QQ, openid.UTF8String, nick.UTF8String, avatarurl.UTF8String, login_param);
}

- (void)tencentDidNotLogin:(BOOL)cancelled{
    if (cancelled) {
        // _result(NO, @"user cancelled");
        NSLog(@"user cancelled");
    }else{
        // _result(NO,nil);
        NSLog(@"Login failed");
    }
}

- (void)tencentDidNotNetWork{
    NSLog(@"network error");
}

+ (BOOL)HandleOpenURL:(NSURL *)url{
    return [TencentOAuth HandleOpenURL:url];
}


@end

#endif

int UIKit_IsAppInstalled(int app)
{
	int installed = 0;
	if (app == SDL_APP_WeChat) {
#if SDL_IPHONE_WX
		if ([WXApi isWXAppInstalled]) {
			installed = 1;
		}
#endif
    } else if (app == SDL_APP_QQ) {
#if SDL_IPHONE_QQ
        if ([QQApiInterface isQQInstalled]) {
            installed = 1;
        }
#endif
    }
	return installed;
}

int UIKit_Login(int type, SDL_LoginCallback callback, const char* secretkey1, const char* secretkey2, void* param)
{
	login_callback = callback;
	login_param = param;
	if (type == SDL_APP_WeChat) {
#if SDL_IPHONE_WX
        login_secretkey1 = [NSString stringWithUTF8String:secretkey1];
        login_secretkey2 = UIKit_GetURLScheme(@"wechat");
        
		SendAuthReq* req =[[SendAuthReq alloc ] init];
		req.scope = @"snsapi_userinfo,snsapi_base";
		req.state = @"0744";
		[WXApi sendReq:req];
		return 0;
#endif
	} else if (type == SDL_APP_QQ) {
#if SDL_IPHONE_QQ
        [[QQAccess defaultAccess] login];
		return 0;
#endif
	}
	return -1;
}

#if SDL_IPHONE_WX
void wechat_registerapp()
{
    NSString* scheme = UIKit_GetURLScheme(@"wechat");
    if (scheme.length) {
        [WXApi registerApp: scheme];
    }
}

int wechat_send(int scene, int type, const uint8_t* data, const int len, const char* name, const char* title, const char* desc, const uint8_t* thumb_data, const int thumb_len, SDL_SendCallback callback, void* param)
{
    WXMediaMessage *message = [WXMediaMessage message];
    SendMessageToWXReq* req = [[SendMessageToWXReq alloc] init];
    
	send_callback = callback;
	send_param = param;

    if (thumb_data && thumb_len && thumb_len < 32768) { // WeChat require thumb data cannot execeed 32K
        NSData* thumb = [NSData dataWithBytes:thumb_data length:thumb_len];
        [message setThumbImage: [[UIImage alloc]initWithData:thumb]];
    }
    
    if (type == SDL_ThirdSendImage) {
        WXImageObject* ext = [WXImageObject object];
        ext.imageData = [NSData dataWithBytes:data length:len];
        message.mediaObject = ext;
        
        req.bText = NO;
        
    } else if (type == SDL_ThirdSendMusic) {
        NSData* tmp = [NSData dataWithBytes:data length:len];
        NSString* dataURL = [[NSString alloc] initWithData:tmp encoding:NSUTF8StringEncoding];
        NSString* musicURL = @"";
        if (name) {
            musicURL = [[NSString alloc] initWithCString:name encoding:NSUTF8StringEncoding];
        }
        
        WXMusicObject* ext = [WXMusicObject object];
        ext.musicUrl = musicURL;
        ext.musicDataUrl = dataURL;
        message.mediaObject = ext;
        
        if (title) {
            message.title = [[NSString alloc] initWithCString:title encoding:NSUTF8StringEncoding];
        }
        if (desc) {
            message.description = [[NSString alloc] initWithCString:desc encoding:NSUTF8StringEncoding];
        }
        req.bText = NO;
    }
    
    req.message = message;
    req.scene = WXSceneTimeline;
    if (scene == SDL_ThirdSceneSession) {
        req.scene = WXSceneSession;
    } else if (scene == SDL_ThirdSceneFavorite) {
        req.scene = WXSceneFavorite;
    }
    [WXApi sendReq:req];
    
    return 0;
}
#endif

int UIKit_Send(int app, int scene, int type, const uint8_t* data, const int len, const char* name, const char* title, const char* desc, const uint8_t* thumb_data, const int thumb_len, SDL_SendCallback callback, void* param)
{
	if (app == SDL_APP_WeChat) {
#if SDL_IPHONE_WX
		return wechat_send(scene, type, data, len, name, title, desc, thumb_data, thumb_len, callback, param);
#endif
	}
    return -1;
}

/* Windows driver bootstrap functions */
static int UIKit_Available(void)
{
    return (1);
}

static SDL_MiniThird* UIKit_CreateThird()
{
    SDL_MiniThird *third;

    // Initialize all variables that we clean on shutdown
    third = (SDL_MiniThird *)SDL_calloc(1, sizeof(SDL_MiniThird));
    if (!third) {
        SDL_OutOfMemory();
        return (0);
    }

    // Set the function pointers
	third->IsAppInstalled = UIKit_IsAppInstalled;
	third->Login = UIKit_Login;
	third->Send = UIKit_Send;

    return third;
}


ThirdBootStrap UIKIT_third = {
    UIKit_Available, UIKit_CreateThird
};