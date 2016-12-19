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

#include <unistd.h>
#include <termios.h> 
#include <fcntl.h> 
#include <errno.h>
#include <time.h>
#include "SDL_peripheral.h"
#include "SDL_log.h"
#include "../../core/android/SDL_android.h"
#include <arpa/inet.h>

SDL_bool SDL_ReadCard(SDL_IDCardInfo* info)
{
	SDL_memset(info, 0, sizeof(SDL_IDCardInfo));
	return Android_JNI_ReadCard(info);
}

static speed_t getBaudrate(int baudrate)
{
	switch (baudrate) {  
	case 0:  
		return B0;  
	case 50:  
		return B50;  
	case 75:  
		return B75;  
	case 110:  
		return B110;  
	case 134:  
		return B134;  
	case 150:  
		return B150;  
	case 200:  
		return B200;  
	case 300:  
		return B300;  
	case 600:  
		return B600;  
	case 1200:  
		return B1200;  
	case 1800:  
		return B1800;  
	case 2400:  
		return B2400;  
	case 4800:  
		return B4800;  
	case 9600:  
		return B9600;  
	case 19200:  
		return B19200;  
	case 38400:  
		return B38400;  
	case 57600:  
		return B57600;  
	case 115200:  
		return B115200;  
	case 230400:  
		return B230400;  
	case 460800:  
		return B460800;  
	case 500000:  
		return B500000;  
	case 576000:  
		return B576000;  
	case 921600:  
		return B921600;  
	case 1000000:  
		return B1000000;  
	case 1152000:  
		return B1152000;  
	case 1500000:  
		return B1500000;  
	case 2000000:  
		return B2000000;  
	case 2500000:  
		return B2500000;  
	case 3000000:  
		return B3000000;  
	case 3500000:  
		return B3500000;  
	case 4000000:  
		return B4000000;  
	default:  
		return -1;  
	}
}

void test_fopen()
{
	const char* path = "/sdcard/sensetime/UUID.file";
	FILE* fp = fopen(path, "wb");
	SDL_Log("test_fopen, fopen(%s, wb), errno: %i, fp: %p", path, errno, fp);  
	if (fp == NULL) {
		return;
	}
	fwrite("1234567890", 10, 1, fp); 
	fclose(fp);
}

/* 
 * Class:     cedric_serial_SerialPort 
 * Method:    open 
 * Signature: (Ljava/lang/String;)V 
 */
SDL_handle SDL_OpenSerialPort(const char* path, int baudrate)
{  
	int fd;  
	speed_t speed;  
/* 
	{
		int ii = 0;
		test_fopen();
	}
*/
	// baudrate = 9600;
	SDL_Log("SDL_OpenSerialPort(%s, %i), E", path? path: "<nil>", baudrate);  
	// Check arguments
	{
		speed = getBaudrate(baudrate);  
		if (speed == -1) {  
			// TODO: throw an exception
			SDL_Log("Invalid baudrate");  
			return SDL_INVALID_HANDLE_VALUE;
		}  
	}  
  
	// Opening device
	{  
	//  fd = open(path, O_RDWR | O_DIRECT | O_SYNC);  
		fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK | O_NDELAY);  
		SDL_Log("open() fd = %d", fd);  
		if (fd == -1) {  
			/* Throw an exception */  
			SDL_Log("Cannot open port %d",baudrate);  
			/* TODO: throw an exception */  
			return SDL_INVALID_HANDLE_VALUE;
		}  
	}  
  
	// Configure device
	{  
		struct termios cfg;  
		if (tcgetattr(fd, &cfg)) {  
			SDL_Log("Configure device tcgetattr() failed 1");
			close(fd);  
			return SDL_INVALID_HANDLE_VALUE;  
		}  
  
		cfmakeraw(&cfg);
		cfsetispeed(&cfg, speed);
		cfsetospeed(&cfg, speed);
  
		// SDL_Log("cfg.c_cc[VTIME]: %i", cfg.c_cc[VTIME]);
		// SDL_Log("cfg.c_cc[VMIN]: %i", cfg.c_cc[VMIN]);

		if (tcsetattr(fd, TCSANOW, &cfg)) {  
			SDL_Log("Configure device tcsetattr() failed 2");  
			close(fd);  
			/* TODO: throw an exception */  
			return SDL_INVALID_HANDLE_VALUE;  
		}  
	}  

	return fd;  
}
  
