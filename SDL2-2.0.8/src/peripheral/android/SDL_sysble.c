#include "../SDL_ble_c.h"
#include "../../core/android/SDL_android.h"
#include "../../events/SDL_events_c.h"
#include "SDL_mutex.h"
#include <errno.h>

#include <android/log.h>
#define posix_print(format, ...) __android_log_print(ANDROID_LOG_INFO, "SDL", format, ##__VA_ARGS__)

static jobject adapter = NULL;
static const char* UUID_CLIENT_CHARACTERISTIC_CONFIG = "00002902-0000-1000-8000-00805f9b34fb";

typedef enum {
	did_discoverperipheral,
	did_connectionstatechange,
	did_servicesdiscovered,
	did_characteristicread,
	did_characteristicwrite,
	did_descriptorwrite,
} BleDidType;

typedef struct {
	uint32_t type;
	jobject device;
	int rssi;
} DidDiscoverPeripheral;

typedef struct {
	uint32_t type;
	jobject gatt;
	SDL_bool connected;
} DidConnectionStateChange;

typedef struct {
	uint32_t type;
	jobject gatt;
	SDL_bool success;
} DidServicesDiscovered;

typedef struct {
	uint32_t type;
	jobject chara;
	jbyteArray buffer;
	SDL_bool notify;
} DidCharacteristicRead;

typedef struct {
	uint32_t type;
	jobject chara;
	SDL_bool success;
} DidCharacteristicWrite;

typedef struct {
	uint32_t type;
	jobject descriptor;
} DidDescriptorWrite;

typedef union {
	uint32_t type;
	DidDiscoverPeripheral DiscoverPeripheral;
	DidConnectionStateChange ConnectionStateChange;
	DidServicesDiscovered ServicesDiscovered;
	DidCharacteristicRead CharacteristicRead;
	DidCharacteristicWrite CharacteristicWrite;
	DidDescriptorWrite DescriptorWrite;
} BleDid;

#define BLEDID_COUNT	24
static BleDid BleDids[BLEDID_COUNT];
static int bledids_wt = 0;
static int bledids_rd = -1;
static SDL_mutex* bledid_lock = NULL;

static int times = 0;
void push_bledid(BleDid* did)
{
	// posix_print("#%i, %i, write, type: %i, <#%i>\n", bledids_wt, SDL_GetTicks(), did->type, times ++);

	if (bledids_rd == -1) {
		bledids_rd = bledids_wt;
	}

	bledids_wt ++;
	if (bledids_wt == BLEDID_COUNT) {
		bledids_wt = 0;
	}

	// max cache is BLEDID_COUNT - 1.
	if (bledids_rd == bledids_wt) {
		// if read at this, to avoid race to loop, move read
		posix_print("bledids_rd(%i) == bledids_wt, will increase.\n", bledids_rd);
		bledids_rd ++;
		if (bledids_rd == BLEDID_COUNT) {
			bledids_rd = 0;
		}
	}
}

static void nativeDiscoverPeripheral(JNIEnv* env, jobject device, int rssi);
static void nativeConnectionStateChange(JNIEnv* env, jobject gatt, SDL_bool connected);
static void nativeServicesDiscovered(JNIEnv* env, jobject gatt, SDL_bool success);
static void nativeCharacteristicRead(JNIEnv* env, jobject chara, jbyteArray buffer, jboolean notify);
static void nativeCharacteristicWrite(JNIEnv* env, jobject chara, jboolean success);
static void nativeDescriptorWrite(JNIEnv* env, jobject descriptor);

// They don't call by Java VM, require to release all local references by themself.
// I cannot make sure not leak release one local references, use PushLocalFrame/PopLocalFrame.
#define LocalReference_Init(env)	\
	const int capacity = 16;	\
	(*(env))->PushLocalFrame(env, capacity)

#define LocalReference_Cleanup(env)	\
	(*(env))->PopLocalFrame(env, NULL)

