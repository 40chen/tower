#include <Arduino.h>
#include <chrono>  //C++时间戳
#include "audio_hal.h"
#include "es8311.h"
#include "es8388.h"
#include "headphone_detect.h"

#define MENU_NUM_MAX    4

std::chrono::steady_clock::time_point _keyUpdate;

uint8_t flag_key_menu = 0;
uint8_t flag_key_normal = 0;
uint8_t key_menu_page = 0;
uint8_t app_mode_page[4] = {0};

void ADC_KeyInit(void);
void Audio_codeC(uint8_t chip_type)
{
    audio_hal_codec_config_t codec_cfg = AUDIO_HAL_ES8388_DEFAULT();
    switch(chip_type)
    {
        case ES8311:
          es8311_init(AUDIO_HAL_SAMPLE_RATE);
          es8311_set_voice_volume(AUDIO_HAL_VOL_DEFAULT);
          es8311_set_mic_gain(ES8311_MIC_GAIN_12DB);
          break;
        case ES8388:
          es8388_init(&codec_cfg);
          es8388_set_voice_volume(AUDIO_HAL_VOL_DEFAULT);
          //start codec
          es8388_ctrl_state(AUDIO_HAL_CODEC_MODE_BOTH,AUDIO_HAL_CTRL_START);
          //es8388_set_mic_gain(MIC_GAIN_12DB);
          headphone_detect_init(HEADPHONE_DETECT);
          break;
    }
    ADC_KeyInit();
}
void ADC_KeyInit(void)
{
    //analogReadResolution(12);
    //analogSetAttenuation(ADC_11db);
    pinMode(ADC_KEY_DETECT_PIN,INPUT);
    pinMode(KEY2_DETECT_PIN,INPUT);
}
void ADC_KeyScan(void)
{
    static uint8_t key_count = 0;
    uint8_t adckey_value = 0;

     auto now = std::chrono::steady_clock::now();
    int64_t dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - _keyUpdate).count();
    if(dt >= 50)
    {
        _keyUpdate = now;
        adckey_value  = digitalRead(ADC_KEY_DETECT_PIN);
        if(adckey_value == LOW){
            flag_key_menu = 1;
        }else{
            ;
        }
        adckey_value  = digitalRead(KEY2_DETECT_PIN);
        if(adckey_value == LOW){
            flag_key_normal = 1;
        }else{
            if(flag_key_normal == 1){
                flag_key_normal = 0;
                app_mode_page[0]++;
                if(app_mode_page[0] >= 8){
                    app_mode_page[0] = 0;
                }
            }
        }
        // key_count++;
        // if(key_count >= 20){
        //     key_count = 0;
        //     Serial.printf("adckey_value = %d\n",adckey_value);
        // }
    }
}


uint8_t app_mode_page_get(uint8_t menu_id)
{
    if(menu_id==0 || menu_id>MENU_NUM_MAX){
        return 0;
    }
    return app_mode_page[menu_id-1];
}
