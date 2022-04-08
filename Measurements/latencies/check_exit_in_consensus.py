import sys
import stem
from stem.control import Controller
from stem.descriptor.remote import their_server_descriptor, get_consensus
from typing import List


def _check_exit():
    endpoint = stem.DirPort("127.0.0.1", 7101)
    exit_local = list(their_server_descriptor(endpoints=[endpoint]))[0]
    consensus = set(get_consensus(endpoints=[endpoint]))
    print(f"Looking for relay with fingerprint {exit_local.fingerprint}")
    if exit_local.fingerprint not in {r.fingerprint for r in consensus}:
        print("ERROR: Relay not found.")
        sys.exit(1)
    print("Relay found!")
    exit_consensus = [r for r in consensus if r.fingerprint == exit_local.fingerprint][
        0
    ]
    print(f"Checking relay has address {exit_local.address}")
    if exit_consensus.address != exit_local.address:
        print(
            f"ERROR: Relay in consensus has incorrect address: {exit_consensus.address}"
        )
        sys.exit(1)
    print("All looks good.")


def _check_relay(port: int, role: str):
    print(f"Checking {role} relay at port {port}")
    endpoint = stem.ORPort("127.0.0.1", port)
    relays = list(their_server_descriptor(endpoints=[endpoint]))
    if len(relays) != 1:
        print(f"ERROR: Expected length one list; got {len(relays)}")
        sys.exit(1)
    relay = relays[0]
    print(f"Found {role} relay: {relay.fingerprint}")
    return relay.fingerprint


def _check_client(bridge_fp: str, exit_fp: str):
    sock_file = "/run/tor/control"
    controller = Controller.from_socket_file(sock_file)
    controller.authenticate()
    print(f"Connected to client via socket [{sock_file}]")
    if controller.get_server_descriptor(bridge_fp) is None:
        print("ERROR: got no bridge from client!")
        sys.exit(1)
    if controller.get_server_descriptor(exit_fp) is None:
        print("ERROR: got no exit from client!")
        sys.exit(1)


def main(args: List[str]):
    bridge_fp = _check_relay(5100, "bridge")
    exit_fp = _check_relay(5101, "exit")
    _check_exit()
    if len(args) > 1:
        _check_client(bridge_fp, exit_fp)


if __name__ == "__main__":
    main(sys.argv)
