#ifdef _SDL_WIN_BLE

#include "../SDL_ble_c.h"
#include "../../core/windows/SDL_windows.h"
#include <errno.h>

// SDL_windows.h forbit use UNICODE
// both setupapi.h and bluetoothleapis.h require WIN8
#undef _WIN32_WINNT
#define _WIN32_WINNT  _WIN32_WINNT_WIN8
#undef NTDDI_VERSION
#define NTDDI_VERSION   NTDDI_VERSION_FROM_WIN32_WINNT(_WIN32_WINNT)
#define INITGUID
#include <setupapi.h>
#include <bluetoothleapis.h>
// #include <cfgmgr32.h>

typedef struct {
	HANDLE hble;
	PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd;
} peripheral_context;


// include DEVPKEY_Device_BusReportedDeviceDesc from WinDDK\7600.16385.1\inc\api\devpkey.h
DEFINE_DEVPROPKEY(DEVPKEY_Device_BusReportedDeviceDesc, 0x540b947e, 0x8b40, 0x45bc, 0xa8, 0xa2, 0x6a, 0x0b, 0x89, 0x4c, 0xbd, 0xa2, 4);     // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_Device_ContainerId, 0x8c7ed206, 0x3f8a, 0x4827, 0xb3, 0xab, 0xae, 0x9e, 0x1f, 0xae, 0xfc, 0x6c, 2);     // DEVPROP_TYPE_GUID
DEFINE_DEVPROPKEY(DEVPKEY_Device_FriendlyName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 14);    // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_DeviceDisplay_Category, 0x78c34fc8, 0x104a, 0x4aca, 0x9e, 0xa4, 0x52, 0x4d, 0x52, 0x99, 0x6e, 0x57, 0x5a);  // DEVPROP_TYPE_STRING_LIST
DEFINE_DEVPROPKEY(DEVPKEY_Device_LocationInfo, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 15);    // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_Device_Manufacturer, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 13);    // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_Device_SecuritySDS, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 26);    // DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING


static void ble_get_params(HDEVINFO hDevInfo, ULONG iDevIndex, const char** macaddr, const char** name)
{
	DWORD dwSize;
	DEVPROPTYPE ulPropertyType;
	SP_DEVINFO_DATA DeviceInfoData;
	WCHAR szBuffer[512];
	char* location = NULL;
	char* desc = NULL;
	char* ptr;
	
	// Find the ones that are driverless
	DeviceInfoData.cbSize = sizeof (DeviceInfoData);
	if (!SetupDiEnumDeviceInfo(hDevInfo, iDevIndex, &DeviceInfoData)) {
		return;
	}
	
	if (SetupDiGetDeviceProperty(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_LocationInfo,
		&ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize, 0)) {
		// Bluetooth LE Device
		// Bluetooth LE Device
		location = WIN_StringToUTF8(szBuffer);
	}
	
	if (SetupDiGetDevicePropertyW(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_BusReportedDeviceDesc,
		&ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize, 0)) {
		// Bluetooth LE Device 00a050031d11
		// Bluetooth LE Device 00a050123456
		desc = WIN_StringToUTF8(szBuffer);
		ptr = SDL_strstr(desc, location);
		if (ptr == desc && SDL_strlen(desc) == SDL_strlen(location) + 1 + SDL_BLE_MAC_ADDR_BYTES * 2) {
			*macaddr = SDL_strdup(ptr + SDL_strlen(location) + 1);
		}
	}
	
	if (SetupDiGetDevicePropertyW(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_FriendlyName,
		&ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize, 0)) {
		// Thermometer
		// JM
		*name = WIN_StringToUTF8(szBuffer);
	}

	if (location) {
		SDL_free(location);
	}
	if (desc) {
		SDL_free(desc);
	}
}

