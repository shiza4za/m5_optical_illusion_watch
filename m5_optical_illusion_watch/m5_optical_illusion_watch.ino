// m5_optical_illusion_watch.ino
// version 0.2.0 | CC BY-NC-SA 4.0 | https://github.com/shiza4za/m5_optical_illusion_watch/blob/main/LICENSE.txt

#include <iostream>
#include <bitset>
#include <vector>
#include <esp_sntp.h>
#include <WiFi.h>
#include <M5Unified.h>





//////////////////////////// カスタマイズ ////////////////////////////

// ★NTP同期時に接続するWi-FiのSSID
constexpr const char* ssid    = "your SSID";
// ★NTP同期時に接続するWi-Fiのpw
constexpr const char* ssid_pw = "your SSID password";


// ★LCD輝度(0〜255で設定可)
const int brightness      = 85;      // デフォルト 85

// ★デフォルトで答え合わせを表示する/しない設定
bool      BtnB_dec_ck     = false;    // デフォルト false

// ★背景色
const int color_lcd       = BLACK;    // デフォルト BLACK
// ★文字色(バッテリ以外)
const int color_text      = WHITE;    // デフォルト WHITE

// ★時刻表示 斜線の色
const int const_global_color_1 = WHITE; // デフォルト WHITE
// ★時刻表示 背景色
const int const_global_color_2 = BLACK; // デフォルト BLACK

// ★バッテリ表示の各文字色
const int color_bt_good   = 0x0723;   // 100-60 デフォルト緑 0x0723
const int color_bt_hmm    = 0xfd66;   //  59-20 デフォルト黄 0xfd66
const int color_bt_danger = 0xfa86;   //  19- 0 デフォルト赤 0xf982

// ★BtnBでLCD輝度MAXにしたときの"BRT"の色
const int color_brt       = 0xfd66;   // デフォルト黄 0xfd66

// ★自動終了するまでの時間(秒)
// 　※ミリ秒ではなく秒です
// 　※多分58秒未満を推奨
// 　※0にすると、勝手にオフしなくなります
#define BUTTON_TIMEOUT 30   // デフォルト 30

// ★BtnC押下時・または一定秒間操作がなかったときに、
// 　powerOffまたはdeepSleepする設定
// 　・trueでpowerOff関数実行。次回電源ボタンを押すと再起動
// 　・falseでdeepSleep関数実行。次回画面など触れると再起動
bool poweroffmode = true;   // デフォルト true

// ★稼働時のディレイ(ms)
const int delay_in_loop = 20;   // デフォルト 20

/////////////////////////////////////////////////////////////////////





// グローバル変数として宣言
auto start_time = time(nullptr);
auto start_time_local = localtime(&start_time);
int start_time_local_sec = start_time_local->tm_sec;
const int defy = 20;
const char* const week_str[7] = {"Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat"};
bool BtnB_lcd_ck = false;

int h_1lv  = 0;
int h_10lv = 0;
int m_1lv  = 0;
int m_10lv = 0;





// バイナリ点滅以外の基本表示一式
void firstScreen() {
  M5.Lcd.clear(color_lcd);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(color_text, color_lcd);

  // NTP・Shutdown文字
  M5.Display.setTextSize(2);
  M5.Lcd.setTextColor(color_text, color_lcd);
  M5.Display.setCursor(55, 16*14);
  M5.Display.printf("NTP");
  M5.Lcd.setTextColor(color_text, color_lcd);
  M5.Display.setCursor(223, 16*14);
  M5.Display.printf("Shutdown");
}





