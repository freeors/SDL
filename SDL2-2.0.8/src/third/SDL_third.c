#include "../SDL_internal.h"
#include "SDL_video.h"
#include "SDL_third_c.h"

static ThirdBootStrap *bootstrap[] = {
#if __WIN32__
    &WINDOWS_third,
#endif
#if __IPHONEOS__
    &UIKIT_third,
#endif
#if __ANDROID__
    &Android_third,
#endif
    NULL
};

static SDL_MiniThird* _this = NULL;

// 0: not installed
// 1: installed
int SDL_IsAppInstalled(int app)
{
	if (_this->IsAppInstalled) {
		return _this->IsAppInstalled(app);
	}
	return 0;
}

// -1: parameter error or fail
// 0: success
int SDL_ThirdLogin(int app, SDL_LoginCallback callback, const char* secretkey1, const char* secretkey2, void* param)
{
	if (_this->Login) {
		return _this->Login(app, callback, secretkey1, secretkey2, param);

	}
	return -1;
}

int SDL_ThirdSend(int app, int scene, int type, const uint8_t* data, const int len, const char* name, const char* title, const char* desc, const uint8_t* thumb, const int thumb_len, SDL_SendCallback callback, void* param)
{
	if (_this->Send) {
		return _this->Send(app, scene, type, data, len, name, title, desc, thumb, thumb_len, callback, param);
	}
	return -1;
}

void SDL_ThirdInit()
{
	SDL_MiniThird* third = NULL;
	int i;

	for (i = 0; bootstrap[i]; ++ i) {
		if (bootstrap[i]->available()) {
			third = bootstrap[i]->create();
			break;
		}
	}

	_this = third;
}

void SDL_ThirdQuit()
{
	SDL_free(_this);
	_this = NULL;
}