#include "../../SDL_internal.h"
#include "SDL_video.h"
#include "SDL_audio.h"
#include "SDL_timer.h"
#include "SDL_ble_c.h"
#import "SDL_uikitble.h"


static NSString *const kCharacteristicUUID = @"00002A2B-0000-1000-8000-00805F9B34FB";
static NSString *const kServiceUUID = @"5A5B1805-0000-1000-8000-00805F9B34FB";

static SDL_uikitble* ble = NULL;

static void discoverServices(SDL_BlePeripheral* peripheral)
{
	CBPeripheral* peripheral2 = (__bridge CBPeripheral *)(peripheral->cookie);
	discover_services_uh(peripheral, (int)[peripheral2.services count]);
	
	if (peripheral->valid_services) {
		int at = 0;
		for (CBService* service2 in peripheral2.services) {
			SDL_BleService* service = peripheral->services + at;
			discover_services_bh(peripheral, service, [service2.UUID.UUIDString UTF8String], (__bridge void *)(service2));
			at ++;
		}
	}
}

static void discoverCharacteristics(SDL_BlePeripheral* peripheral, CBService* service2)
{
    int at = 0;

	SDL_BleService* service = discover_characteristics_uh(peripheral, SDL_BleFindService(peripheral, [service2.UUID.UUIDString UTF8String]), (int)[service2.characteristics count]);
	if (!service) {
		return;
	}

	for (CBCharacteristic* characteristic2 in service2.characteristics) {
		SDL_BleCharacteristic* characteristic = service->characteristics + at;
		uint32_t properties = [characteristic2 properties];
		discover_characteristics_bh(peripheral, service, characteristic, [characteristic2.UUID.UUIDString UTF8String], (__bridge void *)(characteristic2));

		if (properties & CBCharacteristicPropertyBroadcast) {
			characteristic->properties |= SDL_BleCharacteristicPropertyBroadcast;
		}
		if (properties & CBCharacteristicPropertyRead) {
			characteristic->properties |= SDL_BleCharacteristicPropertyRead;
		}
		if (properties & CBCharacteristicPropertyWriteWithoutResponse) {
			characteristic->properties |= SDL_BleCharacteristicPropertyWriteWithoutResponse;
		}
		if (properties & CBCharacteristicPropertyWrite) {
			characteristic->properties |= SDL_BleCharacteristicPropertyWrite;
		}
		if (properties & CBCharacteristicPropertyNotify) {
			characteristic->properties |= SDL_BleCharacteristicPropertyNotify;
		}
		if (properties & CBCharacteristicPropertyIndicate) {
			characteristic->properties |= SDL_BleCharacteristicPropertyIndicate;
		}
		if (properties & CBCharacteristicPropertyAuthenticatedSignedWrites) {
			characteristic->properties |= SDL_BleCharacteristicPropertyAuthenticatedSignedWrites;
		}
		if (properties & CBCharacteristicPropertyExtendedProperties) {
			characteristic->properties |= SDL_BleCharacteristicPropertyExtendedProperties;
		}
		if (properties & CBCharacteristicPropertyNotifyEncryptionRequired) {
			characteristic->properties |= SDL_BleCharacteristicPropertyNotifyEncryptionRequired;
		}
		if (properties & CBCharacteristicPropertyIndicateEncryptionRequired) {
			characteristic->properties |= SDL_BleCharacteristicPropertyIndicateEncryptionRequired;
		}
        at ++;
	}
}

@implementation SDL_uikitble
/*
@synthesize bleManager;

@synthesize peripheraManager;
@synthesize customerCharacteristic;
@synthesize customerService;
*/
// #define currChannel [babySpeaker callbackOnCurrChannel]

+(instancetype)shareSDL_uikitble{
    static SDL_uikitble *share = nil;
    static dispatch_once_t oneToken;
    dispatch_once(&oneToken, ^{
        share = [[SDL_uikitble alloc]init];
    });
    
    return share;
}

-(instancetype)init{
    self = [super init];
    if (self){

        // bleManager = [[CBCentralManager alloc]initWithDelegate:self queue:dispatch_get_main_queue()];
        // peripheraManager = [[CBPeripheralManager alloc]initWithDelegate:self queue:dispatch_get_main_queue()];
    }
    return  self;
}

-(void)initCentralManager
{
    if (!bleManager) {
        bleManager = [[CBCentralManager alloc]initWithDelegate:self queue:dispatch_get_main_queue()];
    }
}