// BtnAで呼出：RTC確認 → Wi-Fi接続 → NTPサーバ接続 → 時刻同期
void connect() {
  M5.Lcd.clear(color_lcd);
  M5.Lcd.setCursor(0, 0);

  // バージョン
  M5.Lcd.setTextColor(0xad55, color_lcd);
  M5.Display.printf("\nversion 0.1.0\n\n");

  // SSIDの参考表示
  M5.Lcd.setTextColor(color_text, color_lcd);
  M5.Display.printf("SSID: ");
  M5.Display.printf("%s\n\n", ssid);
  vTaskDelay(1000);

  // RTC状態表示
  M5.Display.printf("RTC...");
  if (!M5.Rtc.isEnabled()) {
    M5.Lcd.setTextColor(0xfa86, color_lcd);
    M5.Display.printf("ERR. \nPlease power off \nand try again with the \nRTC available.\n\n");
    for (;;) { vTaskDelay(10000); }
  }
  M5.Lcd.setTextColor(0x0723, color_lcd);
  M5.Display.printf("OK.\n\n");

  // Wi-Fi接続
  WiFi.disconnect();
  vTaskDelay(1000);
  M5.Lcd.setTextColor(color_text, color_lcd);
  M5.Display.printf("Wi-Fi...");
  WiFi.begin(ssid, ssid_pw);
  while (WiFi.status() != WL_CONNECTED) { M5.Display.printf("."); vTaskDelay(500); }
  M5.Lcd.setTextColor(0x0723, color_lcd);
  M5.Display.printf("OK.\n\n");
  vTaskDelay(1000);

  // NTP時刻取得
  M5.Lcd.setTextColor(color_text, color_lcd);
  M5.Display.printf("NTP...");
  configTzTime("JST-9", "ntp.nict.jp", "ntp.nict.jp", "ntp.nict.jp");
  while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
    M5.Display.printf("."); vTaskDelay(500);
  }
  M5.Lcd.setTextColor(0x0723, color_lcd);
  M5.Display.printf("OK.\n\n");
  vTaskDelay(1000);
  time_t get_time = time(nullptr);
  tm* local_time = localtime(&get_time);
  local_time->tm_hour += 9;
  time_t jst = mktime(local_time);
  tm* jstTime = gmtime(&jst);
  M5.Rtc.setDateTime(gmtime(&jst));
  vTaskDelay(500);
  WiFi.disconnect();
  vTaskDelay(500);

  firstScreen();
}





// 起動オフの処理選択
void poweroffTask() {
  if        (poweroffmode == true) {
    M5.Power.powerOff();
  } else if (poweroffmode == false) {
    M5.Power.deepSleep(0);
  }
}





// LCD輝度状態によってBRTの文字と色変更
void displayBrt(bool BtnB_lcd_ck) {
  M5.Lcd.setCursor(145, 16*14);
  if (BtnB_lcd_ck == true) {
   M5.Lcd.setTextColor(color_brt, color_lcd);
  } else if (BtnB_lcd_ck == false) {
   M5.Lcd.setTextColor(color_text, color_lcd);
  }
  M5.Display.printf("BRT");
  M5.Lcd.setTextColor(color_text, color_lcd);

  M5.Lcd.setCursor(0, 0);
}





// バッテリ残量表示
void displayBattery() {
  // 100-50 緑
  //  49-20 黄
  //  19- 0 赤
  int battery_level = M5.Power.getBatteryLevel();
  M5.Lcd.setCursor(0, 16*14);
  if      (battery_level <= 100 && battery_level > 50) { M5.Lcd.setTextColor(color_bt_good, color_lcd); }
  else if (battery_level <=  50 && battery_level > 20) { M5.Lcd.setTextColor(color_bt_hmm, color_lcd); }
  else if (battery_level <=  20                      ) { M5.Lcd.setTextColor(color_bt_danger, color_lcd); }
  M5.Display.printf("%03d", battery_level);
  M5.Lcd.setTextColor(color_text, color_lcd);
}





