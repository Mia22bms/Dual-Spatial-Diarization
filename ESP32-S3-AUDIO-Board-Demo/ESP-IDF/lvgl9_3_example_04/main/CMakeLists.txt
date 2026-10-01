# 初始化源文件和包含目录
set(SOURCES
    "./main.c" 
    "./img_volume.c"
    "./hardeware_driver/bsp_board.c"
    "./tca9555_driver/tca9555_driver.c"
    "./lcd_driver/lcd_driver.c"
    "./lvgl_driver/lvgl_driver.c"
    "./touch_driver/touch_driver.c"
    "./speech_det_driver/mic_speech.c"
    "./audio_play_driver/audio_driver.c"
    "./camera_driver/camera_driver.c"
    "./lvgl_ui/lvgl_example.c"
    "./lvgl_ui/lvgl_music/lvgl_music_main.c"
    "./lvgl_ui/lvgl_music/lvgl_music_list.c"
    "./lvgl_ui/lvgl_music/lvgl_music.c"
)

set(INCLUDE_DIRS 
    "."
    "./hardeware_driver"
    "./tca9555_driver"
    "./lcd_driver"
    "./lvgl_driver"
    "./touch_driver"
    "./speech_det_driver"
    "./audio_play_driver"
    "./camera_driver"
    "./lvgl_ui"
    "./lvgl_ui/lvgl_music"
)





if(CONFIG_WAVESHARE_2INCH_TOUCH_LCD)
    list(APPEND SOURCES 
        "./touch_driver/CST816/CST816.c"
    )
    list(APPEND INCLUDE_DIRS
        "./touch_driver/CST816"
    )
endif()

if(CONFIG_WAVESHARE_2_8INCH_TOUCH_LCD)
    list(APPEND SOURCES 
        "./touch_driver/CST328/CST328.c"
    )
    list(APPEND INCLUDE_DIRS
        "./touch_driver/CST328"
    )
endif()



# 注册组件
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
)