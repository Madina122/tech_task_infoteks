# CMakeLists.txt для драйвера EEPROM 25LC040A
# Проект: eeprom_driver
# Описание: Сборка библиотеки драйвера EEPROM с тестовым приложением

cmake_minimum_required(VERSION 3.10)
project(eeprom_driver VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(eeprom_driver eeprom/src/eeprom_25lc040a.cpp)

target_include_directories(eeprom_driver PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(test_eeprom test/test_eeprom.cpp)
target_link_libraries(test_eeprom eeprom_driver)

enable_testing()
add_test(NAME eeprom_test COMMAND test_eeprom)

install(TARGETS eeprom_driver
    EXPORT eeprom_driver-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)