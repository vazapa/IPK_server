import socket
import struct
import argparse
import sys

class IPK24ChatClient:
    def __init__(self, server_address, server_port, udp_timeout, max_retransmissions):
        self.server_address = server_address
        self.server_port = server_port
        self.udp_timeout = udp_timeout
        self.max_retransmissions = max_retransmissions
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.message_id = 0

    def send_message(self, message_type, message_contents=b''):
        header = struct.pack('!B H', message_type, self.message_id)
        self.message_id += 1
        message = header + message_contents
        self.socket.sendto(message, (self.server_address, self.server_port))

    def receive_message(self):
        message, server_address = self.socket.recvfrom(1024)
        message_type, message_id = struct.unpack('!B H', message[:3])
        message_contents = message[3:]
        return message_type, message_id, message_contents, server_address

    def confirm_message(self, ref_message_id):
        confirm_message = struct.pack('!B H', 0x00, ref_message_id)
        self.socket.sendto(confirm_message, (self.server_address, self.server_port))

    def reply_message(self, result, ref_message_id, message_contents=b''):
        reply_header = struct.pack('!B H B', 0x01, self.message_id, result)
        self.message_id += 1
        reply_message = reply_header + struct.pack('!H', ref_message_id) + message_contents
        self.socket.sendto(reply_message, (self.server_address, self.server_port))

    def auth_message(self, username, display_name, secret):
        auth_message = struct.pack('!B H', 0x02, self.message_id)
        auth_message += username.encode() + b'\0' + display_name.encode() + b'\0' + secret.encode() + b'\0'
        self.message_id += 1
        self.socket.sendto(auth_message, (self.server_address, self.server_port))

    def join_message(self, channel_id, display_name):
        join_message = struct.pack('!B H', 0x03, self.message_id)
        join_message += channel_id.encode() + b'\0' + display_name.encode() + b'\0'
        self.message_id += 1
        self.socket.sendto(join_message, (self.server_address, self.server_port))

    def msg_message(self, display_name, message_contents):
        msg_message = struct.pack('!B H', 0x04, self.message_id)
        msg_message += display_name.encode() + b'\0' + message_contents.encode() + b'\0'
        self.message_id += 1
        self.socket.sendto(msg_message, (self.server_address, self.server_port))

    def err_message(self, display_name, error_message):
        err_message = struct.pack('!B H', 0xFE, self.message_id)
        err_message += display_name.encode() + b'\0' + error_message.encode() + b'\0'
        self.message_id += 1
        self.socket.sendto(err_message, (self.server_address, self.server_port))

    def bye_message(self):
        bye_message = struct.pack('!B H', 0xFF, self.message_id)
        self.message_id += 1
        self.socket.sendto(bye_message, (self.server_address, self.server_port))

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

    try:
        while True:
            # Receive and print messages from the server
            message_type, message_id, message_contents, server_address = client.receive_message()
            if message_type == 0x04:  # MSG message type
                sender_name, message_content = message_contents.split(b'\0', 1)
                print(sender_name.decode() + ": " + message_content.decode())

            message = input("Enter your message: ")
            # Use the same display name for sending messages
            client.msg_message(display_name, message)
            
    except KeyboardInterrupt:
        print("\nExiting...")

if __name__ == '__main__':
    main()
