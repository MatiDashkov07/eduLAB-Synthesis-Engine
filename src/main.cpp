#include <Arduino.h>
#include "driver/i2s.h"

// --- 1. הגדרות חומרה (Configuration) ---
// אלו הפינים שבחרת - אנחנו מגדירים אותם כקבועים
#define I2S_BCK_PIN 38
#define I2S_DIN_PIN 39
#define I2S_LRCK_PIN 40

// הגדרות אודיו
#define SAMPLE_RATE 44100 
#define I2S_NUM I2S_NUM_0 // יש ל-ESP32 שני ערוצי I2S, נשתמש בראשון

// משתנים ליצירת הגל
float phase = 0;
float frequency = 440.0; // תו לה (A4)
const float amplitude = 500.0; // עוצמה (עד 32767 ב-16 ביט)

void setup() {
  Serial.begin(115200);
  Serial.println("eduLAB v4.0 POC: Initializing I2S...");

  // --- 2. מבנה ההגדרות (The Config Struct) ---
  // אנחנו אומרים לדרייבר איך להתנהג
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX), // אנחנו המאסטר, ואנחנו משדרים (TX)
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // איכות CD
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // סטריאו
    .communication_format = I2S_COMM_FORMAT_STAND_I2S, // פרוטוקול I2S סטנדרטי (Philips)
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // רמת פסיקות (Interrupts)
    .dma_buf_count = 8, // כמות הבאפרים (DMA)
    .dma_buf_len = 64, // אורך כל באפר
    .use_apll = false, // שימוש בשעון פנימי מדויק (לא קריטי כרגע)
    .tx_desc_auto_clear = true // לנקות באפרים אם יש שקט
  };

  // --- 3. מבנה הפינים (Pin Configuration) ---
  // כאן אנחנו מחברים בין הפינים שבחרת לדרייבר
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCK_PIN,
    .ws_io_num = I2S_LRCK_PIN,   // WS = Word Select = LRCK
    .data_out_num = I2S_DIN_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE // אנחנו לא מקליטים, אין צורך בפין כניסה
  };

  // --- 4. התקנה (Install & Start) ---
  // שלב א': התקנת הדרייבר עם ההגדרות הלוגיות
  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  
  // שלב ב': שיוך הפינים הפיזיים
  i2s_set_pin(I2S_NUM, &pin_config);
  
  // שלב ג': תיקון שעון (לפעמים נדרש ב-ESP32 כדי לדייק את התדר)
  i2s_set_clk(I2S_NUM, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);

  Serial.println("I2S Initialized. Generating Sine Wave...");
}

void loop() {
  // חישוב גל סינוס מתמטי טהור
  // אנחנו ממלאים את הבאפר בזוגות (שמאל וימין)
  
  int16_t sample = (int16_t)(amplitude * sin(phase));
  
  // קידום הפאזה
  // הנוסחה: 2 * PI * Freq / SampleRate
  phase += 2 * PI * frequency / SAMPLE_RATE;
  
  // איפוס הפאזה כדי למנוע גלישה (Overflow) במספרים גדולים
  if (phase >= 2 * PI) {
    phase -= 2 * PI;
  }

  // --- כתיבה ל-DMA ---
  // אנחנו כותבים את אותה דגימה לשמאל ולימין (מונו שמוכפל לסטריאו)
  int16_t stereo_sample[2] = {sample, sample};
  size_t bytes_written;
  
  // הפקודה הזו "חוסמת" (Blocking) עד שיש מקום בבאפר, כך שהקצב נשמר אוטומטית!
  i2s_write(I2S_NUM, &stereo_sample, sizeof(stereo_sample), &bytes_written, portMAX_DELAY);
}