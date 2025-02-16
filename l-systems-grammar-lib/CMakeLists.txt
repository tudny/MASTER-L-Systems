project(L-Systems-grammar)

set(
        LIB_NAME
        L-Systems-grammar
)

set(
        LIBRARY_SOURCES
        src/grammar.cpp
        src/grammar.h
        src/str_utils.h
)

add_library(
        ${LIB_NAME}
        ${LIBRARY_SOURCES}
)

target_link_libraries(
        ${LIB_NAME}
)
