# 初始化源文件和包含目录
set(SOURCES
    "./main.c" 
    "./hardeware_driver/bsp_board.c"
    "./speech_det_driver/mic_speech.c"
    "./tca9555_driver/tca9555_driver.c"
)

set(INCLUDE_DIRS 
    "."
    "./hardeware_driver"
    "./speech_det_driver"
    "./tca9555_driver"
)


# 注册组件
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
)