/* 
* Class:     cedric_serial_SerialPort 
* Method:    close 
* Signature: ()V 
*/  
void SDL_CloseSerialPort(SDL_handle fd)
{ 
	SDL_Log("SDL_CloseSerialPort(%d), E", fd);
	if (fd == SDL_INVALID_HANDLE_VALUE) {
		return;
	}
	close(fd);  
}
  

size_t SDL_ReadSerialPort(SDL_handle fd, void* ptr, size_t size)
{
	// SDL_Log("SDL_ReadSerialPort(%d, %p, %i), E", fd, ptr, size);
	if (fd == SDL_INVALID_HANDLE_VALUE) {
		SDL_Log("SDL_ReadSerialPort, X, fd is SDL_INVALID_HANDLE_VALUE");
		return 0;
	}
	ssize_t ret = read(fd, ptr, size);
	if (ret == -1) {
		// if no data, it will return EAGAIN.
		if (errno != EAGAIN) {
			SDL_Log("SDL_ReadSerialPort, X, errno: %i <==> EAGAIN(%i)", errno, EAGAIN);
		}
		return 0;
	}
	return ret;
}

size_t SDL_WriteSerialPort(SDL_handle fd, const void* ptr, size_t size)
{
	if (fd == SDL_INVALID_HANDLE_VALUE) {
		return 0;
	}
	ssize_t ret = write(fd, ptr, size);
	if (ret == -1) {
		// think error as 0.
		SDL_Log("SDL_WriteSerialPort, X, errno: %i", errno);
		return 0;
	}
	return ret;
}

//
// gpio
//
void SDL_SetGpioRw(uint64_t mask, uint64_t rw)
{
	SDL_Log("SDL_SetGpioRw(0x%16llx, 0x%16llx), E", mask, rw);

}

void SDL_WriteGpio(uint64_t mask, uint64_t val)
{
	SDL_Log("SDL_WriteGpio(0x%16llx, 0x%16llx), E", mask, val);
}

uint64_t SDL_ReadGpio(uint64_t mask)
{
	SDL_Log("SDL_WriteGpio(0x%16llx), E", mask);

	return 0;
}

//
// system control
//
void SDL_Shutdown(void)
{
	Android_JNI_SetProperty("ctl.start", "system_shutdwon");
}

SDL_bool SDL_SetTime(int64_t utctime)
{
	time_t ts = utctime;
	const struct tm* timeptr = localtime(&ts);

	return Android_JNI_SetTime(1900 + timeptr->tm_year, timeptr->tm_mon + 1, timeptr->tm_mday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);
	return SDL_TRUE;
}

void SDL_UpdateApp(const char* package)
{
	Android_JNI_UpdateApp(package);
}

