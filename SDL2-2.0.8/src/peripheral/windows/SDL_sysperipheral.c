/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2018 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#include "SDL_peripheral.h"

#ifndef posix_mku32
	#define posix_mku32(l, h)		((uint32_t)(((uint16_t)(l)) | ((uint32_t)((uint16_t)(h))) << 16))
#endif

#ifndef posix_mku16
	#define posix_mku16(l, h)		((uint16_t)(((uint8_t)(l)) | ((uint16_t)((uint8_t)(h))) << 8))
#endif

SDL_bool SDL_ReadCard(SDL_IDCardInfo* info)
{
	SDL_bool leagor = SDL_TRUE;
	static uint32_t times = 0;

	SDL_memset(info, 0, sizeof(SDL_IDCardInfo));

	if (++ times % 10) {
		return SDL_FALSE;
	}

/*
	{
		int ii = 0;
		return SDL_FALSE;
	}
*/
	info->type = SDL_CardIDCard;
	if (leagor) {
		SDL_strlcpy(info->name, "test", sizeof(info->name));
		info->gender = SDL_GenderMale;
		info->folk = 1;
		SDL_strlcpy(info->birthday, "19791204", sizeof(info->birthday));
		SDL_strlcpy(info->number, "12345678", sizeof(info->number));
		SDL_strlcpy(info->address, "Zhe Jiang", sizeof(info->address));

	} else {
		SDL_strlcpy(info->name, "cotest", sizeof(info->name));
		info->gender = SDL_GenderMale;
		info->folk = 1;
		SDL_strlcpy(info->birthday, "19751004", sizeof(info->birthday));
		SDL_strlcpy(info->number, "12345678X", sizeof(info->number));
		SDL_strlcpy(info->address, "Shang Hai", sizeof(info->address));
	}

	int width = 240;
	int height = 250;
	info->photo = SDL_CreateRGBSurface(0, width, height, 4 * 8,
			0xFF0000, 0xFF00, 0xFF, 0xFF000000); // SDL_PIXELFORMAT_ARGB8888
	uint32_t* pixels = info->photo->pixels;
	for (int row = 0; row < height; row ++) {
		for (int col = 0; col < width; col ++) {
			uint32_t* pixel = pixels + row * width + col;
			if ((times / 10) & 1) {
				*pixel = 0x80ff0000;
			} else {
				*pixel = 0x800000ff;
			}
		}
	}

	return SDL_TRUE;
}

SDL_handle SDL_OpenSerialPort(const char* path, int baudrate)
{
	BYTE parity = NOPARITY;
	//打开串口  
	HANDLE m_hComm = CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL); //独占方式打开串口
	// HANDLE m_hComm = CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL); //独占方式打开串口

	if (m_hComm  == INVALID_HANDLE_VALUE) {  
		// _stprintf(err, _T("打开串口%s 失败，请查看该串口是否已被占用"), port);  
		return SDL_INVALID_HANDLE_VALUE;
	}  
  
	//MessageBox(NULL,_T("打开成功"),_T("提示"),MB_OK);  
  
	//获取串口默认配置  
	DCB dcb;  
	if (!GetCommState(m_hComm, &dcb)) {  
		// MessageBox(NULL, _T("获取串口当前属性参数失败"), _T("提示"), MB_OK);
		CloseHandle(m_hComm);
		return SDL_INVALID_HANDLE_VALUE;
	}  
  
	//配置串口参数  
	dcb.BaudRate  = baudrate;  //波特率  
	dcb.fBinary  = TRUE;            //二进制模式。必须为TRUE  
	dcb.ByteSize  = 8;  //数据位。范围4-8  
	dcb.StopBits  = ONESTOPBIT; //停止位  
  
	if (parity  == NOPARITY) {  
		dcb.fParity  = FALSE;   //奇偶校验。无奇偶校验  
		dcb.Parity  = parity;   //校验模式。无奇偶校验  
	} else {  
		dcb.fParity  = TRUE;        //奇偶校验。  
		dcb.Parity  = parity;   //校验模式。无奇偶校验  
	}  
  
	dcb.fOutxCtsFlow  = FALSE;  //CTS线上的硬件握手  
	dcb.fOutxDsrFlow  = FALSE;  //DST线上的硬件握手  
	dcb.fDtrControl  = DTR_CONTROL_ENABLE;//DTR控制  
	dcb.fDsrSensitivity  = FALSE;  
	dcb.fTXContinueOnXoff  = FALSE;//  
	dcb.fOutX  = FALSE;         //是否使用XON/XOFF协议  
	dcb.fInX  = FALSE;          //是否使用XON/XOFF协议  
	dcb.fErrorChar  = FALSE;        //是否使用发送错误协议  
	dcb.fNull  = FALSE;         //停用null stripping  
	dcb.fRtsControl  = RTS_CONTROL_ENABLE;//  
	dcb.fAbortOnError  = FALSE; //串口发送错误，并不终止串口读写  
  
								//设置串口参数  
	if (!SetCommState(m_hComm, &dcb)) {  
		// MessageBox(NULL, _T("设置串口参数失败"), _T("提示"), MB_OK);
		CloseHandle(m_hComm);
		return SDL_INVALID_HANDLE_VALUE;  
	}  
  
	//设置串口事件  
	SetCommMask(m_hComm, EV_RXCHAR);//在缓存中有字符时产生事件  
	SetupComm(m_hComm, 16384, 16384);  
  
	//设置串口读写时间  
	COMMTIMEOUTS CommTimeOuts;  
	GetCommTimeouts(m_hComm, &CommTimeOuts);  
	CommTimeOuts.ReadIntervalTimeout  = MAXDWORD;  
	CommTimeOuts.ReadTotalTimeoutMultiplier  = 0;  
	CommTimeOuts.ReadTotalTimeoutConstant  = 0;  
	CommTimeOuts.WriteTotalTimeoutMultiplier  = 10;  
	CommTimeOuts.WriteTotalTimeoutConstant  = 1000;  
  
	if (!SetCommTimeouts(m_hComm, &CommTimeOuts)) {  
		// MessageBox(NULL, _T("设置串口时间失败"), _T("提示"), MB_OK);
		CloseHandle(m_hComm);
		return SDL_INVALID_HANDLE_VALUE;  
	}  
