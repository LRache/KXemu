import sys
import termios
import tty
import socket
import select


def main():
    host = "127.0.0.1"
    port = 8192
    
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((host, port))
    server.listen(1)

    # One connection carries both directions.
    client, addr = server.accept()
    print(f"UART connected from {addr}")

    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    tty.setraw(fd)

    running = True
    while running:
        readable, _, _ = select.select([sys.stdin, client], [], [])
        for r in readable:
            if r == sys.stdin:
                char = sys.stdin.read(1)
                if char == '\x03':
                    running = False
                    break
                client.send(char.encode())
            elif r == client:
                data = client.recv(1024)
                if not data:
                    running = False
                    break
                s = data.decode()
                s = s.replace("\n", "\r\n").replace("\x7f", "\b \b")
                sys.stdout.write(s)
                sys.stdout.flush()
    
    termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    client.close()
    server.close()
    print("Close server")


if __name__ == "__main__":
    main()
