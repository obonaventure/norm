#ifndef _NORM_AMT_CHANNEL
#define _NORM_AMT_CHANNEL

#ifdef NORM_AMT

#include "protoSocket.h"

// ProtoSocket subclass that wraps an existing UDP socket fd owned by the AMT
// gateway library. Lets the NORM event loop receive readability notifications
// on the AMT gateway socket without NORM opening or closing the fd itself.
class NormAMTSocket : public ProtoSocket
{
    public:
        NormAMTSocket() : ProtoSocket(ProtoSocket::UDP) {}

        // Adopt an already-open UDP socket fd. Returns false on failure.
        bool Adopt(int socketFd)
        {
            // SetDescriptor() is protected in ProtoChannel but accessible here.
            SetDescriptor(socketFd);
            return true;
        }

        // Release without closing the underlying fd (AMT gateway owns it).
        void Release()
        {
            StopInputNotification();
            SetDescriptor(INVALID_DESCRIPTOR);
        }
};

#endif // NORM_AMT
#endif // _NORM_AMT_CHANNEL
