#include <Arduino.h>

#include <M5Unified.h>
#include <esp_camera.h>
#include <fb_gfx.h>

#define LCD_WIDTH (320)
#define LCD_HEIGHT  (240)
#define LCD_BUF_SIZE (LCD_WIDTH*LCD_HEIGHT*2)

//NNABLA [start]
#include "Validation_inference.h"
#include "Validation_parameters.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
void *_context = NULL;
float *nn_input_buffer;
uint8_t resized_img[NNABLART_VALIDATION_INPUT0_SIZE];
const char classification[6][16] = {"Nothing", "Neutral", "Up", "Down", "Right", "Left"};
//NNABLA [end]

uint16_t lcd_buf[LCD_WIDTH*LCD_HEIGHT];


#define COLOR_WHITE  0x00FFFFFF
#define COLOR_BLACK  0x00000000
#define COLOR_RED    0x000000FF
#define COLOR_GREEN  0x0000FF00
#define COLOR_BLUE   0x00FF0000
#define COLOR_YELLOW (FACE_COLOR_RED | FACE_COLOR_GREEN)
#define COLOR_CYAN   (FACE_COLOR_BLUE | FACE_COLOR_GREEN)
#define COLOR_PURPLE (FACE_COLOR_BLUE | FACE_COLOR_RED)

static camera_config_t camera_config = {
    .pin_pwdn     = -1,
    .pin_reset    = -1,
    .pin_xclk     = 2,
    .pin_sscb_sda = 12,
    .pin_sscb_scl = 11,

    .pin_d7 = 47,
    .pin_d6 = 48,
    .pin_d5 = 16,
    .pin_d4 = 15,
    .pin_d3 = 42,
    .pin_d2 = 41,
    .pin_d1 = 40,
    .pin_d0 = 39,

    .pin_vsync = 46,
    .pin_href  = 38,
    .pin_pclk  = 45,

    .xclk_freq_hz = 20000000,
    .ledc_timer   = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    //.pixel_format = PIXFORMAT_RGB565,
    .pixel_format = PIXFORMAT_GRAYSCALE,
    .frame_size   = FRAMESIZE_QVGA,   // LCDと同じ320x240サイズ
    //.frame_size   = FRAMESIZE_QQVGA,   // LCDに表示しない場合はQQVGA(160x120)にしてメモリを節約
    .jpeg_quality = 0,
    .fb_count     = 2,
    .fb_location  = CAMERA_FB_IN_PSRAM,
    .grab_mode    = CAMERA_GRAB_WHEN_EMPTY,
};

esp_err_t camera_init(){

    //initialize the camera
    M5.In_I2C.release();
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        //Serial.println("Camera Init Failed");
        M5.Display.println("Camera Init Failed");
        return err;
    }

    return ESP_OK;
}

void rgb_print(fb_data_t *fb, uint32_t color, const char *str)
{
    fb_gfx_print(fb, (fb->width - (strlen(str) * 14)) / 2, 10, color, str);
}

int rgb_printf(fb_data_t *fb, uint32_t color, const char *format, ...)
{
    char loc_buf[64];
    char *temp = loc_buf;
    int len;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    len = vsnprintf(loc_buf, sizeof(loc_buf), format, arg);
    va_end(copy);
    if (len >= sizeof(loc_buf))
    {
        temp = (char *)malloc(len + 1);
        if (temp == NULL)
        {
            return 0;
        }
    }
    vsnprintf(temp, len + 1, format, arg);
    va_end(arg);
    rgb_print(fb, color, temp);
    if (len > 64)
    {
        free(temp);
    }
    return len;
}


uint16_t swapBytes(uint16_t num) {
    uint16_t swappedNum = ((num >> 8) & 0x00FF) | ((num << 8) & 0xFF00);
    return swappedNum;
}

void grayscale2rgb565(uint16_t* dst, uint8_t* src, uint32_t px_size){

  for(int i=0; i<px_size; i++){
    uint16_t rb = (float)src[i] * ((float)0x1F / 0xFF);
    uint16_t g = (float)src[i] * ((float)0x3F / 0xFF);

    dst[i] = swapBytes((rb << 11) + (g << 5) + rb);
  }
}