-(void)initPeripheralManager
{
    if (!peripheralManager) {
        peripheralManager = [[CBPeripheralManager alloc]initWithDelegate:self queue:dispatch_get_main_queue()];
    }
}

- (void)peripheralManagerDidUpdateState:(CBPeripheralManager *)peripheral{
    switch (peripheral.state) {
        case CBPeripheralManagerStatePoweredOn:
            NSLog(@"powered on");
            break;
        case CBPeripheralManagerStatePoweredOff:
            NSLog(@"powered off");
            break;
            
        default:
            break;
    }
    
    if (peripheral.state == CBCentralManagerStatePoweredOn) {
        [self setUp];
    }
}

-(void)setUp
{
/*
    [peripheralManager startAdvertising:@{CBAdvertisementDataLocalNameKey:@"libSDL",
                                          CBAdvertisementDataServiceUUIDsKey:@[[CBUUID UUIDWithString:kServiceUUID]]}];
    return;
*/
    CBUUID *characteristicUUID = [CBUUID UUIDWithString:kCharacteristicUUID];
    customerCharacteristic = [[CBMutableCharacteristic alloc]initWithType:characteristicUUID properties:(CBCharacteristicPropertyRead | CBCharacteristicPropertyNotify) value:nil permissions:CBAttributePermissionsReadable];
    
    CBUUID *serviceUUID = [CBUUID UUIDWithString:kServiceUUID];
    customerService = [[CBMutableService alloc]initWithType:serviceUUID primary:YES];
    
    // [customerService setCharacteristics:@[characteristicUUID]];
    [customerService setCharacteristics:@[customerCharacteristic]];
    
    // after it, will call- (void)peripheralManager:(CBPeripheralManager *)peripheral didAddService:(CBService *)service error:(NSError *)error
    [peripheralManager addService:customerService];
}

- (void)peripheralManagerDidStartAdvertising:(CBPeripheralManager *)peripheral error:(NSError *)error{
    NSLog(@"in peripheralManagerDidStartAdvertisiong:error");
}

-(void)updateCharacteristicValue
{
    NSLog(@"in peripheralManagerDidStartAdvertisiong:error");
    
    // NSString *valueStr=[NSString stringWithFormat:@"%@ --%@",kPeripheralName,[NSDate   date]];
    // NSData *value=[valueStr dataUsingEncoding:NSUTF8StringEncoding];

    // [self.peripheraManager updateValue:value forCharacteristic:self.characteristicM onSubscribedCentrals:nil];
}

- (BOOL)updateValue:(NSData *)value forCharacteristic:(CBMutableCharacteristic *)characteristic onSubscribedCentrals:(NSArray *)centrals
{
    NSLog(@"in onSubscribedCentrals:centrals");
    return true;
}

- (void)peripheralManager:(CBPeripheralManager *)peripheral didAddService:(CBService *)service error:(NSError *)error
{
    if (!error) {
        //starts advertising the service
        [peripheralManager startAdvertising:@{CBAdvertisementDataLocalNameKey:@"libSDL",
                                                  CBAdvertisementDataServiceUUIDsKey:@[[CBUUID UUIDWithString:kServiceUUID]]}];
    } else {
        NSLog(@"addService with error: %@", [error localizedDescription]);
    }
}

- (void)peripheralManager:(CBPeripheralManager *)peripheral didReceiveReadRequest:(CBATTRequest *)request
{
    NSLog(@"in peripheralManagerDidStartAdvertisiong:error");
    if ([request.characteristic.UUID isEqual:customerCharacteristic.UUID]) {
        // request.offset == 0
        if (request.offset != 0) {
            [peripheralManager respondToRequest:request
                                       withResult:CBATTErrorInvalidOffset];
            return;
        }
/*
        request.value = [customerCharacteristic.value
                         subdataWithRange:NSMakeRange(request.offset,
                                                      customerCharacteristic.value.length - request.offset)];
*/
        // NSDate* now = [NSDate date];
        
        [peripheralManager respondToRequest:request withResult:CBATTErrorSuccess];
        
    } else {
        [peripheralManager respondToRequest:request withResult:CBATTErrorRequestNotSupported];
    }
}

#pragma mark -received notification

-(void)scanForPeripheralNotifyReceived:(NSNotification *)notify{
//    NSLog(@">>>scanForPeripheralsNotifyReceived");
}

-(void)didDiscoverPeripheralNotifyReceived:(NSNotification *)notify{
//    CBPeripheral *peripheral =[notify.userInfo objectForKey:@"peripheral"];
//    NSLog(@">>>didDiscoverPeripheralNotifyReceived:%@",peripheral.name);
}

