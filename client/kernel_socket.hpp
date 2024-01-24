#ifndef KERNEL_SOCKET_HEADER
#define KERNEL_SOCKET_HEADER

class KernelSocket
{
public:
    virtual ~KernelSocket() {}
    virtual bool connect();
    virtual bool setBrightness(int value);
    virtual bool increaseBrightness();
    virtual bool decreaseBrightness();
    virtual void close();

private:
    int fd;
};

#endif
