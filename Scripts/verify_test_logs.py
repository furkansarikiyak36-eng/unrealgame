#!/usr/bin/env python3
"""Fail-closed parser for MyProject2 headless test logs.

The caller must also check Unreal process exit codes. Log parsing does not replace
process-level validation.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

MECHANICS_RESULT = re.compile(r"SONUÇ:\s*(\d+)\s*geçti,\s*(\d+)\s*kaldı")
MECHANICS_FAILED = re.compile(r"\[MechanicsDemo\]\s+KALDI\s+(.*)")
WOOD_SUCCESS = re.compile(
    r"Test Completed\. Result=\{Success\} Name=\{([^}]+)\} Path=\{([^}]+)\}"
)
WOOD_FAILED = re.compile(
    r"Test Completed\. Result=\{Fail\} Name=\{([^}]+)\} Path=\{([^}]+)\}"
)


def read_log(path: Path, label: str) -> str:
    try:
        return path.read_text(encoding="utf-8", errors="replace")
    except OSError as exc:
        raise ValueError(f"{label}: cannot read log '{path}': {exc}") from exc


def verify_mechanics(path: Path, minimum_passed: int = 1) -> tuple[int, int]:
    text = read_log(path, "Mechanics")
    results = MECHANICS_RESULT.findall(text)
    if not results:
        raise ValueError(
            f"Mechanics: missing 'SONUÇ: N geçti, M kaldı' summary in {path.name}; "
            "the run may have aborted or the log format changed."
        )
    passed, failed = map(int, results[-1])
    failure_details = MECHANICS_FAILED.findall(text)
    if passed < minimum_passed:
        raise ValueError(
            f"Mechanics: only {passed} checks passed; expected at least {minimum_passed}."
        )
    if failed != 0:
        detail = "; ".join(s.strip()[:160] for s in failure_details[:5])
        suffix = f" Details: {detail}" if detail else ""
        raise ValueError(f"Mechanics: {passed} passed, {failed} failed.{suffix}")
    if failure_details:
        raise ValueError(
            f"Mechanics: summary reports zero failures but the log contains "
            f"{len(failure_details)} '[MechanicsDemo] KALDI' lines."
        )
    return passed, failed


def verify_wood(path: Path, minimum_tests: int = 1) -> tuple[int, int]:
    text = read_log(path, "Wood")
    successes = WOOD_SUCCESS.findall(text)
    failures = WOOD_FAILED.findall(text)
    if not successes and not failures:
        raise ValueError(
            f"Wood: no parseable individual Automation Test result found in {path.name}; "
            "the test command may have failed before discovery or the log format changed."
        )
    passed_paths = {test_path for _, test_path in successes}
    failed_paths = {test_path for _, test_path in failures}
    overlap = sorted(passed_paths & failed_paths)
    if overlap:
        raise ValueError(f"Wood: test paths reported both pass and fail: {', '.join(overlap)}")
    if failures:
        details = "; ".join(path for _, path in failures[:8])
        raise ValueError(
            f"Wood: {len(successes)} passed, {len(failures)} failed. Failed: {details}"
        )
    if len(successes) < minimum_tests:
        raise ValueError(
            f"Wood: only {len(successes)} tests passed; expected at least {minimum_tests}."
        )
    if len(passed_paths) != len(successes):
        raise ValueError(
            "Wood: duplicate successful result paths were found; verify the test log was not merged from runs."
        )
    return len(successes), len(failures)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mechanics", required=True, type=Path)
    parser.add_argument("--wood", required=True, type=Path)
    parser.add_argument("--minimum-mechanics-checks", type=int, default=101,
                        help="Fail if fewer than this many MechanicsDemo checks passed (default: 101).")
    parser.add_argument("--minimum-wood-tests", type=int, default=12,
                        help="Fail if fewer than this many unique Wood tests passed (default: 12).")
    args = parser.parse_args()
    if args.minimum_mechanics_checks < 1 or args.minimum_wood_tests < 1:
        parser.error("minimum test counts must be at least 1")
    try:
        mech_passed, mech_failed = verify_mechanics(args.mechanics, args.minimum_mechanics_checks)
        wood_passed, wood_failed = verify_wood(args.wood, args.minimum_wood_tests)
    except ValueError as exc:
        print(f"TEST VERIFICATION FAILED: {exc}", file=sys.stderr)
        return 1
    print(f"MechanicsDemo: {mech_passed} passed, {mech_failed} failed")
    print(f"Wood automation: {wood_passed} passed, {wood_failed} failed")
    print("TEST VERIFICATION PASSED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
