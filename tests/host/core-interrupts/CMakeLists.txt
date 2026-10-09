if(CONFIG_BASE_ISA STREQUAL "riscv")
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.16.1
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
    include(Catch)

    # Compile the emulator once and link it into every host test.
    add_library(rvcore-test-emu OBJECT ${EMU_SRCS})
    target_include_directories(rvcore-test-emu PUBLIC
        "${PROJECT_BINARY_DIR}/include" "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(rvcore-test-emu PUBLIC Threads::Threads)
    if(CONFIG_FPU_SOFT)
        target_link_libraries(rvcore-test-emu PUBLIC softfloat)
    endif()
    add_dependencies(rvcore-test-emu instpat)

    if(CONFIG_WFI_AS_NOP)
        set(interrupt_test_source rvcore-wfi-nop.cpp)
    else()
        set(interrupt_test_source rvcore-interrupt.cpp)
    endif()
    add_executable(rvcore-interrupt-test ${interrupt_test_source})
    target_link_libraries(rvcore-interrupt-test PRIVATE rvcore-test-emu Catch2::Catch2WithMain)
    catch_discover_tests(rvcore-interrupt-test
        TEST_PREFIX "rvcore-interrupt: "
        DISCOVERY_MODE PRE_TEST
        PROPERTIES TIMEOUT 20)

    if(CONFIG_ISA STREQUAL "riscv64")
        add_executable(rvcore-superpage-test rvcore-superpage.cpp)
        target_link_libraries(rvcore-superpage-test PRIVATE rvcore-test-emu Catch2::Catch2WithMain)
        catch_discover_tests(rvcore-superpage-test
            TEST_PREFIX "rvcore-superpage: "
            DISCOVERY_MODE PRE_TEST
            PROPERTIES TIMEOUT 20)
    else()
        message(STATUS "Skipping Sv48/Sv57 superpage test on ${CONFIG_ISA}")
    endif()
endif()
