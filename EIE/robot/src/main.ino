#include <Arduino.h>

#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <ESP32Servo.h>
#include <BluetoothSerial.h>
#include "MPU6050_6Axis_MotionApps20.h"
#include "EEPROM.h"
#include <esp_wifi.h>
#include <tcpip_adapter.h>
#include <lwip/ip4_addr.h> // 包含ip4_addr_ntoa函数的头文件
#include<string>

#define Addr 0x1C
#define SDA 18
#define SCL 19

#define EEPROM_SIZE 64

#define INTERRUPT_PIN   16




/* Servos --------------------------------------------------------------------*/
//define 12 servos for 4 legs
Servo servo[4][3];
//define servos' ports
const int servo_pin[4][3] = {{18,  5,  19}, {  2,  4, 15}, {53, 25,  32}, {14,  27, 13}};
//const int offset[4][3] =    {{30,  -90, 25 }, { 0,  75,  45}, { -30,  30,  55}, { 25,  10, 0}};
const int offset[4][3] =    {{25,  -90, 0 }, { 0,  60,  50}, { -40,  20,  30}, { 25,  20, 10}};
/* Size of the robot ---------------------------------------------------------*/
const float length_a = 55;
const float length_b = 77.5;
const float length_c = 27.5;
const float length_side = 71;
const float z_absolute = -28;
/* Constants for movement ----------------------------------------------------*/
const float z_default = -50, z_up = -30, z_boot = z_absolute;
const float x_default = 62, x_offset = 0;
const float y_start = 0, y_step = 40;
const float y_default = x_default;
/* variables for movement ----------------------------------------------------*/
volatile float site_now[4][3];    //real-time coordinates of the end of each leg
volatile float site_expect[4][3]; //expected coordinates of the end of each leg
float temp_speed[4][3];   //each axis' speed, needs to be recalculated before each movement
float move_speed = 1.4;   //movement speed
float speed_multiple = 1; //movement speed multiple
const float spot_turn_speed = 4;
const float leg_move_speed = 8;
const float body_move_speed = 3;
const float stand_seat_speed = 1;
volatile int rest_counter;      //+1/0.02s, for automatic rest
//functions' parameter
const float KEEP = 255;
//define PI for calculation
const float pi = 3.1415926;
/* Constants for turn --------------------------------------------------------*/
//temp length
const float temp_a = sqrt(pow(2 * x_default + length_side, 2) + pow(y_step, 2));
const float temp_b = 2 * (y_start + y_step) + length_side;
const float temp_c = sqrt(pow(2 * x_default + length_side, 2) + pow(2 * y_start + y_step + length_side, 2));
const float temp_alpha = acos((pow(temp_a, 2) + pow(temp_b, 2) - pow(temp_c, 2)) / 2 / temp_a / temp_b);
//site for turn
const float turn_x1 = (temp_a - length_side) / 2;
const float turn_y1 = y_start + y_step / 2;
const float turn_x0 = turn_x1 - temp_b * cos(temp_alpha);
const float turn_y0 = temp_b * sin(temp_alpha) - turn_y1 - length_side;

int FRFoot = 0;
int FRElbow = 0;
int FRShdr = 0;
int FLFoot = 0;
int FLElbow = 0;
int FLShdr = 0;
int RRFoot = 0;
int RRElbow = 0;
int RRShdr = 0;
int RLFoot = 0;
int RLElbow = 0;
int RLShdr = 0;

static uint32_t currentTime = 0;
static uint16_t previousTime = 0;
uint16_t cycleTime = 0;
//超声波避障变量定义
float       distance;
const int   trig_echo = 23;

int act = 1;
char val;

String header;
const char *ssid = "QuadBot-T-22"; //热点名称，自定义
const char *password = "1234567891";//连接密码，自定义
void Task1code( void *pvParameters );
void Task2code( void *pvParameters );
//wifi在core0，其他在core1；1为大核
WiFiServer server(80);


//*IMU-------------------------------------------
class IMU {
  private:
    MPU6050 imu;
    float euler[3];         // [psi, theta, phi]    Euler angle container
    float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
    int16_t temperature;

    // MPU control/status vars
    bool dmpReady = false;  // set true if DMP init was successful

  public:
    int init(uint8_t pin);
    void update();

    float getYaw();
    float getPitch();
    float getRoll();

    int16_t getTemperature();
};

IMU imu;

volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
void dmpDataReady() {
  mpuInterrupt = true;
}
//*----------------------------------------------------

void setup()
{
 

  Serial.begin(115200);
  Serial.println("Robot starts initialization");
  //IMU输入初始化
  imu.init(INTERRUPT_PIN);

  //initialize default parameter
  set_site(0, x_default - x_offset, y_start + y_step, z_boot);
  set_site(1, x_default - x_offset, y_start + y_step, z_boot);
  set_site(2, x_default + x_offset, y_start, z_boot);
  set_site(3, x_default + x_offset, y_start, z_boot);
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      site_now[i][j] = site_expect[i][j];
    }
  }
  //start servo service
  Serial.println("Servo service started");
  //initialize servos
  servo_attach();
  Serial.println("Servos initialized");
  Serial.println("Robot initialization Complete");

  IPAddress customIP(192, 168, 98, 1); // 自定义的 IP 地址
  IPAddress subnet(255, 255, 255, 0);  // 子网掩码


  WiFi.softAPConfig(customIP, customIP, subnet);

  WiFi.softAP(ssid, password);//不写password，即热点开放不加密
  IPAddress myIP = WiFi.softAPIP();//此为默认IP地址192.168.4.1，也可在括号中自行填入自定义IP地址
  Serial.print("AP IP address:");
  Serial.println(myIP);
  server.begin();
  Serial.println("Server started");

  
  xTaskCreatePinnedToCore(Task1, "Task1", 10000, NULL, 1, NULL,  0);  //最后一个参数至关重要，决定这个任务创建在哪个核上.PRO_CPU 为 0, APP_CPU 为 1,或者 tskNO_AFFINITY 允许任务在两者上运行.
  xTaskCreatePinnedToCore(Task2, "Task2", 10000, NULL, 1, NULL,  1);//
  //实现任务的函数名称（task1）；任务的任何名称（“ task1”等）；分配给任务的堆栈大小，以字为单位；任务输入参数（可以为NULL）；任务的优先级（0是最低优先级）；任务句柄（可以为NULL）；任务将运行的内核ID（0或1）
}

