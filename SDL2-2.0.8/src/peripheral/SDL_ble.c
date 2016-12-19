#include "../SDL_internal.h"
#include "SDL_video.h"
#include "SDL_timer.h"
#include "SDL_ble_c.h"
#include "SDL_hints.h"
#include "SDL_audio.h"

static BleBootStrap *bootstrap[] = {
#if __WIN32__
#ifdef _SDL_WIN_BLE
    &WINDOWS_ble,
#endif
#endif
#if __IPHONEOS__
    &UIKIT_ble,
#endif
#if __ANDROID__
    &Android_ble,
#endif
    NULL
};

static SDL_MiniBle* _this = NULL;
static SDL_bool quit = SDL_FALSE;

#define MAX_BLE_PERIPHERALS			48

static SDL_BlePeripheral* ble_peripherals[MAX_BLE_PERIPHERALS];
static int valid_ble_peripherals = 0;
SDL_BleCallbacks* current_callbacks = NULL;
SDL_BlePeripheral* connected_peripheral = NULL;

void mac_addr_str_2_uc6(const char* str, uint8_t* uc6, const char separator)
{
	char value_str[3];
	int i, at;
	int should_len = SDL_BLE_MAC_ADDR_BYTES * 2 + (separator? SDL_BLE_MAC_ADDR_BYTES - 1: 0);
	if (!str || SDL_strlen(str) != should_len) {
		SDL_memset(uc6, 0, SDL_BLE_MAC_ADDR_BYTES);
		return;
	}

	value_str[2] = '\0';
	for (i = 0, at = 0; i < SDL_BLE_MAC_ADDR_BYTES; i ++) {
		value_str[0] = str[at ++];
		value_str[1] = str[at ++];
		uc6[i] = (uint8_t)SDL_strtol(value_str, NULL, 16);
		if (separator && i != SDL_BLE_MAC_ADDR_BYTES - 1) {
			if (str[at] != separator) {
				SDL_memset(uc6, 0, SDL_BLE_MAC_ADDR_BYTES);
				return;
			}
			at ++;
		}
	}
}