void WIN_ScanPeripherals(const char* uuid)
{
	HRESULT hr = E_FAIL;

	HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_BLUETOOTHLE_DEVICE_INTERFACE, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);

	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	SP_DEVINFO_DATA deviceInfoData;
	deviceInfoData.cbSize = sizeof (deviceInfoData);

	// ULONG nGuessCount = MAXLONG;
	// for(ULONG iDevIndex=0; iDevIndex {
	ULONG iDevIndex = 0;
	while (iDevIndex < 10){
		if (SetupDiEnumDeviceInterfaces(hDevInfo, 0, &GUID_BLUETOOTHLE_DEVICE_INTERFACE, iDevIndex, &deviceInterfaceData)) {
			DWORD cbRequired = 0;

			SetupDiGetDeviceInterfaceDetail(hDevInfo,
				&deviceInterfaceData,
				0,
				0,
				&cbRequired,
				0);

			if (ERROR_INSUFFICIENT_BUFFER == GetLastError()) {
				PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd =
					(PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR,
					cbRequired);

				if (pdidd) {
					SDL_bool require_free = SDL_TRUE;
					pdidd->cbSize = sizeof(*pdidd);
					if (SetupDiGetDeviceInterfaceDetail(hDevInfo,
						&deviceInterfaceData,
						pdidd,
						cbRequired,
						&cbRequired,
						0))
					{
						char* address = NULL;
						char* name = NULL;
						ble_get_params(hDevInfo, iDevIndex, &address, &name);
						if (address && name) {
							int rssi = -1;
							uint8_t uc6[SDL_BLE_MAC_ADDR_BYTES];
							mac_addr_str_2_uc6(address, uc6, '\0');
							SDL_BlePeripheral* peripheral = discover_peripheral_uh_macaddr(uc6, name);
							if (!mac_addr_valid(peripheral->mac_addr)) {
								mac_addr_str_2_uc6(address, peripheral->mac_addr, '\0');
								peripheral_context* context = SDL_malloc(sizeof(peripheral_context));
								context->hble = INVALID_HANDLE_VALUE;
								context->pdidd = pdidd;
								peripheral->cookie = context;
								require_free = SDL_FALSE;
							}
							discover_peripheral_bh(peripheral, rssi);
						}
						if (address) {
							SDL_free(address);
						}
						if (name) {
							SDL_free(name);
						}
					}
					if (require_free) {
						LocalFree(pdidd);
					}
				}

			}

		} else if (GetLastError() == ERROR_NO_MORE_ITEMS) { // No more items
			//m_status.Format("ERROR_NO_MORE_ITEMS");
			break;
		}
		iDevIndex++;
	}
	SetupDiDestroyDeviceInfoList(hDevInfo);
}

