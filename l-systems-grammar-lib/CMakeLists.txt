project(L-Systems-grammar-lib)

set(
        LIB_NAME
        L-Systems-grammar-lib
)

set(
        LIBRARY_SOURCES
        src/grammar.cpp
        src/grammar.h
)

add_library(
        ${LIB_NAME}
        ${LIBRARY_SOURCES}
)

target_link_libraries(
        ${LIB_NAME}
        PRIVATE L-Systems-lib
        PRIVATE L-Systems-grammar-raw-lib
)
