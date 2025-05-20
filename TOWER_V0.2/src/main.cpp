#include <Arduino.h>
#include <btMusicBox.h>
#include <Adafruit_NeoPixel.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include "ESP_I2S.h"
#include "ESP_SR.h"

/*---------- 硬件引脚定义（完全保持原始定义）----------*/
#define PIN12 12    // 第一组LED数据引脚
#define PIN13 13    // 第二组LED数据引脚
#define LED_NUM 14  // 每组LED数量

#define I2C_SDA   23
#define I2C_SCL   22
#define I2S_SCLK  19  
#define I2S_DSDIN 5
#define I2S_LRCK  18



// 蓝牙双模初始化
void initBluetoothDualMode() {
  // 配置蓝牙控制器
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  bt_cfg.mode = ESP_BT_MODE_BTDM; // 双模模式
  bt_cfg.bt_max_acl_conn = 1;     // 经典蓝牙最大连接数
  bt_cfg.ble_max_conn = 1;        // BLE最大连接数

  // 初始化控制器
  if(esp_bt_controller_init(&bt_cfg) != ESP_OK) {
    Serial.println("蓝牙控制器初始化失败");
    return;
  }

  // 启用控制器
  if(esp_bt_controller_enable(ESP_BT_MODE_BTDM) != ESP_OK) {
    Serial.println("蓝牙控制器启用失败");
    return;
  }

  // 初始化蓝牙协议栈
  esp_bluedroid_init();
  esp_bluedroid_enable();
}


// 步进电机控制引脚（保持原始定义）
const int a = 14;   // 线圈A+
const int b = 27;   // 线圈A-
const int c = 26;   // 线圈B+
const int d = 25;   // 线圈B-

/*---------- 全局状态变量（新增非阻塞控制变量）----------*/
String MSG;            // 原始消息变量（保持名称不变）

// 电机控制相关变量
unsigned long motorLastStep = 0;  // 最后步进时间戳
int motorPhase = 0;               // 当前步进相位（0-3）
bool motorRunning = false;        // 电机运行标志

// LED控制相关变量
unsigned long ledLastUpdate = 0;  // 最后LED更新时间
int ledPosition = 0;              // 当前LED动画位置
uint32_t currentColor = 0;        // 当前颜色值
bool colorAnimating = false;      // 单色动画标志
bool waterEffect = false;         // 水流动画标志

/*---------- BLE配置（保持原始UUID）----------*/
btMusicBox audio = btMusicBox("TOWERV0.2_AUDIO");
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