static void WIN_ConnectPeripheral(SDL_BlePeripheral* peripheral)
{
	peripheral_context* context = peripheral->cookie;
	if (context->hble != INVALID_HANDLE_VALUE) {
		CloseHandle(context->hble);
	}
	context->hble =
		CreateFile(context->pdidd->DevicePath,
		GENERIC_READ | GENERIC_WRITE,
		// GENERIC_READ,
		// FILE_SHARE_READ | FILE_SHARE_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	connect_peripheral_bh(peripheral, context->hble != INVALID_HANDLE_VALUE ? 0 : -1 * EFAULT);
}

static void WIN_DisconnectPeripheral(SDL_BlePeripheral* peripheral)
{
	peripheral_context* context = peripheral->cookie;
	CloseHandle(context->hble);
	context->hble = INVALID_HANDLE_VALUE;

	// On android, think all disconnect to except disconnect.
	// posix_print("nativeConnectionStateChange, will call disconnect_peripheral_bh\n");
	disconnect_peripheral_bh(peripheral, -1 * EFAULT);
}

static char* GUID_TO_STRING(const GUID* uuid)
{
	int at = 0;
	const char ntoa_table[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	char* ret = SDL_strdup("00000000-0000-0000-0000-000000000000");

	ret[at++] = ntoa_table[(uuid->Data1 & 0xf0000000) >> 28];
	ret[at++] = ntoa_table[(uuid->Data1 & 0x0f000000) >> 24];
	ret[at++] = ntoa_table[(uuid->Data1 & 0x00f00000) >> 20];
	ret[at++] = ntoa_table[(uuid->Data1 & 0x000f0000) >> 16];
	ret[at++] = ntoa_table[(uuid->Data1 & 0x0000f000) >> 12];
	ret[at++] = ntoa_table[(uuid->Data1 & 0x00000f00) >> 8];
	ret[at++] = ntoa_table[(uuid->Data1 & 0x000000f0) >> 4];
	ret[at++] = ntoa_table[uuid->Data1 & 0xf];
	at++;

	ret[at++] = ntoa_table[(uuid->Data2 & 0xf000) >> 12];
	ret[at++] = ntoa_table[(uuid->Data2 & 0x0f00) >> 8];
	ret[at++] = ntoa_table[(uuid->Data2 & 0x00f0) >> 4];
	ret[at++] = ntoa_table[uuid->Data2 & 0xf];
	at++;

	ret[at++] = ntoa_table[(uuid->Data3 & 0xf000) >> 12];
	ret[at++] = ntoa_table[(uuid->Data3 & 0x0f00) >> 8];
	ret[at++] = ntoa_table[(uuid->Data3 & 0x00f0) >> 4];
	ret[at++] = ntoa_table[uuid->Data3 & 0xf];
	at++;

	ret[at++] = ntoa_table[(uuid->Data4[0] & 0xf0) >> 4];
	ret[at++] = ntoa_table[uuid->Data4[0] & 0xf];
	ret[at++] = ntoa_table[(uuid->Data4[1] & 0xf0) >> 4];
	ret[at++] = ntoa_table[uuid->Data4[1] & 0xf];
	at++;

	ret[at++] = ntoa_table[(uuid->Data4[2] & 0xf0) >> 4];
	ret[at++] = ntoa_table[uuid->Data4[2] & 0xf];
	ret[at++] = ntoa_table[(uuid->Data4[3] & 0xf0) >> 4];
	ret[at++] = ntoa_table[uuid->Data4[3] & 0xf];
	ret[at++] = ntoa_table[(uuid->Data4[4] & 0xf0) >> 4];
	ret[at++] = ntoa_table[uuid->Data4[4] & 0xf];
	ret[at++] = ntoa_table[(uuid->Data4[5] & 0xf0) >> 4];
	ret[at++] = ntoa_table[uuid->Data4[5] & 0xf];
	ret[at++] = ntoa_table[(uuid->Data4[6] & 0xf0) >> 4];
	ret[at++] = ntoa_table[uuid->Data4[6] & 0xf];
	ret[at++] = ntoa_table[(uuid->Data4[7] & 0xf0) >> 4];
	ret[at++] = ntoa_table[uuid->Data4[7] & 0xf];

	return ret;
}

static char* BTH_LE_UUID_TO_STRING(const BTH_LE_UUID* uuid)
{
	char* ret;
	int at = 0;
	if (uuid->IsShortUuid) {
		const char ntoa_table[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
		ret = SDL_strdup("0000");
		ret[at++] = ntoa_table[(uuid->Value.ShortUuid & 0xf000) >> 12];
		ret[at++] = ntoa_table[(uuid->Value.ShortUuid & 0x0f00) >> 8];
		ret[at++] = ntoa_table[(uuid->Value.ShortUuid & 0x00f0) >> 4];
		ret[at++] = ntoa_table[uuid->Value.ShortUuid & 0xF];
	}
	else {
		ret = GUID_TO_STRING(&(uuid->Value.LongUuid));
	}
	return ret;
}

static GUID BTH_LE_UUID_TO_GUID(const BTH_LE_UUID* bth_le_uuid) 
{
	if (bth_le_uuid->IsShortUuid) {
		GUID result = BTH_LE_ATT_BLUETOOTH_BASE_GUID;
		result.Data1 += bth_le_uuid->Value.ShortUuid;
		return result;
	}
	else {
		return bth_le_uuid->Value.LongUuid;
	}
}

static PBTH_LE_GATT_SERVICE GetServices(HANDLE hble, int* services)
{
	*services = 0;
	HRESULT hr = E_FAIL;
	USHORT serviceBufferCount, numServices;
	PBTH_LE_GATT_SERVICE pServiceBuffer;

	////////////////////////////////////////////////////////////////////////////
	// Determine Services Buffer Size
	////////////////////////////////////////////////////////////////////////////

	hr = BluetoothGATTGetServices(
		hble,
		0,
		NULL,
		&serviceBufferCount,
		BLUETOOTH_GATT_FLAG_NONE);

	if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr) {
		return NULL;
	}

	pServiceBuffer = (PBTH_LE_GATT_SERVICE)SDL_malloc(sizeof(BTH_LE_GATT_SERVICE)* serviceBufferCount);

	if (NULL == pServiceBuffer) {
		return NULL;
	} else {
		SDL_memset(pServiceBuffer, 0, sizeof(BTH_LE_GATT_SERVICE)* serviceBufferCount);
	}

	////////////////////////////////////////////////////////////////////////////
	// Retrieve Services
	////////////////////////////////////////////////////////////////////////////

	hr = BluetoothGATTGetServices(
		hble,
		serviceBufferCount,
		pServiceBuffer,
		&numServices,
		BLUETOOTH_GATT_FLAG_NONE);

	if (S_OK != hr) {
		SDL_free(pServiceBuffer);
		pServiceBuffer = NULL;

	} else {
		*services = serviceBufferCount;
	}
	return pServiceBuffer;
}

static PBTH_LE_GATT_CHARACTERISTIC GetCharacteristics(HANDLE hCurrService, PBTH_LE_GATT_SERVICE currGattService, int* charas)
{
	USHORT charBufferSize, numChars;
	PBTH_LE_GATT_CHARACTERISTIC pCharBuffer = NULL;

	*charas = 0;
	////////////////////////////////////////////////////////////////////////////
	// Determine Characteristic Buffer Size
	////////////////////////////////////////////////////////////////////////////

	HRESULT hr = BluetoothGATTGetCharacteristics(
		hCurrService,
		currGattService,
		0,
		NULL,
		&charBufferSize,
		BLUETOOTH_GATT_FLAG_NONE);

	if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr) {
		return NULL;
	}

	if (charBufferSize > 0) {
		pCharBuffer = (PBTH_LE_GATT_CHARACTERISTIC)SDL_malloc(charBufferSize * sizeof(BTH_LE_GATT_CHARACTERISTIC));

		if (NULL == pCharBuffer) {
			return NULL;
		} else {
			SDL_memset(pCharBuffer, 0, charBufferSize * sizeof(BTH_LE_GATT_CHARACTERISTIC));
		}

		////////////////////////////////////////////////////////////////////////////
		// Retrieve Characteristics
		////////////////////////////////////////////////////////////////////////////

		hr = BluetoothGATTGetCharacteristics(
			hCurrService,
			currGattService,
			charBufferSize,
			pCharBuffer,
			&numChars,
			BLUETOOTH_GATT_FLAG_NONE);

		if (S_OK != hr || numChars != charBufferSize) {
			SDL_free(pCharBuffer);
			pCharBuffer = NULL;

		} else {
			*charas = charBufferSize;
		}
	}
	return pCharBuffer;
}

static PBTH_LE_GATT_DESCRIPTOR GetDescriptors(HANDLE hCurrService, PBTH_LE_GATT_CHARACTERISTIC currGattChar, int* descs)
{
	USHORT descriptorBufferSize, numDescriptors;

	*descs = 0;
	////////////////////////////////////////////////////////////////////////////
	// Determine Descriptor Buffer Size
	////////////////////////////////////////////////////////////////////////////
	HRESULT hr = BluetoothGATTGetDescriptors(
		hCurrService,
		currGattChar,
		0,
		NULL,
		&descriptorBufferSize,
		BLUETOOTH_GATT_FLAG_NONE);

	if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr) {
		return NULL;
	}

	PBTH_LE_GATT_DESCRIPTOR pDescriptorBuffer;
	if (descriptorBufferSize > 0) {
		pDescriptorBuffer = (PBTH_LE_GATT_DESCRIPTOR)SDL_malloc(descriptorBufferSize * sizeof(BTH_LE_GATT_DESCRIPTOR));

		if (NULL == pDescriptorBuffer) {
			return NULL;
		} else {
			SDL_memset(pDescriptorBuffer, 0, descriptorBufferSize);
		}

		////////////////////////////////////////////////////////////////////////////
		// Retrieve Descriptors
		////////////////////////////////////////////////////////////////////////////

		hr = BluetoothGATTGetDescriptors(
			hCurrService,
			currGattChar,
			descriptorBufferSize,
			pDescriptorBuffer,
			&numDescriptors,
			BLUETOOTH_GATT_FLAG_NONE);

		if (S_OK != hr || numDescriptors != descriptorBufferSize) {
			SDL_free(pDescriptorBuffer);
			pDescriptorBuffer = NULL;
		} else {
			*descs = descriptorBufferSize;
		}
	}

	return pDescriptorBuffer;
}