/*  
	//创建线程，读取数据  
	HANDLE hReadCommThread = (HANDLE)_beginthreadex(NULL, 0, (PTHREEA_START)CommProc, (LPVOID) this, 0, NULL);  
*/
	return m_hComm;
}

void SDL_CloseSerialPort(SDL_handle h)
{
	if (h == SDL_INVALID_HANDLE_VALUE) {
		return;
	}
	CloseHandle(h);
}

size_t SDL_ReadSerialPort(SDL_handle h, void* ptr, size_t size)
{
	if (h == INVALID_HANDLE_VALUE) {
		return 0;
	}


	DWORD read_byte;
	BOOL ok = ReadFile(h, ptr, size, &read_byte, NULL);
	if (!ok) {
		read_byte = 0;
	}

	return read_byte;
}

size_t SDL_WriteSerialPort(SDL_handle h, const void* ptr, size_t size)
{
	const char* ptr2 = ptr;

	if (h == INVALID_HANDLE_VALUE) {
		return 0;
	}
	// clear serial port
	PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);

	DWORD write_byte = 0;
	BOOL ok = WriteFile(h, ptr, size, &write_byte, NULL);
	if (!ok) {
		write_byte = 0;
	}

	// clear serial port
	// PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);
	return write_byte;
}

void SDL_Shutdown(void)
{
}

SDL_bool SDL_SetTime(int64_t utctime)
{
	return SDL_FALSE;
}

void SDL_UpdateApp(const char* package)
{
}

void SDL_GetIpConfiguration(SDL_IpConfiguration* ipconfig)
{
	ipconfig->type = SDL_IpConfigurationStatic;
	ipconfig->ipv4 = SDL_TRUE;
	ipconfig->ipaddress = SDL_FOURCC(192, 168, 1, 120);
	ipconfig->gateway = SDL_FOURCC(192, 168, 1, 1);
	ipconfig->netmask = SDL_FOURCC(255, 255, 255, 0);
	ipconfig->dns[0] = SDL_FOURCC(9, 10, 11, 12);
	ipconfig->dns[1] = SDL_FOURCC(19, 110, 111, 112);
}

void SDL_SetIpConfiguration(const SDL_IpConfiguration* ipconfig)
{
}

void SDL_GetOsInfo(SDL_OsInfo* info)
{
	SDL_strlcpy(info->manufacturer, "Microsoft", sizeof(info->manufacturer));
	SDL_snprintf(info->display, sizeof(info->display), "%s", "windows");
}

SDL_bool SDL_PrintText(const uint8_t* bytes, int size)
{
	return SDL_TRUE;
}

void SDL_EnableRelay(SDL_bool enable)
{
}

void SDL_TurnOnFlashlight(SDL_LightColor color)
{
}


/* vi: set ts=4 sw=4 expandtab: */
