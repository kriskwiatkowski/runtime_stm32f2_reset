function(target_postbuild_executable tgt)
    set(ELF_FILE ${CMAKE_CURRENT_BINARY_DIR}/$<TARGET_FILE_BASE_NAME:${tgt}>)
    set(HEX_FILE ${ELF_FILE}.hex)
    set(SECTIONS_FILE ${ELF_FILE}.sections)
    set(LST_FILE ${ELF_FILE}.lst)
    set(BIN_FILE ${ELF_FILE}.bin)

    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        add_custom_command(
            TARGET ${tgt} POST_BUILD
            COMMAND ${OBJCOPY} -Obinary ${ELF_FILE} ${BIN_FILE}
            COMMAND ${OBJCOPY} -Oihex ${ELF_FILE} ${HEX_FILE}
            COMMENT "Invoking: objcopy")

        add_custom_command(
            TARGET ${tgt} POST_BUILD
            COMMAND ${OBJDUMP} -h -D ${ELF_FILE} > ${SECTIONS_FILE}
            COMMAND ${OBJDUMP} -S --disassemble ${ELF_FILE} > ${LST_FILE}
            COMMENT "Invoking: objdump")
    elseif(CMAKE_C_COMPILER_ID STREQUAL "ARMCC")
        add_custom_command(
            TARGET ${tgt} POST_BUILD
            COMMENT "Invoking: arm-fromelf"
            COMMAND ${ARM_FROMELF} --bin --output=${BIN_FILE} $<TARGET_FILE:${tgt}>
            COMMAND ${ARM_FROMELF} --i32 --output=${HEX_FILE} $<TARGET_FILE:${tgt}>)
    endif()

    set_property(
        TARGET ${tgt} APPEND
        PROPERTY ADDITIONAL_CLEAN_FILES
        ${HEX_FILE} ${BIN_FILE} ${SECTIONS_FILE} ${LST_FILE})
endfunction()