void Android_PumpBleDid(SDL_bool foreground)
{
	// in order to avoid Lock/Unlock, place condition outer.
	if (bledids_rd == -1) {
		return;
	}

	SDL_LockMutex(bledid_lock);
	// below condition should place again after LockMutex.
	// other thread may modify beldids_rd when this thread LockMutex.
	if (bledids_rd == -1) {
		posix_print("Android_PumpBleDid, bledids_rd == -1, %s\n", foreground? "foreground": "background");
		SDL_UnlockMutex(bledid_lock);
		return;
	}

	JNIEnv* env = Android_JNI_GetEnv();
	LocalReference_Init(env);

	for ( ; bledids_rd != bledids_wt; ) {
		BleDid* did = &BleDids[bledids_rd];
		// posix_print("#%i, %i, read, type: %i, %s\n", bledids_rd, SDL_GetTicks(), did->type, foreground? "foreground": "background");

		if (did->type == did_discoverperipheral) {
			nativeDiscoverPeripheral(env, did->DiscoverPeripheral.device, did->DiscoverPeripheral.rssi);

		} else if (did->type == did_connectionstatechange) {
			nativeConnectionStateChange(env, did->ConnectionStateChange.gatt, did->ConnectionStateChange.connected);

		} else if (did->type == did_servicesdiscovered) {
			nativeServicesDiscovered(env, did->ServicesDiscovered.gatt, did->ServicesDiscovered.success);

		} else if (did->type == did_characteristicread) {
			nativeCharacteristicRead(env, did->CharacteristicRead.chara, did->CharacteristicRead.buffer, did->CharacteristicRead.notify);

		} else if (did->type == did_characteristicwrite) {
			nativeCharacteristicWrite(env, did->CharacteristicWrite.chara, did->CharacteristicWrite.success);

		} else if (did->type == did_descriptorwrite) {
			nativeDescriptorWrite(env, did->DescriptorWrite.descriptor);
		}

		// if read at this, to avoid race to loop, move read
		bledids_rd ++;
		if (bledids_rd == BLEDID_COUNT) {
			bledids_rd = 0;
		}
	}
	LocalReference_Cleanup(env);

	// no readable data
	bledids_rd = -1;
	SDL_UnlockMutex(bledid_lock);
}

void nativeDiscoverPeripheral(JNIEnv* env, jobject device, int rssi)
{
	jmethodID mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, device), "getAddress", "()Ljava/lang/String;");
	jstring jaddress = (*env)->CallObjectMethod(env, device, mid);

	uint8_t uc6[SDL_BLE_MAC_ADDR_BYTES];
	const char* address = (*env)->GetStringUTFChars(env, jaddress, NULL);
	mac_addr_str_2_uc6(address, uc6, ':');
	if (!mac_addr_valid(uc6)) {
		// invalid mac addr. do nothing.
		(*env)->ReleaseStringUTFChars(env, jaddress, address);
		(*env)->DeleteLocalRef(env, jaddress);
		(*env)->DeleteGlobalRef(env, device);
		return;
	}

	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, device), "getName", "()Ljava/lang/String;");
	jstring jname = (*env)->CallObjectMethod(env, device, mid);

	// name maybe null.
	const char* name = NULL;
	if (jname) {
		name = (*env)->GetStringUTFChars(env, jname, NULL);
	}

	// On android, there isn't unique variable. Since mac_addr is unique, use include it's SDL_BlePeripheral.
	SDL_BlePeripheral* peripheral = discover_peripheral_uh_macaddr(uc6, name);
	if (!mac_addr_valid(peripheral->mac_addr)) {
		posix_print("scan new peripheral: %p, address: %s, name: %s\n", peripheral, address, name);
		mac_addr_str_2_uc6(address, peripheral->mac_addr, ':');
	}
	discover_peripheral_bh(peripheral, rssi);

	(*env)->ReleaseStringUTFChars(env, jaddress, address);
	(*env)->DeleteLocalRef(env, jaddress);
	if (jname) {
		// if name is null, call ReleaseStringUTFChars will result in access-deny.
		(*env)->ReleaseStringUTFChars(env, jname, name);
		(*env)->DeleteLocalRef(env, jname);
	}

	// release global reference
	(*env)->DeleteGlobalRef(env, device);
}

void PUSH_BLEDID_BH()
{
	if (didbackground) {
		Android_PumpBleDid(SDL_FALSE);
	}
}

#define PUSH_BLEDID(EVALUATE)	\
	SDL_LockMutex(bledid_lock);	\
	BleDid* did = &BleDids[bledids_wt];	\
	EVALUATE	\
	push_bledid(did);	\
	SDL_UnlockMutex(bledid_lock);	\
	PUSH_BLEDID_BH();

JNIEXPORT void JNICALL Java_org_libsdl_app_SDLActivity_nativeDiscoverPeripheral(
    JNIEnv* env, jclass jcls,
    jobject device, jint rssi)
{
	PUSH_BLEDID(
		did->type = did_discoverperipheral;
		did->DiscoverPeripheral.device = (*env)->NewGlobalRef(env, device);
		did->DiscoverPeripheral.rssi = rssi;
		);
}