void servo_attach(void)
{
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      servo[i][j].attach(servo_pin[i][j], 500, 2500);
      delay(100);
    }
  }
}

void servo_detach(void)
{
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      servo[i][j].detach();
      delay(100);
    }
  }
}

void writeKeyValue(int8_t key, int8_t value)
{
  EEPROM.write(key, value);
  EEPROM.commit();
}

int8_t readKeyValue(int8_t key)
{
  Serial.println("read");
  Serial.println(key);

  int8_t value = EEPROM.read(key);
}

void loop()
{
  imu.update();
  delay(20);
  // put your main code here, to run repeatedly:
}

void Task1(void *pvParameters)
{
  set_home();//在这里可以添加一些代码，这样的话这个任务执行时会先执行一次这里的内容（当然后面进入while循环之后不会再执行这部分了）
  while (1)
  {
    vTaskDelay(20);
    WiFiClient client = server.available();  //监听连入设备
    if (client)
    {
      String currentLine = "";
      while (client.connected())
      {
        if (client.available())
        {
          char c = client.read();
          header += c;
          if (c == '\n')
          {
            if (currentLine.length() == 0)
            {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();
              client.println("<!DOCTYPE html><html>");
              client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              client.println("<meta charset=\"UTF-8\">");
              client.println("<link rel=\"icon\" href=\"data:,\">");
              client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
              client.println(".button { background-color: #096625; border: none; color: white; padding: 20px 25px; ");
              client.println("text-decoration: none; font-size: 18px; margin: 2px; cursor: pointer;}");
              client.println(".button2 {background-color: #555555; border: none; color: white; padding: 16px 20px;text-decoration: none; font-size: 16px; margin: 1px; cursor: pointer;}</style></head>");
              client.println("<body><h1>四足机器人控制</h1>");
              client.println("<p><a href=\"/20/on\"><button class=\"button\">前进</button></a></p>");
              client.println("<p><a href=\"/21/on\"><button class=\"button\">左转</button></a><a href=\"/22/on\"><button class=\"button\">停止</button></a><a href=\"/23/on\"><button class=\"button\">右转</button></a></p>");
              client.println("<p><a href=\"/24/on\"><button class=\"button\">后退</button></a></p>");
              client.println("<p><a href=\"/25/on\"><button class=\"button2\">挥手</button></a><a href=\"/26/on\"><button class=\"button2\">握手</button></a><a href=\"/27/on\"><button class=\"button2\">跳舞</button></a><a  href=\"/28/on\"><button class=\"button2\">坐下</button></a></p>");
              client.println("</body></html>");
              client.println();
              if (header.indexOf("GET /20/on") >= 0) //前进
              {
                step_forward(1);
              }
              if (header.indexOf("GET /21/on") >= 0) //后退
              {
                step_back(1);
              }
              if (header.indexOf("GET /22/on") >= 0) //左转
              {
                turn_left(1);
              }
              if (header.indexOf("GET /23/on") >= 0) //右转
              {
                turn_right(1);
              }
              if (header.indexOf("GET /24/on") >= 0) //停止
              {
                val = 'b';
                Serial.println(val);
              }
              if (header.indexOf("GET /25/on") >= 0) //握手
              {
                hand_shake(1);
              }
              if (header.indexOf("GET /26/on") >= 0) //抬头
              {
                up();
              }
              if (header.indexOf("GET /27/on") >= 0) //招手
              {
                hand_wave(1);
              }
              if (header.indexOf("GET /28/on") >= 0) //左扭
              {
                FLShdr += 4; FRShdr -= 4;
                RLShdr -= 4; RRShdr += 4;
                stand();
              }
              if (header.indexOf("GET /29/on") >= 0) //低头
              {
                down();
              }
              if (header.indexOf("GET /30/on") >= 0) //右扭
              {
                FLShdr -= 4; FRShdr += 4;
                RLShdr += 4; RRShdr -= 4;
                stand();
              }
              if (header.indexOf("GET /31/on") >= 0) //左倾
              {
                B_left();
              }
              if (header.indexOf("GET /32/on") >= 0) //抬升
              {
                FLElbow -= 4; FRElbow -= 4; //--
                RLElbow -= 4; RRElbow -= 4; //--
                FLFoot -= 4; FRFoot -= 4; //++
                RLFoot -= 4; RRFoot -= 4; //++
                stand();
              }
              if (header.indexOf("GET /33/on") >= 0) //右倾
              {
                B_right();
              }
              if (header.indexOf("GET /34/on") >= 0) //服务
              {
                Auto_blance();
              }
              if (header.indexOf("GET /35/on") >= 0) //下蹲
              {
                FLElbow += 4; FRElbow += 4;
                RLElbow += 4; RRElbow += 4;
                FLFoot += 4; FRFoot += 4;
                RLFoot += 4; RRFoot += 4;
                stand();
              }
              if (header.indexOf("GET /36/on") >= 0) //复位
              {
                set_home();
              }
              if (header.indexOf("GET /37/on") >= 0) //坐下
              {
                sit();
              }
              if (header.indexOf("GET /38/on") >= 0) //跳舞
              {
                body_dance(10);
              }
              if (header.indexOf("GET /39/on") >= 0) //开始避障
              {
                Avoid();//
              }
              if (header.indexOf("GET /40/on") >= 0) //结束避障
              {
                sit();
                b_init();
                FLElbow = 0; FRElbow = 0;
                RLElbow = 0; RRElbow = 0;
                FLFoot = 0; FRFoot = 0;
                RLFoot = 0; RRFoot = 0;
                FLShdr = 0; FRShdr = 0;
                RLShdr = 0; RRShdr = 0;
                stand(); //
              }
              if(header.indexOf("GET /41/on") >= 0)//检测到人脸
              {
                sit();
                b_init();
                FLElbow = 0; FRElbow = 0;
                RLElbow = 0; RRElbow = 0;
                FLFoot = 0; FRFoot = 0;
                RLFoot = 0; RRFoot = 0;
                FLShdr = 0; FRShdr = 0;
                RLShdr = 0; RRShdr = 0;
                stand(); //

                delay(1000);

                hand_wave(1);

                delay(1000);

                Avoid();//


              }

              break;
            }
            else
            {
              currentLine = ""; //如果收到是换行，则清空字符串变量
            }
          }
          else if (c != '\r')
          {
            currentLine += c;
          }
        }
        vTaskDelay(1);
      }
      header = "";
      client.stop(); //断开连接
    }
  }
}

