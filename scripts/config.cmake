function(parse_kconfig config_file)
    file(STRINGS "${config_file}" lines)
    
    foreach(line IN LISTS lines)
        if(line MATCHES "^# CONFIG_([A-Za-z0-9_]+) is not set")
            set(${CMAKE_MATCH_1} OFF PARENT_SCOPE)
            continue()
        endif()

        if(line MATCHES "^CONFIG_([A-Za-z0-9_]+)=([yYnN]|\".*\")$")
            set(var_name "${CMAKE_MATCH_1}")
            set(var_value "${CMAKE_MATCH_2}")

            if(var_value MATCHES "^(y|Y)$")
                set(var_value ON)
            elseif(var_value MATCHES "^(n|N)$")
                set(var_value OFF)
            else()
                string(REGEX REPLACE "^\"(.*)\"$" "\\1" var_value "${var_value}")
            endif()

            set(${var_name} "${var_value}" PARENT_SCOPE)
        endif()
    endforeach()
endfunction()

parse_kconfig(${CMAKE_SOURCE_DIR}/configs/.config)

string(TOUPPER ${BASE_ISA} BASE_ISA_UPPER)
