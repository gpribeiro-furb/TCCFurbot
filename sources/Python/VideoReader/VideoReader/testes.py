import subprocess
import platform

def ping_ip(ip_address):
    # Check platform
    param = '-n' if platform.system().lower()=='windows' else '-c'

    # Building the command. Ex: "ping -c 1 google.com"
    command = ['ping', param, '1', ip_address]

    try:
        # Pinging
        output = subprocess.check_output(command, stderr=subprocess.STDOUT, universal_newlines=True)
        print(f"Success:\n{output}")
    except subprocess.CalledProcessError as e:
        print(f"Failed to ping {ip_address}\n{e.output}")

# Example usage
if __name__ == "__main__":
    ip_address = '192.168.1.15'
    ping_ip(ip_address)