bool Auto_blance (void)
{
  while (1)
  {
    vTaskDelay(10);
    WiFiClient client = server.available();  //监听连入设备

    //函数退出
    if (client)
    {
      String currentLine = "";
      while (client.connected())
      {
        if (client.available())
        {
          char c = client.read();
          header += c;
          if (c == '\n')
          {
            if (currentLine.length() == 0)
            {
              if (header.indexOf("GET /36/on") >= 0) //点击复位进行退出
              {
                set_home();
                return true;
              }
              else
              {
                return false;
              }
            }
          }
        }
      }
    }


    //平衡程序
    if (imu.getPitch() > 3)
    {
      down();
    }
    if (imu.getPitch() < -3)
    {
      up();
    }
    if (imu.getRoll() > 3)
    {
      B_left();
    }

    if (imu.getRoll() < -3)
    {
      B_right();
    }
    delay(50);
  }


}


void capture_picture()
{
  Serial.println("capture picture");

  int flag=0;

  

  WiFiClient client;
  if (client.connect("192.168.98.2", 8080)) {
      // Construct the HTTP request
    String request = "GET /capture HTTP/1.1\r\n";
    request += "Host: 192.168.98.2\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";
     // Send the HTTP request
    client.print(request);
    flag=1;
  }
  else if (client.connect("192.168.98.3", 8080)) {
      // Construct the HTTP request
    String request = "GET /capture HTTP/1.1\r\n";
    request += "Host: 192.168.98.3\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";
     // Send the HTTP request
    client.print(request);
    flag=1;
  }
 else if (client.connect("192.168.98.4", 8080)) {
      // Construct the HTTP request
    String request = "GET /capture HTTP/1.1\r\n";
    request += "Host: 192.168.98.4\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";
     // Send the HTTP request
    client.print(request);
    flag=1;
  }
 else if (client.connect("192.168.98.5", 8080)) {
      // Construct the HTTP request
    String request = "GET /capture HTTP/1.1\r\n";
    request += "Host: 192.168.98.5\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";
     // Send the HTTP request
    client.print(request);
    flag=1;
  }
 else if (client.connect("192.168.98.6", 8080)) {
      // Construct the HTTP request
    String request = "GET /capture HTTP/1.1\r\n";
    request += "Host: 192.168.98.6\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";
     // Send the HTTP request
    client.print(request);
    flag=1;
  }


  if(!flag)
  {
    Serial.println("error when connect with esp and computer");
  }

 

  // Check server response
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }

  // Close the connection
  client.stop();
}



bool Avoid(void)
{
  while (1)
  {
    vTaskDelay(10);
    WiFiClient client = server.available();  //监听连入设备
    pinMode(trig_echo, OUTPUT);                      //设置Trig_RX_SCL_I/O为输出

    digitalWrite(trig_echo, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig_echo, LOW);                    //Trig_RX_SCL_I/O脚输出10US高电平脉冲触发信号

    pinMode(trig_echo, INPUT);                       //设置Trig_RX_SCL_I/O为输入，接收模块反馈的距离信号

    distance  = pulseIn(trig_echo, HIGH);            //计数接收到的高电平时间
    //Serial.print("原始值: ");
    //Serial.print(distance);
    distance  = distance * 340 / 2 / 10000;          //计算距离 1：声速：340M/S  2：实际距离为1/2声速距离 3：计数时钟为1US//温补公式：c=(331.45+0.61t/℃)m•s-1 (其中331.45是在0度）
    Serial.print("距离: ");
    Serial.print(distance);
    Serial.println("CM");                            //串口输出距离信号
    pinMode(trig_echo, OUTPUT);                      //设置Trig_RX_SCL_I/O为输出，准备下次测量
    // Serial.println("");                              //换行

    if (client)
    {
      String currentLine = "";
      while (client.connected())
      {
        if (client.available())
        {
          char c = client.read();
          header += c;
          if (c == '\n')
          {
            if (currentLine.length() == 0)
            {
              if (header.indexOf("GET /40/on") >= 0) //结束避障
              {
                set_home();
                return true;
              }
              else
              {
                return false;
              }
            }
          }
        }
      }
    }
    delay(30);                                       //单次测离完成后加30mS的延时再进行下次测量。防止近距离测量时，测量到上次余波，导致测量不准确。

    if (distance <= 30)
    {
      stand();
      capture_picture();
      delay(1000);
      turn_left(5);
    }
    else
    {
      step_forward(3);
    }

  }
}

void Task2(void *pvParameters)
{
  while (1)
  {
    vTaskDelay(15);
    servo_service();
  }
}

