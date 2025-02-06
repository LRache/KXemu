#ifndef __KXEMU_UTILS_RSP_H__
#define __KXEMU_UTILS_RSP_H__

#include <string>
#include <vector>
#include <queue>
#include <arpa/inet.h>

// This is an implementation of GDB Remote Serial Protocol (RSP)
class RSP {
private:
    int server;
    int client;
    struct sockaddr_in address;
    std::queue<std::string> commandQueue;
public:
    enum CommandType {
        Empty,       // The command queue is empty.
        Error,       // Meet error when call next_command().
        Continue,    // c and C - Continue the target.
        Step,        // s and S - Step the target.
        GeneralReg,  // g and G - Read or write general registers.
        Kill,        // k - Kill the target. The semantics of this are not clearly defined. Most targets should probably ignore it.
        SpecificReg, // p and P - Read or write a specific register.
        Memory,      // m and M - Read or write main memory.
        Offset,      // qOffset - Report the offsets to use when relocating downloaded code.
        Symbol,      // qSymbol - Request any symbol table data. A minimal implementation should request no data.
        LoadData,    // X - Load binary data.
        Breakpoint,  // z and Z - Clear or set breakpoints or watchpoints.
        Report,      // ? - Report why the target halted.
    };

    RSP();
    ~RSP();

    bool open_rsp(int port);
    bool wait_client();
    int next_command(std::vector<std::string> &args, bool blocked);
    int send_reply(int cmd);
    int send_reply(const std::string &reply);
};

#endif
