if(NOT DEFINED OUTPUT_FILE)
    message(FATAL_ERROR "OUTPUT_FILE is required")
endif()

string(TIMESTAMP BUILD_DATE "%Y/%m/%d")
set(CONTENT "#pragma once
#define APP_BUILD_DATE \"${BUILD_DATE}\"
")

if(EXISTS "${OUTPUT_FILE}")
    file(READ "${OUTPUT_FILE}" EXISTING_CONTENT)
else()
    set(EXISTING_CONTENT "")
endif()

if(NOT EXISTING_CONTENT STREQUAL CONTENT)
    get_filename_component(OUTPUT_DIR "${OUTPUT_FILE}" DIRECTORY)
    file(MAKE_DIRECTORY "${OUTPUT_DIR}")
    file(WRITE "${OUTPUT_FILE}" "${CONTENT}")
endif()
