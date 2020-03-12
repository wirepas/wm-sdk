var NAVTREE =
[
  [ "Wirepas Single-MCU SDK", "index.html", [
    [ "Single-MCU Operation Overview", "index.html", [
      [ "Application", "index.html#application", null ],
      [ "Application-specific Hardware Abstraction Layer (HAL)", "index.html#sdk_hal", null ],
      [ "Wirepas Mesh Single MCU API", "index.html#single_mcu_api", null ],
      [ "Wirepas Mesh Single MCU libraries wrapper", "index.html#sdk_libraries", null ],
      [ "Wirepas Mesh Stack", "index.html#wirepas_firmware", null ],
      [ "Wirepas Mesh Bootloader", "index.html#bootloader", null ],
      [ "Custom bootloader extension", "index.html#bootloader_extension", null ],
      [ "The physical Hardware", "index.html#hardware", null ]
    ] ],
    [ "API", "dd/d9c/programming_interface.html", null ],
    [ "Application Examples", "d5/d7b/application_examples.html", null ],
    [ "How to Develop Application with SDK", "de/d7a/how_to_develop.html", [
      [ "Pre-requirements", "de/d7a/how_to_develop.html#licensing", [
        [ "Debugging", "de/d7a/how_to_develop.html#debugging", null ],
        [ "Installing firmware image", "de/d7a/how_to_develop.html#firmware_linking", null ]
      ] ],
      [ "Create new application", "de/d7a/how_to_develop.html#development_of_a_new_application", [
        [ "Copy of an application example", "de/d7a/how_to_develop.html#copy_of_an_application_example", null ],
        [ "Change default network", "de/d7a/how_to_develop.html#change_default_network_address_and_channel", null ],
        [ "Change of app_area_id", "de/d7a/how_to_develop.html#change_of_app_area_id", null ],
        [ "Configuration of a node", "de/d7a/how_to_develop.html#configuration_of_a_node", null ]
      ] ],
      [ "Build application", "de/d7a/how_to_develop.html#build_application", null ],
      [ "Test application", "de/d7a/how_to_develop.html#test_application", null ],
      [ "Flashing device", "de/d7a/how_to_develop.html#flashing_device", null ],
      [ "Using OTAP", "de/d7a/how_to_develop.html#using_otap", null ],
      [ "Define custom board definition", "de/d7a/how_to_develop.html#define_custom_board", null ],
      [ "Application startup", "de/d7a/how_to_develop.html#app_init", null ],
      [ "Adding new source files", "de/d7a/how_to_develop.html#adding_new_source_files_to_the_application", null ],
      [ "Recommendations", "de/d7a/how_to_develop.html#recommendations", [
        [ "Security for scratchpad images", "de/d7a/how_to_develop.html#security_for_scratchpad_images", null ],
        [ "Optimization of throughput", "de/d7a/how_to_develop.html#optimization_of_network_throughput", null ],
        [ "Free resources", "de/d7a/how_to_develop.html#free_resources", null ],
        [ "Power consumption", "de/d7a/how_to_develop.html#power_consumption", null ],
        [ "How to store data in persistent memory", "de/d7a/how_to_develop.html#persistent_memory", [
          [ "Using storage library", "de/d7a/how_to_develop.html#storage_library", null ],
          [ "Using platform specific storage", "de/d7a/how_to_develop.html#platform_specific_storage", null ],
          [ "Using a dedicated area in flash", "de/d7a/how_to_develop.html#dedicated_area", null ]
        ] ]
      ] ]
    ] ],
    [ "Single-MCU API Operation Principle", "d7/d28/application_operation.html", [
      [ "Single-MCU API Operation Principle", "d7/d28/application_operation.html#operation_principle", null ],
      [ "Build process", "d7/d28/application_operation.html#build", null ],
      [ "Memory Partitioning", "d7/d28/application_operation.html#memory_partitioning", null ],
      [ "Application Detection", "d7/d28/application_operation.html#application_detection", null ],
      [ "Cooperative MCU Access", "d7/d28/application_operation.html#cooperative_mcu_access", [
        [ "Periodic Application Callback Function", "d7/d28/application_operation.html#periodic_application", null ],
        [ "Asynchronous Application Callback Functions", "d7/d28/application_operation.html#asynchronous_application_callback_functions", null ],
        [ "Application Interrupt", "d7/d28/application_operation.html#application_interrupt", [
          [ "Deferred interrupt", "d7/d28/application_operation.html#deferred_interrupt", null ],
          [ "Fast interrupt", "d7/d28/application_operation.html#fast_interrupt", null ]
        ] ],
        [ "Which kind of interrupt for which purpose", "d7/d28/application_operation.html#which_kind_of_interrupt_for_which_purpose", null ],
        [ "Execution time limits", "d7/d28/application_operation.html#execution_time_limits", null ]
      ] ]
    ] ],
    [ "SDK Environment", "dd/d6a/sdk_environment.html", [
      [ "Installation of SDK Environment", "dd/d6a/sdk_environment.html#installation_of_sdk_environment", [
        [ "Windows installation using WSL", "dd/d6a/sdk_environment.html#windows_installation", [
          [ "Compiler installation", "dd/d6a/sdk_environment.html#wsl_compiler", null ],
          [ "Miscellaneous installation", "dd/d6a/sdk_environment.html#wsl_misc", null ]
        ] ],
        [ "Installation on Ubuntu Linux", "dd/d6a/sdk_environment.html#linux_installation", null ]
      ] ],
      [ "Checking the installation validity", "dd/d6a/sdk_environment.html#checking_the_installation_validity", null ],
      [ "Flashing devices", "dd/d6a/sdk_environment.html#flashing_guideline", [
        [ "Flashing Nordic nRF52 devices", "dd/d6a/sdk_environment.html#flashing_nordic_nrf52_devices", null ],
        [ "Flashing Silicon Labs Devices", "dd/d6a/sdk_environment.html#flashing_silabs_devices", null ]
      ] ],
      [ "Resources on Nordic nRF52832 & nRF52840", "dd/d6a/sdk_environment.html#nordic_nrf52832_nrf52840", [
        [ "Flash Memory available for application on nRF52", "dd/d6a/sdk_environment.html#flash_memory_nrf52", null ],
        [ "RAM Memory available for application on nRF52", "dd/d6a/sdk_environment.html#ram_memory_nrf52", null ],
        [ "Peripherals accessible by stack only", "dd/d6a/sdk_environment.html#peripherals_accessible_by_stack_only", null ],
        [ "Peripherals Shared between the stack and the application", "dd/d6a/sdk_environment.html#peripherals_shared_between_the_stack_and_the_application", null ],
        [ "Peripherals available for the application", "dd/d6a/sdk_environment.html#peripherals_available_for_the_application", null ]
      ] ],
      [ "Resources on EFR32xG12", "dd/d6a/sdk_environment.html#efr32xg12", [
        [ "Flash Memory available for application on EFR32", "dd/d6a/sdk_environment.html#flash_memory_efr32", null ],
        [ "RAM Memory available for application on EFR32", "dd/d6a/sdk_environment.html#ram_memory_efr32", null ],
        [ "Peripherals accessible by stack only", "dd/d6a/sdk_environment.html#peripherals_accessible_by_stack_only2", null ],
        [ "Peripherals Shared between the stack and the application", "dd/d6a/sdk_environment.html#peripherals_shared_between_the_stack_and_the_application2", null ],
        [ "Peripherals available for the application", "dd/d6a/sdk_environment.html#peripherals_available_for_the_application2", null ]
      ] ]
    ] ],
    [ "SDK file structure", "d3/d03/sdk_file_structure.html", [
      [ "api/ Libraries offered by stack", "d3/d03/sdk_file_structure.html#api_folder", [
        [ "Application and Library Versioning", "d3/d03/sdk_file_structure.html#application_and_library_versioning", null ]
      ] ],
      [ "board/ Board definitions", "d3/d03/sdk_file_structure.html#board_folder", [
        [ "converter configuration", "d3/d03/sdk_file_structure.html#DCDC", null ]
      ] ],
      [ "bootloader/ Bootloader configuration", "d3/d03/sdk_file_structure.html#bootloader_folder", null ],
      [ "debug/ Debug printing", "d3/d03/sdk_file_structure.html#debug_folder", null ],
      [ "doc/ Doxygen documentation", "d3/d03/sdk_file_structure.html#doc_folder", null ],
      [ "image/ Firmware images folder", "d3/d03/sdk_file_structure.html#image_folder", null ],
      [ "libraries/ SDK Internal API", "d3/d03/sdk_file_structure.html#libraries_folder", null ],
      [ "makefile", "d3/d03/sdk_file_structure.html#makefile", null ],
      [ "mcu/ Low level hardware services (HAL)", "d3/d03/sdk_file_structure.html#mcu_folder", [
        [ "mcu/common Common MCU files", "d3/d03/sdk_file_structure.html#mcu_common", [
          [ "start.c", "d3/d03/sdk_file_structure.html#mcu_common_start_c", null ]
        ] ],
        [ "mcu/hal_api Low level hardware API", "d3/d03/sdk_file_structure.html#mcu_hal_api", null ],
        [ "mcu/<hardware>", "d3/d03/sdk_file_structure.html#mcu_specific_files", null ],
        [ "Linker file", "d3/d03/sdk_file_structure.html#linker_file", [
          [ "Flash Memory", "d3/d03/sdk_file_structure.html#flash_memory", null ],
          [ "RAM memory", "d3/d03/sdk_file_structure.html#ram_memory", null ]
        ] ]
      ] ],
      [ "projects/ Doxygen files", "d3/d03/sdk_file_structure.html#project_folder", null ],
      [ "source/ Application examples", "d3/d03/sdk_file_structure.html#source_folder", [
        [ "app.c", "d3/d03/sdk_file_structure.html#source_app_c", null ],
        [ "config.mk", "d3/d03/sdk_file_structure.html#source_config_mk", [
          [ "app_specific_area_id", "d3/d03/sdk_file_structure.html#app_specific_area_id", null ],
          [ "Application version", "d3/d03/sdk_file_structure.html#app_version", null ],
          [ "TARGET_BOARDS", "d3/d03/sdk_file_structure.html#app_target_boards", null ]
        ] ],
        [ "makefile", "d3/d03/sdk_file_structure.html#source_makefile", [
          [ "APP_PRINTING", "d3/d03/sdk_file_structure.html#source_makefile_app_printing", null ],
          [ "APP_SCHEDULER", "d3/d03/sdk_file_structure.html#source_makefile_app_scheduler", null ],
          [ "CFLAGS", "d3/d03/sdk_file_structure.html#source_makefile_cflags", null ],
          [ "HAL_BUTTON", "d3/d03/sdk_file_structure.html#source_makefile_hal_button", null ],
          [ "HAL_HW_DELAY", "d3/d03/sdk_file_structure.html#source_makefile_hal_hw_delay", null ],
          [ "HAL_I2C", "d3/d03/sdk_file_structure.html#source_makefile_hal_i2c", null ],
          [ "HAL_LED", "d3/d03/sdk_file_structure.html#source_makefile_hal_led", null ],
          [ "HAL_PERSISTENT_MEMORY", "d3/d03/sdk_file_structure.html#source_makefile_hal_persistent_memory", null ],
          [ "HAL_SPI", "d3/d03/sdk_file_structure.html#source_makefile_hal_spi", null ],
          [ "HAL_UART", "d3/d03/sdk_file_structure.html#source_makefile_hal_uart", null ],
          [ "INCLUDES", "d3/d03/sdk_file_structure.html#source_makefile_includes", null ],
          [ "LIBS", "d3/d03/sdk_file_structure.html#source_makefile_libs", null ],
          [ "SHARED_LIBDATA", "d3/d03/sdk_file_structure.html#source_makefile_shared_libdata", null ],
          [ "SRCS", "d3/d03/sdk_file_structure.html#source_makefile_srcs", null ],
          [ "SW_AES", "d3/d03/sdk_file_structure.html#source_makefile_sw_aes", null ]
        ] ]
      ] ],
      [ "tools/ Various tools used in build process", "d3/d03/sdk_file_structure.html#tools_folder", [
        [ "genscratchpad.py", "d3/d03/sdk_file_structure.html#genscratchpad_py", null ],
        [ "INI_FILE", "d3/d03/sdk_file_structure.html#config_mk_ini_file", null ]
      ] ],
      [ "utils/ Utility and helper services", "d3/d03/sdk_file_structure.html#util_folder", null ]
    ] ],
    [ "Wirepas Mesh Concepts", "d5/df0/concepts.html", [
      [ "Application Configuration Data", "d5/df0/concepts.html#appconfig", null ],
      [ "Node addressing", "d5/df0/concepts.html#addressing", null ],
      [ "Source and Destination Endpoints", "d5/df0/concepts.html#endpoint", null ],
      [ "Communication Directions", "d5/df0/concepts.html#direction", null ]
    ] ],
    [ "Data Structures", "annotated.html", "annotated" ],
    [ "Data Structure Index", "classes.html", null ],
    [ "Data Fields", "functions.html", [
      [ "All", "functions.html", "functions_dup" ],
      [ "Variables", "functions_vars.html", "functions_vars" ]
    ] ],
    [ "File List", "files.html", "files" ],
    [ "Globals", "globals.html", [
      [ "All", "globals.html", "globals_dup" ],
      [ "Functions", "globals_func.html", "globals_func" ],
      [ "Variables", "globals_vars.html", null ],
      [ "Typedefs", "globals_type.html", "globals_type" ],
      [ "Enumerations", "globals_enum.html", null ],
      [ "Enumerator", "globals_eval.html", "globals_eval" ],
      [ "Macros", "globals_defs.html", "globals_defs" ]
    ] ],
    [ "Example applications", "examples.html", "examples" ]
  ] ]
];

var NAVTREEINDEX =
[
"annotated.html",
"d2/d9e/cbor_8h.html#a65aa4315fb9e17b2d070d96aa2f30e03",
"d2/dbd/data_8h.html#acc7ee0a7ec28292d2c1be47c4a23f78d",
"d5/d29/ublox__b204_2board_8h.html#a0af72e93971553bc01429840dbcede95",
"d8/d4b/advertiser_8h.html#a35397bf14e1b5c7390cb6cf9ba9c9ec9",
"da/d90/ruuvi_evk_2app_8c-example.html",
"db/de4/otap_8h.html#a57696c59349ed7140e3f27834e94dd91",
"dd/d2c/bl__interface_8h.html#ad44b615021ed3ccb734fcaf583ef4a03",
"dd/ddd/silabs__brd4254a_2board_8h.html#aca8fcd9d16228b415f8158d08dfa416e",
"df/d8f/updater_8h.html#aebb70c2aab3407a9f05334c47131a43b"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';