void set_home (void)
{
  sit();
  b_init();
  FLElbow = 0; FRElbow = 0;
  RLElbow = 0; RRElbow = 0;
  FLFoot = 0; FRFoot = 0;
  RLFoot = 0; RRFoot = 0;
  FLShdr = 0; FRShdr = 0;
  RLShdr = 0; RRShdr = 0;
  stand();
}
//抬头
void up(void)
{
  if (-45 < FLElbow && FLElbow < 45)
  {
    FLElbow -= 4; FRElbow -= 4;
    RLElbow += 4; RRElbow += 4;
    FLFoot -= 4; FRFoot -= 4;
    RLFoot += 4; RRFoot += 4;
    stand();
  }
  else
  {
    set_home();
  }

}
//低头
void down(void)
{
  if (-45 < FLElbow && FLElbow < 45)
  {
    FLElbow += 4; FRElbow += 4;
    RLElbow -= 4; RRElbow -= 4;
    FLFoot += 4; FRFoot += 4;
    RLFoot -= 4; RRFoot -= 4;
    stand();
  }
  else
  {
    set_home();
  }

}

void B_left(void)
{
  if (-45 < RRFoot && RRFoot < 45)
  {
    if (!is_stand()) stand();
    FLElbow += 4; FRElbow -= 4;
    RLElbow += 4; RRElbow -= 4;
    FLFoot -= 4; FRFoot += 4;
    RLFoot -= 4; RRFoot += 4;
    stand();
  }
  else
  {
    set_home();
  }
}

void B_right(void)
{
  if (-45 < RRFoot && RRFoot < 45)
  {
    if (!is_stand()) stand();
    FLElbow -= 4; FRElbow += 4;
    RLElbow -= 4; RRElbow += 4;
    FLFoot += 4; FRFoot -= 4;
    RLFoot += 4; RRFoot -= 4;
    stand();
  }
  else
  {
    set_home();
  }
}

/*
  - is_stand
   ---------------------------------------------------------------------------*/
bool is_stand(void)
{
  if (site_now[0][2] == z_default)
    return true;
  else
    return false;
}

/*
  - sit
  - blocking function
   ---------------------------------------------------------------------------*/
void sit(void)
{
  move_speed = stand_seat_speed;
  for (int leg = 0; leg < 4; leg++)
  {
    set_site(leg, KEEP, KEEP, z_boot);
  }
  wait_all_reach();
}

/*
  - stand
  - blocking function
   ---------------------------------------------------------------------------*/
void stand(void)
{
  move_speed = stand_seat_speed;
  for (int leg = 0; leg < 4; leg++)
  {
    set_site(leg, KEEP, KEEP, z_default);
  }
  wait_all_reach();
}


/*
  - Body init
  - blocking function
   ---------------------------------------------------------------------------*/
void b_init(void)
{
  //stand();
  set_site(0, x_default, y_default, z_default);
  set_site(1, x_default, y_default, z_default);
  set_site(2, x_default, y_default, z_default);
  set_site(3, x_default, y_default, z_default);
  wait_all_reach();
}


/*
  - spot turn to left
  - blocking function
  - parameter step steps wanted to turn
   ---------------------------------------------------------------------------*/
void turn_left(unsigned int step)
{
  move_speed = spot_turn_speed;
  while (step-- > 0)
  {
    if (site_now[3][1] == y_start)
    {
      //leg 3&1 move
      set_site(3, x_default + x_offset, y_start, z_up);
      wait_all_reach();

      set_site(0, turn_x1 - x_offset, turn_y1, z_default);
      set_site(1, turn_x0 - x_offset, turn_y0, z_default);
      set_site(2, turn_x1 + x_offset, turn_y1, z_default);
      set_site(3, turn_x0 + x_offset, turn_y0, z_up);
      wait_all_reach();

      set_site(3, turn_x0 + x_offset, turn_y0, z_default);
      wait_all_reach();

      set_site(0, turn_x1 + x_offset, turn_y1, z_default);
      set_site(1, turn_x0 + x_offset, turn_y0, z_default);
      set_site(2, turn_x1 - x_offset, turn_y1, z_default);
      set_site(3, turn_x0 - x_offset, turn_y0, z_default);
      wait_all_reach();

      set_site(1, turn_x0 + x_offset, turn_y0, z_up);
      wait_all_reach();

      set_site(0, x_default + x_offset, y_start, z_default);
      set_site(1, x_default + x_offset, y_start, z_up);
      set_site(2, x_default - x_offset, y_start + y_step, z_default);
      set_site(3, x_default - x_offset, y_start + y_step, z_default);
      wait_all_reach();

      set_site(1, x_default + x_offset, y_start, z_default);
      wait_all_reach();
    }
    else
    {
      //leg 0&2 move
      set_site(0, x_default + x_offset, y_start, z_up);
      wait_all_reach();

      set_site(0, turn_x0 + x_offset, turn_y0, z_up);
      set_site(1, turn_x1 + x_offset, turn_y1, z_default);
      set_site(2, turn_x0 - x_offset, turn_y0, z_default);
      set_site(3, turn_x1 - x_offset, turn_y1, z_default);
      wait_all_reach();

      set_site(0, turn_x0 + x_offset, turn_y0, z_default);
      wait_all_reach();

      set_site(0, turn_x0 - x_offset, turn_y0, z_default);
      set_site(1, turn_x1 - x_offset, turn_y1, z_default);
      set_site(2, turn_x0 + x_offset, turn_y0, z_default);
      set_site(3, turn_x1 + x_offset, turn_y1, z_default);
      wait_all_reach();

      set_site(2, turn_x0 + x_offset, turn_y0, z_up);
      wait_all_reach();

      set_site(0, x_default - x_offset, y_start + y_step, z_default);
      set_site(1, x_default - x_offset, y_start + y_step, z_default);
      set_site(2, x_default + x_offset, y_start, z_up);
      set_site(3, x_default + x_offset, y_start, z_default);
      wait_all_reach();

      set_site(2, x_default + x_offset, y_start, z_default);
      wait_all_reach();
    }
  }
}

