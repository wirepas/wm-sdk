var bl__interface_8h =
[
    [ "bl_flash_info_t", "dd/d2c/bl__interface_8h.html#d3/d91/structbl__flash__info__t", [
      [ "byte_write_call_time", "dd/d2c/bl__interface_8h.html#a8fb09e00542be47a78c14dfa208e4c1e", null ],
      [ "byte_write_time", "dd/d2c/bl__interface_8h.html#a5c8c5fbb4b56f5d44cb03b3cdf30749f", null ],
      [ "erase_sector_size", "dd/d2c/bl__interface_8h.html#af7004f5b20e3ba6d15d646de90981cb6", null ],
      [ "flash_size", "dd/d2c/bl__interface_8h.html#aaf8d6e5eae9a9544614ad8b7a6718478", null ],
      [ "is_busy_call_time", "dd/d2c/bl__interface_8h.html#a3cce340593a310f60cbd24a2e611c0bf", null ],
      [ "page_write_call_time", "dd/d2c/bl__interface_8h.html#ae02544f600ba251e1c03712b2fce90ff", null ],
      [ "page_write_time", "dd/d2c/bl__interface_8h.html#a9fc0b8defd420744f18e5ee478f0af6b", null ],
      [ "sector_erase_call_time", "dd/d2c/bl__interface_8h.html#ad16d4dc3fe2433035df69fc4359355fe", null ],
      [ "sector_erase_time", "dd/d2c/bl__interface_8h.html#ad8bef042481e7be25f2152ae5fcb025b", null ],
      [ "write_alignment", "dd/d2c/bl__interface_8h.html#aaae3f76bb2618fc8b0ec8581339e329e", null ],
      [ "write_page_size", "dd/d2c/bl__interface_8h.html#a7620fa44fbe0000c302271facd29eef4", null ]
    ] ],
    [ "bl_memory_area_info_t", "dd/d2c/bl__interface_8h.html#db/db8/structbl__memory__area__info__t", [
      [ "area_id", "dd/d2c/bl__interface_8h.html#a81960fb2435617be18814941ef8dbdf5", null ],
      [ "area_physical_address", "dd/d2c/bl__interface_8h.html#ac6f27fdedcf2c9a28b933a964b92b002", null ],
      [ "area_size", "dd/d2c/bl__interface_8h.html#ab5bf64913db2d60d783c7fe279f5804f", null ],
      [ "external_flash", "dd/d2c/bl__interface_8h.html#a76d7d81cba0b73beced4b1c6f56c7ef6", null ],
      [ "flash", "dd/d2c/bl__interface_8h.html#a3819ab9bd420adcf9b7e2df3671190cf", null ],
      [ "has_header", "dd/d2c/bl__interface_8h.html#aa920dc7dbbfa201dba2cbf981dcbb70a", null ],
      [ "type", "dd/d2c/bl__interface_8h.html#a6563248a292e1f03988daea7933ff724", null ]
    ] ],
    [ "bl_memory_area_header_t", "dd/d2c/bl__interface_8h.html#da/d59/structbl__memory__area__header__t", [
      [ "crc", "dd/d2c/bl__interface_8h.html#aa60093a9a5d5d17864cfda66c47733e3", null ],
      [ "devel", "dd/d2c/bl__interface_8h.html#a8ba0daaeb065965b4a90a46d5710a278", null ],
      [ "length", "dd/d2c/bl__interface_8h.html#aebb70c2aab3407a9f05334c47131a43b", null ],
      [ "maint", "dd/d2c/bl__interface_8h.html#ab100c9f998e75a52cb8af37fd3050cc2", null ],
      [ "major", "dd/d2c/bl__interface_8h.html#a5bd4e4c943762926c8f653b6224cced2", null ],
      [ "minor", "dd/d2c/bl__interface_8h.html#ae2f416b0a34b7beb4ed3873d791ac393", null ],
      [ "seq", "dd/d2c/bl__interface_8h.html#a3f47451edd2274525a52e40f4e26c399", null ]
    ] ],
    [ "bl_scrat_info_t", "dd/d2c/bl__interface_8h.html#d2/d8c/structbl__scrat__info__t", [
      [ "area_length", "dd/d2c/bl__interface_8h.html#a12f2d8ef6086299a48851ef0cc8cd229", null ],
      [ "crc", "dd/d2c/bl__interface_8h.html#aa60093a9a5d5d17864cfda66c47733e3", null ],
      [ "dedicated", "dd/d2c/bl__interface_8h.html#aef2e3a12f0e858f36657ee293a600269", null ],
      [ "erase_time", "dd/d2c/bl__interface_8h.html#a203b61605e331e52697c4ea1a7cc24f3", null ],
      [ "flags", "dd/d2c/bl__interface_8h.html#aa2585d779da0ab21273a8d92de9a0ebe", null ],
      [ "length", "dd/d2c/bl__interface_8h.html#aebb70c2aab3407a9f05334c47131a43b", null ],
      [ "seq", "dd/d2c/bl__interface_8h.html#a3f47451edd2274525a52e40f4e26c399", null ],
      [ "status", "dd/d2c/bl__interface_8h.html#ade20423e91627f07e610924cb0081623", null ],
      [ "type", "dd/d2c/bl__interface_8h.html#ad44b615021ed3ccb734fcaf583ef4a03", null ]
    ] ],
    [ "memory_area_services_t", "dd/d2c/bl__interface_8h.html#d3/df8/structmemory__area__services__t", [
      [ "getAreaHeader", "dd/d2c/bl__interface_8h.html#a52acbd1af3182212912a3c8a201ca3c9", null ],
      [ "getAreaInfo", "dd/d2c/bl__interface_8h.html#a3fb4f66814ec5b6cf9f0229ee247327e", null ],
      [ "getAreaList", "dd/d2c/bl__interface_8h.html#a1ceabc7e38723528918a15e3a40b0e6d", null ],
      [ "getIdfromType", "dd/d2c/bl__interface_8h.html#aca95cc4203183dad368b30bfaa7e30a3", null ],
      [ "isBusy", "dd/d2c/bl__interface_8h.html#aff72347ad1a2bd449777bf43a6e6c478", null ],
      [ "startErase", "dd/d2c/bl__interface_8h.html#ad3b71abd956f731e6eaa3cea9ebb86a9", null ],
      [ "startRead", "dd/d2c/bl__interface_8h.html#ad97a369d9544e47df808c07dec54730f", null ],
      [ "startWrite", "dd/d2c/bl__interface_8h.html#a2fa3d5f5bb582a9569e70c812dc88d73", null ]
    ] ],
    [ "scratchpad_services_t", "dd/d2c/bl__interface_8h.html#d3/d20/structscratchpad__services__t", [
      [ "begin", "dd/d2c/bl__interface_8h.html#afc3fbc94ba0f0c2c705c9630849db1d1", null ],
      [ "clear", "dd/d2c/bl__interface_8h.html#ac32c4794cb17887d7419167e6ab3f838", null ],
      [ "getInfo", "dd/d2c/bl__interface_8h.html#ab0ca607d89ba3a5caf058d8387fda4a2", null ],
      [ "getValidity", "dd/d2c/bl__interface_8h.html#a6dae030dc2cfe690533465a9d0f05c1f", null ],
      [ "read", "dd/d2c/bl__interface_8h.html#ab9b0ec6026ac2025e55fb44884f466c4", null ],
      [ "setBootable", "dd/d2c/bl__interface_8h.html#a568991e7dc3a7b19d97d99b37c4054cf", null ],
      [ "write", "dd/d2c/bl__interface_8h.html#a92c7092dca00a14267567097a06044fa", null ]
    ] ],
    [ "bl_interface_t", "dd/d2c/bl__interface_8h.html#db/d6f/structbl__interface__t", [
      [ "memory_area_services_p", "dd/d2c/bl__interface_8h.html#a31cf9805894e88c65d6ec0611d8d10e4", null ],
      [ "scratchpad_services_p", "dd/d2c/bl__interface_8h.html#a618c3916d715a1709848abadfb64bd21", null ],
      [ "version", "dd/d2c/bl__interface_8h.html#acd99bb05ca015e7d74448acb1deba7ca", null ]
    ] ],
    [ "bl_memory_area_getAreaHeader_f", "dd/d2c/bl__interface_8h.html#a9aee550ac0d15d2569c2090e8786dd17", null ],
    [ "bl_memory_area_getAreaInfo_f", "dd/d2c/bl__interface_8h.html#a5fb3900f9b422fb576ff07a737a1de66", null ],
    [ "bl_memory_area_getAreaList_f", "dd/d2c/bl__interface_8h.html#a2a0621cceb3524809381675f1403429c", null ],
    [ "bl_memory_area_getIdfromType_f", "dd/d2c/bl__interface_8h.html#a0d576b9c7b5fd16e56b35e2423723741", null ],
    [ "bl_memory_area_id_t", "dd/d2c/bl__interface_8h.html#aca9fb4e5bd1294dd6c03fd2d87881eaa", null ],
    [ "bl_memory_area_isBusy_f", "dd/d2c/bl__interface_8h.html#ad2dca2b4052fa7e38a7617ed2e4cc7d9", null ],
    [ "bl_memory_area_startErase_f", "dd/d2c/bl__interface_8h.html#aab92cc49d01944ed1b8184df2fa92eea", null ],
    [ "bl_memory_area_startRead_f", "dd/d2c/bl__interface_8h.html#acbb819507d940a4e44b7c919079b173f", null ],
    [ "bl_memory_area_startWrite_f", "dd/d2c/bl__interface_8h.html#ab55515696634e18f590e8f54cb7ab461", null ],
    [ "bl_scrat_begin_f", "dd/d2c/bl__interface_8h.html#a8efcf129eb910cf2c6b8177e4afea5b7", null ],
    [ "bl_scrat_clear_f", "dd/d2c/bl__interface_8h.html#a46785bd9106188b60252c64f7613f419", null ],
    [ "bl_scrat_getInfo_f", "dd/d2c/bl__interface_8h.html#aa691912b1f952358843a0d2b7b77d642", null ],
    [ "bl_scrat_getValidity_f", "dd/d2c/bl__interface_8h.html#aaee1e05709b70d3c30b1b5e59aa139cf", null ],
    [ "bl_scrat_read_f", "dd/d2c/bl__interface_8h.html#a340ed48afb839a9e8fe28106e805503a", null ],
    [ "bl_scrat_setBootable_f", "dd/d2c/bl__interface_8h.html#adff3b843f46e4448e823aa862ad5eead", null ],
    [ "bl_scrat_write_f", "dd/d2c/bl__interface_8h.html#ac65bde741bce8da8e08d37416e367f4f", null ],
    [ "bl_interface_res_e", "dd/d2c/bl__interface_8h.html#a9a1d6078d92f4d46b59e7f80ef16b898", [
      [ "BL_RES_OK", "dd/d2c/bl__interface_8h.html#a9a1d6078d92f4d46b59e7f80ef16b898a7a604f7aa52ae0e5d1ec50d7a2f1bad3", null ],
      [ "BL_RES_ERROR", "dd/d2c/bl__interface_8h.html#a9a1d6078d92f4d46b59e7f80ef16b898a1a28228a2c91c43fd8ae40e7c992a15f", null ],
      [ "BL_RES_BUSY", "dd/d2c/bl__interface_8h.html#a9a1d6078d92f4d46b59e7f80ef16b898a2ee0384cd2c081a6fc24112d192d5b41", null ],
      [ "BL_RES_NODRIVER", "dd/d2c/bl__interface_8h.html#a9a1d6078d92f4d46b59e7f80ef16b898aa33b97eaed0079f77319da86ba853228", null ],
      [ "BL_RES_PARAM", "dd/d2c/bl__interface_8h.html#a9a1d6078d92f4d46b59e7f80ef16b898afb4414d01fa614225f3716184ac8c9d3", null ],
      [ "BL_RES_INVALID_AREA", "dd/d2c/bl__interface_8h.html#a9a1d6078d92f4d46b59e7f80ef16b898a44dab6e6be58bd1cdba9747cbb5fb47a", null ],
      [ "BL_RES_TIMEOUT", "dd/d2c/bl__interface_8h.html#a9a1d6078d92f4d46b59e7f80ef16b898a2ab1d6279ea7f577784720607c222cef", null ],
      [ "BL_RES_INVALID_STATE", "dd/d2c/bl__interface_8h.html#a9a1d6078d92f4d46b59e7f80ef16b898a5dac3bfc8056c46f46bf86fb4dcaa225", null ]
    ] ],
    [ "bl_memory_area_type_e", "dd/d2c/bl__interface_8h.html#a827b71e6d5cdcaa075983c98ddc7563d", [
      [ "BL_MEM_AREA_TYPE_BOOTLOADER", "dd/d2c/bl__interface_8h.html#a827b71e6d5cdcaa075983c98ddc7563da154df4370ce1d216531e69893a307807", null ],
      [ "BL_MEM_AREA_TYPE_STACK", "dd/d2c/bl__interface_8h.html#a827b71e6d5cdcaa075983c98ddc7563da5570cac91bc7a272b41590de7d8502ff", null ],
      [ "BL_MEM_AREA_TYPE_APPLICATION", "dd/d2c/bl__interface_8h.html#a827b71e6d5cdcaa075983c98ddc7563da148901414d194417bf14f1fb073654fb", null ],
      [ "BL_MEM_AREA_TYPE_PERSISTENT", "dd/d2c/bl__interface_8h.html#a827b71e6d5cdcaa075983c98ddc7563dad173a9da687288900137f0504eec1e78", null ],
      [ "BL_MEM_AREA_TYPE_SCRATCHPAD", "dd/d2c/bl__interface_8h.html#a827b71e6d5cdcaa075983c98ddc7563da8f99c8c5cb79e89fc6a5820446db2790", null ],
      [ "BL_MEM_AREA_TYPE_USER", "dd/d2c/bl__interface_8h.html#a827b71e6d5cdcaa075983c98ddc7563dab805c0d10c777c77f21b80944278f296", null ]
    ] ],
    [ "bl_scrat_type_e", "dd/d2c/bl__interface_8h.html#abcd1a4851130b269c0f236bde1ff9568", [
      [ "BL_SCRAT_TYPE_BLANK", "dd/d2c/bl__interface_8h.html#abcd1a4851130b269c0f236bde1ff9568a86cc25ea6cf3de5ca7abaa37bfc61f40", null ],
      [ "BL_SCRAT_TYPE_PRESENT", "dd/d2c/bl__interface_8h.html#abcd1a4851130b269c0f236bde1ff9568a2bc031c76ae08a4df5c2ec7841a02495", null ],
      [ "BL_SCRAT_TYPE_PROCESS", "dd/d2c/bl__interface_8h.html#abcd1a4851130b269c0f236bde1ff9568a83904e0abe6bce1c65f12f5d397b30e5", null ]
    ] ],
    [ "bl_scrat_valid_e", "dd/d2c/bl__interface_8h.html#a5a648cb2d7d4e0e4b98ef5a5463b453a", [
      [ "BL_SCRAT_IS_UNKNOWN", "dd/d2c/bl__interface_8h.html#a5a648cb2d7d4e0e4b98ef5a5463b453aa14cc7a1cc3a16d2e231bda057e29179b", null ],
      [ "BL_SCRAT_IS_CLEAR", "dd/d2c/bl__interface_8h.html#a5a648cb2d7d4e0e4b98ef5a5463b453aabc72013fa33efc5452a2e6c893c44c2f", null ],
      [ "BL_SCRAT_IS_NOTAG", "dd/d2c/bl__interface_8h.html#a5a648cb2d7d4e0e4b98ef5a5463b453aa4a2269a7824261062eced9a1f0f31caa", null ],
      [ "BL_SCRAT_IS_INVALID_HEADER", "dd/d2c/bl__interface_8h.html#a5a648cb2d7d4e0e4b98ef5a5463b453aa160116d4d83cbb385a9da2e26af0cf19", null ],
      [ "BL_SCRAT_IS_INVALID_CRC", "dd/d2c/bl__interface_8h.html#a5a648cb2d7d4e0e4b98ef5a5463b453aadb8dcb2904d35ef7a628c93361d6ab90", null ],
      [ "BL_SCRAT_IS_INVALID", "dd/d2c/bl__interface_8h.html#a5a648cb2d7d4e0e4b98ef5a5463b453aa50a16b24a19842fdf5810883fc631bb9", null ],
      [ "BL_SCRAT_IS_VALID", "dd/d2c/bl__interface_8h.html#a5a648cb2d7d4e0e4b98ef5a5463b453aa2203300604771cbe808ee9fd420b1427", null ]
    ] ],
    [ "bl_scrat_write_status_e", "dd/d2c/bl__interface_8h.html#a89973f10c4faed0bd6bd5da92853f566", [
      [ "BL_SCRAT_WRITE_STATUS_OK", "dd/d2c/bl__interface_8h.html#a89973f10c4faed0bd6bd5da92853f566abc90e14faff21f4230d5684e5f21b137", null ],
      [ "BL_SCRAT_WRITE_STATUS_COMPLETED_OK", "dd/d2c/bl__interface_8h.html#a89973f10c4faed0bd6bd5da92853f566a197e69190c6c3a4db60dc1a533f640b9", null ],
      [ "BL_SCRAT_WRITE_STATUS_COMPLETED_ERROR", "dd/d2c/bl__interface_8h.html#a89973f10c4faed0bd6bd5da92853f566af778baf12a8eb8231e7035448c6e1ab2", null ],
      [ "BL_SCRAT_WRITE_STATUS_NOT_ONGOING", "dd/d2c/bl__interface_8h.html#a89973f10c4faed0bd6bd5da92853f566ac6b645d4199cebb678289fa14b332ccf", null ],
      [ "BL_SCRAT_WRITE_STATUS_INVALID_START", "dd/d2c/bl__interface_8h.html#a89973f10c4faed0bd6bd5da92853f566a793dc6e1e2858f7bb52d4ca9e13d6116", null ],
      [ "BL_SCRAT_WRITE_STATUS_INVALID_NUM_BYTES", "dd/d2c/bl__interface_8h.html#a89973f10c4faed0bd6bd5da92853f566a8299272d49a2a7cabf07e499ee57485e", null ],
      [ "BL_SCRAT_WRITE_STATUS_INVALID_HEADER", "dd/d2c/bl__interface_8h.html#a89973f10c4faed0bd6bd5da92853f566a0d74e804cd93f0e00d5cafc53ba494cd", null ],
      [ "BL_SCRAT_WRITE_STATUS_INVALID_NULL_BYTES", "dd/d2c/bl__interface_8h.html#a89973f10c4faed0bd6bd5da92853f566a5f9b8b5d216d4d02bfba907ae9f49a7f", null ],
      [ "BL_SCRAT_WRITE_STATUS_FLASH_ERROR", "dd/d2c/bl__interface_8h.html#a89973f10c4faed0bd6bd5da92853f566ace0b656348cce6c4f9a8ecd7080fb623", null ]
    ] ],
    [ "BL_MEMORY_AREA_MAX_AREAS", "dd/d2c/bl__interface_8h.html#a35b0af8a72e4be9715042ea14c5e8ece", null ],
    [ "BL_MEMORY_AREA_UNDEFINED", "dd/d2c/bl__interface_8h.html#a935dcebd09fec384e242e1f45b02ac9d", null ]
];