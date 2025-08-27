import sys
import termios
import tty
import socket
import select


def main():
    host = "127.0.0.1"
    port = 8192
    
    sendServer = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    recvServer = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    sendServer.bind((host, port))
    recvServer.bind((host, port + 1))
    sendServer.listen(1)
    recvServer.listen(1)

    sendClient, sendAddr = sendServer.accept()
    print(f"Send server connected to {sendAddr}")
    recvClient, recvAddr = recvServer.accept()
    print(f"Recv server connected to {recvAddr}")

    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    tty.setraw(fd)

    running = True
    while running:
        readable, _, _ = select.select([sys.stdin, recvClient], [], [])
        for r in readable:
            if r == sys.stdin:
                char = sys.stdin.read(1)
                if char == '\x03':
                    running = False
                    break
                sendClient.send(char.encode())
            elif r == recvClient:
                data = recvClient.recv(1024)
                s = data.decode()
                s = s.replace("\n", "\r\n").replace("\x7f", "\b \b")
                sys.stdout.write(s)
                sys.stdout.flush()
    
    termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    sendServer.close()
    recvServer.close()
    print("Close server")


if __name__ == "__main__":
    main()