/*---------- 外设对象初始化（保持原始方式）----------*/
Adafruit_NeoPixel np1(LED_NUM, PIN12, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel np2(LED_NUM, PIN13, NEO_GRB + NEO_KHZ800);

/*================ 原始任务函数（签名完全不变）================*/
void rotate() {
  motorRunning = true;  // 设置电机运行标志
  motorLastStep = 0;    // 重置步进计时器
  motorPhase = 0;       // 重置步进相位
}

void stop() {
  motorRunning = false; // 停止电机运行
  // 保持原始停止逻辑
  digitalWrite(a, LOW);
  digitalWrite(b, LOW);
  digitalWrite(c, LOW);
  digitalWrite(d, LOW);
}

void white_led() {
  currentColor = np1.Color(255, 255, 255); // 设置白色
  colorAnimating = true;                   // 启动单色动画
  waterEffect = false;                     // 关闭水流效果

}

void red_led() {
  currentColor = np1.Color(255, 0, 0);
  colorAnimating = true;
  waterEffect = false;

}

void orange_led() {
  currentColor = np1.Color(255, 165, 0);
  colorAnimating = true;
  waterEffect = false;
}

void yellow_led() {
  currentColor = np1.Color(255, 255, 0);
  colorAnimating = true;
  waterEffect = false;
}

void green_led() {
  currentColor = np1.Color(0, 255, 0);
  colorAnimating = true;
  waterEffect = false;
}

void cyan_led() {
  currentColor = np1.Color(0, 255, 255);
  colorAnimating = true;
  waterEffect = false;
}

void blue_led() {
  currentColor = np1.Color(0, 0, 255);
  colorAnimating = true;
  waterEffect = false;
}

void purple_led() {
  currentColor = np1.Color(128, 0, 128);
  colorAnimating = true;
  waterEffect = false;
}

void pink_led() {
  currentColor = np1.Color(255, 192, 203);
  colorAnimating = true;
  waterEffect = false;

}

void water_led() {
  waterEffect = true;    // 启用水流效果
  colorAnimating = false;// 关闭单色动画
  ledPosition = 0;       // 重置起始位置
}

void off_led() {
  colorAnimating = false; // 停止所有动画
  waterEffect = false;
  np1.clear();           // 立即清除LED
  np2.clear();
  np1.show();
  np2.show();
}

/*================ 优化后的后台更新函数 ================*/
void updateMotor() {
  // 每10ms执行一次步进（保持原始延迟时间）
  if(millis() - motorLastStep < 10) return;

  // 四相步进逻辑（保持原始相位顺序）
  switch(motorPhase % 4) {
    case 0:
      digitalWrite(a, LOW);
      digitalWrite(b, LOW);
      digitalWrite(c, LOW);
      digitalWrite(d, HIGH);
      break;
    case 1:
      digitalWrite(a, LOW);
      digitalWrite(b, LOW);
      digitalWrite(c, HIGH);
      digitalWrite(d, LOW);
      break;
    case 2:
      digitalWrite(a, LOW);
      digitalWrite(b, HIGH);
      digitalWrite(c, LOW);
      digitalWrite(d, LOW);
      break;
    case 3:
      digitalWrite(a, LOW);
      digitalWrite(b, LOW);
      digitalWrite(c, LOW);
      digitalWrite(d, LOW);
      break;
  }

  motorPhase++;          // 移动到下一相位
  motorLastStep = millis(); // 更新时间戳
}

void updateLEDs() {
  // 每200ms更新LED（保持原始动画速度）
  if(millis() - ledLastUpdate < 200) return;

  // 单色流水灯效果
  // if(colorAnimating) {
  //   np1.clear();  // 清除前一个LED
  //   np2.clear();
    
  //   // 设置当前LED颜色（保持原始逐个点亮方式）
  //   np1.setPixelColor(ledPosition, currentColor);
  //   np2.setPixelColor(ledPosition, currentColor);
    
  //   ledPosition = (ledPosition + 1) % LED_NUM; // 移动位置
  // }
  
  // 单色灯效果
  if(colorAnimating) {
    np1.clear();  // 清除前一个LED
    np2.clear();
    
    // 设置所有LED颜色（一次性点亮）
    for (int i = 0; i < LED_NUM; i++) {
      np1.setPixelColor(i, currentColor);
      np2.setPixelColor(i, currentColor);
    }
    colorAnimating = false; // 关闭动画标志，确保只点亮一遍
  }

  // 水流动画效果
  if(waterEffect) {
    // 正向流水
    uint8_t r = random(256); // 保持原始随机颜色生成
    uint8_t g = random(256);
    uint8_t b = random(256);
    for(int i=0; i<LED_NUM; i++) {
      np1.setPixelColor(i, r, g, b);
      np2.setPixelColor(i, r, g, b);
    }
    
    // 反向流水（保持原始往返效果）
    for(int i=LED_NUM-1; i>=0; i--) {
      np1.setPixelColor(i, r, g, b);
      np2.setPixelColor(i, r, g, b);
    }
  }

  np1.show(); // 统一更新显示
  np2.show();
  ledLastUpdate = millis(); // 更新时间戳
}

void bt_on() ;  // 声明蓝牙音频初始化函数

/*================ BLE回调处理（保持原始结构）================*/
class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    
    // 保持原始消息处理逻辑
    if (!value.empty()) {
      MSG = "";
      for (char c : value) {
        if (isprint(c)) MSG += c;
      }
      
      Serial.print("Received: ");
      Serial.println(MSG.c_str());
    }

    // 保持原始命令分支结构
    if (MSG == "white")       white_led();
    else if (MSG == "red")    red_led();
    else if (MSG == "orange") orange_led();
    else if (MSG == "yellow") yellow_led();
    else if (MSG == "green")  green_led();
    else if (MSG == "cyan")   cyan_led();
    else if (MSG == "blue")   blue_led();
    else if (MSG == "purple") purple_led();
    else if (MSG == "pink")   pink_led();
    else if (MSG == "water")  water_led();
    else if (MSG == "off")    off_led();
    else if (MSG == "BT")     bt_on();
    else if (MSG == "rotate") rotate();
    else if (MSG == "stop")   stop();
  }
};

void bt_on() {
  audio.begin();        // 初始化蓝牙音频
  audio.reconnect();    // 尝试重新连接
}

I2SClass i2s; // 创建一个I2S类的实例，用于I2S通信

// 生成的命令列表，使用以下命令生成：
// python3 tools/gen_sr_commands.py "Turn on the light,Switch on the light;Turn off the light,Switch off the light,Go dark;Start fan;Stop fan"