void nativeConnectionStateChange(JNIEnv* env, jobject gatt, SDL_bool connected)
{
	// address = gatt.getDevice().getAddress();
	jmethodID mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, gatt), "getDevice", "()Landroid/bluetooth/BluetoothDevice;");
	jobject device = (*env)->CallObjectMethod(env, gatt, mid);
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, device), "getAddress", "()Ljava/lang/String;");
	jstring jaddress = (*env)->CallObjectMethod(env, device, mid);

	uint8_t uc6[SDL_BLE_MAC_ADDR_BYTES];
	const char* address = (*env)->GetStringUTFChars(env, jaddress, NULL);
	mac_addr_str_2_uc6(address, uc6, ':');
	(*env)->ReleaseStringUTFChars(env, jaddress, address);

	SDL_BlePeripheral* peripheral = find_peripheral_from_macaddr(uc6);
	if (connected) {
		connect_peripheral_bh(peripheral, 0);
	} else {
		// On android, think all disconnect to except disconnect.
		// posix_print("nativeConnectionStateChange, will call disconnect_peripheral_bh\n");
		disconnect_peripheral_bh(peripheral, -1 * EFAULT);
	}

	// release global reference
	(*env)->DeleteGlobalRef(env, gatt);
}

JNIEXPORT void JNICALL Java_org_libsdl_app_SDLActivity_nativeConnectionStateChange(
    JNIEnv* env, jclass jcls,
    jobject gatt, jboolean connected)
{
	PUSH_BLEDID(
		did->type = did_connectionstatechange;
		did->ConnectionStateChange.gatt = (*env)->NewGlobalRef(env, gatt);
		did->ConnectionStateChange.connected = connected? SDL_TRUE: SDL_FALSE;
		);
}