-(void)connectToPeripheralNotifyReceived:(NSNotification *)notify{
//    NSLog(@">>>connectToPeripheralNotifyReceived");
}

-(void)scanPeripherals:(const char*)uuid
{
    // [bleManager scanForPeripheralsWithServices:nil options:@{CBCentralManagerScanOptionAllowDuplicatesKey:@YES}];
    NSMutableArray* uuidArray = [[NSMutableArray alloc] init];
    if (uuid && uuid[0] != '\0') {
        [uuidArray addObject: [CBUUID UUIDWithString: [[NSString alloc] initWithUTF8String:uuid]]];
    }
    // [bleManager scanForPeripheralsWithServices:uuidArray options:@{CBCentralManagerScanOptionAllowDuplicatesKey:@YES}];
    
    [bleManager scanForPeripheralsWithServices:uuidArray options:nil];
    // [bleManager scanForPeripheralsWithServices:@[[CBUUID UUIDWithString:@"1809"]] options:nil];
}

-(void)connectToPeripheral:(CBPeripheral *)peripheral{
    [bleManager connectPeripheral:peripheral options:nil];
    //

}

-(void)stopConnectAllPerihperals{
/*
    for (int i=0;i<connectedPeripherals.count;i++) {
        [bleManager cancelPeripheralConnection:connectedPeripherals[i]];
    }
    connectedPeripherals = [[NSMutableArray alloc]init];
*/
    NSLog(@">>>babyBluetooth stop");
}

-(void)stopScan
{
    [bleManager stopScan];
}

#pragma mark -CBCentralManagerDelegate

- (void)centralManagerDidUpdateState:(CBCentralManager *)central{
 
    switch (central.state) {
        case CBCentralManagerStateUnknown:
            NSLog(@">>>CBCentralManagerStateUnknown");
            break;
        case CBCentralManagerStateResetting:
            NSLog(@">>>CBCentralManagerStateResetting");
            break;
        case CBCentralManagerStateUnsupported:
            NSLog(@">>>CBCentralManagerStateUnsupported");
            break;
        case CBCentralManagerStateUnauthorized:
            NSLog(@">>>CBCentralManagerStateUnauthorized");
            break;
        case CBCentralManagerStatePoweredOff:
            NSLog(@">>>CBCentralManagerStatePoweredOff");
            break;
        case CBCentralManagerStatePoweredOn:
            NSLog(@">>>CBCentralManagerStatePoweredOn");
            break;
        default:
            break;
    }
	if (central.state == CBCentralManagerStatePoweredOn) {
        [self scanPeripherals: NULL];

	} else if (central.state == CBCentralManagerStatePoweredOff) {
		release_peripherals();
	}
}

- (void)centralManager:(CBCentralManager *)central didDiscoverPeripheral:(CBPeripheral *)peripheral2 advertisementData:(NSDictionary *)advertisementData RSSI:(NSNumber *)RSSI
{
    // const char* name = [peripheral2.name UTF8String];
	SDL_BlePeripheral* peripheral = discover_peripheral_uh_cookie((__bridge void *)(peripheral2), [peripheral2.name UTF8String]);
	if (!peripheral->cookie) {
		peripheral->cookie = (void *)CFBridgingRetain(peripheral2);

		NSLog(@"peripheral: %@ ------", peripheral2.name);
		for (id key in advertisementData) {
			NSLog(@"key: %@ ,value: %@", key, [advertisementData objectForKey:key]);
		}
    
        // parse KCBAdvDataServiceUUIDs, result maybe like 1809,ccc0
        NSArray* service_uuids = [advertisementData objectForKey:@"kCBAdvDataServiceUUIDs"];
        int count = [service_uuids count], start;
        size_t require_size = 0;
        for (int at = 0; at < count; at ++) {
            const char* uuid = [[[service_uuids objectAtIndex: at] UUIDString] UTF8String];
            require_size += SDL_strlen(uuid) + 1; // , or \0'
        }
        if (require_size) {
            peripheral->uuid = (char*)SDL_malloc(require_size);
        }
        start = 0;
        for (int at = 0; at < count; at ++) {
            if (at) {
                peripheral->uuid[start] = ',';
                start ++;
            }
            const char* uuid = [[[service_uuids objectAtIndex: at] UUIDString] UTF8String];
            size_t s = SDL_strlen(uuid);
            memcpy(peripheral->uuid + start, uuid, s);
            start += s;
            if (at == count - 1) {
                peripheral->uuid[start] = '\0';
            }
        }
        
		// kCBAdvDataManufacturerData, only copy.
		NSData* manufacturer_data = [advertisementData objectForKey:@"kCBAdvDataManufacturerData"];
		if (manufacturer_data) {
			peripheral->manufacturer_data_len = (int)[manufacturer_data length];
			peripheral->manufacturer_data = (unsigned char*)SDL_malloc(peripheral->manufacturer_data_len);
            memcpy(peripheral->manufacturer_data, [manufacturer_data bytes], peripheral->manufacturer_data_len);
		}
	}
	discover_peripheral_bh(peripheral, [RSSI intValue]);
}

