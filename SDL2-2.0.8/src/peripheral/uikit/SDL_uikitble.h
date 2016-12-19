#import <Foundation/Foundation.h>
#import <CoreBluetooth/CoreBluetooth.h>

@interface SDL_uikitble : NSObject<CBCentralManagerDelegate, CBPeripheralDelegate, CBPeripheralManagerDelegate> {
    
@public
    // 主设备
    CBCentralManager* bleManager;
    
    // @property(strong,nonatomic) CBCentralManager* bleManager;
    
    // @property(strong,nonatomic) CBPeripheralManager *peripheraManager;
    // @property(strong,nonatomic) CBMutableCharacteristic *customerCharacteristic;
    // @property (strong,nonatomic) CBMutableService *customerService;

    CBPeripheralManager* peripheralManager;
    CBMutableCharacteristic* customerCharacteristic;
    CBMutableService* customerService;
}



//扫描Peripherals
-(void)scanPeripherals:(const char*)uuid;
//连接Peripherals
-(void)connectToPeripheral:(CBPeripheral *)peripheral;
//断开所以已连接的设备
-(void)stopConnectAllPerihperals;
//停止扫描
-(void)stopScan;

/**
 * 单例构造方法
 * @return BabyBluetooth共享实例
 */
+(instancetype)shareSDL_uikitble;

@end