/*
  - spot turn to right
  - blocking function
  - parameter step steps wanted to turn
   ---------------------------------------------------------------------------*/
void turn_right(unsigned int step)
{
  move_speed = spot_turn_speed;
  while (step-- > 0)
  {
    if (site_now[2][1] == y_start)
    {
      //leg 2&0 move
      set_site(2, x_default + x_offset, y_start, z_up);
      wait_all_reach();

      set_site(0, turn_x0 - x_offset, turn_y0, z_default);
      set_site(1, turn_x1 - x_offset, turn_y1, z_default);
      set_site(2, turn_x0 + x_offset, turn_y0, z_up);
      set_site(3, turn_x1 + x_offset, turn_y1, z_default);
      wait_all_reach();

      set_site(2, turn_x0 + x_offset, turn_y0, z_default);
      wait_all_reach();

      set_site(0, turn_x0 + x_offset, turn_y0, z_default);
      set_site(1, turn_x1 + x_offset, turn_y1, z_default);
      set_site(2, turn_x0 - x_offset, turn_y0, z_default);
      set_site(3, turn_x1 - x_offset, turn_y1, z_default);
      wait_all_reach();

      set_site(0, turn_x0 + x_offset, turn_y0, z_up);
      wait_all_reach();

      set_site(0, x_default + x_offset, y_start, z_up);
      set_site(1, x_default + x_offset, y_start, z_default);
      set_site(2, x_default - x_offset, y_start + y_step, z_default);
      set_site(3, x_default - x_offset, y_start + y_step, z_default);
      wait_all_reach();

      set_site(0, x_default + x_offset, y_start, z_default);
      wait_all_reach();
    }
    else
    {
      //leg 1&3 move
      set_site(1, x_default + x_offset, y_start, z_up);
      wait_all_reach();

      set_site(0, turn_x1 + x_offset, turn_y1, z_default);
      set_site(1, turn_x0 + x_offset, turn_y0, z_up);
      set_site(2, turn_x1 - x_offset, turn_y1, z_default);
      set_site(3, turn_x0 - x_offset, turn_y0, z_default);
      wait_all_reach();

      set_site(1, turn_x0 + x_offset, turn_y0, z_default);
      wait_all_reach();

      set_site(0, turn_x1 - x_offset, turn_y1, z_default);
      set_site(1, turn_x0 - x_offset, turn_y0, z_default);
      set_site(2, turn_x1 + x_offset, turn_y1, z_default);
      set_site(3, turn_x0 + x_offset, turn_y0, z_default);
      wait_all_reach();

      set_site(3, turn_x0 + x_offset, turn_y0, z_up);
      wait_all_reach();

      set_site(0, x_default - x_offset, y_start + y_step, z_default);
      set_site(1, x_default - x_offset, y_start + y_step, z_default);
      set_site(2, x_default + x_offset, y_start, z_default);
      set_site(3, x_default + x_offset, y_start, z_up);
      wait_all_reach();

      set_site(3, x_default + x_offset, y_start, z_default);
      wait_all_reach();
    }
  }
}

/*
  - go forward
  - blocking function
  - parameter step steps wanted to go
   ---------------------------------------------------------------------------*/
void step_forward(unsigned int step)
{
  move_speed = leg_move_speed;
  while (step-- > 0)
  {
    Serial.println(step);
    if (site_now[2][1] == y_start)
    {
      //leg 2&1 move
      set_site(2, x_default + x_offset, y_start, z_up);
      wait_all_reach();
      set_site(2, x_default + x_offset, y_start + 2 * y_step, z_up);
      wait_all_reach();
      set_site(2, x_default + x_offset, y_start + 2 * y_step, z_default);
      wait_all_reach();

      move_speed = body_move_speed;

      set_site(0, x_default + x_offset, y_start, z_default);
      set_site(1, x_default + x_offset, y_start + 2 * y_step, z_default);
      set_site(2, x_default - x_offset, y_start + y_step, z_default);
      set_site(3, x_default - x_offset, y_start + y_step, z_default);
      wait_all_reach();

      move_speed = leg_move_speed;

      set_site(1, x_default + x_offset, y_start + 2 * y_step, z_up);
      wait_all_reach();
      set_site(1, x_default + x_offset, y_start, z_up);
      wait_all_reach();
      set_site(1, x_default + x_offset, y_start, z_default);
      wait_all_reach();
    }
    else
    {
      //leg 0&3 move
      set_site(0, x_default + x_offset, y_start, z_up);
      wait_all_reach();
      set_site(0, x_default + x_offset, y_start + 2 * y_step, z_up);
      wait_all_reach();
      set_site(0, x_default + x_offset, y_start + 2 * y_step, z_default);
      wait_all_reach();

      move_speed = body_move_speed;

      set_site(0, x_default - x_offset, y_start + y_step, z_default);
      set_site(1, x_default - x_offset, y_start + y_step, z_default);
      set_site(2, x_default + x_offset, y_start, z_default);
      set_site(3, x_default + x_offset, y_start + 2 * y_step, z_default);
      wait_all_reach();

      move_speed = leg_move_speed;

      set_site(3, x_default + x_offset, y_start + 2 * y_step, z_up);
      wait_all_reach();
      set_site(3, x_default + x_offset, y_start, z_up);
      wait_all_reach();
      set_site(3, x_default + x_offset, y_start, z_default);
      wait_all_reach();
    }
  }
}

/*
  - go back
  - blocking function
  - parameter step steps wanted to go
   ---------------------------------------------------------------------------*/
