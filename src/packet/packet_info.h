#ifndef PACKET_INFO_H
#define PACKET_INFO_H

#include "acl_roles_handler.h"

/// Describes a packet's interpretation details.
class PacketInfo
{
  public:
    ACLRole::Permission acl_permission; //!< The permissions necessary for the packet.
    int min_args;                       //!< The minimum arguments needed for the packet to be interpreted correctly / make sense.
    QString header;

    static PacketInfo CreateInfo(const QString &header, int min = 0, ACLRole::Permission permission = ACLRole::Permission::NONE){
        PacketInfo create{
            .acl_permission = permission,
            .min_args = qMax(0, min),
            .header = header};
        return create;
    }
};
#endif
