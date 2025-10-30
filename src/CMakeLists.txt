# Build keyhook as static library to evitar DLL externa
add_library(keyhook STATIC
  keyhook.cpp
  keyhook.h
  app/protection.cpp
  app/protection.h
)

target_compile_definitions(keyhook PRIVATE KEYHOOK_EXPORTS)
target_compile_definitions(keyhook PRIVATE UNICODE _UNICODE)
target_compile_definitions(keyhook PRIVATE WIN32_LEAN_AND_MEAN)
target_link_libraries(keyhook PRIVATE user32 psapi advapi32)

# Windows API app (basic UI)
add_executable(keymapper_app WIN32
  app/config.h
  app/config.cpp
  app/manifest.xml
  app/antidebug.h
  app/antidebug.cpp
  app/protection.h
  app/protection.cpp
  app/protection_config.h
  app/main.cpp
  app/ui.cpp
  app/ui.h
  app/tray.cpp
  app/tray.h
)

# Set manifest for administrator privileges
set_target_properties(keymapper_app PROPERTIES
  LINK_FLAGS "/MANIFEST:NO"
)

# Add manifest file
if(MSVC)
  set_target_properties(keymapper_app PROPERTIES
    LINK_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO"
  )
  
  # Create resource file for manifest and icon
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/manifest.rc
    "IDI_ICON1 ICON \"${CMAKE_SOURCE_DIR}/imagens/logo.ico\"\n"
    "1 24 \"${CMAKE_CURRENT_SOURCE_DIR}/app/manifest.xml\"\n"
    "\n"
    "VS_VERSION_INFO VERSIONINFO\n"
    " FILEVERSION 1,0,0,0\n"
    " PRODUCTVERSION 1,0,0,0\n"
    " FILEFLAGSMASK 0x3fL\n"
    " FILEFLAGS 0x0L\n"
    " FILEOS 0x40004L\n"
    " FILETYPE 0x1L\n"
    " FILESUBTYPE 0x0L\n"
    "BEGIN\n"
    "    BLOCK \"StringFileInfo\"\n"
    "    BEGIN\n"
    "        BLOCK \"040904B0\"\n"
    "        BEGIN\n"
    "            VALUE \"CompanyName\", \"geanswag\"\n"
    "            VALUE \"FileDescription\", \"KeyMapper\"\n"
    "            VALUE \"ProductName\", \"KeyMapper\"\n"
    "            VALUE \"ProductVersion\", \"${KEYMAPPER_VERSION}\"\n"
    "            VALUE \"FileVersion\", \"${KEYMAPPER_VERSION}\"\n"
    "            VALUE \"InternalName\", \"KeyMapper\"\n"
    "            VALUE \"OriginalFilename\", \"keymapper_app.exe\"\n"
    "            VALUE \"LegalCopyright\", \"(c) geanswag\"\n"
    "        END\n"
    "    END\n"
    "    BLOCK \"VarFileInfo\"\n"
    "    BEGIN\n"
    "        VALUE \"Translation\", 0x0409, 1200\n"
    "    END\n"
    "END\n"
  )
  
  target_sources(keymapper_app PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/manifest.rc)
endif()

target_link_libraries(keymapper_app PRIVATE keyhook user32 shell32 gdi32 comctl32 advapi32 dwmapi shlwapi)
target_compile_definitions(keymapper_app PRIVATE UNICODE _UNICODE)
target_compile_definitions(keymapper_app PRIVATE KEYHOOK_STATIC)

# Version macro for UI footer
set(KEYMAPPER_VERSION "6.0.0")
if(DEFINED ENV{APP_VERSION})
  set(KEYMAPPER_VERSION $ENV{APP_VERSION})
endif()
target_compile_definitions(keymapper_app PRIVATE KEYMAPPER_VERSION="${KEYMAPPER_VERSION}")

# Removido: cópia de imagens. Agora os assets estão embutidos via .qrc

install(TARGETS keymapper_app RUNTIME DESTINATION .)
