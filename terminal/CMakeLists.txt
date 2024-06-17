cmake_minimum_required(VERSION 3.10)

# 프로젝트 이름 및 버전
project(RFIDMenu VERSION 1.0)

# C 표준 설정
set(CMAKE_C_STANDARD 99)

# 소스 파일 목록
set(SOURCES
    main.c
    menu.c
    lcd.c
    input.c
)

# 실행 파일 생성
add_executable(rfid_menu ${SOURCES})

# pthread 라이브러리 링크
find_package(Threads REQUIRED)

# cJSON 경로 설정
include_directories(/usr/include/cjson)
target_link_libraries(rfid_menu PRIVATE Threads::Threads cjson)