// 定义语音识别命令的枚举值
enum {
  SR_CMD_TURN_ON_THE_LIGHT, // 打开灯的命令
  SR_CMD_TURN_OFF_THE_LIGHT, // 关闭灯的命令
  SR_CMD_START_FAN, // 启动风扇的命令
  SR_CMD_STOP_FAN, // 停止风扇的命令
};

// 定义语音识别命令的数组，每个命令包含ID、文本和发音
static const sr_cmd_t sr_commands[] = {
  {0, "Turn on the light", "TkN nN jc LiT"}, // 打开灯的命令及其发音
  {0, "Switch on the light", "SWgp nN jc LiT"}, // 切换打开灯的命令及其发音
  {1, "Turn off the light", "TkN eF jc LiT"}, // 关闭灯的命令及其发音
  {1, "Switch off the light", "SWgp eF jc LiT"}, // 切换关闭灯的命令及其发音
  {1, "Go dark", "Gb DnRK"}, // 变暗的命令及其发音
  {2, "Start fan", "STnRT FaN"}, // 启动风扇的命令及其发音
  {3, "Stop fan", "STnP FaN"}, // 停止风扇的命令及其发音
};

// 定义语音识别事件的回调函数
void onSrEvent(sr_event_t event, int command_id, int phrase_id) {
  switch (event) { // 根据事件类型执行不同的操作
    case SR_EVENT_WAKEWORD: // 如果检测到唤醒词
      Serial.println("WakeWord Detected!"); // 打印唤醒词检测信息
      break;
    case SR_EVENT_WAKEWORD_CHANNEL: // 如果检测到唤醒词的通道
      Serial.printf("WakeWord Channel %d Verified!\n", command_id); // 打印唤醒词通道信息
      ESP_SR.setMode(SR_MODE_COMMAND);  // 切换到命令检测模式
      break;
    case SR_EVENT_TIMEOUT: // 如果检测到超时
      Serial.println("Timeout Detected!"); // 打印超时信息
      ESP_SR.setMode(SR_MODE_WAKEWORD);  // 切换回唤醒词检测模式
      break;
    case SR_EVENT_COMMAND: // 如果检测到命令
      Serial.printf("Command %d Detected! %s\n", command_id, sr_commands[phrase_id].str); // 打印命令信息
      switch (command_id) { // 根据命令ID执行操作
        case SR_CMD_TURN_ON_THE_LIGHT:  white_led(); break; // 打开灯
        case SR_CMD_TURN_OFF_THE_LIGHT: off_led(); break; // 关闭灯
        case SR_CMD_START_FAN:          rotate(); break; // 旋转
        case SR_CMD_STOP_FAN:           stop(); break; // 停止旋转
        default:                        Serial.println("Unknown Command!"); break; // 未知命令
      }
      ESP_SR.setMode(SR_MODE_COMMAND);  // 允许在超时前继续检测命令
      // ESP_SR.setMode(SR_MODE_WAKEWORD); // 切换回唤醒词检测模式（注释掉的代码）
      break;
    default: Serial.println("Unknown Event!"); break; // 未知事件
  }
}


/*================ 初始化函数（保持原始结构）================*/
void setup() {
  Serial.begin(115200);
  
  // 初始化电机引脚（保持原始方式）
  pinMode(a, OUTPUT);
  pinMode(b, OUTPUT);
  pinMode(c, OUTPUT);
  pinMode(d, OUTPUT);
  stop(); // 确保电机初始停止

  // 初始化LED（保持原始配置）
  np1.begin();
  np2.begin();
  np1.clear();
  np2.clear();
  np1.show();
  np2.show();

  initBluetoothDualMode(); // 初始化蓝牙双模
  
  // BLE初始化（保持原始流程）
  BLEDevice::init("TOWER V0.2_BLE");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  pServer->getAdvertising()->start();

  // 音频初始化（保持原始配置）
  Wire.end();             // 释放原始I2C总线
  Wire.begin(I2C_SDA, I2C_SCL);   // 重新初始化I2C
  Audio_codeC(ES8311);       // 保持原始编码器设置
  audio.I2S(I2S_SCLK, I2S_DSDIN, I2S_LRCK);
  audio.volume(1.0);         // 保持原始音量设置

  // GPIO初始化（保持原始配置）
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);    // 保持原始GPIO状态
}

/*================ 主循环（非阻塞架构）================*/
void loop() {
  // 电机状态检测
  if(motorRunning) {
    updateMotor(); // 非阻塞步进控制
  }

  // LED状态检测
  if(colorAnimating|waterEffect) {
    updateLEDs(); // 非阻塞动画更新
  }

  delay(10); // 维持系统响应
}