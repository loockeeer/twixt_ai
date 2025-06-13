cmake_minimum_required(VERSION 3.30)
project(twixt_ai C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Ofast")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")
# Trouver les bibliothèques Cairo et GTK
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk+-3.0)
pkg_check_modules(CAIRO REQUIRED cairo)

add_executable(twixt_ai main.c montecarlo.c twixt.c
        serializer.h
        serializer.c
        zobrist.h
        zobrist.c
        zobrist.c
        rand.h
        rand.c)
# Ajouter les répertoires d'inclusion et les bibliothèques
target_include_directories(twixt_ai PRIVATE ${GTK_INCLUDE_DIRS} ${CAIRO_INCLUDE_DIRS})
target_link_libraries(twixt_ai PRIVATE ${GTK_LIBRARIES} ${CAIRO_LIBRARIES})

# Ajouter les options de compilation spécifiques
target_compile_options(twixt_ai PRIVATE ${GTK_CFLAGS_OTHER} ${CAIRO_CFLAGS_OTHER})