void step_back(unsigned int step)
{
  move_speed = leg_move_speed;
  while (step-- > 0)
  {
    if (site_now[3][1] == y_start)
    {
      //leg 3&0 move
      set_site(3, x_default + x_offset, y_start, z_up);
      wait_all_reach();
      set_site(3, x_default + x_offset, y_start + 2 * y_step, z_up);
      wait_all_reach();
      set_site(3, x_default + x_offset, y_start + 2 * y_step, z_default);
      wait_all_reach();

      move_speed = body_move_speed;

      set_site(0, x_default + x_offset, y_start + 2 * y_step, z_default);
      set_site(1, x_default + x_offset, y_start, z_default);
      set_site(2, x_default - x_offset, y_start + y_step, z_default);
      set_site(3, x_default - x_offset, y_start + y_step, z_default);
      wait_all_reach();

      move_speed = leg_move_speed;

      set_site(0, x_default + x_offset, y_start + 2 * y_step, z_up);
      wait_all_reach();
      set_site(0, x_default + x_offset, y_start, z_up);
      wait_all_reach();
      set_site(0, x_default + x_offset, y_start, z_default);
      wait_all_reach();
    }
    else
    {
      //leg 1&2 move
      set_site(1, x_default + x_offset, y_start, z_up);
      wait_all_reach();
      set_site(1, x_default + x_offset, y_start + 2 * y_step, z_up);
      wait_all_reach();
      set_site(1, x_default + x_offset, y_start + 2 * y_step, z_default);
      wait_all_reach();

      move_speed = body_move_speed;

      set_site(0, x_default - x_offset, y_start + y_step, z_default);
      set_site(1, x_default - x_offset, y_start + y_step, z_default);
      set_site(2, x_default + x_offset, y_start + 2 * y_step, z_default);
      set_site(3, x_default + x_offset, y_start, z_default);
      wait_all_reach();

      move_speed = leg_move_speed;

      set_site(2, x_default + x_offset, y_start + 2 * y_step, z_up);
      wait_all_reach();
      set_site(2, x_default + x_offset, y_start, z_up);
      wait_all_reach();
      set_site(2, x_default + x_offset, y_start, z_default);
      wait_all_reach();
    }
  }
}

// add by RegisHsu

void body_left(int i)
{
  set_site(0, site_now[0][0] + i, KEEP, KEEP);
  set_site(1, site_now[1][0] + i, KEEP, KEEP);
  set_site(2, site_now[2][0] - i, KEEP, KEEP);
  set_site(3, site_now[3][0] - i, KEEP, KEEP);
  wait_all_reach();
}

void body_right(int i)
{
  set_site(0, site_now[0][0] - i, KEEP, KEEP);
  set_site(1, site_now[1][0] - i, KEEP, KEEP);
  set_site(2, site_now[2][0] + i, KEEP, KEEP);
  set_site(3, site_now[3][0] + i, KEEP, KEEP);
  wait_all_reach();
}

void hand_wave(int i)
{
  float x_tmp;
  float y_tmp;
  float z_tmp;
  move_speed = 1;
  if (site_now[3][1] == y_start)
  {
    body_right(15);
    x_tmp = site_now[2][0];
    y_tmp = site_now[2][1];
    z_tmp = site_now[2][2];
    move_speed = body_move_speed;
    for (int j = 0; j < i; j++)
    {
      set_site(2, turn_x1, turn_y1, 50);
      wait_all_reach();
      set_site(2, turn_x0, turn_y0, 50);
      wait_all_reach();
    }
    set_site(2, x_tmp, y_tmp, z_tmp);
    wait_all_reach();
    move_speed = 1;
    body_left(15);
  }
  else
  {
    body_left(15);
    x_tmp = site_now[0][0];
    y_tmp = site_now[0][1];
    z_tmp = site_now[0][2];
    move_speed = body_move_speed;
    for (int j = 0; j < i; j++)
    {
      set_site(0, turn_x1, turn_y1, 50);
      wait_all_reach();
      set_site(0, turn_x0, turn_y0, 50);
      wait_all_reach();
    }
    set_site(0, x_tmp, y_tmp, z_tmp);
    wait_all_reach();
    move_speed = 1;
    body_right(15);
  }
}

void hand_shake(int i)
{
  float x_tmp;
  float y_tmp;
  float z_tmp;
  move_speed = 1;
  if (site_now[3][1] == y_start)
  {
    body_right(15);
    x_tmp = site_now[2][0];
    y_tmp = site_now[2][1];
    z_tmp = site_now[2][2];
    move_speed = body_move_speed;
    for (int j = 0; j < i; j++)
    {
      set_site(2, x_default - 30, y_start + 2 * y_step, 55);
      wait_all_reach();
      set_site(2, x_default - 30, y_start + 2 * y_step, 10);
      wait_all_reach();
    }
    set_site(2, x_tmp, y_tmp, z_tmp);
    wait_all_reach();
    move_speed = 1;
    body_left(15);
  }
  else
  {
    body_left(15);
    x_tmp = site_now[0][0];
    y_tmp = site_now[0][1];
    z_tmp = site_now[0][2];
    move_speed = body_move_speed;
    for (int j = 0; j < i; j++)
    {
      set_site(0, x_default - 30, y_start + 2 * y_step, 55);
      wait_all_reach();
      set_site(0, x_default - 30, y_start + 2 * y_step, 10);
      wait_all_reach();
    }
    set_site(0, x_tmp, y_tmp, z_tmp);
    wait_all_reach();
    move_speed = 1;
    body_right(15);
  }
}

void head_up(int i)
{
  set_site(0, KEEP, KEEP, site_now[0][2] - i);
  set_site(1, KEEP, KEEP, site_now[1][2] + i);
  set_site(2, KEEP, KEEP, site_now[2][2] - i);
  set_site(3, KEEP, KEEP, site_now[3][2] + i);
  wait_all_reach();
}