esp_err_t camera_capture_and_classification(){
  //acquire a frame
  M5.In_I2C.release();
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    //Serial.println("Camera Capture Failed");
    M5.Display.println("Camera Capture Failed");
    return ESP_FAIL;
  }

  //NNABLA [start]
  // 28x28にリサイズ
  stbir_resize_uint8(fb->buf, fb->width, fb->height, 0, resized_img, 28, 28, 0, 1);

  for (int i = 0; i < NNABLART_VALIDATION_INPUT0_SIZE; i++) {
      nn_input_buffer[i] = resized_img[i];
  }

  // 推論
  int64_t infer_time = esp_timer_get_time();
  nnablart_validation_inference(_context);
  infer_time = (esp_timer_get_time() - infer_time) / 1000;

  // 推論結果をフェッチ
  float *probs = nnablart_validation_output_buffer(_context, 0);

  int top_class = 0;
  float top_probability = 0.0f;
  for (int classNo = 0; classNo < NNABLART_VALIDATION_OUTPUT0_SIZE; classNo++) {
      if (top_probability < probs[classNo]) {
          top_probability = probs[classNo];
          top_class = classNo;
      }
  }
  
  Serial.printf("Result %d   Inferrence-time %ums\n", top_class, (uint32_t)infer_time);
  //NNABLA [end]

  grayscale2rgb565(lcd_buf, fb->buf, fb->width * fb->height);

  fb_data_t rfbLcd;
  rfbLcd.width = fb->width;
  rfbLcd.height = fb->height;
  rfbLcd.data = (uint8_t*)lcd_buf;
  rfbLcd.bytes_per_pixel = 2;
  rfbLcd.format = FB_RGB565;

  rgb_print(&rfbLcd, COLOR_GREEN, classification[top_class]);

  //replace this with your own function
  //process_image(fb->width, fb->height, fb->format, fb->buf, fb->len);
  M5.Display.startWrite();
  M5.Display.setAddrWindow((LCD_WIDTH - fb->width)/2, (LCD_HEIGHT - fb->height)/2, fb->width, fb->height);
  //M5.Display.writePixels((uint16_t*)fb->buf, int(fb->len / 2));
  M5.Display.writePixels((uint16_t*)lcd_buf, int(fb->len));
  M5.Display.endWrite();

  //return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);
  return ESP_OK;
}

void setup() {
  auto cfg = M5.config();
  cfg.output_power = true;
  M5.begin(cfg);

  M5.Display.setFont(&fonts::efontJA_24);
  M5.Display.println("HelloWorld");

  _context = nnablart_validation_allocate_context(Validation_parameters);     //NNABLA
  nn_input_buffer = nnablart_validation_input_buffer(_context, 0);     //NNABLA

  camera_init();

}

void loop() {
  //Serial.println("Hello World");  // Print text on the serial port.在串口输出文本

#if 0   // Check free size of heap memory
  Serial.printf("===============================================================\n");
  Serial.printf("Mem Test\n");
  Serial.printf("===============================================================\n");
  Serial.printf("esp_get_free_heap_size()                              : %6d\n", esp_get_free_heap_size() );
  Serial.printf("heap_caps_get_free_size(MALLOC_CAP_DMA)               : %6d\n", heap_caps_get_free_size(MALLOC_CAP_DMA) );
  Serial.printf("heap_caps_get_free_size(MALLOC_CAP_SPIRAM)            : %6d\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM) );
  Serial.printf("heap_caps_get_free_size(MALLOC_CAP_INTERNAL)          : %6d\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL) );
  Serial.printf("heap_caps_get_free_size(MALLOC_CAP_DEFAULT)           : %6d\n", heap_caps_get_free_size(MALLOC_CAP_DEFAULT) );
  delay(1000);          // Delay [ms]
#endif

  camera_capture_and_classification();

}
