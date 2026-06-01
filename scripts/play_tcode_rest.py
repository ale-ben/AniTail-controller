#!/usr/bin/env python3
"""Play a local TCODE file by posting movement commands to AniTail REST API."""

import argparse
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

MOVEMENT_CODES = {"G0", "G1", "G28"}


def strip_comments(line: str) -> str:
    """Remove common inline comment markers and trim whitespace."""
    for marker in (";", "#", "//"):
        idx = line.find(marker)
        if idx != -1:
            line = line[:idx]
    return line.strip()


def token_value(tokens: list[str], prefix: str) -> str | None:
    """Return the value part of a token with a known prefix, e.g. T500 -> 500."""
    for token in tokens:
        if token.startswith(prefix) and len(token) > len(prefix):
            return token[len(prefix) :]
    return None


def parse_timing_seconds(tokens: list[str]) -> float:
    """Parse local delay for G4 command in seconds from P(ms) or S(sec)."""
    p_val = token_value(tokens, "P")
    if p_val is not None:
        return float(p_val) / 1000.0

    s_val = token_value(tokens, "S")
    if s_val is not None:
        return float(s_val)

    raise ValueError("G4 requires P<ms> or S<sec>")


def g1_duration_seconds(tokens: list[str]) -> float | None:
    """Parse G1 duration from T(ms)."""
    t_val = token_value(tokens, "T")
    if t_val is None:
        return None
    return float(t_val) / 1000.0


def send_command(url: str, command: str, timeout: float) -> tuple[bool, str]:
    """Send one command via HTTP POST and return (success, response_or_error)."""
    try:
        data = command.encode("utf-8")
        req = urllib.request.Request(url, data=data, method="POST")
        with urllib.request.urlopen(req, timeout=timeout) as response:
            body = response.read().decode("utf-8", errors="replace")
            return True, f"{response.status} {body}".strip()
    except urllib.error.HTTPError as err:
        body = err.read().decode("utf-8", errors="replace")
        return False, f"HTTP {err.code} {body}".strip()
    except urllib.error.URLError as err:
        return False, f"Connection error: {err.reason}"
    except Exception as err:  # noqa: BLE001
        return False, f"Unexpected error: {err}"


def process_file(
    file_path: Path,
    url: str,
    timeout: float,
    dry_run: bool,
    wait_g1: bool,
    verbose: bool,
    max_restarts: int | None,
) -> int:
    """Process file line-by-line. Returns process exit code."""
    sent_count = 0
    delay_count = 0

    with file_path.open("r", encoding="utf-8") as handle:
        raw_lines = handle.readlines()

    line_index = 0
    restart_count = 0
    try:
        while line_index < len(raw_lines):
            line_number = line_index + 1
            raw_line = raw_lines[line_index]
            line_index += 1

            line = strip_comments(raw_line)
            if not line:
                continue

            tokens = line.split()
            code = tokens[0].upper()

            if code == "S1":
                if max_restarts is not None and restart_count >= max_restarts:
                    if verbose or dry_run:
                        print(f"[line {line_number}] max restarts reached ({max_restarts}), stopping")
                    break

                restart_count += 1
                if verbose or dry_run:
                    print(f"[line {line_number}] restart file (S1) -> loop #{restart_count}")
                line_index = 0
                continue

            if code == "G4":
                try:
                    delay_s = parse_timing_seconds(tokens)
                except ValueError as err:
                    print(f"[line {line_number}] Invalid G4 delay: {err}", file=sys.stderr)
                    return 2

                delay_count += 1
                if verbose or dry_run:
                    print(f"[line {line_number}] delay {delay_s:.3f}s")
                if not dry_run:
                    time.sleep(delay_s)
                continue

            if code not in MOVEMENT_CODES:
                if verbose:
                    print(f"[line {line_number}] skip: {line}")
                continue

            if dry_run:
                print(f"[line {line_number}] send: {line}")
                sent_count += 1
            else:
                ok, response = send_command(url, line, timeout)
                if ok:
                    print(f"[line {line_number}] sent: {line} -> {response}")
                    sent_count += 1
                else:
                    print(f"[line {line_number}] failed: {line} -> {response}", file=sys.stderr)
                    return 1

            if code == "G1" and wait_g1:
                duration_s = g1_duration_seconds(tokens)
                if duration_s is not None and duration_s > 0:
                    if verbose or dry_run:
                        print(f"[line {line_number}] wait {duration_s:.3f}s for G1")
                    if not dry_run:
                        time.sleep(duration_s)
    except KeyboardInterrupt:
        print("Interrupted by user (Ctrl+C).")
        print(
            f"Stopped: sent={sent_count}, local_delays={delay_count}, restarts={restart_count}"
        )
        return 130

    print(
        f"Completed: sent={sent_count}, local_delays={delay_count}, restarts={restart_count}"
    )
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Read a TCODE file and execute it against AniTail REST endpoint."
    )
    parser.add_argument("file", type=Path, help="Path to .tcode/.txt file")
    parser.add_argument("--host", default="192.168.1.51", help="AniTail host/IP")
    parser.add_argument("--port", type=int, default=80, help="AniTail HTTP port")
    parser.add_argument("--endpoint", default="/tcode", help="REST endpoint path")
    parser.add_argument(
        "--timeout",
        type=float,
        default=3.0,
        help="HTTP request timeout in seconds",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Parse and print actions without network calls or sleeping",
    )
    parser.add_argument(
        "--no-wait-g1",
        action="store_true",
        help="Do not wait locally for G1 T<ms> after sending",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Print skipped lines and timing details",
    )
    parser.add_argument(
        "--max-restarts",
        type=int,
        default=None,
        help="Optional limit for S1 restarts (default: unlimited)",
    )
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    file_path: Path = args.file
    if not file_path.exists():
        print(f"File not found: {file_path}", file=sys.stderr)
        return 2

    url = f"http://{args.host}:{args.port}{args.endpoint}"
    return process_file(
        file_path=file_path,
        url=url,
        timeout=args.timeout,
        dry_run=args.dry_run,
        wait_g1=not args.no_wait_g1,
        verbose=args.verbose,
        max_restarts=args.max_restarts,
    )


if __name__ == "__main__":
    raise SystemExit(main())