void nativeServicesDiscovered(JNIEnv* env, jobject gatt, SDL_bool success)
{
	SDL_BlePeripheral* peripheral = connected_peripheral;
	if (!success) {
		if (current_callbacks && current_callbacks->discover_services) {
			current_callbacks->discover_services(peripheral, -1 * EFAULT);
		}
		return;
	}

	// if jobject is parameter, requrie call GetObjectClass, or result in below error:
	//     Class 'xxx' was optimized without verification; not verifying now

	// prepare PROPERTY_XXX
	jclass classClass = (*env)->FindClass(env, "android/bluetooth/BluetoothGattCharacteristic");
	jfieldID fid = (*env)->GetStaticFieldID(env, classClass, "PROPERTY_BROADCAST", "I");
	const int PROPERTY_BROADCAST = (*env)->GetStaticIntField(env, classClass, fid);
	fid = (*env)->GetStaticFieldID(env, classClass, "PROPERTY_READ", "I");
	const int PROPERTY_READ = (*env)->GetStaticIntField(env, classClass, fid);
	fid = (*env)->GetStaticFieldID(env, classClass, "PROPERTY_WRITE_NO_RESPONSE", "I");
	const int PROPERTY_WRITE_NO_RESPONSE = (*env)->GetStaticIntField(env, classClass, fid);
	fid = (*env)->GetStaticFieldID(env, classClass, "PROPERTY_WRITE", "I");
	const int PROPERTY_WRITE = (*env)->GetStaticIntField(env, classClass, fid);
	fid = (*env)->GetStaticFieldID(env, classClass, "PROPERTY_NOTIFY", "I");
	const int PROPERTY_NOTIFY = (*env)->GetStaticIntField(env, classClass, fid);
	fid = (*env)->GetStaticFieldID(env, classClass, "PROPERTY_INDICATE", "I");
	const int PROPERTY_INDICATE = (*env)->GetStaticIntField(env, classClass, fid);
	fid = (*env)->GetStaticFieldID(env, classClass, "PROPERTY_SIGNED_WRITE", "I");
	const int PROPERTY_SIGNED_WRITE = (*env)->GetStaticIntField(env, classClass, fid);
	fid = (*env)->GetStaticFieldID(env, classClass, "PROPERTY_EXTENDED_PROPS", "I");
	const int PROPERTY_EXTENDED_PROPS = (*env)->GetStaticIntField(env, classClass, fid);

	// List<BluetoothGattService> services = gatt.getServices();
	// int count = services.size();
	jmethodID mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, gatt), "getServices", "()Ljava/util/List;");
	jobject services = (*env)->CallObjectMethod(env, gatt, mid);
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, services), "size", "()I");
	int count = (*env)->CallIntMethod(env, services, mid);

	discover_services_uh(peripheral, count);

	int at, at2;
	for (at = 0; at < count; at ++) {
		// BluetoothGattService service = services.get(at);
		// uuid = service.getUuid().toString();
		mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, services), "get", "(I)Ljava/lang/Object;");
		jobject service = (*env)->CallObjectMethod(env, services, mid, at);

		mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, service), "getUuid", "()Ljava/util/UUID;");
		jobject uuid = (*env)->CallObjectMethod(env, service, mid);
		mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, uuid), "toString", "()Ljava/lang/String;");
		jstring uuid_jstr = (*env)->CallObjectMethod(env, uuid, mid);

		const char* uuid_cstr = (*env)->GetStringUTFChars(env, uuid_jstr, 0);
		posix_print("nativeServicesDiscovered, service#%i: %s\n", at, uuid_cstr);

		SDL_BleService* ble_service = peripheral->services + at;
		discover_services_bh(peripheral, ble_service, uuid_cstr, NULL);

		(*env)->ReleaseStringUTFChars(env, uuid_jstr, uuid_cstr);
		(*env)->DeleteLocalRef(env, uuid_jstr);

		//
		// discover characteristics of this service
		//

		// List<BluetoothGattCharacteristic> charas = service.getCharacteristics();
		// int count2 = charas.size();
		jmethodID mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, service), "getCharacteristics", "()Ljava/util/List;");
		jobject charas = (*env)->CallObjectMethod(env, service, mid);
		mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, charas), "size", "()I");
		int count2 = (*env)->CallIntMethod(env, charas, mid);

		ble_service = discover_characteristics_uh(peripheral, peripheral->services + at, count2);
		for (at2 = 0; at2 < count2; at2 ++) {
			// BluetoothGattCharacteristic chara = charas.get(at2);
			// uuid = chara.getUuid().toString();
			mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, charas), "get", "(I)Ljava/lang/Object;");
			jobject chara = (*env)->CallObjectMethod(env, charas, mid, at2);
			mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, chara), "getUuid", "()Ljava/util/UUID;");
			uuid = (*env)->CallObjectMethod(env, chara, mid);
			mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, uuid), "toString", "()Ljava/lang/String;");
			uuid_jstr = (*env)->CallObjectMethod(env, uuid, mid);
			uuid_cstr = (*env)->GetStringUTFChars(env, uuid_jstr, 0);
			posix_print("nativeServicesDiscovered, characteristic#%i: %s\n", at2, uuid_cstr);

			SDL_BleCharacteristic* ble_characteristic = ble_service->characteristics + at2;
			discover_characteristics_bh(peripheral, ble_service, ble_characteristic, uuid_cstr, (*env)->NewGlobalRef(env, chara));

			(*env)->ReleaseStringUTFChars(env, uuid_jstr, uuid_cstr);
			(*env)->DeleteLocalRef(env, uuid_jstr);

			// int property2 = chara.getProperties();
			mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, chara), "getProperties", "()I");
			int property2 = (*env)->CallIntMethod(env, chara, mid);
			(*env)->DeleteLocalRef(env, chara);

			// parse property.
			if (property2 & PROPERTY_BROADCAST) {
				ble_characteristic->properties |= SDL_BleCharacteristicPropertyBroadcast;
			}
			if (property2 & PROPERTY_READ) {
				ble_characteristic->properties |= SDL_BleCharacteristicPropertyRead;
			}
			if (property2 & PROPERTY_WRITE_NO_RESPONSE) {
				ble_characteristic->properties |= SDL_BleCharacteristicPropertyWriteWithoutResponse;
			}
			if (property2 & PROPERTY_WRITE) {
				ble_characteristic->properties |= SDL_BleCharacteristicPropertyWrite;
			}
			if (property2 & PROPERTY_NOTIFY) {
				ble_characteristic->properties |= SDL_BleCharacteristicPropertyNotify;
			}
			if (property2 & PROPERTY_INDICATE) {
				ble_characteristic->properties |= SDL_BleCharacteristicPropertyIndicate;
			}
			if (property2 & PROPERTY_SIGNED_WRITE) {
				ble_characteristic->properties |= SDL_BleCharacteristicPropertyAuthenticatedSignedWrites;
			}
			if (property2 & PROPERTY_EXTENDED_PROPS) {
				ble_characteristic->properties |= SDL_BleCharacteristicPropertyExtendedProperties;
			}
		}

		(*env)->DeleteLocalRef(env, service);
	}

	if (current_callbacks && current_callbacks->discover_services) {
		current_callbacks->discover_services(peripheral, 0);
	}

	// release global reference
	(*env)->DeleteGlobalRef(env, gatt);
}

JNIEXPORT void JNICALL Java_org_libsdl_app_SDLActivity_nativeServicesDiscovered(
    JNIEnv* env, jclass jcls,
    jobject gatt, jboolean success)
{
	PUSH_BLEDID(
		did->type = did_servicesdiscovered;
		did->ServicesDiscovered.gatt = (*env)->NewGlobalRef(env, gatt);
		did->ServicesDiscovered.success = success? SDL_TRUE: SDL_FALSE;
		);
}