// 背景表示
void backPattern(bool btnb) {
  for (int s = 0; s <= 18; s++) {
    for (int t = 0; t <= 12; t++) {

      if        (t == 0 || t == 12) {
        fillTri(s*17, t*17, const_global_color_1, const_global_color_2, btnb);

      } else if (t == 1 || t == 11) {
        if (s == 3 || s == 9 || s == 15) {
          fillTri(s*17, t*17, const_global_color_1, const_global_color_2, btnb);
        }

      } else if (t == 2 || t == 10) {
        if ((s >= 0 && s <= 1) || s == 3 || s == 5 || s == 7 || s == 9 || s == 11 || s == 13 || s == 15 || (s >= 17 && s <= 18)) {
          fillTri(s*17, t*17, const_global_color_1, const_global_color_2, btnb);
        }

      } else if (t == 3 || t == 9) {
        if (s == 1 || s == 3 || s == 9 || s == 15 || s == 17) {
          fillTri(s*17, t*17, const_global_color_1, const_global_color_2, btnb);
        }

      } else if (t == 4 || t == 8) {
        if (s == 1 || s == 3 || s == 5 || s == 7 || s == 9 || s == 11 || s == 13 || s == 15 || s == 17) {
          fillTri(s*17, t*17, const_global_color_1, const_global_color_2, btnb);
        }

      } else if (t == 5 || t == 7) {
        if (s == 1 || s == 3 || s == 9 || s == 15 || s == 17) {
          fillTri(s*17, t*17, const_global_color_1, const_global_color_2, btnb);
        }

      } else if (t == 6) {
        if (s == 1 || (s >= 3 && s <= 15) || s == 17) {
          fillTri(s*17, t*17, const_global_color_1, const_global_color_2, btnb);
        }
      }

      if        (s == 0 || s == 18) {
        if (t == 1 || (t >= 3 && t <= 9 ) || t == 11) {
          fillTriTurn_forBack(s*17, t*17, const_global_color_1, const_global_color_2, btnb);
        }

      } else if (s == 1 || s == 17) {
        if (t == 1 || t == 11) {
          fillTriTurn_forBack(s*17, t*17, const_global_color_1, const_global_color_2, btnb);
        }

      } else if (s == 2 || s == 16) {
        if (t >= 1 && t <= 11) {
          fillTriTurn_forBack(s*17, t*17, const_global_color_1, const_global_color_2, btnb);
        }
      }
    }
  }
}


void fillTri(int x, int y, int gcolor_1, int gcolor_2, bool btnb) {
  if (btnb == true) {
    M5.Lcd.fillRect(x, y, 17, 17, gcolor_2);
  } else if (btnb == false) {
    M5.Lcd.fillTriangle( 6+x,  0+y,  11+x,  0+y,  11+x,  5+y, gcolor_1);
    M5.Lcd.fillTriangle(11+x,  5+y,  16+x,  5+y,  16+x, 10+y, gcolor_1);
    M5.Lcd.fillTriangle(12+x,  0+y,  12+x,  4+y,  16+x,  4+y, gcolor_1);
    M5.Lcd.fillTriangle( 0+x,  5+y,   0+x, 11+y,   6+x, 11+y, gcolor_1);
    M5.Lcd.fillTriangle( 5+x, 10+y,   5+x, 16+y,  11+x, 16+y, gcolor_1);
    M5.Lcd.fillTriangle( 1+x, 12+y,   4+x, 12+y,   4+x, 15+y, gcolor_1);

    M5.Lcd.fillTriangle(13+x,  0+y,  16+x,  0+y,  16+x,  3+y, gcolor_2);
    M5.Lcd.fillTriangle( 0+x, 12+y,   0+x, 16+y,   4+x, 16+y, gcolor_2);
    M5.Lcd.fillTriangle( 5+x,  0+y,   1+x,  4+y,  15+x, 10+y, gcolor_2);
    M5.Lcd.fillTriangle( 1+x,  5+y,  11+x, 15+y,  15+x, 11+y, gcolor_2);
    M5.Lcd.fillRect    ( 0+x,  0+y,   6  ,  5  ,              gcolor_2);
    M5.Lcd.fillRect    (12+x, 11+y,   5  ,  6  ,              gcolor_2);
  }
}