static void WIN_GetServices(SDL_BlePeripheral* peripheral)
{
	peripheral_context* context = peripheral->cookie;
	int count, count2;
	PBTH_LE_GATT_SERVICE pServiceBuffer = GetServices(context->hble, &count);

	if (!pServiceBuffer) {
		if (current_callbacks && current_callbacks->discover_services) {
			current_callbacks->discover_services(peripheral, -1 * EFAULT);
		}
		return;
	}
	discover_services_uh(peripheral, count);

	int at, at2;
	for (at = 0; at < count; at++) {
		PBTH_LE_GATT_SERVICE service = pServiceBuffer + at;
		char* uuid_cstr = BTH_LE_UUID_TO_STRING(&service->ServiceUuid);

		SDL_BleService* ble_service = peripheral->services + at;
		discover_services_bh(peripheral, ble_service, uuid_cstr, service);
		SDL_free(uuid_cstr);

		
		//
		// discover characteristics of this service
		//
		PBTH_LE_GATT_CHARACTERISTIC pCharBuffer = GetCharacteristics(context->hble, service, &count2);

		ble_service = discover_characteristics_uh(peripheral, peripheral->services + at, count2);
		for (at2 = 0; at2 < count2; at2++) {
			// BluetoothGattCharacteristic chara = charas.get(at2);
			// uuid = chara.getUuid().toString();
			PBTH_LE_GATT_CHARACTERISTIC chara = pCharBuffer + at2;;
			uuid_cstr = BTH_LE_UUID_TO_STRING(&chara->CharacteristicUuid);

			SDL_BleCharacteristic* ble_characteristic = ble_service->characteristics + at2;
			discover_characteristics_bh(peripheral, ble_service, ble_characteristic, uuid_cstr, chara);
			SDL_free(uuid_cstr);

			// int property2 = chara.getProperties();
			// parse property.
			if (chara->IsBroadcastable) {
				ble_characteristic->properties |= SDL_BleCharacteristicPropertyBroadcast;
			}
			if (chara->IsReadable) {
				ble_characteristic->properties |= SDL_BleCharacteristicPropertyRead;
			}
			if (chara->IsWritableWithoutResponse) {
				ble_characteristic->properties |= SDL_BleCharacteristicPropertyWriteWithoutResponse;
			}
			if (chara->IsWritable) {
				ble_characteristic->properties |= SDL_BleCharacteristicPropertyWrite;
			}
			if (chara->IsNotifiable) {
				ble_characteristic->properties |= SDL_BleCharacteristicPropertyNotify;
			}
			if (chara->IsIndicatable) {
				ble_characteristic->properties |= SDL_BleCharacteristicPropertyIndicate;
			}
			if (chara->IsSignedWritable) {
				ble_characteristic->properties |= SDL_BleCharacteristicPropertyAuthenticatedSignedWrites;
			}
			if (chara->HasExtendedProperties) {
				ble_characteristic->properties |= SDL_BleCharacteristicPropertyExtendedProperties;
			}
		}
		// SDL_free(pCharBuffer);
	}

	if (current_callbacks && current_callbacks->discover_services) {
		current_callbacks->discover_services(peripheral, 0);
	}

	// SDL_free(pServiceBuffer);
}

