import socket
import struct
import argparse
import selectors
import sys

class IPK24ChatClient:
    def __init__(self, server_address, server_port, udp_timeout, max_retransmissions):
        self.server_address = server_address
        self.server_port = server_port
        self.udp_timeout = udp_timeout
        self.max_retransmissions = max_retransmissions
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.setblocking(False)  # Set socket to non-blocking mode
        self.message_id = 0
        self.selector = selectors.DefaultSelector()
        self.selector.register(self.socket, selectors.EVENT_READ, self.receive_message)

    def send_message(self, message_type, message_contents=b''):
        header = struct.pack('!B H', message_type, self.message_id)
        self.message_id += 1
        message = header + message_contents
        self.socket.sendto(message, (self.server_address, self.server_port))

    def receive_message(self, sock, mask):
        try:
            message, server_address = self.socket.recvfrom(1024)
            message_type, message_id = struct.unpack('!B H', message[:3])
            message_contents = message[3:]
            if message_type == 0x04:  # MSG message type
                sender_name, message_content = message_contents.split(b'\0', 1)
                print(sender_name.decode() + ": " + message_content.decode())
        except BlockingIOError:
            pass

    def auth_message(self, username, display_name, secret):
        message_type = 0x02
        auth_message = username.encode() + b'\0' + display_name.encode() + b'\0' + secret.encode() + b'\0'
        self.send_message(message_type, auth_message)

    def msg_message(self, display_name, message_contents):
        message_type = 0x04
        msg_message = display_name.encode() + b'\0' + message_contents.encode() + b'\0'
        self.send_message(message_type, msg_message)

def parse_args():
    parser = argparse.ArgumentParser(description='IPK24-CHAT client')
    parser.add_argument('-t', metavar='protocol', choices=['tcp', 'udp'], default='udp', help='Transport protocol (tcp or udp)')
    parser.add_argument('-s', metavar='server', required=True, help='Server IP address or hostname')
    parser.add_argument('-p', metavar='port', type=int, default=4567, help='Server port')
    parser.add_argument('-d', metavar='timeout', type=int, default=250, help='UDP confirmation timeout')
    parser.add_argument('-r', metavar='retransmissions', type=int, default=3, help='Maximum number of UDP retransmissions')
    return parser.parse_args()

def main():
    args = parse_args()

    if args.t != 'udp':
        print("Only UDP transport protocol is supported for now.")
        return

    client = IPK24ChatClient(args.s, args.p, args.d, args.r)
    # Authenticate with the desired display name
    display_name = "Borec2"
    client.auth_message("username", display_name, "Secret")

    while True:
        # Check for incoming messages
        events = client.selector.select(timeout=0)
        for key, mask in events:
            callback = key.data
            callback(key.fileobj, mask)

        try:
            # Non-blocking input handling
            message = sys.stdin.readline().strip()
            if message:
                client.msg_message(display_name, message)
        except KeyboardInterrupt:
            print("\nExiting...")
            break

if __name__ == '__main__':
    main()