-(void)disconnect:(id)sender
{
    [bleManager stopScan];
}

- (void)centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)peripheral2
{
    SDL_BlePeripheral* peripheral = find_peripheral_from_cookie((__bridge void *)(peripheral2));
	connect_peripheral_bh(peripheral, 0);

    [peripheral2 setDelegate:self];
}

-(void)centralManager:(CBCentralManager *)central didFailToConnectPeripheral:(CBPeripheral *)peripheral2 error:(NSError *)error
{
    SDL_BlePeripheral* peripheral = find_peripheral_from_cookie((__bridge void *)(peripheral2));
    NSLog(@">>>connect to（%@）fail, readon:%@", [peripheral2 name], [error localizedDescription]);
	connect_peripheral_bh(peripheral, -1 * EFAULT);
}

- (void)centralManager:(CBCentralManager *)central didDisconnectPeripheral:(CBPeripheral *)peripheral2 error:(NSError *)error
{
    // both normal and except disconnect will enter it.
    SDL_BlePeripheral* peripheral = find_peripheral_from_cookie((__bridge void *)(peripheral2));
    int error2 = 0;
    if (error) {
        NSLog(@">>>disconnect peripheral（%@）, reason:%@",[peripheral2 name], [error localizedDescription]);
        error2 = -1 * EFAULT;
    }
	disconnect_peripheral_bh(peripheral, error2);
}

- (void)peripheral:(CBPeripheral *)peripheral didDiscoverServices:(NSError *)error
{
    NSLog(@">>>services: %@", peripheral.services);
	SDL_BlePeripheral* device = find_peripheral_from_cookie((__bridge void *)(peripheral));
	int error2 = 0;
    if (!error) {
		discoverServices(device);

    } else {
		NSLog(@">>>Discovered services for %@ with error: %@", peripheral.name, [error localizedDescription]);
        error2 = -1 * EFAULT;
	}

    if (current_callbacks && current_callbacks->discover_services) {
        current_callbacks->discover_services(device, error2);
    }
}


-(void)peripheral:(CBPeripheral *)peripheral didDiscoverCharacteristicsForService:(CBService *)service error:(NSError *)error
{
    SDL_BlePeripheral* device = find_peripheral_from_cookie((__bridge void *)(peripheral));
	int error2 = 0;
    if (!error) {
		discoverCharacteristics(device, service);

	} else {
        // NSLog(@"error Discovered characteristics for %@ with error: %@", service.UUID, [error localizedDescription]);
        error2 = -1 * EFAULT;
    }

	if (current_callbacks && current_callbacks->discover_characteristics) {
        current_callbacks->discover_characteristics(device, SDL_BleFindService(device, [service.UUID.UUIDString UTF8String]), error2);
    }
}

-(void)peripheral:(CBPeripheral *)peripheral2 didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error
{
    SDL_BlePeripheral* peripheral = find_peripheral_from_cookie((__bridge void *)(peripheral2));
    NSData* data;
    if (error) {
        // NSLog(@"error didUpdateValueForCharacteristic %@ with error: %@", characteristic.UUID, [error localizedDescription]);
        
    } else {
		// NSLog(@"didUpdateValueForCharacteristic %@", characteristic.UUID);
        data = characteristic.value;
/*
		// Byte
		unsigned char *testByte = (unsigned char*)[data bytes];

        char exp = testByte[4];
        double temp = (testByte[1] + (testByte[2] << 8) + (testByte[3] << 16)) * pow(10, exp);
        printf("len: %d, flags: %d, temp: %7.2f, (%d %d %d %d)\n", [data length], testByte[0], temp, testByte[1], testByte[2], testByte[3], testByte[4]);
 */
	}

	if (current_callbacks && current_callbacks->read_characteristic) {
        current_callbacks->read_characteristic(peripheral, find_characteristic_from_cookie(peripheral, (__bridge void *)(characteristic)), (const unsigned char*)[data bytes], (int)[data length]);
    }
}