static void read_ipconfiguration(const char* filepath, SDL_IpConfiguration* ipconfiguration)
{
	SDL_Log("read_ipconfiguration, E, open %s", filepath? filepath: "<nil>");

	memset(ipconfiguration, 0, sizeof(SDL_IpConfiguration));
	ipconfiguration->type = SDL_IpConfigurationDhcp;
	ipconfiguration->ipv4 = SDL_TRUE;
	ipconfiguration->netmask = SDL_Swap32(0xffffff00);

	int32_t fsize = 0;
	SDL_RWops* __src = SDL_RWFromFile(filepath, "rb");
	if (!__src) {
		SDL_Log("read_ipconfiguration, E, open %s fail", filepath? filepath: "<nil>");
		return;
	}
	fsize = SDL_RWsize(__src);
	if (fsize < 4) {
		SDL_RWclose(__src);
		return;
	}
	SDL_RWseek(__src, 0, RW_SEEK_SET);
	
	char* data = (char*)SDL_malloc(fsize);
	SDL_RWread(__src, data, 1, fsize);

	uint16_t u16n;
	uint32_t u32n;
	memcpy(&u32n, data, 4);
	u32n = SDL_Swap32(u32n);
	if (u32n != 2) {
		return;
	}
	int pos = 4;
	const char* key;
	const char* value;
	int dns_at = 0;
	while (pos < fsize) {
		memcpy(&u16n, data + pos, 2);
		u16n = SDL_Swap16(u16n);
		pos += 2;
		key = data + pos;
		if (SDL_strlen(key) != u16n) {
			break;
		}
		pos += SDL_strlen(key);

		// value
		memcpy(&u16n, data + pos, 2);
		u16n = SDL_Swap16(u16n);
		pos += 2;
		value = data + pos;
		if (SDL_strlen(value) != u16n) {
			break;
		}

		if (!SDL_strcmp(key, "ipAssignment")) {
			if (!SDL_strcmp(value, "STATIC")) {
				ipconfiguration->type = SDL_IpConfigurationStatic;
			} else {
				ipconfiguration->type = SDL_IpConfigurationDhcp;
			}
			pos += u16n;

		} else if (!SDL_strcmp(key, "linkAddress")) {
			ipconfiguration->ipaddress = inet_addr(value);
			pos += u16n;
			pos += 4;

		} else if (!SDL_strcmp(key, "gateway")) {
			if (u16n != 0) {
				break;
			}
			pos += 2;
			memcpy(&u32n, data + pos, 4);
			u32n = SDL_Swap32(u32n);
			if (u32n != 1) {
				break;
			}
			pos += 4;
			memcpy(&u16n, data + pos, 2);
			u16n = SDL_Swap16(u16n);
			pos += 2;
			value = data + pos;
			ipconfiguration->gateway = inet_addr(value);
			pos += u16n;

		} else if (!SDL_strcmp(key, "dns")) {
			if (dns_at < 2) {
				ipconfiguration->dns[dns_at ++] = inet_addr(value);
			}
			pos += u16n;

		} else if (!SDL_strcmp(key, "id")) {
			if (u16n != 0) {
				break;
			}
			pos += 2;

			memcpy(&u16n, data + pos, 2);
			u16n = SDL_Swap16(u16n);
			pos += 2;
			pos += u16n;

		} else {
			pos += u16n;
		}
	}

	struct in_addr in;
	in.s_addr = (uint32_t)(ipconfiguration->ipaddress);
	SDL_Log("read_ipconfiguration, ipaddr: %s", inet_ntoa(in));

	SDL_free(data);
	SDL_RWclose(__src);
}

static void write_utf(SDL_RWops* fp, const char* value)
{
	uint16_t u16n = SDL_strlen(value);
	u16n = SDL_Swap16(u16n);
	SDL_RWwrite(fp, &u16n, 1, 2);
	SDL_RWwrite(fp, value, 1, SDL_strlen(value));
}

