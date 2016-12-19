#include "../SDL_third_c.h"

int WIN_IsAppInstalled(int app)
{
	return 0;
}

int WIN_Login(int app, SDL_LoginCallback callback, const char* secretkey1, const char* secretkey2, void* param)
{
	if (app == SDL_APP_WeChat) {
		// for app debug!
		callback(app, "o3LILj_Ne3ZLHeLEmWUyTjgh6lJ4", NULL, NULL, param);

	} else if (app == SDL_APP_QQ) {
		// for app debug!
		callback(app, "9F5578D0959B584D132241EFD8A6D262", NULL, NULL, param);
	} else {
		return -1;
	}
	return -1;
}

// Windows driver bootstrap functions
static int WIN_Available(void)
{
    return (1);
}

static SDL_MiniThird* WIN_CreateBle(void)
{
    SDL_MiniThird *third;

    // Initialize all variables that we clean on shutdown
    third = (SDL_MiniThird *)SDL_calloc(1, sizeof(SDL_MiniThird));
    if (!third) {
        SDL_OutOfMemory();
        return (0);
    }

    // Set the function pointers
	third->IsAppInstalled = WIN_IsAppInstalled;
	third->Login = WIN_Login;

    return third;
}


ThirdBootStrap WINDOWS_third = {
    WIN_Available, WIN_CreateBle
};