static void WIN_GetCharacteristics(const SDL_BlePeripheral* peripheral, SDL_BleService* service)
{
	if (peripheral != connected_peripheral) {
		return;
	}
	if (current_callbacks && current_callbacks->discover_characteristics) {
		current_callbacks->discover_characteristics(connected_peripheral, service, 0);
	}
}

static HANDLE createServiceHandle(const BTH_LE_UUID* bth_le_uuid, SDL_bool read_write)
{
	GUID BTDeviceInterfaceGUID = BTH_LE_UUID_TO_GUID(bth_le_uuid);
	HRESULT hr = E_FAIL;

	HDEVINFO hDevInfo = SetupDiGetClassDevs(&BTDeviceInterfaceGUID, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
	if (hDevInfo == INVALID_HANDLE_VALUE) {
		return INVALID_HANDLE_VALUE;
	}

	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	SP_DEVINFO_DATA deviceInfoData;
	deviceInfoData.cbSize = sizeof (deviceInfoData);

	// ULONG nGuessCount = MAXLONG;
	// for(ULONG iDevIndex=0; iDevIndex {
	ULONG iDevIndex = 0;
	while (iDevIndex < 10){
		if (SetupDiEnumDeviceInterfaces(hDevInfo, 0, &BTDeviceInterfaceGUID, iDevIndex, &deviceInterfaceData)) {
			DWORD cbRequired = 0;

			SetupDiGetDeviceInterfaceDetail(hDevInfo,
				&deviceInterfaceData,
				0,
				0,
				&cbRequired,
				0);

			if (ERROR_INSUFFICIENT_BUFFER == GetLastError()) {
				PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd =
					(PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR,
					cbRequired);

				if (pdidd) {
					pdidd->cbSize = sizeof(*pdidd);
					if (SetupDiGetDeviceInterfaceDetail(hDevInfo,
						&deviceInterfaceData,
						pdidd,
						cbRequired,
						&cbRequired,
						0))
					{
						HANDLE hble =
							CreateFile(pdidd->DevicePath,
							read_write? GENERIC_WRITE | GENERIC_READ : GENERIC_READ,
							FILE_SHARE_READ,
							NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							NULL);
						return hble;
						
					}
					LocalFree(pdidd);
				}

			}

		}
		else if (GetLastError() == ERROR_NO_MORE_ITEMS) { // No more items
			//m_status.Format("ERROR_NO_MORE_ITEMS");
			break;
		}
		iDevIndex++;
	}
	SetupDiDestroyDeviceInfoList(hDevInfo);

	return INVALID_HANDLE_VALUE;
}

static void WIN_ReadCharacteristic(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic)
{
	HRESULT hr;
	USHORT charValueDataSize;
	peripheral_context* context = peripheral->cookie;
	PBTH_LE_GATT_SERVICE service_cookie = characteristic->service->cookie;
	PBTH_LE_GATT_CHARACTERISTIC currGattChar = characteristic->cookie;
	PBTH_LE_GATT_CHARACTERISTIC_VALUE pCharValueBuffer = NULL;

	////////////////////////////////////////////////////////////////////////////
	// Determine Characteristic Value Buffer Size
	////////////////////////////////////////////////////////////////////////////
	HANDLE hService = createServiceHandle(&service_cookie->ServiceUuid, SDL_FALSE);
	if (hService == INVALID_HANDLE_VALUE) {
		return;
	}

	hr = BluetoothGATTGetCharacteristicValue(
		hService,
		currGattChar,
		0,
		NULL,
		&charValueDataSize,
		BLUETOOTH_GATT_FLAG_NONE | BLUETOOTH_GATT_FLAG_FORCE_READ_FROM_DEVICE);

	if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) == hr) {
		pCharValueBuffer = (PBTH_LE_GATT_CHARACTERISTIC_VALUE)SDL_malloc(charValueDataSize);
		if (pCharValueBuffer) {
			SDL_memset(pCharValueBuffer, 0, charValueDataSize);
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// Retrieve the Characteristic Value
	////////////////////////////////////////////////////////////////////////////

	if (pCharValueBuffer) {
		hr = BluetoothGATTGetCharacteristicValue(
			hService,
			currGattChar,
			(ULONG)charValueDataSize,
			pCharValueBuffer,
			NULL,
			BLUETOOTH_GATT_FLAG_NONE);

		if (S_OK == hr) {
			if (current_callbacks && current_callbacks->read_characteristic) {
				current_callbacks->read_characteristic(peripheral, characteristic, pCharValueBuffer->Data, pCharValueBuffer->DataSize);
			}
		}
	}

	CloseHandle(hService);
	// Free before going to next iteration, or memory leak.
	SDL_free(pCharValueBuffer);
	pCharValueBuffer = NULL;
}

// BluetoothApis.dll thread
VOID Callback(_In_ BTH_LE_GATT_EVENT_TYPE EventType, _In_ PVOID EventOutParameter, _In_opt_ PVOID Context) 
{
	PBLUETOOTH_GATT_VALUE_CHANGED_EVENT pevent = EventOutParameter;
	SDL_BleCharacteristic* characteristic = Context;

	if (!connected_peripheral) {
		return;
	}
	
	if (current_callbacks && current_callbacks->read_characteristic) {
		current_callbacks->read_characteristic(connected_peripheral, characteristic, pevent->CharacteristicValue->Data, pevent->CharacteristicValue->DataSize);
	}
}

static void WIN_NotifyCharacteristic(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic)
{
	peripheral_context* context = peripheral->cookie;
	PBTH_LE_GATT_SERVICE service_cookie = characteristic->service->cookie;
	PBTH_LE_GATT_CHARACTERISTIC currGattChar = characteristic->cookie;

	HANDLE hService = createServiceHandle(&service_cookie->ServiceUuid, SDL_TRUE);
	if (hService == INVALID_HANDLE_VALUE) {
		return;
	}

	BLUETOOTH_GATT_VALUE_CHANGED_EVENT_REGISTRATION reg;
	reg.NumCharacteristics = 0x1;
	reg.Characteristics[0] = *currGattChar;

	BLUETOOTH_GATT_EVENT_HANDLE  event_handle;
	HRESULT hr = BluetoothGATTRegisterEvent(
		hService,
		CharacteristicValueChangedEvent,
		&reg,
		Callback,
		characteristic,
		&event_handle,
		BLUETOOTH_GATT_FLAG_NONE);

	int descs;
	PBTH_LE_GATT_DESCRIPTOR pDescriptorBuffer = NULL, parentDescriptor = NULL;
	if (SUCCEEDED(hr)) {
		pDescriptorBuffer = GetDescriptors(context->hble, currGattChar, &descs);
		if (pDescriptorBuffer) {
			// static USHORT UUID_CLIENT_CHARACTERISTIC_CONFIG = 0x2902;
			for (int at = 0; at < descs; at++) {
				PBTH_LE_GATT_DESCRIPTOR desc = pDescriptorBuffer + at;
				if (desc->DescriptorType == ClientCharacteristicConfiguration) {
					parentDescriptor = desc;
					break;
				}
			}
		}
	}
	if (parentDescriptor) {
		SDL_bool indicate = characteristic->properties & SDL_BleCharacteristicPropertyIndicate ? SDL_TRUE : SDL_FALSE;
		// BluetoothGattDescriptor descriptor = chara.getDescriptor(UUID.fromString(CLIENT_CHARACTERISTIC_CONFIG));
		// descriptor.setValue(enable? BluetoothGattDescriptor.ENABLE_INDICATION_VALUE: BluetoothGattDescriptor.DISABLE_NOTIFICATION_VALUE);
		// mBluetoothGatt.writeDescriptor(descriptor);
		BTH_LE_GATT_DESCRIPTOR_VALUE newValue;
		SDL_memset(&newValue, 0, sizeof(newValue));

		newValue.DescriptorType = ClientCharacteristicConfiguration;
		newValue.ClientCharacteristicConfiguration.IsSubscribeToIndication = indicate? TRUE: FALSE;
		newValue.ClientCharacteristicConfiguration.IsSubscribeToNotification = indicate? FALSE: TRUE;

		// Subscribe to an event. block until receive ble response.
		hr = BluetoothGATTSetDescriptorValue(hService,
			parentDescriptor,
			&newValue,
			BLUETOOTH_GATT_FLAG_NONE);

		if (current_callbacks && current_callbacks->notify_characteristic) {
			current_callbacks->notify_characteristic(peripheral, characteristic, SUCCEEDED(hr) ? 0 : -1 * EFAULT);
		}
	}
	CloseHandle(hService);

	if (pDescriptorBuffer) {
		SDL_free(pDescriptorBuffer);
	}
}

static void WIN_WriteCharacteristic(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic, const unsigned char* data, int size)
{
	HRESULT hr;
	peripheral_context* context = peripheral->cookie;
	PBTH_LE_GATT_SERVICE service_cookie = characteristic->service->cookie;
	PBTH_LE_GATT_CHARACTERISTIC currGattChar = characteristic->cookie;

	////////////////////////////////////////////////////////////////////////////
	// Determine Characteristic Value Buffer Size
	////////////////////////////////////////////////////////////////////////////
	HANDLE hService = createServiceHandle(&service_cookie->ServiceUuid, SDL_TRUE);
	if (hService == INVALID_HANDLE_VALUE) {
		return;
	}

	BTH_LE_GATT_RELIABLE_WRITE_CONTEXT ReliableWriteContext = 0;
	hr = BluetoothGATTBeginReliableWrite(hService,
		&ReliableWriteContext,
		BLUETOOTH_GATT_FLAG_NONE);

	if (SUCCEEDED(hr)) {
		size_t required_length = size + offsetof(BTH_LE_GATT_CHARACTERISTIC_VALUE, Data);
		PBTH_LE_GATT_CHARACTERISTIC_VALUE newValue = SDL_malloc(required_length);
		newValue->DataSize = size;
		SDL_memcpy(newValue->Data, data, size);

		// Set the new characteristic value
		hr = BluetoothGATTSetCharacteristicValue(hService,
			currGattChar,
			newValue,
			0,
			BLUETOOTH_GATT_FLAG_NONE);

		if (current_callbacks && current_callbacks->write_characteristic) {
			current_callbacks->write_characteristic(peripheral, characteristic, SUCCEEDED(hr) ? 0 : -1 * EFAULT);
		} 

		SDL_free(newValue);
	}

	if (ReliableWriteContext != 0) {
		BluetoothGATTEndReliableWrite(hService,
			ReliableWriteContext,
			BLUETOOTH_GATT_FLAG_NONE);
	}

	CloseHandle(hService);
}

static void WIN_ReleaseCharacteristicCookie(const SDL_BleCharacteristic* characteristic, int at)
{
	if (at == 0) {
		SDL_free(characteristic->cookie);
	}
}

static void WIN_ReleaseServiceCookie(const SDL_BleService* service, int at)
{
	if (at == 0) {
		SDL_free(service->cookie);
	}
}

static void WIN_ReleaseCookie(const SDL_BlePeripheral* peripheral)
{
	peripheral_context* context = peripheral->cookie;
	if (context->hble != INVALID_HANDLE_VALUE) {
		CloseHandle(context->hble);
	}
	LocalFree(context->pdidd);
	SDL_free(context);
}

// Windows driver bootstrap functions
static int WIN_Available(void)
{
    return (1);
}

static SDL_MiniBle* WIN_CreateBle(void)
{
    SDL_MiniBle *ble;

    // Initialize all variables that we clean on shutdown
    ble = (SDL_MiniBle *)SDL_calloc(1, sizeof(SDL_MiniBle));
    if (!ble) {
        SDL_OutOfMemory();
        return (0);
    }

    // Set the function pointers
	ble->ScanPeripherals = WIN_ScanPeripherals;
	ble->ConnectPeripheral = WIN_ConnectPeripheral;
	ble->DisconnectPeripheral = WIN_DisconnectPeripheral;
	ble->GetServices = WIN_GetServices;
	ble->GetCharacteristics = WIN_GetCharacteristics;
	ble->ReadCharacteristic = WIN_ReadCharacteristic;
	ble->NotifyCharacteristic = WIN_NotifyCharacteristic;
	ble->WriteCharacteristic = WIN_WriteCharacteristic;
	ble->ReleaseServiceCookie = WIN_ReleaseServiceCookie;
	ble->ReleaseCharacteristicCookie = WIN_ReleaseCharacteristicCookie;
	ble->ReleaseCookie = WIN_ReleaseCookie;
	// ble->Quit = WIN_Quit;

    return ble;
}


BleBootStrap WINDOWS_ble = {
    WIN_Available, WIN_CreateBle
};

#endif
