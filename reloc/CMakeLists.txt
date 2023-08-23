
add_executable(o65reloc
    o65reloc.c
)

target_link_libraries(o65reloc PUBLIC o65)

install(TARGETS o65reloc DESTINATION bin)
