set(SHARED_SRC
    "../shared/src/common/serialize.cpp"
    "../shared/src/common/time.cpp"
    "../shared/src/common/io_buffer.cpp"
    "../shared/src/common/json.cpp"
    "../shared/src/common/util/str_utils.cpp"
    "../shared/src/common/img.cpp"
    "../shared/src/common/img/font.cpp"
    "../shared/src/common/img/img_bw.cpp"
    "../shared/src/common/img/img_gray.cpp"
    "../shared/src/common/img/img_jpeg.cpp"
    "../shared/src/common/img/img_yuyv.cpp"
    "../shared/src/common/img/img_rgb.cpp"
    "../shared/src/common/img/img_rgba.cpp"
    "../shared/src/common/img/img_rgba32.cpp"
    "../shared/src/common/img/img_rgb32.cpp"
    "../shared/src/common/img/img_gray32.cpp"
    "../shared/src/common/img/img_gray32.cpp"
    "../shared/src/common/external/jpgd.cpp"
    "../shared/src/common/external/jpge.cpp"
)

file(READ ../model/compile_info.json CPL_INFO)
string(JSON MDL_SOURCES_ARR      GET     ${CPL_INFO} sources)
string(JSON MDL_SOURCES_ARR_LEN  LENGTH  ${CPL_INFO} sources)

MATH(EXPR MDL_SOURCES_ARR_LEN2 "${MDL_SOURCES_ARR_LEN}-1")
foreach(IDX RANGE ${MDL_SOURCES_ARR_LEN2})
    string(JSON CUR_SRC GET ${MDL_SOURCES_ARR} ${IDX} )
    list(APPEND MDL_SOURCES ../${CUR_SRC})
endforeach()

idf_component_register(SRCS "main.cpp" ${SHARED_SRC} ${MDL_SOURCES} "../html_files.cpp" "setup_wifi.cpp" "abmt_link.cpp" "com_device.cpp" "os.cpp" "model_adapter_std.cpp" "mutex.cpp"
                    INCLUDE_DIRS "." ".." "../shared")

target_compile_options(${COMPONENT_TARGET} PUBLIC "-Wno-narrowing")