void head_down(int i)
{
  set_site(0, KEEP, KEEP, site_now[0][2] + i);
  set_site(1, KEEP, KEEP, site_now[1][2] - i);
  set_site(2, KEEP, KEEP, site_now[2][2] + i);
  set_site(3, KEEP, KEEP, site_now[3][2] - i);
  wait_all_reach();
}

void body_dance(int i)
{
  float x_tmp;
  float y_tmp;
  float z_tmp;
  float body_dance_speed = 2;
  sit();
  move_speed = 1;
  set_site(0, x_default, y_default, KEEP);
  set_site(1, x_default, y_default, KEEP);
  set_site(2, x_default, y_default, KEEP);
  set_site(3, x_default, y_default, KEEP);
  wait_all_reach();
  //stand();
  set_site(0, x_default, y_default, z_default - 20);
  set_site(1, x_default, y_default, z_default - 20);
  set_site(2, x_default, y_default, z_default - 20);
  set_site(3, x_default, y_default, z_default - 20);
  wait_all_reach();
  move_speed = body_dance_speed;
  head_up(30);
  for (int j = 0; j < i; j++)
  {
    if (j > i / 4)
      move_speed = body_dance_speed * 2;
    if (j > i / 2)
      move_speed = body_dance_speed * 3;
    set_site(0, KEEP, y_default - 20, KEEP);
    set_site(1, KEEP, y_default + 20, KEEP);
    set_site(2, KEEP, y_default - 20, KEEP);
    set_site(3, KEEP, y_default + 20, KEEP);
    wait_all_reach();
    set_site(0, KEEP, y_default + 20, KEEP);
    set_site(1, KEEP, y_default - 20, KEEP);
    set_site(2, KEEP, y_default + 20, KEEP);
    set_site(3, KEEP, y_default - 20, KEEP);
    wait_all_reach();
  }
  move_speed = body_dance_speed;
  head_down(30);
}


/*
  - microservos service /timer interrupt function/50Hz
  - when set site expected,this function move the end point to it in a straight line
  - temp_speed[4][3] should be set before set expect site,it make sure the end point
   move in a straight line,and decide move speed.
   ---------------------------------------------------------------------------*/
void servo_service(void)
{
  static float alpha, beta, gamma;

  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      if (abs(site_now[i][j] - site_expect[i][j]) >= abs(temp_speed[i][j]))
        site_now[i][j] += temp_speed[i][j];
      else
        site_now[i][j] = site_expect[i][j];
    }

    cartesian_to_polar(alpha, beta, gamma, site_now[i][0], site_now[i][1], site_now[i][2]);
    polar_to_servo(i, alpha, beta, gamma);
  }

  rest_counter++;
}

/*
  - set one of end points' expect site
  - this founction will set temp_speed[4][3] at same time
  - non - blocking function
   ---------------------------------------------------------------------------*/
void set_site(int leg, float x, float y, float z)
{
  float length_x = 0, length_y = 0, length_z = 0;

  if (x != KEEP)
    length_x = x - site_now[leg][0];
  if (y != KEEP)
    length_y = y - site_now[leg][1];
  if (z != KEEP)
    length_z = z - site_now[leg][2];

  float length = sqrt(pow(length_x, 2) + pow(length_y, 2) + pow(length_z, 2));

  temp_speed[leg][0] = length_x / length * move_speed * speed_multiple;
  temp_speed[leg][1] = length_y / length * move_speed * speed_multiple;
  temp_speed[leg][2] = length_z / length * move_speed * speed_multiple;

  if (x != KEEP)
    site_expect[leg][0] = x;
  if (y != KEEP)
    site_expect[leg][1] = y;
  if (z != KEEP)
    site_expect[leg][2] = z;
}

/*
  - wait one of end points move to expect site
  - blocking function
   ---------------------------------------------------------------------------*/
void wait_reach(int leg)
{
  while (1)
  {
    if (site_now[leg][0] == site_expect[leg][0])
      if (site_now[leg][1] == site_expect[leg][1])
        if (site_now[leg][2] == site_expect[leg][2])
          break;

    vTaskDelay(1);
  }
}

/*
  - wait all of end points move to expect site
  - blocking function
   ---------------------------------------------------------------------------*/
void wait_all_reach(void)
{
  for (int i = 0; i < 4; i++)
    wait_reach(i);
}

/*
  - trans site from cartesian to polar
  - mathematical model 2/2
   ---------------------------------------------------------------------------*/
void cartesian_to_polar(volatile float &alpha, volatile float &beta, volatile float &gamma, volatile float x, volatile float y, volatile float z)
{
  //calculate w-z degree
  float v, w;
  w = (x >= 0 ? 1 : -1) * (sqrt(pow(x, 2) + pow(y, 2)));
  v = w - length_c;
  alpha = atan2(z, v) + acos((pow(length_a, 2) - pow(length_b, 2) + pow(v, 2) + pow(z, 2)) / 2 / length_a / sqrt(pow(v, 2) + pow(z, 2)));
  beta = acos((pow(length_a, 2) + pow(length_b, 2) - pow(v, 2) - pow(z, 2)) / 2 / length_a / length_b);
  //calculate x-y-z degree
  gamma = (w >= 0) ? atan2(y, x) : atan2(-y, -x);
  //trans degree pi->180
  alpha = alpha / pi * 180;
  beta = beta / pi * 180;
  gamma = gamma / pi * 180;
}