void fillTriTurn(int x, int y, int gcolor_1, int gcolor_2) {
  // M5.Lcd.fillRect(x, y, x, y, BLACK);
  M5.Lcd.fillTriangle( 5+x,  0+y,  11+x,  0+y,   5+x,  6+y, gcolor_1);
  M5.Lcd.fillTriangle( 0+x,  5+y,   6+x,  5+y,   0+x, 11+y, gcolor_1);
  M5.Lcd.fillTriangle( 4+x,  1+y,   1+x,  4+y,   4+x,  4+y, gcolor_1);
  M5.Lcd.fillTriangle(16+x,  6+y,  16+x, 11+y,  11+x, 11+y, gcolor_1);
  M5.Lcd.fillTriangle(11+x, 11+y,  11+x, 16+y,   6+x, 16+y, gcolor_1);
  M5.Lcd.fillTriangle(12+x, 12+y,  16+x, 12+y,  12+x, 16+y, gcolor_1);

  M5.Lcd.fillTriangle( 0+x,  0+y,   4+x,  0+y,   0+x,  4+y, gcolor_2);
  M5.Lcd.fillTriangle(16+x, 13+y,  13+x, 16+y,  16+x, 16+y, gcolor_2);
  M5.Lcd.fillTriangle(11+x,  1+y,   1+x, 11+y,   5+x, 15+y, gcolor_2);
  M5.Lcd.fillTriangle(12+x,  1+y,  16+x,  5+y,   6+x, 15+y, gcolor_2);
  M5.Lcd.fillRect    (12+x,  0+y,   5  ,  6  ,              gcolor_2);
  M5.Lcd.fillRect    ( 0+x, 12+y,   6  ,  5  ,              gcolor_2);
}

void fillTriTurn_forBack(int x, int y, int gcolor_1, int gcolor_2, bool btnb) {
  // M5.Lcd.fillRect(x, y, x, y, BLACK);
  if (btnb == true) {
    M5.Lcd.fillRect(x, y, 17, 17, gcolor_2);
  } else if (btnb == false) {
    M5.Lcd.fillTriangle( 5+x,  0+y,  11+x,  0+y,   5+x,  6+y, gcolor_1);
    M5.Lcd.fillTriangle( 0+x,  5+y,   6+x,  5+y,   0+x, 11+y, gcolor_1);
    M5.Lcd.fillTriangle( 4+x,  1+y,   1+x,  4+y,   4+x,  4+y, gcolor_1);
    M5.Lcd.fillTriangle(16+x,  6+y,  16+x, 11+y,  11+x, 11+y, gcolor_1);
    M5.Lcd.fillTriangle(11+x, 11+y,  11+x, 16+y,   6+x, 16+y, gcolor_1);
    M5.Lcd.fillTriangle(12+x, 12+y,  16+x, 12+y,  12+x, 16+y, gcolor_1);

    M5.Lcd.fillTriangle( 0+x,  0+y,   4+x,  0+y,   0+x,  4+y, gcolor_2);
    M5.Lcd.fillTriangle(16+x, 13+y,  13+x, 16+y,  16+x, 16+y, gcolor_2);
    M5.Lcd.fillTriangle(11+x,  1+y,   1+x, 11+y,   5+x, 15+y, gcolor_2);
    M5.Lcd.fillTriangle(12+x,  1+y,  16+x,  5+y,   6+x, 15+y, gcolor_2);
    M5.Lcd.fillRect    (12+x,  0+y,   5  ,  6  ,              gcolor_2);
    M5.Lcd.fillRect    ( 0+x, 12+y,   6  ,  5  ,              gcolor_2);
  }
}