static void write_ipconfiguration(const char* filepath, const SDL_IpConfiguration* ipconfiguration)
{
	SDL_Log("write_ipconfiguration, E, open %s", filepath? filepath: "<nil>");

	if (ipconfiguration->type != SDL_IpConfigurationDhcp && ipconfiguration->type != SDL_IpConfigurationStatic) {
		SDL_Log("write_ipconfiguration, X, invalid type");
		return;
	}
	if (!ipconfiguration->ipv4) {
		SDL_Log("write_ipconfiguration, X, only support ipv4");
		return;
	}
	SDL_RWops* __dst = SDL_RWFromFile(filepath, "w+b");
	if (!__dst) {
		SDL_Log("write_ipconfiguration, X, open %s fail", filepath? filepath: "<nil>");
		return;
	}
	struct in_addr in;
	uint32_t u32n = 2;
	uint16_t u16n;
	u32n = SDL_Swap32(u32n);
	SDL_RWwrite(__dst, &u32n, 1, 4);

	write_utf(__dst, "ipAssignment");
	if (ipconfiguration->type == SDL_IpConfigurationStatic) {
		write_utf(__dst, "STATIC");
		// ipaddress
		write_utf(__dst, "linkAddress");
		in.s_addr = (uint32_t)(ipconfiguration->ipaddress);
		write_utf(__dst, inet_ntoa(in));
		u32n = 0x18;
		u32n = SDL_Swap32(u32n);
		SDL_RWwrite(__dst, &u32n, 1, 4);
		// gateway
		write_utf(__dst, "gateway");
		u32n = 0x0;
		u32n = SDL_Swap32(u32n);
		SDL_RWwrite(__dst, &u32n, 1, 4);
		u32n = 0x1;
		u32n = SDL_Swap32(u32n);
		SDL_RWwrite(__dst, &u32n, 1, 4);
		in.s_addr = (uint32_t)(ipconfiguration->gateway);
		write_utf(__dst, inet_ntoa(in));
		// dns
		for (int i = 0; i < 2; i ++) {
			if (ipconfiguration->dns[i] == 0) {
				continue;
			}
			write_utf(__dst, "dns");
			in.s_addr = (uint32_t)(ipconfiguration->dns[i]);
			write_utf(__dst, inet_ntoa(in));
		}

	} else {
		write_utf(__dst, "DHCP");

	}

	// proxySettings = NONE
	write_utf(__dst, "proxySettings");
	write_utf(__dst, "NONE");

	// id
	write_utf(__dst, "id");
	u32n = 0x0;
	u32n = SDL_Swap32(u32n);
	SDL_RWwrite(__dst, &u32n, 1, 4);
	write_utf(__dst, "eos");

	SDL_RWclose(__dst);
}

void SDL_GetIpConfiguration(SDL_IpConfiguration* ipconfig)
{
	const char* ipconfig_txt = "/data/misc/ethernet/ipconfig.txt";

	Android_JNI_SetProperty("ctl.start", "system_chmod:/data/misc/ethernet/ipconfig.txt");
	read_ipconfiguration(ipconfig_txt, ipconfig);
}

void SDL_SetIpConfiguration(const SDL_IpConfiguration* ipconfig)
{
	const char* ipconfig_txt = "/data/misc/ethernet/ipconfig.txt";
/*
	ipconfig.type = SDL_IpConfigurationStatic;
	ipconfig.ipv4 = SDL_TRUE;
	ipconfig.ipaddress = SDL_FOURCC(192, 168, 1, 120);
	ipconfig.gateway = SDL_FOURCC(192, 168, 1, 1);
	ipconfig.netmask = SDL_FOURCC(255, 255, 255, 0);
	ipconfig.dns[0] = SDL_FOURCC(9, 10, 11, 12);
	ipconfig.dns[1] = SDL_FOURCC(19, 110, 111, 112);
*/
	write_ipconfiguration(ipconfig_txt, ipconfig);
	Android_JNI_SetProperty("ctl.start", "system_chmod:/data/misc/ethernet/ipconfig.txt");
}

void SDL_GetOsInfo(SDL_OsInfo* info)
{
	Android_JNI_GetOsInfo(info);
}

SDL_bool SDL_PrintText(const uint8_t* bytes, int size)
{
	return Android_JNI_PrintText(bytes, size);
}

void SDL_EnableRelay(SDL_bool enable)
{
	Android_EnableRelay(enable);
}

void SDL_TurnOnFlashlight(SDL_LightColor color)
{
	Android_TurnOnFlashlight(color);
}


/* vi: set ts=4 sw=4 expandtab: */
