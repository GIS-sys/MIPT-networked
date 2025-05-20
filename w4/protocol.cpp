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

void send_entity_state(ENetPeer *peer, uint16_t eid, float x, float y, float size)
{
    BitStream bs;
    bs << E_CLIENT_TO_SERVER_STATE << eid << x << y << size;
    enet_peer_send(peer, 0, bs.to_enet_packet(ENET_PACKET_FLAG_RELIABLE));
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float size)
{
    BitStream bs;
    bs << E_SERVER_TO_CLIENT_SNAPSHOT << eid << x << y << size;
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

void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &size)
{
    BitStream bs;
    bs.from_enet_packet(packet);
    bs.read_n(sizeof(MessageType));
    bs >> eid >> x >> y >> size;
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &size)
{
    BitStream bs;
    bs.from_enet_packet(packet);
    bs.read_n(sizeof(MessageType));
    bs >> eid >> x >> y >> size;
}

