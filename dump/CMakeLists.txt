
add_executable(o65dump
    o65dump.c
    instructions.h
)

target_link_libraries(o65dump PUBLIC o65)

install(TARGETS o65dump DESTINATION bin)
