#include "protocol.h"
#include <cstring> // memcpy
#include "bitstream.h"


void send_join(ENetPeer *peer)
{
    BitStream bs;
    bs << E_CLIENT_TO_SERVER_JOIN;
    enet_peer_send(peer, 0, bs.to_enet_packet(ENET_PACKET_FLAG_RELIABLE));
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
    BitStream bs;
    bs << E_SERVER_TO_CLIENT_NEW_ENTITY << ent;
    enet_peer_send(peer, 0, bs.to_enet_packet(ENET_PACKET_FLAG_RELIABLE));
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
    BitStream bs;
    bs << E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY << eid;
    enet_peer_send(peer, 0, bs.to_enet_packet(ENET_PACKET_FLAG_RELIABLE));
}

void send_entity_input(ENetPeer *peer, uint16_t eid, float thr, float steer)
{
    BitStream bs;
    bs << E_CLIENT_TO_SERVER_INPUT << eid << thr << steer;
    enet_peer_send(peer, 0, bs.to_enet_packet(ENET_PACKET_FLAG_UNSEQUENCED));
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float ori)
{
    BitStream bs;
    bs << E_SERVER_TO_CLIENT_SNAPSHOT << eid << x << y << ori;
    enet_peer_send(peer, 0, bs.to_enet_packet(ENET_PACKET_FLAG_UNSEQUENCED));
}

void send_time_msec(ENetPeer *peer, uint32_t timeMsec)
{
    BitStream bs;
    bs << E_SERVER_TO_CLIENT_TIME_MSEC << timeMsec;
    enet_peer_send(peer, 0, bs.to_enet_packet(ENET_PACKET_FLAG_RELIABLE));
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
    BitStream bs;
    bs.from_enet_packet(packet);
    bs.read_n(sizeof(MessageType));
    bs >> ent;
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
    BitStream bs;
    bs.from_enet_packet(packet);
    bs.read_n(sizeof(MessageType));
    bs >> eid;
}

void deserialize_entity_input(ENetPacket *packet, uint16_t &eid, float &thr, float &steer)
{
    BitStream bs;
    bs.from_enet_packet(packet);
    bs.read_n(sizeof(MessageType));
    bs >> eid >> thr >> steer;
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &ori)
{
    BitStream bs;
    bs.from_enet_packet(packet);
    bs.read_n(sizeof(MessageType));
    bs >> eid >> x >> y >> ori;
}

void deserialize_time_msec(ENetPacket *packet, uint32_t &timeMsec)
{
    BitStream bs;
    bs.from_enet_packet(packet);
    bs.read_n(sizeof(MessageType));
    bs >> timeMsec;
}