// caller require call SDL_free to free result. 
char* mac_addr_uc6_2_str(const uint8_t* uc6, const char separator)
{
	const char ntoa_table[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

	int i, at;
	int should_len = SDL_BLE_MAC_ADDR_BYTES * 2 + (separator? SDL_BLE_MAC_ADDR_BYTES - 1: 0);
	char* result = (char*)SDL_malloc(should_len + 1);
	result[should_len] = '\0';

	for (i = 0, at = 0; i < SDL_BLE_MAC_ADDR_BYTES; i ++) {
		result[at ++] = ntoa_table[(uc6[i] & 0xf0) >> 4];
		result[at ++] = ntoa_table[uc6[i] & 0x0f];
		if (separator && i != SDL_BLE_MAC_ADDR_BYTES - 1) {
			result[at ++] = separator;
		}
	}
	return result;
}

SDL_bool mac_addr_valid(const uint8_t* addr)
{
	int at;
	for (at = 0; at < SDL_BLE_MAC_ADDR_BYTES; at ++) {
		if (addr[at]) {
			return SDL_TRUE;
		}
	}
	return SDL_FALSE;
}

// caller require call SDL_free to free result. 
char* get_full_uuid(const char* uuid)
{
	const int full_size = 36; // 32 + 4
	int size = SDL_strlen(uuid);
	char* result;

	if (size == full_size) {
		return SDL_strdup(uuid);
	}
	if (size != 4) {
		return NULL;
	}

	result = SDL_strdup("0000xxxx-0000-1000-8000-00805f9b34fb");
	SDL_memcpy(result + 4, uuid, 4);
	return result;
}

void get_full_uuid2(const char* uuid, char* result)
{
	const int full_size = 36; // 32 + 4
	int size = SDL_strlen(uuid);

	if (size == full_size) {
		SDL_strlcpy(result, uuid, full_size + 1);
		return;
	}
	if (size != 4) {
		result[0] = '\0';
		return;
	}

	SDL_memcpy(result, "0000xxxx-0000-1000-8000-00805f9b34fb", full_size);
	SDL_memcpy(result + 4, uuid, 4);
	result[full_size] = '\0';
}

SDL_BlePeripheral* find_peripheral_from_cookie(const void* cookie)
{
	int at;
	SDL_BlePeripheral* peripheral = NULL;
	for (at = 0; at < valid_ble_peripherals; at ++) {
        peripheral = ble_peripherals[at];
        if (peripheral->cookie == cookie) {
            return peripheral;
        }
	}
	return NULL;
}

SDL_BlePeripheral* find_peripheral_from_macaddr(const uint8_t* mac_addr)
{
	int at;
	SDL_BlePeripheral* peripheral = NULL;
	for (at = 0; at < valid_ble_peripherals; at ++) {
        peripheral = ble_peripherals[at];
        if (mac_addr_equal(peripheral->mac_addr, mac_addr)) {
            return peripheral;
        }
	}
	return NULL;
}

static SDL_BleService* find_service(SDL_BlePeripheral* peripheral, const char* uuid)
{
	int at;
	SDL_BleService* tmp;
	for (at = 0; at < peripheral->valid_services; at ++) {
		tmp = peripheral->services + at;
		if (SDL_BleUuidEqual(tmp->uuid, uuid)) {
			return tmp;
		}
	}
	return NULL;
}

static void free_characteristics(SDL_BleService* service)
{
	if (service->characteristics) {
		int at;
		for (at = 0; at < service->valid_characteristics; at ++) {
			SDL_BleCharacteristic* characteristic = service->characteristics + at;
			if (characteristic->cookie && _this->ReleaseCharacteristicCookie) {
				_this->ReleaseCharacteristicCookie(characteristic, at);
			}
			characteristic->cookie = NULL;
			SDL_free(characteristic->uuid);
		}
		SDL_free(service->characteristics);
		service->characteristics = NULL;
		service->valid_characteristics = 0;
	}
}

static void free_service(SDL_BleService* service, int at)
{
	free_characteristics(service);
	if (service->cookie && _this->ReleaseServiceCookie) {
		_this->ReleaseServiceCookie(service, at);
	}
	service->cookie = NULL;
	if (service->uuid) {
		SDL_free(service->uuid);
	}
	service->cookie = NULL;
}

static void free_services(SDL_BlePeripheral* peripheral)
{
	if (peripheral->services) {
		int at;
		for (at = 0; at < peripheral->valid_services; at ++) {
			free_service(peripheral->services + at, at);
		}
		SDL_free(peripheral->services);
		peripheral->services = NULL;
		peripheral->valid_services = 0;
	}
}

SDL_BleCharacteristic* find_characteristic_from_cookie(const SDL_BlePeripheral* peripheral, const void* cookie)
{
	int at, at2;
	SDL_BleService* service;
	SDL_BleCharacteristic* tmp;
	for (at = 0; at < peripheral->valid_services; at ++) {
		service = peripheral->services + at;
		for (at2 = 0; at2 < service->valid_characteristics; at2 ++) {
			tmp = service->characteristics + at2;
			if (tmp->cookie == cookie) {
				return tmp;
			}
		}	
	}
	return NULL;
}

SDL_BleCharacteristic* find_characteristic_from_uuid(const SDL_BlePeripheral* peripheral, const char* service_uuid, const char* chara_uuid)
{
	int at, at2;
	SDL_BleService* service;
	SDL_BleCharacteristic* tmp;
	for (at = 0; at < peripheral->valid_services; at ++) {
		service = peripheral->services + at;
		if (!SDL_BleUuidEqual(service->uuid, service_uuid)) {
			continue;
		}
		for (at2 = 0; at2 < service->valid_characteristics; at2 ++) {
			tmp = service->characteristics + at2;
			if (SDL_BleUuidEqual(tmp->uuid, chara_uuid)) {
				return tmp;
			}
		}	
	}
	return NULL;
}

static SDL_BlePeripheral* malloc_peripheral(const char* name)
{
	size_t s;
	SDL_BlePeripheral* peripheral = (SDL_BlePeripheral*)SDL_malloc(sizeof(SDL_BlePeripheral));
	SDL_memset(peripheral, 0, sizeof(SDL_BlePeripheral));
	ble_peripherals[valid_ble_peripherals ++] = peripheral;
    
	// field: name
	if (!name || name[0] == '\0') {
		s = 0;
	} else {
		s = SDL_strlen(name);
	}

	peripheral->name = (char*)SDL_malloc(s + 1);
	if (s) {
		SDL_memcpy(peripheral->name, name, s);
	}
	peripheral->name[s] = '\0';

	return peripheral;
}

SDL_BlePeripheral* discover_peripheral_uh_cookie(const void* cookie, const char* name)
{
	SDL_BlePeripheral* peripheral = find_peripheral_from_cookie(cookie);
	if (peripheral) {
		return peripheral;
	}

	return malloc_peripheral(name);
}

SDL_BlePeripheral* discover_peripheral_uh_macaddr(const uint8_t* mac_addr, const char* name)
{
	SDL_BlePeripheral* peripheral = find_peripheral_from_macaddr(mac_addr);
	if (peripheral) {
		return peripheral;
	}

	return malloc_peripheral(name);
}

void discover_peripheral_bh(SDL_BlePeripheral* peripheral, int rssi)
{
	peripheral->rssi = rssi;
	peripheral->receive_advertisement = SDL_GetTicks();

	if (current_callbacks && current_callbacks->discover_peripheral) {
		current_callbacks->discover_peripheral(peripheral);
	}
}

void connect_peripheral_bh(SDL_BlePeripheral* peripheral, int error)
{
	if (!error) {
		connected_peripheral = peripheral;
	}
	if (current_callbacks && current_callbacks->connect_peripheral) {
		current_callbacks->connect_peripheral(peripheral, error);
	}
}

void disconnect_peripheral_bh(SDL_BlePeripheral* peripheral, int error)
{
	if (!peripheral) {
		// when release a connected peripheral, miniBle maybe send null peripheral into it.
		return;
	}
	// memory data of service and characteristic are relative of one connection.
	// since this connection is disconnect, this data is invalid. release these data.
	free_services(peripheral);
	if (current_callbacks && current_callbacks->disconnect_peripheral) {
		current_callbacks->disconnect_peripheral(peripheral, error);
	}
	connected_peripheral = NULL;
	if (error) {
		// except disconnect, think as lost.
		SDL_BleReleasePeripheral(peripheral);
	}
}

void discover_services_uh(SDL_BlePeripheral* peripheral, int services)
{
	free_services(peripheral);
	peripheral->valid_services = services;

	if (peripheral->valid_services) {
		peripheral->services = (SDL_BleService*)SDL_malloc(peripheral->valid_services * sizeof(SDL_BleService));
		SDL_memset(peripheral->services, 0, peripheral->valid_services * sizeof(SDL_BleService));
	}
}

void discover_services_bh(SDL_BlePeripheral* peripheral, SDL_BleService* service, const char* uuid, void* cookie)
{
	size_t s = SDL_strlen(uuid);
	service->uuid = (char*)SDL_malloc(s + 1);
	SDL_memcpy(service->uuid, uuid, s);
	service->uuid[s] = '\0';
	service->cookie = cookie;
}

SDL_BleService* discover_characteristics_uh(SDL_BlePeripheral* peripheral, SDL_BleService* service, int characteristics)
{
    free_characteristics(service);
	service->valid_characteristics = characteristics;

	service->characteristics = (SDL_BleCharacteristic*)SDL_malloc(service->valid_characteristics * sizeof(SDL_BleCharacteristic));
	SDL_memset(service->characteristics, 0, service->valid_characteristics * sizeof(SDL_BleCharacteristic));

    return service;
}

void discover_characteristics_bh(SDL_BlePeripheral* peripheral, SDL_BleService* service, SDL_BleCharacteristic* characteristic, const char* uuid, void* cookie)
{
	size_t s = SDL_strlen(uuid);
	characteristic->uuid = (char*)SDL_malloc(s + 1);
	SDL_memcpy(characteristic->uuid, uuid, s);
	characteristic->uuid[s] = '\0';
	characteristic->service = service;
	characteristic->cookie = cookie;
	characteristic->properties = 0;
	// after it, miniBle should fill field: properties.
}

void release_peripherals()
{
	while (valid_ble_peripherals) {
		SDL_BlePeripheral* device = ble_peripherals[0];
		SDL_BleReleasePeripheral(device);
	}

	// valid_ble_peripherals = NULL;
	// ble = NULL;
}

void SDL_BleSetCallbacks(SDL_BleCallbacks* callbacks)
{
	current_callbacks = callbacks;
}

void SDL_BleScanPeripherals(const char* uuid)
{
	if (!_this) {
		return;
	}
	if (quit) {
		// SDL_PeripheralQuit will call SDL_BleReleasePeripheral.
		// if this peripheral is connected, app maybe call re-scan to keep discover. and discover maybe result to connect again.

		// --disconnect a peripheral
		// --rescan   <--of course, SDL don't want app to call it, but cannot avoid.
		// --discover a peripheral  <---here exists run after. once it is first, will result serious result and must avoid it.
		// --release peripheral memory.
		return;
	}

	if (_this->ScanPeripherals) {
		_this->ScanPeripherals(uuid);
	}
}

void SDL_BleStopScanPeripherals()
{
	if (!_this) {
		return;
	}
	if (_this->StopScanPeripherals) {
		_this->StopScanPeripherals();
	}
}

SDL_BleService* SDL_BleFindService(SDL_BlePeripheral* peripheral, const char* uuid)
{
	return find_service(peripheral, uuid);
}

SDL_BleCharacteristic* SDL_BleFindCharacteristic(SDL_BlePeripheral* peripheral, const char* service_uuid, const char* uuid)
{
	return find_characteristic_from_uuid(peripheral, service_uuid, uuid);
}

void SDL_BleReleasePeripheral(SDL_BlePeripheral* peripheral)
{
	int at;
	for (at = 0; at < valid_ble_peripherals; at ++) {
		SDL_BlePeripheral* tmp = ble_peripherals[at];
		if (tmp == peripheral) {
			break;
		}
	}
	if (at == valid_ble_peripherals) {
		return;
	}

	// if this peripheral is connected, disconnect first!
	// if ((_this->IsConnected && _this->IsConnected(peripheral))) {
	if (peripheral == connected_peripheral) {
		// connected peripheral maybe disconnect. 1)when peripheral is connected, bluetooth module is disabled by user.
        
		// user may quit app directly. in this case, state return CBPeripheralStateDisconnected as if it is connecting.
		// It require that connected peripherl should disconnect before release, except quit.
		// In order to safe, don't let app call SDL_BleReleasePeripheral directly.
		// here, don't call SDL_BleDisconnectPeripheral, only call app-disocnnect to let app to right state.
		if (current_callbacks && current_callbacks->disconnect_peripheral) {
			current_callbacks->disconnect_peripheral(peripheral, 0);
		}
		connected_peripheral = NULL;
	}
    
	if (current_callbacks && current_callbacks->release_peripheral) {
		current_callbacks->release_peripheral(peripheral);
	}
    
	if (peripheral->name) {
		SDL_free(peripheral->name);
		peripheral->name = NULL;
	}
	if (peripheral->uuid) {
		SDL_free(peripheral->uuid);
		peripheral->uuid = NULL;
	}
	if (peripheral->manufacturer_data) {
		SDL_free(peripheral->manufacturer_data);
		peripheral->manufacturer_data = NULL;
		peripheral->manufacturer_data_len = 0;
	}
	free_services(peripheral);
    
	if (peripheral->cookie && _this->ReleaseCookie) {
		_this->ReleaseCookie(peripheral);
	}
	peripheral->cookie = NULL;
    
	SDL_free(peripheral);
	if (at < valid_ble_peripherals - 1) {
		SDL_memcpy(ble_peripherals + at, ble_peripherals + (at + 1), (valid_ble_peripherals - at - 1) * sizeof(SDL_BlePeripheral*));
	}
	ble_peripherals[valid_ble_peripherals - 1] = NULL;
    
	valid_ble_peripherals --;
}

void SDL_BleStartAdvertise()
{
	if (!_this) {
		return;
	}
	if (_this->StartAdvertise) {
		_this->StartAdvertise();
	}
}

void SDL_BleConnectPeripheral(SDL_BlePeripheral* peripheral)
{
	if (_this->ConnectPeripheral) {
		_this->ConnectPeripheral(peripheral);
	}
	return;
}

void SDL_BleDisconnectPeripheral(SDL_BlePeripheral* peripheral)
{
	if (_this->DisconnectPeripheral) {
		_this->DisconnectPeripheral(peripheral);
	}
	return;
}

void SDL_BleGetServices(SDL_BlePeripheral* peripheral)
{
	if (_this->GetServices) {
		_this->GetServices(peripheral);
	}
}

void SDL_BleGetCharacteristics(SDL_BlePeripheral* peripheral, SDL_BleService* service)
{
	if (_this->GetCharacteristics) {
		_this->GetCharacteristics(peripheral, service);
	}
}

void SDL_BleReadCharacteristic(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic)
{
	if (!(characteristic->properties & SDL_BleCharacteristicPropertyRead)) {
		return;
	}
	if (_this->ReadCharacteristic) {
		_this->ReadCharacteristic(peripheral, characteristic);
	}
}

void SDL_BleNotifyCharacteristic(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic, SDL_bool enable)
{
	if (!enable) {
		// current not support disable notify.
		return;
	}

	if (!(characteristic->properties & (SDL_BleCharacteristicPropertyNotify | SDL_BleCharacteristicPropertyIndicate))) {
		return;
	}
	if (_this->NotifyCharacteristic) {
		_this->NotifyCharacteristic(peripheral, characteristic);
	}
}

void SDL_BleWriteCharacteristic(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic, const uint8_t* data, int size)
{
	if (!(characteristic->properties & SDL_BleCharacteristicPropertyWrite)) {
		return;
	}
	if (_this->WriteCharacteristic) {
		_this->WriteCharacteristic(peripheral, characteristic, data, size);
	}
}

void SDL_BleDiscoverDescriptors(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic)
{
	if (_this->DiscoverDescriptors) {
		_this->DiscoverDescriptors(peripheral, characteristic);
	}
}

int SDL_BleAuthorizationStatus()
{
	if (_this->AuthorizationStatus) {
		return _this->AuthorizationStatus();
	}
	return 0;
}

SDL_bool SDL_BleUuidEqual(const char* uuid1, const char* uuid2)
{
	// return SDL_strcasecmp(uuid1, uuid2)? SDL_FALSE: SDL_TRUE;
	const int full_size = 36; // 32 + 4
	static char _uuid1[37];
	static char _uuid2[37];
	int size1 = SDL_strlen(uuid1);
	int size2 = SDL_strlen(uuid2);
	const char* ptr1 = uuid1;
	const char* ptr2 = uuid2;

	if (size1 > full_size ||  size2 > full_size) {
		return SDL_FALSE;
	}
	if (size1 == size2) {
		return SDL_strcasecmp(ptr1, ptr2)? SDL_FALSE: SDL_TRUE;
	}
	if (size1 != full_size) {
		get_full_uuid2(uuid1, _uuid1);
		ptr1 = _uuid1;
	}
	if (size2 != full_size) {
		get_full_uuid2(uuid2, _uuid2);
		ptr2 = _uuid2;
	}
	return SDL_strcasecmp(ptr1, ptr2)? SDL_FALSE: SDL_TRUE;
}

void SDL_PeripheralInit()
{
	SDL_MiniBle* ble = NULL;
	int i;
	const char* hint = SDL_GetHint(SDL_HINT_BLE);

	if (!hint || hint[0] == '0') {
		return;
	}

	for (i = 0; bootstrap[i]; ++ i) {
		if (bootstrap[i]->available()) {
			ble = bootstrap[i]->create();
			break;
		}
	}

	SDL_memset(ble_peripherals, 0, sizeof(ble_peripherals));

	_this = ble;
}

void SDL_PeripheralQuit()
{
	if (!_this) {
		return;
	}

	quit = SDL_TRUE;
	release_peripherals();

	if (_this->Quit) {
		_this->Quit();
	}
	SDL_free(_this);
	_this = NULL;
}