void nativeCharacteristicRead(JNIEnv* env, jobject chara, jbyteArray buffer, jboolean notify)
{
	jmethodID mid;

	// uuid = chara.getUuid();
	// uuid_str = uuid.toString();
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, chara), "getUuid", "()Ljava/util/UUID;");
	jobject uuid = (*env)->CallObjectMethod(env, chara, mid);
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, uuid), "toString", "()Ljava/lang/String;");
	jstring chara_uuid_jstr = (*env)->CallObjectMethod(env, uuid, mid);

	// service = chara.getService();
	// uuid = chara.getUuid();
	// uuid_str = uuid.toString();
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, chara), "getService", "()Landroid/bluetooth/BluetoothGattService;");
	jobject service = (*env)->CallObjectMethod(env, chara, mid);
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, service), "getUuid", "()Ljava/util/UUID;");
	uuid = (*env)->CallObjectMethod(env, service, mid);
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, uuid), "toString", "()Ljava/lang/String;");
	jstring service_uuid_jstr = (*env)->CallObjectMethod(env, uuid, mid);

	const char* service_uuid_cstr = (*env)->GetStringUTFChars(env, service_uuid_jstr, 0);
	const char* chara_uuid_cstr = (*env)->GetStringUTFChars(env, chara_uuid_jstr, 0);

	jbyte* elements = (*env)->GetByteArrayElements(env, buffer, NULL);
	uint8_t* data = elements;
	int len = (*env)->GetArrayLength(env, (jbyteArray)buffer);

	SDL_BlePeripheral* peripheral = connected_peripheral;
	if (current_callbacks && current_callbacks->read_characteristic) {
		current_callbacks->read_characteristic(peripheral, find_characteristic_from_uuid(peripheral, service_uuid_cstr, chara_uuid_cstr), data, len);
	}
	(*env)->ReleaseByteArrayElements(env, buffer, elements, JNI_ABORT);

	(*env)->ReleaseStringUTFChars(env, service_uuid_jstr, service_uuid_cstr);
	(*env)->ReleaseStringUTFChars(env, chara_uuid_jstr, chara_uuid_cstr);

	(*env)->DeleteLocalRef(env, service_uuid_jstr);
	(*env)->DeleteLocalRef(env, chara_uuid_jstr);

	// release global reference
	(*env)->DeleteGlobalRef(env, chara);
	(*env)->DeleteGlobalRef(env, buffer);
}

JNIEXPORT void JNICALL Java_org_libsdl_app_SDLActivity_nativeCharacteristicRead(
    JNIEnv* env, jclass jcls,
    jobject chara, jbyteArray buffer, jboolean notify)
{
	PUSH_BLEDID(
		did->type = did_characteristicread;
		did->CharacteristicRead.chara = (*env)->NewGlobalRef(env, chara);
		did->CharacteristicRead.buffer = (*env)->NewGlobalRef(env, buffer);
		did->CharacteristicRead.notify = notify? SDL_TRUE: SDL_FALSE;
		);
}

void nativeCharacteristicWrite(JNIEnv* env, jobject chara, jboolean success)
{
	jmethodID mid;

	// uuid = chara.getUuid();
	// uuid_str = uuid.toString();
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, chara), "getUuid", "()Ljava/util/UUID;");
	jobject uuid = (*env)->CallObjectMethod(env, chara, mid);
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, uuid), "toString", "()Ljava/lang/String;");
	jstring chara_uuid_jstr = (*env)->CallObjectMethod(env, uuid, mid);

	// service = chara.getService();
	// uuid = chara.getUuid();
	// uuid_str = uuid.toString();
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, chara), "getService", "()Landroid/bluetooth/BluetoothGattService;");
	jobject service = (*env)->CallObjectMethod(env, chara, mid);
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, service), "getUuid", "()Ljava/util/UUID;");
	uuid = (*env)->CallObjectMethod(env, service, mid);
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, uuid), "toString", "()Ljava/lang/String;");
	jstring service_uuid_jstr = (*env)->CallObjectMethod(env, uuid, mid);

	const char* service_uuid_cstr = (*env)->GetStringUTFChars(env, service_uuid_jstr, 0);
	const char* chara_uuid_cstr = (*env)->GetStringUTFChars(env, chara_uuid_jstr, 0);

	SDL_BlePeripheral* peripheral = connected_peripheral;
	if (current_callbacks && current_callbacks->write_characteristic) {
		current_callbacks->write_characteristic(peripheral, find_characteristic_from_uuid(peripheral, service_uuid_cstr, chara_uuid_cstr), success? 0: -1 * EFAULT);
	}

	(*env)->ReleaseStringUTFChars(env, service_uuid_jstr, service_uuid_cstr);
	(*env)->ReleaseStringUTFChars(env, chara_uuid_jstr, chara_uuid_cstr);

	(*env)->DeleteLocalRef(env, service_uuid_jstr);
	(*env)->DeleteLocalRef(env, chara_uuid_jstr);

	// release global reference
	(*env)->DeleteGlobalRef(env, chara);
}

