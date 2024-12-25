#include "kdb/rsp.h"
#include "debug.h"
#include "log.h"
#include "utils/gdb-rsp.h"
#include <string>
#include <vector>

void rsp::rsp_mainloop(int port) {
    RSP server;
    if (!server.open_rsp(port)) {
        INFO("Failed to open RSP server");
        return;
    }

    server.wait_client();

    std::vector<std::string> args;
    server.next_command(args, true);
    server.next_command(args, true);

    INFO("RSP server end");
}
