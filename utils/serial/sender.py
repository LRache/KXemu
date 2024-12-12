import sys
import termios
import tty

def read_single_char():
    """读取单个字符，不显示在命令行中"""
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(fd)  # 设置为原始模式，关闭回显
        char = sys.stdin.read(1)  # 读取一个字符
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)  # 恢复终端设置
    return char

def main():
    print("请输入字符，按下 ESC 键退出：")
    while True:
        char = read_single_char()
        if char == '\x1b':
            print("\n退出程序")
            break
        else:
            print(f"您输入了：{char}")

if __name__ == "__main__":
    main()