///*
//  - trans site from polar to microservos
//  - mathematical model map to fact
//  - the errors saved in eeprom will be add
//   ---------------------------------------------------------------------------*/
//void polar_to_servo(int leg, float alpha, float beta, float gamma)
//{
//  if (leg == 0)
//  {
//    alpha = 90 - alpha;
//    beta = beta;
//    gamma += 90;
//  }
//  else if (leg == 1)
//  {
//    alpha += 90;
//    beta = 180 - beta;
//    gamma = 90 - gamma;
//  }
//  else if (leg == 2)
//  {
//    alpha += 90;
//    beta = 180 - beta;
//    gamma = 90 - gamma;
//  }
//  else if (leg == 3)
//  {
//    alpha = 90 - alpha;
//    beta = beta;
//    gamma += 90;
//  }
//
//  servo[leg][0].write(alpha+offset[leg][0]);
//  servo[leg][1].write(beta+offset[leg][1]);
//  servo[leg][2].write(gamma+offset[leg][2]);
//}

/*
  - trans site from polar to microservos
  - mathematical model map to fact
  - the errors saved in eeprom will be add
   ---------------------------------------------------------------------------*/
void polar_to_servo(int leg, float alpha, float beta, float gamma)
{
  if (leg == 0) //Front Right
  {
    alpha = 90 - alpha - FRElbow; //elbow (- is up)
    beta = beta - FRFoot; //foot (- is up)
    gamma = 90 - gamma - FRShdr;    // shoulder (- is left)
  }
  else if (leg == 1) //Rear Right
  {
    alpha += 90 + RRElbow; //elbow (+ is up)
    beta = 180 - beta + RRFoot; //foot (+ is up)
    gamma = 90 + gamma + RRShdr; // shoulder (+ is left)
  }
  else if (leg == 2) //Front Left
  {
    alpha += 90 + FLElbow; //elbow (+ is up)
    beta = 180 - beta + FLFoot; //foot (+ is up)
    gamma = 90 + gamma + FLShdr;// shoulder (+ is left)
  }
  else if (leg == 3) // Rear Left
  {
    alpha = 90 - alpha - RLElbow; //elbow (- is up)
    beta = beta - RLFoot; //foot; (- is up)
    gamma = 90 - gamma - RLShdr;// shoulder (- is left)
  }
  //  int AL = ((850/180)*alpha);if (AL > 580) AL=580;if (AL < 25) AL=25;pwm.setPWM(servo_pin[leg][0],0,AL);
  //  int BE = ((850/180)*beta);if (BE > 580) BE=580;if (BE < 25) BE=25;pwm.setPWM(servo_pin[leg][1],0,BE);
  //  int GA = ((580/180)*gamma);if (GA > 580) GA=580;if (GA < 25) GA=25;pwm.setPWM(servo_pin[leg][2],0,GA);

  servo[leg][0].write(alpha + offset[leg][0]);
  servo[leg][1].write(beta + offset[leg][1]);
  servo[leg][2].write(gamma + offset[leg][2]);
}

//读取IMU数据
int IMU::init(uint8_t pin)
{
  uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
  uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
  uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)

  // initialize i2c
  Wire.begin();
  Wire.setClock(400000);

  // initialize device
  Serial.println("Initializing I2C devices...");
  imu.initialize();

  // verify connection
  Serial.println("Testing device connections...");
  if (imu.testConnection()) {
    Serial.println("MPU6050 connection successful");
  } else {
    Serial.println("MPU6050 connection failed");
    return -1;
  }

  pinMode(pin, INPUT);

  // load and configure the DMP
  devStatus = imu.dmpInitialize();

  // supply your own gyro offsets here, scaled for min sensitivity
  imu.setXGyroOffset(220);
  imu.setYGyroOffset(76);
  imu.setZGyroOffset(-85);
  imu.setZAccelOffset(1788); // 1688 factory default for my test chip

  // make sure it worked (returns 0 if so)
  if (devStatus == 0) {
    // Calibration Time: generate offsets and calibrate our MPU6050
    imu.CalibrateAccel(6);
    imu.CalibrateGyro(6);
    imu.PrintActiveOffsets();
    // turn on the DMP, now that it's ready
    Serial.println(F("Enabling DMP..."));
    imu.setDMPEnabled(true);

    // enable Arduino interrupt detection
    Serial.print(F("Enabling interrupt detection (Arduino external interrupt "));
    Serial.print(digitalPinToInterrupt(INTERRUPT_PIN));
    Serial.println(F(")..."));
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
    mpuIntStatus = imu.getIntStatus();

    // set our DMP Ready flag so the main loop() function knows it's okay to use it
    Serial.println(F("DMP ready! Waiting for first interrupt..."));
    dmpReady = true;

    // get expected DMP packet size for later comparison
    packetSize = imu.dmpGetFIFOPacketSize();
  } else {
    // ERROR!
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
    // (if it's going to break, usually the code will be 1)
    Serial.print(F("DMP Initialization failed (code "));
    Serial.print(devStatus);
    Serial.println(F(")"));
  }
}

void IMU::update()
{
  // orientation/motion vars
  Quaternion q;           // [w, x, y, z]         quaternion container
  VectorInt16 aa;         // [x, y, z]            accel sensor measurements
  VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
  VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
  VectorFloat gravity;    // [x, y, z]            gravity vector

  // MPU control/status vars
  uint8_t fifoBuffer[64]; // FIFO storage buffer

  // if programming failed, don't try to do anything
  if (!dmpReady) return;

  // read a packet from FIFO
  if (imu.dmpGetCurrentFIFOPacket(fifoBuffer)) { // Get the Latest packet
    // display Euler angles in degrees
    imu.dmpGetQuaternion(&q, fifoBuffer);
    imu.dmpGetGravity(&gravity, &q);
    imu.dmpGetYawPitchRoll(ypr, &q, &gravity);
  }

  // read temperature
  temperature = imu.getTemperature();
}

float IMU::getYaw()
{
  return ypr[0] * 180 / M_PI;
}

float IMU::getPitch()
{
  return ypr[1] * 180 / M_PI;
}

float IMU::getRoll()
{
  return ypr[2] * 180 / M_PI;
}

int16_t IMU::getTemperature()
{
  return temperature;
}