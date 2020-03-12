var state_8h =
[
    [ "app_lib_state_nbor_info_t", "db/d3b/state_8h.html#d1/db3/structapp__lib__state__nbor__info__t", [
      [ "address", "db/d3b/state_8h.html#ac0d31ca829f934cccd89f8054e02773e", null ],
      [ "channel", "db/d3b/state_8h.html#a715f5cb061d11eb75981741eda4dafcd", null ],
      [ "cost", "db/d3b/state_8h.html#a403b6f77447a921497e186aff3107823", null ],
      [ "last_update", "db/d3b/state_8h.html#ad4342e40daf8d6d7b1e7ef34bc9b332c", null ],
      [ "link_reliability", "db/d3b/state_8h.html#aa1c1e61cb5a2c63c5ea2df4bf954f35e", null ],
      [ "norm_rssi", "db/d3b/state_8h.html#a1ede7a83e9e8052506010057c6bf3353", null ],
      [ "rx_power", "db/d3b/state_8h.html#a5394085ff97c98685b8c004dfac8c229", null ],
      [ "tx_power", "db/d3b/state_8h.html#a933494b037d387c3ebda7cdba7aee20b", null ],
      [ "type", "db/d3b/state_8h.html#a1d127017fb298b889f4ba24752d08b8e", null ]
    ] ],
    [ "app_lib_state_nbor_list_t", "db/d3b/state_8h.html#d2/d8b/structapp__lib__state__nbor__list__t", [
      [ "nbors", "db/d3b/state_8h.html#a8c89ee9781052eff7f801dc9970f9a2b", null ],
      [ "number_nbors", "db/d3b/state_8h.html#a2a7b7d438e30f610f3e562f99bedb55e", null ]
    ] ],
    [ "app_lib_state_beacon_rx_t", "db/d3b/state_8h.html#d7/dc8/structapp__lib__state__beacon__rx__t", [
      [ "address", "db/d3b/state_8h.html#adeb8e3ced78abf9ec8931cd0d7a2d2c0", null ],
      [ "cost", "db/d3b/state_8h.html#a403b6f77447a921497e186aff3107823", null ],
      [ "is_ll", "db/d3b/state_8h.html#a0fd32c72c2f1c2e6b715c14fbcc41a0c", null ],
      [ "is_sink", "db/d3b/state_8h.html#a6149885080d7e0559447dcbccedcfe3b", null ],
      [ "rssi", "db/d3b/state_8h.html#ac4ea00add191a3290e428c5fdc838d1f", null ],
      [ "txpower", "db/d3b/state_8h.html#ad701b4a3eb1ea31ef7721e2656e7b7a8", null ],
      [ "type", "db/d3b/state_8h.html#a1d127017fb298b889f4ba24752d08b8e", null ]
    ] ],
    [ "app_lib_state_hops_adjust_t", "db/d3b/state_8h.html#dc/d6e/structapp__lib__state__hops__adjust__t", [
      [ "address", "db/d3b/state_8h.html#adeb8e3ced78abf9ec8931cd0d7a2d2c0", null ],
      [ "hops_left", "db/d3b/state_8h.html#acf176bd8b11c3f2a4fd06463260cac25", null ]
    ] ],
    [ "app_lib_state_route_info_t", "db/d3b/state_8h.html#d3/d74/structapp__lib__state__route__info__t", [
      [ "channel", "db/d3b/state_8h.html#a7dd60397aa2d574e39a1c1b1bd6ab527", null ],
      [ "cost", "db/d3b/state_8h.html#a403b6f77447a921497e186aff3107823", null ],
      [ "next_hop", "db/d3b/state_8h.html#a653da2f157ea3e19bd8b6d5428d53df6", null ],
      [ "sink", "db/d3b/state_8h.html#a47db745cdbea97be8e0a296e33fadfc7", null ],
      [ "state", "db/d3b/state_8h.html#a42584f0a814c647a1d733326491a5e43", null ]
    ] ],
    [ "app_lib_state_t", "db/d3b/state_8h.html#d6/d1c/structapp__lib__state__t", [
      [ "getAccessCycle", "db/d3b/state_8h.html#a102ebd48fb4fe9465cb0be239b6c6131", null ],
      [ "getDiagInterval", "db/d3b/state_8h.html#a9d5a8caf2ff52aa8bde8c4efc6f4e7dd", null ],
      [ "getEnergy", "db/d3b/state_8h.html#ab2f3d28cd2e66d7aec551063d0985f9d", null ],
      [ "getNbors", "db/d3b/state_8h.html#ac9df9a80c1a0436c905becf059d42140", null ],
      [ "getRouteCount", "db/d3b/state_8h.html#ab925de033c92a5e6dcf7003dd0bd9099", null ],
      [ "getRouteInfo", "db/d3b/state_8h.html#a442fb84dfbc35e4764df3010da81dc13", null ],
      [ "getSinkCost", "db/d3b/state_8h.html#a56b210c006bde33677ea063613e601d8", null ],
      [ "getStackState", "db/d3b/state_8h.html#ac17a6229495f670e628bc37d8078fc3f", null ],
      [ "setEnergy", "db/d3b/state_8h.html#aaef7c03a0b02d86d78ec7bccacacc4f2", null ],
      [ "setHopsLeftCb", "db/d3b/state_8h.html#aeb72d6f4f5c8d6e9bd5721386afc28ee", null ],
      [ "setOnBeaconCb", "db/d3b/state_8h.html#a209670d47cfe0e627f4abd3b5dc2d9cb", null ],
      [ "setOnScanNborsCb", "db/d3b/state_8h.html#ad714526ae1f1bf0d8cd0effcd1f5848f", null ],
      [ "setRouteCb", "db/d3b/state_8h.html#ac513718a2b4a24ee87b6111ba5380462", null ],
      [ "setSinkCost", "db/d3b/state_8h.html#ad387be6eccd064044799cf2fecaab303", null ],
      [ "startScanNbors", "db/d3b/state_8h.html#aba04929155b94d47efba6d825956798d", null ],
      [ "startStack", "db/d3b/state_8h.html#a6179954360d3375cc35175bf915f2b05", null ],
      [ "stopStack", "db/d3b/state_8h.html#a4f55f4c19aade4513ce938d9a13c1689", null ]
    ] ],
    [ "app_lib_state_adjust_hops_cb_f", "db/d3b/state_8h.html#a3267f9bceba68a533913c17735c3229c", null ],
    [ "app_lib_state_get_access_cycle_f", "db/d3b/state_8h.html#ae9b5748c1ee30d8d0bc255dd823b1b43", null ],
    [ "app_lib_state_get_diag_interval_f", "db/d3b/state_8h.html#a4bd2f57d2d2c3dd677a975f9357a4cfc", null ],
    [ "app_lib_state_get_energy_f", "db/d3b/state_8h.html#a7d91ea6e468a4fba5865d08c517ae710", null ],
    [ "app_lib_state_get_nbors_f", "db/d3b/state_8h.html#a3d59d4e9fe0da787cf8d3cb3dfd6f27c", null ],
    [ "app_lib_state_get_route_count_f", "db/d3b/state_8h.html#a702a884e76fd9995ce23966c364d3da1", null ],
    [ "app_lib_state_get_route_f", "db/d3b/state_8h.html#a61feb96e0a7184cc92deb87a8c4ff73d", null ],
    [ "app_lib_state_get_sink_cost_f", "db/d3b/state_8h.html#ae3fd6f2213be070de56a6bba9faa7c97", null ],
    [ "app_lib_state_get_stack_state_f", "db/d3b/state_8h.html#a4fe40f470377e62840ef486a6487bbf8", null ],
    [ "app_lib_state_on_beacon_cb_f", "db/d3b/state_8h.html#a782056251116f37a1ee0f731d2cfd864", null ],
    [ "app_lib_state_on_scan_nbors_cb_f", "db/d3b/state_8h.html#afb6cbd7d805f4f02f6adce60a2639549", null ],
    [ "app_lib_state_route_changed_cb_f", "db/d3b/state_8h.html#a852c311feba83121773686ae871a89d1", null ],
    [ "app_lib_state_set_adjust_hops_cb_f", "db/d3b/state_8h.html#a5ae00c16838dd21ddc7e3560b4283b9c", null ],
    [ "app_lib_state_set_energy_f", "db/d3b/state_8h.html#ac0ce120cbfb8f7e5ba1acad712279d6a", null ],
    [ "app_lib_state_set_on_beacon_cb_f", "db/d3b/state_8h.html#af0ab19ea11433eac7456960b7652e323", null ],
    [ "app_lib_state_set_on_scan_nbors_with_type_cb_f", "db/d3b/state_8h.html#abdcd384dad0742bd6716564f8d67240e", null ],
    [ "app_lib_state_set_route_cb_f", "db/d3b/state_8h.html#a828287002dce7a99dca31ac61093db3c", null ],
    [ "app_lib_state_set_sink_cost_f", "db/d3b/state_8h.html#a391c58a53ecd5239664488e9c01dfc72", null ],
    [ "app_lib_state_start_scan_nbors_f", "db/d3b/state_8h.html#a3f005b84b93e45bc27baf71fef5d9320", null ],
    [ "app_lib_state_start_stack_f", "db/d3b/state_8h.html#a8b9ddf8d4323784f6a437060c30f31b1", null ],
    [ "app_lib_state_stop_stack_f", "db/d3b/state_8h.html#a39ad217d32c2986151820eb24c74e618", null ],
    [ "app_lib_state_beacon_type_e", "db/d3b/state_8h.html#a56dba5ba5a60c45f2340fcdd3b6f7279", [
      [ "APP_LIB_STATE_BEACON_TYPE_NB", "db/d3b/state_8h.html#a56dba5ba5a60c45f2340fcdd3b6f7279aa9ea15ec52506e425d96d427e2502ca3", null ],
      [ "APP_LIB_STATE_BEACON_TYPE_CB", "db/d3b/state_8h.html#a56dba5ba5a60c45f2340fcdd3b6f7279a1cbda5f8f60ab4b52257122148a94b16", null ]
    ] ],
    [ "app_lib_state_nbor_type_e", "db/d3b/state_8h.html#a2dc70e8eb3a0f15da5b9f9a44805fc01", [
      [ "APP_LIB_STATE_NEIGHBOR_IS_NEXT_HOP", "db/d3b/state_8h.html#a2dc70e8eb3a0f15da5b9f9a44805fc01a7e85991ebcbff20b19727da8bd6611f7", null ],
      [ "APP_LIB_STATE_NEIGHBOR_IS_MEMBER", "db/d3b/state_8h.html#a2dc70e8eb3a0f15da5b9f9a44805fc01a83defa796d19c363d6206839543bcefd", null ],
      [ "APP_LIB_STATE_NEIGHBOR_IS_CLUSTER", "db/d3b/state_8h.html#a2dc70e8eb3a0f15da5b9f9a44805fc01af6aa5411b85b3d879c251d433a1d8f56", null ]
    ] ],
    [ "app_lib_state_route_state_e", "db/d3b/state_8h.html#a512a55293699bdd367478f021aa13c6a", [
      [ "APP_LIB_STATE_ROUTE_STATE_INVALID", "db/d3b/state_8h.html#a512a55293699bdd367478f021aa13c6aa733d37f4e3ad3314eca05bb02ec6a8ad", null ],
      [ "APP_LIB_STATE_ROUTE_STATE_PENDING", "db/d3b/state_8h.html#a512a55293699bdd367478f021aa13c6aa504a1c158c8ee70e7c2f55d73b36b201", null ],
      [ "APP_LIB_STATE_ROUTE_STATE_VALID", "db/d3b/state_8h.html#a512a55293699bdd367478f021aa13c6aae131b8523ae385c533f3a1801f4458cb", null ]
    ] ],
    [ "app_lib_state_scan_nbors_type_e", "db/d3b/state_8h.html#ae829d4242b3afe89f01822fe660b62b7", [
      [ "APP_LIB_STATE_SCAN_NBORS_ALL", "db/d3b/state_8h.html#ae829d4242b3afe89f01822fe660b62b7a26113ac18cb4f59d9ba0fa0c9119adcc", null ],
      [ "APP_LIB_STATE_SCAN_NBORS_ONLY_REQUESTED", "db/d3b/state_8h.html#ae829d4242b3afe89f01822fe660b62b7a8bad4deb7660820ff7dbfdd36a87e675", null ]
    ] ],
    [ "app_lib_state_stack_state_e", "db/d3b/state_8h.html#ab534305bdd66195140b3f750b9fdf21d", [
      [ "APP_LIB_STATE_STARTED", "db/d3b/state_8h.html#ab534305bdd66195140b3f750b9fdf21dab3f2f3f400130619e71f1790296841b8", null ],
      [ "APP_LIB_STATE_STOPPED", "db/d3b/state_8h.html#ab534305bdd66195140b3f750b9fdf21da480e1999349ac7782a0983cf87565855", null ],
      [ "APP_LIB_STATE_NODE_ADDRESS_NOT_SET", "db/d3b/state_8h.html#ab534305bdd66195140b3f750b9fdf21da36109220a35bfbdbc48576de86dddd0d", null ],
      [ "APP_LIB_STATE_NETWORK_ADDRESS_NOT_SET", "db/d3b/state_8h.html#ab534305bdd66195140b3f750b9fdf21da1820eef823041dac0a6fe4431f034ecc", null ],
      [ "APP_LIB_STATE_NETWORK_CHANNEL_NOT_SET", "db/d3b/state_8h.html#ab534305bdd66195140b3f750b9fdf21da47e47f83a0bdfd800c41a61474025d0e", null ],
      [ "APP_LIB_STATE_ROLE_NOT_SET", "db/d3b/state_8h.html#ab534305bdd66195140b3f750b9fdf21dabbfa64aabfea60355a76a056d4b7926a", null ],
      [ "APP_LIB_STATE_APP_CONFIG_DATA_NOT_SET", "db/d3b/state_8h.html#ab534305bdd66195140b3f750b9fdf21daeb831b237a2f5894a8556220cf8cee1a", null ],
      [ "APP_LIB_STATE_ACCESS_DENIED", "db/d3b/state_8h.html#ab534305bdd66195140b3f750b9fdf21da7d2765458a04895714ec8bc566f20ced", null ]
    ] ],
    [ "APP_LIB_STATE_COST_UNKNOWN", "db/d3b/state_8h.html#aae0927a4946b7cd662ad3f2a0ab1995f", null ],
    [ "APP_LIB_STATE_INVALID_ROUTE_COST", "db/d3b/state_8h.html#ad57d94677d6e5717f3ca6e411c2a2d8c", null ],
    [ "APP_LIB_STATE_LINKREL_UNKNOWN", "db/d3b/state_8h.html#a6de103f15cc8258945c7f5f6847d174d", null ],
    [ "APP_LIB_STATE_NAME", "db/d3b/state_8h.html#ad42c937f109dd38844dd4a6a65a3b4d5", null ],
    [ "APP_LIB_STATE_VERSION", "db/d3b/state_8h.html#a7970435ad6c9d729f2c1b8aa7a0c6720", null ]
];