JNIEXPORT void JNICALL Java_org_libsdl_app_SDLActivity_nativeCharacteristicWrite(
    JNIEnv* env, jclass jcls,
    jobject chara, jboolean success)
{
	PUSH_BLEDID(
		did->type = did_characteristicwrite;
		did->CharacteristicWrite.chara = (*env)->NewGlobalRef(env, chara);
		did->CharacteristicWrite.success = success? SDL_TRUE: SDL_FALSE;
		);
}

void nativeDescriptorWrite(JNIEnv* env, jobject descriptor)
{
	jmethodID mid;

	// uuid = descriptor.getUuid();
	// descriptor_uuid_str = uuid.toString();
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, descriptor), "getUuid", "()Ljava/util/UUID;");
	jobject uuid = (*env)->CallObjectMethod(env, descriptor, mid);
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, uuid), "toString", "()Ljava/lang/String;");
	jstring descriptor_uuid_jstr = (*env)->CallObjectMethod(env, uuid, mid);

	// chara = descriptor.getCharacteristic();
	// uuid = chara.getUuid();
	// uuid_str = uuid.toString();
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, descriptor), "getCharacteristic", "()Landroid/bluetooth/BluetoothGattCharacteristic;");
	jobject chara = (*env)->CallObjectMethod(env, descriptor, mid);
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, chara), "getUuid", "()Ljava/util/UUID;");
	uuid = (*env)->CallObjectMethod(env, chara, mid);
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, uuid), "toString", "()Ljava/lang/String;");
	jstring chara_uuid_jstr = (*env)->CallObjectMethod(env, uuid, mid);

	// service = chara.getService();
	// uuid = chara.getUuid();
	// uuid_str = uuid.toString();
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, chara), "getService", "()Landroid/bluetooth/BluetoothGattService;");
	jobject service = (*env)->CallObjectMethod(env, chara, mid);
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, service), "getUuid", "()Ljava/util/UUID;");
	uuid = (*env)->CallObjectMethod(env, service, mid);
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, uuid), "toString", "()Ljava/lang/String;");
	jstring service_uuid_jstr = (*env)->CallObjectMethod(env, uuid, mid);

	const char* service_uuid_cstr = (*env)->GetStringUTFChars(env, service_uuid_jstr, 0);
	const char* chara_uuid_cstr = (*env)->GetStringUTFChars(env, chara_uuid_jstr, 0);
	const char* descriptor_uuid_cstr = (*env)->GetStringUTFChars(env, descriptor_uuid_jstr, 0);

	if (SDL_BleUuidEqual(descriptor_uuid_cstr, UUID_CLIENT_CHARACTERISTIC_CONFIG)) {
		// this is notify
		SDL_BlePeripheral* peripheral = connected_peripheral;
		if (current_callbacks && current_callbacks->notify_characteristic) {
			current_callbacks->notify_characteristic(peripheral, find_characteristic_from_uuid(peripheral, service_uuid_cstr, chara_uuid_cstr), 0);
		}
	}
	(*env)->ReleaseStringUTFChars(env, service_uuid_jstr, service_uuid_cstr);
	(*env)->ReleaseStringUTFChars(env, chara_uuid_jstr, chara_uuid_cstr);
	(*env)->ReleaseStringUTFChars(env, descriptor_uuid_jstr, descriptor_uuid_cstr);

	(*env)->DeleteLocalRef(env, service_uuid_jstr);
	(*env)->DeleteLocalRef(env, chara_uuid_jstr);
	(*env)->DeleteLocalRef(env, descriptor_uuid_jstr);

	// release global reference
	(*env)->DeleteGlobalRef(env, descriptor);
}

JNIEXPORT void JNICALL Java_org_libsdl_app_SDLActivity_nativeDescriptorWrite(
    JNIEnv* env, jclass jcls,
    jobject descriptor)
{
	PUSH_BLEDID(
		did->type = did_descriptorwrite;
		did->DescriptorWrite.descriptor = (*env)->NewGlobalRef(env, descriptor);
		);
}

static void Android_ScanPeripherals(const char* uuid)
{
	if (!adapter) {
		return;
	}
	Android_JNI_BleScanPeripherals(uuid);
}

static void Android_StopScanPeripherals()
{
	if (!adapter) {
		return;
	}
    Android_JNI_BleStopScanPeripherals();
}

static void Android_ReleaseCookie(const SDL_BlePeripheral* peripheral)
{
	jobject gatt = peripheral->cookie;
	JNIEnv* env = Android_JNI_GetEnv();
	LocalReference_Init(env);

	jmethodID mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, gatt), "close", "()V");
	posix_print("Android_ReleaseCookie, mid: %p\n", mid);

	(*env)->CallVoidMethod(env, gatt, mid);
	(*env)->DeleteGlobalRef(env, gatt);

	LocalReference_Cleanup(env);
}