void zero( int x, int y, int gcolor_1, int gcolor_2, bool btnb) {
  fillTriTurn(17*(1+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(2+y), gcolor_1, gcolor_2);
  fillTri    (17*(3+x), 17*(2+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(2+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTri    (17*(2+x), 17*(3+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(3+x), 17*(3+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(4+x), 17*(3+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(4+y), gcolor_1, gcolor_2);
  fillTri    (17*(3+x), 17*(4+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(4+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(5+y), gcolor_1, gcolor_2);}
void one(  int x, int y, int gcolor_1, int gcolor_2, bool btnb) {
  fillTriTurn(17*(1+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTri    (17*(4+x), 17*(1+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(5+x), 17*(1+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(1+x), 17*(2+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(3+x), 17*(2+y), gcolor_1, gcolor_2);
  fillTri    (17*(5+x), 17*(2+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(1+x), 17*(3+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(2+x), 17*(3+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(3+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTri    (17*(4+x), 17*(3+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(5+x), 17*(3+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(1+x), 17*(4+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(3+x), 17*(4+y), gcolor_1, gcolor_2);
  fillTri    (17*(5+x), 17*(4+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(1+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(5+y), gcolor_1, gcolor_2);}
void two(  int x, int y, int gcolor_1, int gcolor_2, bool btnb) {
  fillTriTurn(17*(1+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTri    (17*(1+x), 17*(2+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(3+x), 17*(2+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(2+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(4+y), gcolor_1, gcolor_2);
  fillTri    (17*(3+x), 17*(4+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(5+x), 17*(4+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(1+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(5+y), gcolor_1, gcolor_2);}
void three(int x, int y, int gcolor_1, int gcolor_2, bool btnb) {
  fillTriTurn(17*(1+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTri    (17*(1+x), 17*(2+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(3+x), 17*(2+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(2+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTri    (17*(1+x), 17*(4+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(3+x), 17*(4+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(4+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(5+y), gcolor_1, gcolor_2);}
void four( int x, int y, int gcolor_1, int gcolor_2, bool btnb) {
  fillTriTurn(17*(1+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTri    (17*(2+x), 17*(1+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(3+x), 17*(1+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(4+x), 17*(1+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(2+y), gcolor_1, gcolor_2);
  fillTri    (17*(3+x), 17*(2+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(2+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTri    (17*(1+x), 17*(4+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(3+x), 17*(4+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(4+y), gcolor_1, gcolor_2);
  fillTri    (17*(1+x), 17*(5+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(2+x), 17*(5+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(3+x), 17*(5+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(4+x), 17*(5+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(5+y), gcolor_1, gcolor_2);}
void five( int x, int y, int gcolor_1, int gcolor_2, bool btnb) {
  fillTriTurn(17*(1+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(2+y), gcolor_1, gcolor_2);
  fillTri    (17*(3+x), 17*(2+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(5+x), 17*(2+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(1+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTri    (17*(1+x), 17*(4+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(3+x), 17*(4+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(4+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(5+y), gcolor_1, gcolor_2);}
void six(  int x, int y, int gcolor_1, int gcolor_2, bool btnb) {
  fillTriTurn(17*(1+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(2+y), gcolor_1, gcolor_2);
  fillTri    (17*(3+x), 17*(2+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(5+x), 17*(2+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(1+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(4+y), gcolor_1, gcolor_2);
  fillTri    (17*(3+x), 17*(4+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(4+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(5+y), gcolor_1, gcolor_2);}
void seven(int x, int y, int gcolor_1, int gcolor_2, bool btnb) {
  fillTriTurn(17*(1+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(2+y), gcolor_1, gcolor_2);
  fillTri    (17*(3+x), 17*(2+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(2+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTri    (17*(2+x), 17*(3+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(3+x), 17*(3+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(4+x), 17*(3+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTri    (17*(1+x), 17*(4+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(3+x), 17*(4+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(4+y), gcolor_1, gcolor_2);
  fillTri    (17*(1+x), 17*(5+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(2+x), 17*(5+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(3+x), 17*(5+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(4+x), 17*(5+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(5+y), gcolor_1, gcolor_2);}
void eight(int x, int y, int gcolor_1, int gcolor_2, bool btnb) {
  fillTriTurn(17*(1+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(2+y), gcolor_1, gcolor_2);
  fillTri    (17*(3+x), 17*(2+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(2+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(4+y), gcolor_1, gcolor_2);
  fillTri    (17*(3+x), 17*(4+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(4+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(5+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(5+y), gcolor_1, gcolor_2);}
void nine( int x, int y, int gcolor_1, int gcolor_2, bool btnb) {
  fillTriTurn(17*(1+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(1+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(2+y), gcolor_1, gcolor_2);
  fillTri    (17*(3+x), 17*(2+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(2+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(1+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(2+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(3+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(4+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTriTurn(17*(5+x), 17*(3+y), gcolor_1, gcolor_2);
  fillTri    (17*(1+x), 17*(4+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(3+x), 17*(4+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(4+y), gcolor_1, gcolor_2);
  fillTri    (17*(1+x), 17*(5+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(2+x), 17*(5+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(3+x), 17*(5+y), gcolor_1, gcolor_2, btnb);
  fillTri    (17*(4+x), 17*(5+y), gcolor_1, gcolor_2, btnb);
  fillTriTurn(17*(5+x), 17*(5+y), gcolor_1, gcolor_2);}

void number(int x, int y, int gcolor_1, int gcolor_2, int num, bool btnb) {
  if      (num == 0) { zero( x, y, gcolor_1, gcolor_2, btnb); }
  else if (num == 1) { one(  x, y, gcolor_1, gcolor_2, btnb); }
  else if (num == 2) { two(  x, y, gcolor_1, gcolor_2, btnb); }
  else if (num == 3) { three(x, y, gcolor_1, gcolor_2, btnb); }
  else if (num == 4) { four( x, y, gcolor_1, gcolor_2, btnb); }
  else if (num == 5) { five( x, y, gcolor_1, gcolor_2, btnb); }
  else if (num == 6) { six(  x, y, gcolor_1, gcolor_2, btnb); }
  else if (num == 7) { seven(x, y, gcolor_1, gcolor_2, btnb); }
  else if (num == 8) { eight(x, y, gcolor_1, gcolor_2, btnb); }
  else if (num == 9) { nine( x, y, gcolor_1, gcolor_2, btnb); }
}

// ////////////////////////////////////////////////////////////////////////////////



void setup() {
  auto cfg = M5.config();

  // ★外部のRTCを読み取る場合は、コメント外します
  // 　Core2はRTC内蔵しているので不要
  // cfg.external_rtc  = true;

  M5.begin(cfg);
  M5.Displays(0).setTextSize(2);
  M5.Lcd.setBrightness(brightness);

  firstScreen();

  // 操作なし時間確認用 基準秒
  start_time = time(nullptr);
  start_time_local = localtime(&start_time);
  start_time_local_sec = start_time_local->tm_sec;


  backPattern(BtnB_dec_ck);


}



// ////////////////////////////////////////////////////////////////////////////////



void loop() {
  vTaskDelay(delay_in_loop);
  M5.update();

  displayBrt(BtnB_lcd_ck);


  auto now_time = time(nullptr);
  auto now_time_local = localtime(&now_time);


  // 操作なし時間確認、経過したら起動オフ
  int now_time_local_sec = now_time_local->tm_sec;

  if (start_time_local_sec > now_time_local_sec) {
    now_time_local_sec += 60;
  }
  if (BUTTON_TIMEOUT != 0) {
    if (now_time_local_sec - start_time_local_sec > BUTTON_TIMEOUT) {
      poweroffTask();
    }
  }




  // ボタン操作
    auto get_detail = M5.Touch.getDetail(1);
    // m5::touch_state_t get_state;
    // get_state = get_detail.state;

    // BtnA ちょっと長押しでWi-Fi・NTPサーバ接続開始
    if (M5.BtnA.wasHold()) {
      connect();
      start_time_local_sec = now_time_local_sec;
    }

    // タッチで答え合わせ表示/非表示
    if (get_detail.wasClicked()) {
      if (BtnB_dec_ck == false) {
        BtnB_dec_ck = true;
        start_time_local_sec = now_time_local_sec;
      } else if (BtnB_dec_ck == true) {
        BtnB_dec_ck = false;
        start_time_local_sec = now_time_local_sec;
        // BtnBで呼び出した答え合わせ表示を塗り潰して無理やり非表示
        // M5.Display.fillRect(0, 0, 60, 222, color_lcd);
      }
    }

    // BtnB ちょっと長押しでLCD輝度MAX(brightness値<->MAX)
    if (M5.BtnB.wasHold()) {
      if (BtnB_lcd_ck == false) {
        BtnB_lcd_ck = true;
        M5.Lcd.setBrightness(255);
        start_time_local_sec = now_time_local_sec;
      } else if (BtnB_lcd_ck == true) {
        BtnB_lcd_ck = false;
        M5.Lcd.setBrightness(brightness);
        start_time_local_sec = now_time_local_sec;
      }
    }

    // BtnC ちょっと長押しで起動オフ
    if (M5.BtnC.wasHold()) {
      poweroffTask();
    }
  //


  backPattern(BtnB_dec_ck);



  // zero(0, 0);
  // vTaskDelay(1000);
  // one(0, 0);
  // vTaskDelay(1000);
  // two(0, 0);
  // vTaskDelay(1000);
  // three(0, 0);
  // vTaskDelay(1000);
  // four(0, 0);
  // vTaskDelay(1000);
  // five(0, 0);
  // vTaskDelay(1000);
  // six(0, 0);
  // vTaskDelay(1000);
  // seven(0, 0);
  // vTaskDelay(1000);
  // eight(0, 0);
  // vTaskDelay(1000);
  // nine(0, 0);
  // vTaskDelay(1000);









  int year = now_time_local->tm_year+1900;
  int month = now_time_local->tm_mon+1;
  int day = now_time_local->tm_mday;
  int week = now_time_local->tm_wday;
  int hour = now_time_local->tm_hour;
  int min = now_time_local->tm_min;
  int sec = now_time_local->tm_sec;




  if        (hour <= 9) {
    h_1lv  = hour;
    h_10lv = 0;
  } else if (hour >= 10 && hour <= 19) {
    h_1lv  = hour-10;
    h_10lv = 1;
  } else if (hour >= 20 && hour <= 23) {
    h_1lv  = hour-20;
    h_10lv = 2;
  }
  // h_10lv = 2;
  // h_1lv = 1;
  number(6+3, 0, const_global_color_1, const_global_color_2, h_1lv, BtnB_dec_ck);
  number(0+3, 0, const_global_color_1, const_global_color_2, h_10lv, BtnB_dec_ck);

  if        (min <= 9) {
    m_1lv  = min;
    m_10lv = 0;
  } else if (min >= 10 && min <= 19) {
    m_1lv  = min-10;
    m_10lv = 1;
  } else if (min >= 20 && min <= 29) {
    m_1lv  = min-20;
    m_10lv = 2;
  } else if (min >= 30 && min <= 39) {
    m_1lv  = min-30;
    m_10lv = 3;
  } else if (min >= 40 && min <= 49) {
    m_1lv  = min-40;
    m_10lv = 4;
  } else if (min >= 50 && min <= 59) {
    m_1lv  = min-50;
    m_10lv = 5;
  }
  // m_10lv = 2;
  // m_1lv = 0;
  number(6+3, 6, const_global_color_1, const_global_color_2, m_1lv, BtnB_dec_ck);
  number(0+3, 6, const_global_color_1, const_global_color_2, m_10lv, BtnB_dec_ck);







  M5.Display.setTextSize(2);
  //


  displayBattery();


}
