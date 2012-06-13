#ifndef LOADER_H
#define LOADER_H

class Loader
{
public:
    Loader();

    bool load(const char* path, int vid, int pid);

private:
    enum Command {
        CmdGetVersion,
        CmdWriteMemory,
        CmdSetAddrPtr,
        CmdGetAddrPtr,
        CmdJump,
        CmdAbort
    };
};

#endif // LOADER_H