static void Android_ConnectPeripheral(SDL_BlePeripheral* peripheral)
{
	if (!adapter) {
		return;
	}

	char* address = mac_addr_uc6_2_str(peripheral->mac_addr, ':');

	JNIEnv* env = Android_JNI_GetEnv();
	LocalReference_Init(env);

	jmethodID mid;
	if (peripheral->cookie) {
		Android_ReleaseCookie(peripheral);
		peripheral->cookie = NULL;
	}

	// BluetoothDevice device = mBluetoothAdapter.getRemoteDevice(address);
	jstring jaddress = (jstring)((*env)->NewStringUTF(env, address));
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, adapter), "getRemoteDevice", "(Ljava/lang/String;)Landroid/bluetooth/BluetoothDevice;");
	jobject device = (*env)->CallObjectMethod(env, adapter, mid, jaddress);

	// mGattCallback = SDLActivity.mGattCallback
	jclass mActivityClass = Android_JNI_GetActivityClass();
	jfieldID fid = (*env)->GetStaticFieldID(env, mActivityClass, "mGattCallback", "Landroid/bluetooth/BluetoothGattCallback;");
	jobject mGattCallback = (*env)->GetStaticObjectField(env, mActivityClass, fid);

	// context = SDLActivity.getContext(); 
	mid = (*env)->GetStaticMethodID(env, mActivityClass, "getContext","()Landroid/content/Context;");
	jobject context = (*env)->CallStaticObjectMethod(env, mActivityClass, mid);

	// gatt = device.connectGatt(context, false, mSingleton.mGattCallback);
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, device), "connectGatt", "(Landroid/content/Context;ZLandroid/bluetooth/BluetoothGattCallback;)Landroid/bluetooth/BluetoothGatt;");
	jobject gatt = (*env)->CallObjectMethod(env, device, mid, context, JNI_FALSE, mGattCallback);
	peripheral->cookie = (*env)->NewGlobalRef(env, gatt);

	LocalReference_Cleanup(env);
	SDL_free(address);
}

static void Android_DisconnectPeripheral(SDL_BlePeripheral* peripheral)
{
	jobject gatt = peripheral->cookie;
	if (!gatt) {
		return;
	}

	JNIEnv* env = Android_JNI_GetEnv();
	LocalReference_Init(env);
	
	// mBluetoothGatt.disconnect();
	jmethodID mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, gatt), "disconnect", "()V");
	(*env)->CallVoidMethod(env, gatt, mid);

	LocalReference_Cleanup(env);
}

static void Android_GetServices(SDL_BlePeripheral* peripheral)
{
	jobject gatt = peripheral->cookie;
	if (!gatt) {
		return;
	}

	JNIEnv* env = Android_JNI_GetEnv();
	LocalReference_Init(env);

	jmethodID mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, gatt), "discoverServices", "()Z");
	(*env)->CallBooleanMethod(env, gatt, mid);

	LocalReference_Cleanup(env);
}

static void Android_GetCharacteristics(const SDL_BlePeripheral* peripheral, SDL_BleService* service)
{
	if (peripheral != connected_peripheral) {
		return;
	}
	if (current_callbacks && current_callbacks->discover_characteristics) {
        current_callbacks->discover_characteristics(connected_peripheral, service, 0);
    }
}

static void Android_ReadCharacteristic(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic)
{
	jobject gatt = peripheral->cookie;
	if (!gatt) {
		return;
	}

	JNIEnv* env = Android_JNI_GetEnv();
	LocalReference_Init(env);

	// mBluetoothGatt.readCharacteristic(chara);
	jobject chara = characteristic->cookie;
	jmethodID mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, gatt), "readCharacteristic", "(Landroid/bluetooth/BluetoothGattCharacteristic;)Z");
	(*env)->CallBooleanMethod(env, gatt, mid, chara);

	LocalReference_Cleanup(env);
}