-(void)peripheral:(CBPeripheral *)peripheral2 didDiscoverDescriptorsForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error
{
	SDL_BlePeripheral* peripheral = find_peripheral_from_cookie((__bridge void *)(peripheral2));
    NSData* data;
	int error2 = 0;
    if (error) {
        NSLog(@"error Discovered DescriptorsForCharacteristic for %@ with error: %@", characteristic.UUID, [error localizedDescription]);
        error2 = -1 * EFAULT;
        
    } else {
        data = characteristic.value;
	}

	if (current_callbacks && current_callbacks->discover_descriptors) {
        current_callbacks->discover_descriptors(peripheral, find_characteristic_from_cookie(peripheral, (__bridge void *)(characteristic)), error2);
    }
/*
    for (CBDescriptor *d in characteristic.descriptors) {
        error2 = 0;
        // [peripheral readValueForDescriptor:d];
    }
*/
}

-(void)peripheral:(CBPeripheral *)peripheral didUpdateValueForDescriptor:(CBDescriptor *)descriptor error:(NSError *)error{

    if (error)
    {
        NSLog(@"error didUpdateValueForDescriptor for %@ with error: %@", descriptor.UUID, [error localizedDescription]);
        return;
    }
}

- (void)peripheral:(CBPeripheral *)peripheral2 didUpdateNotificationStateForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error
{
    SDL_BlePeripheral* peripheral = find_peripheral_from_cookie((__bridge void *)(peripheral2));
    int error2 = 0;
    if (error) {
        NSLog(@"Changing notification state for %@ with error: %@", characteristic.UUID, [error localizedDescription]);
        error2 = -1 * EFAULT;
    }
    if (current_callbacks && current_callbacks->notify_characteristic) {
        current_callbacks->notify_characteristic(peripheral, find_characteristic_from_cookie(peripheral, (__bridge void *)(characteristic)), error2);
    }
}


-(void)peripheral:(CBPeripheral *)peripheral2 didWriteValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error
{
    SDL_BlePeripheral* peripheral = find_peripheral_from_cookie((__bridge void *)(peripheral2));
    int error2 = 0;
    if (error) {
        NSLog(@"Write characterstic for %@ with error: %@", characteristic.UUID, [error localizedDescription]);
        error2 = -1 * EFAULT;
    }

    if (current_callbacks && current_callbacks->write_characteristic) {
        current_callbacks->write_characteristic(peripheral, find_characteristic_from_cookie(peripheral, (__bridge void *)(characteristic)), error2);
    }

}

-(void)peripheral:(CBPeripheral *)peripheral didWriteValueForDescriptor:(CBDescriptor *)descriptor error:(NSError *)error{
//    NSLog(@">>>didWriteValueForCharacteristic");
//    NSLog(@">>>uuid:%@,new value:%@",descriptor.UUID,descriptor.value);
}

@end

void UIKit_ScanPeripherals(const char* uuid)
{
    if (!ble) {
        ble = [SDL_uikitble shareSDL_uikitble];
    }
        
	if (!ble->bleManager) {
		[ble initCentralManager];
    } else {
        [ble scanPeripherals:uuid];
    }
}

void UIKit_StopScanPeripherals()
{
    if (!ble) {
        return;
    }
    [ble->bleManager stopScan];
}

void UIKit_StartAdvertise()
{
    if (!ble) {
        ble = [SDL_uikitble shareSDL_uikitble];
    }
    
    if (!ble->peripheralManager) {
        [ble initPeripheralManager];
    }
}

void UIKit_ConnectPeripheral(SDL_BlePeripheral* peripheral)
{
	CBPeripheral *data = (__bridge CBPeripheral *)peripheral->cookie;
	[ble connectToPeripheral:(CBPeripheral*)data];
}

void UIKit_DisconnectPeripheral(SDL_BlePeripheral* peripheral)
{
	CBPeripheral *data = (__bridge CBPeripheral *)peripheral->cookie;
	[ble->bleManager cancelPeripheralConnection:(CBPeripheral*)data];
}

void UIKit_GetServices(SDL_BlePeripheral* peripheral)
{
	CBPeripheral *data = (__bridge CBPeripheral *)peripheral->cookie;
	[data discoverServices:nil];
}

