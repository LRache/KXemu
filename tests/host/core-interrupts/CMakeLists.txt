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

    add_executable(task-timer-test task-timer.cpp
        ${PROJECT_SOURCE_DIR}/src/utils/task-timer/task-timer.cpp)
    target_include_directories(task-timer-test PRIVATE "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(task-timer-test PRIVATE Threads::Threads Catch2::Catch2WithMain)
    catch_discover_tests(task-timer-test
        TEST_PREFIX "task-timer: "
        DISCOVERY_MODE PRE_TEST
        PROPERTIES TIMEOUT 20)

    # Compile the emulator once and link it into every host test.
    add_library(rvcore-test-emu OBJECT ${EMU_SRCS})
    target_include_directories(rvcore-test-emu PUBLIC
        "${PROJECT_BINARY_DIR}/include" "${PROJECT_SOURCE_DIR}/include")
    target_link_libraries(rvcore-test-emu PUBLIC Threads::Threads asio)
    if(CONFIG_FPU_SOFT)
        target_link_libraries(rvcore-test-emu PUBLIC softfloat)
    endif()
    add_dependencies(rvcore-test-emu instpat)

    if(CONFIG_DIFFTEST)
        # Difftest harts never poll devices or wait in WFI.
        add_executable(rvcore-difftest-test rvcore-difftest.cpp)
        target_link_libraries(rvcore-difftest-test PRIVATE rvcore-test-emu Catch2::Catch2WithMain)
        catch_discover_tests(rvcore-difftest-test
            TEST_PREFIX "rvcore-difftest: "
            DISCOVERY_MODE PRE_TEST
            PROPERTIES TIMEOUT 20)
    else()
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

        add_executable(device-irq-test device-irq.cpp)
        target_link_libraries(device-irq-test PRIVATE rvcore-test-emu Catch2::Catch2WithMain)
        catch_discover_tests(device-irq-test
            TEST_PREFIX "device-irq: "
            DISCOVERY_MODE PRE_TEST
            PROPERTIES TIMEOUT 20)

        add_executable(platform-timer-test platform-timer.cpp)
        target_link_libraries(platform-timer-test PRIVATE rvcore-test-emu Catch2::Catch2WithMain)
        catch_discover_tests(platform-timer-test
            TEST_PREFIX "platform-timer: "
            DISCOVERY_MODE PRE_TEST
            PROPERTIES TIMEOUT 20)
    endif()

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
