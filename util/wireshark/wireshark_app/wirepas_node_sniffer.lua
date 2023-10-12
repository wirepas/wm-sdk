wirepas_node_sniffer = Proto("wirepas_node_sniffer","Wirepas sniffer")
wirepas_node_sniffer.fields.src_add = ProtoField.uint32("wirepas_node_sniffer.src_add", "Source Address")
wirepas_node_sniffer.fields.dst_add = ProtoField.uint32("wirepas_node_sniffer.dst_add", "Destination Address")
wirepas_node_sniffer.fields.qos = ProtoField.uint8("wirepas_node_sniffer.qos", "QoS")
wirepas_node_sniffer.fields.src_ep = ProtoField.uint8("wirepas_node_sniffer.src_ep", "Source Endpoint")
wirepas_node_sniffer.fields.dst_ep = ProtoField.uint8("wirepas_node_sniffer.dst_ep", "Destination Endpoint")
wirepas_node_sniffer.fields.rssi = ProtoField.int8("wirepas_node_sniffer.rssi", "Rssi")
wirepas_node_sniffer.fields.delay = ProtoField.uint32("wirepas_node_sniffer.delay", "Delay")

-- create a function to dissect it
function wirepas_node_sniffer.dissector(buffer, pinfo, tree)
    HEADER_LEN = 16
    if buffer:len() > HEADER_LEN then
        local src_add = buffer(0,4)
        local dst_add = buffer(4,4)
        local qos = buffer(8,1)
        local src_ep = buffer(9,1)
        local dst_ep = buffer(10,1)
        local rssi = buffer(11,1)
        local delay = buffer(12,4)

        pinfo.cols.protocol = "Wirepas Raw"

        pinfo.cols.src = src_add:uint()
        pinfo.cols.dst = dst_add:uint()

        local t = tree:add(wirepas_node_sniffer, buffer(0,HEADER_LEN), "Wirepas Node Sniffer")
        t:add_le(wirepas_node_sniffer.fields.src_add, src_add)
        t:add_le(wirepas_node_sniffer.fields.dst_add, dst_add)
        t:add_le(wirepas_node_sniffer.fields.qos, qos)
        t:add_le(wirepas_node_sniffer.fields.src_ep, src_ep)
        t:add_le(wirepas_node_sniffer.fields.dst_ep, dst_ep)
        t:add_le(wirepas_node_sniffer.fields.rssi, rssi)
        t:add_le(wirepas_node_sniffer.fields.delay, delay)
        if (src_ep:uint() == 128 and dst_ep:uint() == 02) then
            Dissector.get("ipv6"):call(buffer(HEADER_LEN):tvb(), pinfo, tree)
        --elseif (src_ep:uint() == 1 and dst_ep:uint() == 1) then
        --    pinfo.cols.protocol = "Wirepas Custom app"
        end
    end
end

local wtap_encap_table = DissectorTable.get("wtap_encap")
wtap_encap_table:add(wtap.USER0, wirepas_node_sniffer)