void UIKit_GetCharacteristics(const SDL_BlePeripheral* peripheral, SDL_BleService* service)
{
	CBPeripheral* peripheral2 = (__bridge CBPeripheral *)peripheral->cookie;
	CBService* service2 = (__bridge CBService *)service->cookie;

	[peripheral2 discoverCharacteristics:nil forService:service2];
}

void UIKit_ReadCharacteristic(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic)
{
	CBPeripheral* peripheral2 = (__bridge CBPeripheral *)peripheral->cookie;
	CBCharacteristic* characteristic2 = (__bridge CBCharacteristic *)characteristic->cookie;
	[peripheral2 readValueForCharacteristic:characteristic2];
}

void UIKit_NotifyCharacteristic(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic)
{
	CBPeripheral* peripheral2 = (__bridge CBPeripheral *)peripheral->cookie;
	CBCharacteristic* characteristic2 = (__bridge CBCharacteristic *)characteristic->cookie;
	[peripheral2 setNotifyValue:YES forCharacteristic:characteristic2];
}

void UIKit_WriteCharacteristic(SDL_BlePeripheral* peripheral, SDL_BleCharacteristic* characteristic, const unsigned char* data, int size)
{
	CBPeripheral* peripheral2 = (__bridge CBPeripheral *)peripheral->cookie;
	CBCharacteristic* characteristic2 = (__bridge CBCharacteristic *)characteristic->cookie;
	NSData* data2 = [NSData dataWithBytes:data length:size];
	[peripheral2 writeValue:data2 forCharacteristic:characteristic2 type:CBCharacteristicWriteWithResponse];
}

void UIKit_DiscoverDescriptors(const SDL_BlePeripheral* peripheral, const SDL_BleCharacteristic* characteristic)
{
	CBPeripheral* peripheral2 = (__bridge CBPeripheral *)peripheral->cookie;
	CBCharacteristic* characteristic2 = (__bridge CBCharacteristic *)characteristic->cookie;
	[peripheral2 discoverDescriptorsForCharacteristic:characteristic2];
}

int UIKit_AuthorizationStatus()
{
    CBPeripheralManagerAuthorizationStatus status = [CBPeripheralManager authorizationStatus];
    int retval = status;
    return retval;
}

int UIKit_IsConnected(const SDL_BlePeripheral* peripheral)
{
/* 
 typedef enum {
    CBPeripheralStateDisconnected = 0,
    CBPeripheralStateConnecting,
    CBPeripheralStateConnected,
} CBPeripheralState;
*/
	// exclude CBPeripheralStateConnecting
	CBPeripheral* peripheral2 = (__bridge CBPeripheral *)peripheral->cookie;
	int state = [peripheral2 state];
    return state == CBPeripheralStateConnected? 1: 0;
}

void UIKit_ReleaseCookie(const SDL_BlePeripheral* peripheral)
{
	CFRelease(peripheral->cookie);
}

/* Windows driver bootstrap functions */
static int UIKit_Available(void)
{
    return (1);
}

static SDL_MiniBle* UIKit_CreateBle()
{
    SDL_MiniBle *ble;

    /* Initialize all variables that we clean on shutdown */
    ble = (SDL_MiniBle *)SDL_calloc(1, sizeof(SDL_MiniBle));
    if (!ble) {
        SDL_OutOfMemory();
        return (0);
    }

    /* Set the function pointers */
	ble->ScanPeripherals = UIKit_ScanPeripherals;
	ble->StopScanPeripherals = UIKit_StopScanPeripherals;
	ble->StartAdvertise = UIKit_StartAdvertise;
	ble->ConnectPeripheral = UIKit_ConnectPeripheral;
	ble->DisconnectPeripheral = UIKit_DisconnectPeripheral;
	ble->GetServices = UIKit_GetServices;
	ble->GetCharacteristics = UIKit_GetCharacteristics;
	ble->ReadCharacteristic = UIKit_ReadCharacteristic;
	ble->NotifyCharacteristic = UIKit_NotifyCharacteristic;
	ble->WriteCharacteristic = UIKit_WriteCharacteristic;
	ble->DiscoverDescriptors = UIKit_DiscoverDescriptors;
	ble->AuthorizationStatus = UIKit_AuthorizationStatus;
	ble->IsConnected = UIKit_IsConnected;
	ble->ReleaseCookie = UIKit_ReleaseCookie;

    return ble;
}


BleBootStrap UIKIT_ble = {
    UIKit_Available, UIKit_CreateBle
};
