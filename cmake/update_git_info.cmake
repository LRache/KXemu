# 独立的git信息更新脚本
# 这个脚本会被自定义目标调用来确保git信息始终最新

find_package(Git QUIET)

if(GIT_FOUND)
    # 获取完整的commit hash
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    # 获取短commit hash
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH_SHORT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    # 获取分支名
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    # 获取最后提交日期
    execute_process(
        COMMAND ${GIT_EXECUTABLE} log -1 --format=%cd --date=iso
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_DATE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    # 检查是否有未提交的修改（工作区和暂存区）
    execute_process(
        COMMAND ${GIT_EXECUTABLE} diff --quiet
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE GIT_DIRTY_RESULT
        ERROR_QUIET
    )
    
    execute_process(
        COMMAND ${GIT_EXECUTABLE} diff --cached --quiet
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE GIT_STAGED_RESULT
        ERROR_QUIET
    )
    
    if(GIT_DIRTY_RESULT EQUAL 0 AND GIT_STAGED_RESULT EQUAL 0)
        set(GIT_IS_DIRTY "false")
        set(GIT_DIRTY_SUFFIX "")
    else()
        set(GIT_IS_DIRTY "true")
        set(GIT_DIRTY_SUFFIX "-dirty")
    endif()
else()
    set(GIT_COMMIT_HASH "unknown")
    set(GIT_COMMIT_HASH_SHORT "unknown")
    set(GIT_BRANCH "unknown")
    set(GIT_COMMIT_DATE "unknown")
    set(GIT_IS_DIRTY "false")
    set(GIT_DIRTY_SUFFIX "")
endif()

configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/git_info.hpp.in
    ${CMAKE_BINARY_DIR}/include/autogen/git_info.hpp
    @ONLY
)
