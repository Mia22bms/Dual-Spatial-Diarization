# 初始化源文件和包含目录
set(SOURCES
    "./main.c"
    "./hardeware_driver/bsp_board.c"
    "./audio_play_driver/audio_driver.c"
    "./tca9555_driver/tca9555_driver.c"
    "./status_led.c"

)

set(INCLUDE_DIRS 
    "."
    "./hardeware_driver"
    "./audio_play_driver"
    "./tca9555_driver"

)


# 注册组件
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
)