cmake_minimum_required(VERSION 3.16)

project(wallpaper_downloader VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON) # Importante para procesar .qrc
set(CMAKE_AUTOUIC ON)

# Paquetes requeridos
find_package(Qt6 REQUIRED COMPONENTS Widgets Network LinguistTools)

# --- CONFIGURACIÓN LINUX (KDE KF6) ---
if(UNIX AND NOT APPLE AND NOT ANDROID)
    find_package(KF6 REQUIRED COMPONENTS WindowSystem Config)
endif()

# Archivos fuente
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    wallhaven_api.cpp
    wallhaven_api.h
    resources.qrc  # AÑADIDO: Archivo de recursos
    resources.rc
)

# Archivo de traducción
set(TS_FILES wallpaper_downloader_es_CO.ts)

qt_add_executable(wallpaper_downloader
    MANUAL_FINALIZATION
    WIN32
    ${PROJECT_SOURCES}
    ${TS_FILES}
)

qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

foreach(qm ${QM_FILES})
    get_filename_component(qm_name ${qm} NAME)
    set_source_files_properties(${qm} PROPERTIES QT_RESOURCE_ALIAS ${qm_name})
endforeach()

qt_add_resources(wallpaper_downloader "translations"
    PREFIX "/i18n"
    FILES ${QM_FILES}
)

target_link_libraries(wallpaper_downloader PRIVATE Qt6::Widgets Qt6::Network)

if(WIN32)
    target_link_libraries(wallpaper_downloader PRIVATE dwmapi)
elseif(UNIX AND NOT APPLE AND NOT ANDROID)
    target_link_libraries(wallpaper_downloader PRIVATE KF6::WindowSystem KF6::ConfigCore KF6::ConfigGui)
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(wallpaper_downloader)
endif()