static void Android_NotifyCharacteristic(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic)
{
	jobject gatt = peripheral->cookie;
	if (!gatt) {
		return;
	}
	SDL_bool indicate = characteristic->properties & SDL_BleCharacteristicPropertyIndicate? SDL_TRUE: SDL_FALSE;
	JNIEnv* env = Android_JNI_GetEnv();
	LocalReference_Init(env);

	// mBluetoothGatt.setCharacteristicNotification(chara, enable);
	jobject chara = characteristic->cookie;
	jmethodID mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, gatt), "setCharacteristicNotification", "(Landroid/bluetooth/BluetoothGattCharacteristic;Z)Z");
	(*env)->CallBooleanMethod(env, gatt, mid, chara, JNI_TRUE);

	// BluetoothGattDescriptor descriptor = chara.getDescriptor(UUID.fromString(CLIENT_CHARACTERISTIC_CONFIG));
	jclass classClass = (*env)->FindClass(env, "java/util/UUID");
	jstring jstr = (jstring)((*env)->NewStringUTF(env, UUID_CLIENT_CHARACTERISTIC_CONFIG));
	mid = (*env)->GetStaticMethodID(env, classClass, "fromString", "(Ljava/lang/String;)Ljava/util/UUID;");
	jobject juuid = (*env)->CallStaticObjectMethod(env, classClass, mid, jstr);
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, chara), "getDescriptor", "(Ljava/util/UUID;)Landroid/bluetooth/BluetoothGattDescriptor;");
	jobject descriptor = (*env)->CallObjectMethod(env, chara, mid, juuid);

	// descriptor.setValue(enable? BluetoothGattDescriptor.ENABLE_INDICATION_VALUE: BluetoothGattDescriptor.DISABLE_NOTIFICATION_VALUE);
	classClass = (*env)->FindClass(env, "android/bluetooth/BluetoothGattDescriptor");
	jfieldID fid = (*env)->GetStaticFieldID(env, classClass, indicate? "ENABLE_INDICATION_VALUE": "ENABLE_NOTIFICATION_VALUE", "[B");
	jbyteArray value = (*env)->GetStaticObjectField(env, classClass, fid);
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, descriptor), "setValue", "([B)Z");
	(*env)->CallBooleanMethod(env, descriptor, mid, value);

	// mBluetoothGatt.writeDescriptor(descriptor);
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, gatt), "writeDescriptor", "(Landroid/bluetooth/BluetoothGattDescriptor;)Z");
	(*env)->CallBooleanMethod(env, gatt, mid, descriptor);

	LocalReference_Cleanup(env);
}

static void Android_WriteCharacteristic(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic, const unsigned char* data, int size)
{
	jobject gatt = peripheral->cookie;
	if (!gatt) {
		return;
	}

	JNIEnv* env = Android_JNI_GetEnv();
	LocalReference_Init(env);

	// chara.setValue(dataLocal);
	jobject chara = characteristic->cookie;
	jbyteArray dataLocal = (*env)->NewByteArray(env, size);
	(*env)->SetByteArrayRegion(env, dataLocal, 0, size, data);
	jmethodID mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, chara), "setValue", "([B)Z");
	(*env)->CallBooleanMethod(env, chara, mid, dataLocal);
	(*env)->DeleteLocalRef(env, dataLocal);

	// mBluetoothGatt.writeCharacteristic(chara);
	mid = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, gatt), "writeCharacteristic", "(Landroid/bluetooth/BluetoothGattCharacteristic;)Z");
	(*env)->CallBooleanMethod(env, gatt, mid, chara);

	LocalReference_Cleanup(env);
}

static void Android_ReleaseCharacteristicCookie(const SDL_BleCharacteristic* characteristic, int at)
{
	posix_print("Android_ReleaseCharacteristicCookie, cookie: %p\n", characteristic->cookie);
	JNIEnv* env = Android_JNI_GetEnv();
	(*env)->DeleteGlobalRef(env, characteristic->cookie);
}

static void Android_Quit()
{
	SDL_DestroyMutex(bledid_lock);
	bledid_lock = NULL;

	if (adapter) {
		JNIEnv* env = Android_JNI_GetEnv();
		(*env)->DeleteGlobalRef(env, adapter);
		adapter = NULL;
	}
}

static void initialize_ble()
{
	adapter = Android_JNI_BleInitialize();
}

// Windows driver bootstrap functions
static int Android_Available(void)
{
	return (1);
}

static SDL_MiniBle* Android_CreateBle()
{
	SDL_MiniBle *ble;

	// Initialize all variables that we clean on shutdown
    ble = (SDL_MiniBle *)SDL_calloc(1, sizeof(SDL_MiniBle));
	if (!ble) {
		SDL_OutOfMemory();
		return (0);
	}

	// Set the function pointers
	ble->ScanPeripherals = Android_ScanPeripherals;
	ble->StopScanPeripherals = Android_StopScanPeripherals;
	ble->ConnectPeripheral = Android_ConnectPeripheral;
	ble->DisconnectPeripheral = Android_DisconnectPeripheral;
	ble->GetServices = Android_GetServices;
	ble->GetCharacteristics = Android_GetCharacteristics;
	ble->ReadCharacteristic = Android_ReadCharacteristic;
	ble->NotifyCharacteristic = Android_NotifyCharacteristic;
	ble->WriteCharacteristic = Android_WriteCharacteristic;
	ble->ReleaseCookie = Android_ReleaseCookie;
	ble->ReleaseCharacteristicCookie = Android_ReleaseCharacteristicCookie;
	ble->Quit = Android_Quit;

	bledid_lock = SDL_CreateMutex();
	initialize_ble();
    return ble;
}


BleBootStrap Android_ble = {
	Android_Available, Android_CreateBle
};

