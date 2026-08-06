#!/usr/bin/env python3
"""
ft_irc_tester.py — automated socket-based test bot for PRIVMSG / TOPIC / MODE

Usage:
    python3 ft_irc_tester.py <port> <password>

No external dependencies (raw sockets only), Python 3, works against a
C++98 ft_irc server implementation.

Clients spun up per run:
    alice  -> becomes channel operator (creates #test by JOINing first)
    bob    -> regular member
    carol  -> regular member
    dave   -> outsider (never joins #test unless a specific test needs it)

Test IDs are stable across reruns so you can diff pass/fail sets between
refactors. IDs preserved from the previous test-plan revision:
    M23, M29, M29-single, M30, M33, M40, M41, M42
New tests extend the same numbering scheme (P.. for PRIVMSG, T.. for TOPIC,
M.. for MODE) without renumbering the anchors above.

Exit code is 0 if all tests pass, 1 otherwise (handy for CI/regression use).
"""

import socket
import sys
import time
import re

HOST = "127.0.0.1"
RECV_TIMEOUT = 2.0
CHANNEL = "#test"


# --------------------------------------------------------------------------
# Low-level client
# --------------------------------------------------------------------------

class Client:
    def __init__(self, name, port, password):
        self.name = name
        self.port = port
        self.password = password
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(RECV_TIMEOUT)
        self.buf = ""

    def connect(self):
        self.sock.connect((HOST, self.port))

    def send(self, line):
        self.sock.sendall((line + "\r\n").encode("utf-8", errors="replace"))

    def register(self, nick=None):
        nick = nick or self.name
        if self.password:
            self.send(f"PASS {self.password}")
        self.send(f"NICK {nick}")
        self.send(f"USER {nick} 0 * :{nick.capitalize()} Tester")
        self.drain(0.4)

    def _read_more(self, timeout=None):
        old = self.sock.gettimeout()
        if timeout is not None:
            self.sock.settimeout(timeout)
        try:
            data = self.sock.recv(65536)
        except socket.timeout:
            return False
        finally:
            if timeout is not None:
                self.sock.settimeout(old)
        if not data:
            return False
        self.buf += data.decode("utf-8", errors="replace")
        return True

    def drain(self, timeout=RECV_TIMEOUT):
        """Read everything currently available (best effort) and return lines."""
        end = time.time() + timeout
        while time.time() < end:
            got = self._read_more(timeout=max(0.05, end - time.time()))
            if not got:
                break
        lines = [l for l in self.buf.split("\r\n") if l]
        self.buf = ""
        return lines

    def expect(self, pattern, timeout=RECV_TIMEOUT, flags=0):
        """
        Wait up to `timeout` seconds for a line matching `pattern` (regex).
        Returns the matching line, or None if not found. Non-matching lines
        are consumed and discarded (kept out of subsequent expect() calls).
        """
        end = time.time() + timeout
        rx = re.compile(pattern, flags)
        while True:
            for line in self.buf.split("\r\n"):
                if line and rx.search(line):
                    idx = self.buf.find(line)
                    self.buf = self.buf[idx + len(line):]
                    return line
            remaining = end - time.time()
            if remaining <= 0:
                return None
            if not self._read_more(timeout=remaining):
                # nothing new arrived and we're out of time on next loop
                if time.time() >= end:
                    return None

    def close(self):
        try:
            self.send("QUIT :bye")
        except OSError:
            pass
        try:
            self.sock.close()
        except OSError:
            pass


# --------------------------------------------------------------------------
# Test harness
# --------------------------------------------------------------------------

class Runner:
    def __init__(self):
        self.results = []  # (id, passed, detail)

    def check(self, test_id, condition, detail=""):
        self.results.append((test_id, bool(condition), detail))
        status = "PASS" if condition else "FAIL"
        print(f"[{status}] {test_id:14s} {detail}")

    def summary(self):
        total = len(self.results)
        passed = sum(1 for _, ok, _ in self.results if ok)
        failed = total - passed
        print("\n" + "=" * 60)
        print(f"TOTAL: {total}   PASS: {passed}   FAIL: {failed}")
        if failed:
            print("\nFailed tests:")
            for tid, ok, detail in self.results:
                if not ok:
                    print(f"  - {tid}: {detail}")
        print("=" * 60)
        return failed == 0


# Numeric replies we reference (RFC 1459 / RFC 2812)
RPL_NOTOPIC = "331"
RPL_TOPIC = "332"
RPL_CHANNELMODEIS = "324"
ERR_NOSUCHNICK = "401"
ERR_NOSUCHCHANNEL = "403"
ERR_CANNOTSENDTOCHAN = "404"
ERR_NORECIPIENT = "411"
ERR_NOTEXTTOSEND = "412"
ERR_UNKNOWNMODE = "472"
ERR_NEEDMOREPARAMS = "461"
ERR_CHANOPRIVSNEEDED = "482"
ERR_USERNOTINCHANNEL = "441"
ERR_NOTONCHANNEL = "442"


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <port> <password>")
        sys.exit(2)

    port = int(sys.argv[1])
    password = sys.argv[2]
    r = Runner()

    alice = Client("alice", port, password)
    bob = Client("bob", port, password)
    carol = Client("carol", port, password)
    dave = Client("dave", port, password)

    for c in (alice, bob, carol, dave):
        c.connect()
        c.register()

    # ---- Setup: alice creates channel (becomes op), bob & carol join ----
    alice.send(f"JOIN {CHANNEL}")
    alice.drain(0.3)
    bob.send(f"JOIN {CHANNEL}")
    bob.drain(0.3)
    carol.send(f"JOIN {CHANNEL}")
    carol.drain(0.3)
    time.sleep(0.2)
    # flush join broadcasts so they don't leak into later expect() calls
    for c in (alice, bob, carol):
        c.drain(0.3)

    # ======================================================================
    # PRIVMSG tests (P-series)
    # ======================================================================

    # P01: direct message delivered to recipient
    alice.send("PRIVMSG bob :hello there")
    line = bob.expect(r"PRIVMSG bob :hello there", timeout=2)
    r.check("P01", line is not None, "direct PRIVMSG alice->bob delivered")

    # P02: channel message delivered to other members, not echoed to sender
    alice.send(f"PRIVMSG {CHANNEL} :hi channel")
    line_bob = bob.expect(rf"PRIVMSG {re.escape(CHANNEL)} :hi channel", timeout=2)
    r.check("P02", line_bob is not None, "channel PRIVMSG delivered to bob")
    line_carol = carol.expect(rf"PRIVMSG {re.escape(CHANNEL)} :hi channel", timeout=2)
    r.check("P02b", line_carol is not None, "channel PRIVMSG delivered to carol")
    echoed = alice.expect(rf"PRIVMSG {re.escape(CHANNEL)} :hi channel", timeout=0.5)
    r.check("P02c", echoed is None, "sender does NOT receive their own channel PRIVMSG echo")

    # P03: PRIVMSG to nonexistent nick -> 401
    alice.send("PRIVMSG ghostuser :yo")
    line = alice.expect(rf"{ERR_NOSUCHNICK}", timeout=2)
    r.check("P03", line is not None, "ERR_NOSUCHNICK (401) for unknown nick target")

    # P04: PRIVMSG with no recipient -> 411
    alice.send("PRIVMSG")
    line = alice.expect(rf"{ERR_NORECIPIENT}", timeout=2)
    r.check("P04", line is not None, "ERR_NORECIPIENT (411) when target missing")

    # P05: PRIVMSG with recipient but no text -> 412
    alice.send("PRIVMSG bob")
    line = alice.expect(rf"{ERR_NOTEXTTOSEND}", timeout=2)
    r.check("P05", line is not None, "ERR_NOTEXTTOSEND (412) when message text missing")

    # P06: PRIVMSG to nonexistent channel -> 403 or 401 depending on impl (spec: no such nick/channel)
    alice.send("PRIVMSG #ghostchannel :hey")
    line = alice.expect(rf"({ERR_NOSUCHCHANNEL}|{ERR_NOSUCHNICK})", timeout=2)
    r.check("P06", line is not None, "error for PRIVMSG to nonexistent channel")

    # P07: PRIVMSG from outsider (dave, not in channel) -> depending on +n default, likely allowed
    # unless +n (no external messages) is set; test baseline (no modes set yet)
    dave.send(f"PRIVMSG {CHANNEL} :outsider msg")
    line = bob.expect(rf"PRIVMSG {re.escape(CHANNEL)} :outsider msg", timeout=2)
    r.check("P07", line is not None,
            "outsider PRIVMSG delivered when no restrictive channel mode set (baseline)")

    # P08: trailing-param integrity — colons/spaces inside message preserved verbatim
    msg_text = "multi word : with colon and  double space"
    alice.send(f"PRIVMSG bob :{msg_text}")
    line = bob.expect(re.escape(msg_text), timeout=2)
    r.check("P08", line is not None, "trailing PRIVMSG text with colon/spaces preserved exactly")

    # ======================================================================
    # TOPIC tests (T-series)
    # ======================================================================

    # T01: query topic when none set yet -> RPL_NOTOPIC (331)
    alice.send(f"TOPIC {CHANNEL}")
    line = alice.expect(rf"{RPL_NOTOPIC}", timeout=2)
    r.check("T01", line is not None, "RPL_NOTOPIC (331) when no topic set")

    # T02: op sets topic, gets broadcast to channel members
    alice.send(f"TOPIC {CHANNEL} :Welcome to the test channel")
    line_bob = bob.expect(r"TOPIC .*:Welcome to the test channel", timeout=2)
    r.check("T02", line_bob is not None, "TOPIC set by op broadcast to members")

    # T03: query topic after it's set -> RPL_TOPIC (332) with correct text
    carol.send(f"TOPIC {CHANNEL}")
    line = carol.expect(rf"{RPL_TOPIC}.*:Welcome to the test channel", timeout=2)
    r.check("T03", line is not None, "RPL_TOPIC (332) returns correct topic text")

    # T04: TOPIC on nonexistent channel -> 403
    alice.send("TOPIC #ghostchannel")
    line = alice.expect(rf"{ERR_NOSUCHCHANNEL}", timeout=2)
    r.check("T04", line is not None, "ERR_NOSUCHCHANNEL (403) for TOPIC on unknown channel")

    # T05: TOPIC with no channel param at all -> 461
    alice.send("TOPIC")
    line = alice.expect(rf"{ERR_NEEDMOREPARAMS}", timeout=2)
    r.check("T05", line is not None, "ERR_NEEDMOREPARAMS (461) when TOPIC given no args")

    # Set +t so non-ops need privileges to change topic
    alice.send(f"MODE {CHANNEL} +t")
    alice.drain(0.3)
    bob.drain(0.3)
    carol.drain(0.3)

    # T06: non-op TOPIC change with +t set -> 482
    bob.send(f"TOPIC {CHANNEL} :bob tries to change it")
    line = bob.expect(rf"{ERR_CHANOPRIVSNEEDED}", timeout=2)
    r.check("T06", line is not None, "ERR_CHANOPRIVSNEEDED (482) for non-op TOPIC set under +t")

    # T07: op can still set topic when +t is set
    alice.send(f"TOPIC {CHANNEL} :Op-only topic now")
    line = bob.expect(r"TOPIC .*:Op-only topic now", timeout=2)
    r.check("T07", line is not None, "op TOPIC set still works under +t")

    # Remove +t for rest of tests
    alice.send(f"MODE {CHANNEL} -t")
    alice.drain(0.3); bob.drain(0.3); carol.drain(0.3)

    # T08: non-op TOPIC change with +t NOT set -> succeeds
    bob.send(f"TOPIC {CHANNEL} :bob's topic, no +t")
    line = carol.expect(r"TOPIC .*:bob's topic, no \+t", timeout=2)
    r.check("T08", line is not None, "non-op TOPIC change succeeds when +t not set")

    # ======================================================================
    # MODE tests (M-series)
    # ======================================================================

    # --- baseline single-flag sets/queries ---

    # M01: +i set by op
    alice.send(f"MODE {CHANNEL} +i")
    line = bob.expect(r"MODE .*\+i", timeout=2)
    r.check("M01", line is not None, "+i broadcast to channel")

    # M02: query shows +i via RPL_CHANNELMODEIS (324)
    alice.send(f"MODE {CHANNEL}")
    line = alice.expect(rf"{RPL_CHANNELMODEIS}.*\+.*i", timeout=2)
    r.check("M02", line is not None, "RPL_CHANNELMODEIS (324) reflects +i")

    # M03: -i unsets it
    alice.send(f"MODE {CHANNEL} -i")
    line = bob.expect(r"MODE .*-i", timeout=2)
    r.check("M03", line is not None, "-i broadcast to channel")

    # M04: +k sets a channel key
    alice.send(f"MODE {CHANNEL} +k secretkey")
    line = bob.expect(r"MODE .*\+k secretkey", timeout=2)
    r.check("M04", line is not None, "+k <key> broadcast with key argument")

    # M05: -k unsets the key (per common impl, -k takes no arg but some require it; accept either)
    alice.send(f"MODE {CHANNEL} -k")
    line = bob.expect(r"MODE .*-k", timeout=2)
    r.check("M05", line is not None, "-k broadcast (key removed)")

    # M06: +o grants operator to bob
    alice.send(f"MODE {CHANNEL} +o bob")
    line = carol.expect(r"MODE .*\+o bob", timeout=2)
    r.check("M06", line is not None, "+o bob broadcast")

    # M07: -o revokes operator from bob
    alice.send(f"MODE {CHANNEL} -o bob")
    line = carol.expect(r"MODE .*-o bob", timeout=2)
    r.check("M07", line is not None, "-o bob broadcast")

    # ---- M23: +l/-l sentinel regression (0 not -1 / no UINT64_MAX leak) ----
    alice.send(f"MODE {CHANNEL} +l 5")
    line = bob.expect(r"MODE .*\+l 5", timeout=2)
    r.check("M23-set", line is not None, "+l 5 broadcast correctly")

    alice.send(f"MODE {CHANNEL} -l")
    line = bob.expect(r"MODE .*-l", timeout=2)
    r.check("M23-unset", line is not None, "-l broadcast correctly")

    alice.send(f"MODE {CHANNEL}")
    line = alice.expect(rf"{RPL_CHANNELMODEIS}", timeout=2)
    leaked = False
    if line:
        # UINT64_MAX or any absurdly large number would indicate the old bug
        leaked = bool(re.search(r"1844674407|18446744073709551615|-1\b", line))
    r.check("M23", line is not None and not leaked,
            f"no UINT64_MAX/-1 sentinel leak in RPL_CHANNELMODEIS after -l: {line!r}")

    # ---- M29 / M29-single / M30: consolidated single-broadcast format ----

    # M29-single: a lone flag in a command still produces exactly one clean MODE line
    alice.send(f"MODE {CHANNEL} +i")
    line = bob.expect(r"^:\S+ MODE \S+ \+i\s*$", timeout=2)
    r.check("M29-single", line is not None,
            f"single-flag MODE command produces one well-formed broadcast line: {line!r}")
    alice.send(f"MODE {CHANNEL} -i")
    bob.drain(0.3)

    # M29: multiple flags in one command consolidate into a single broadcast line
    alice.send(f"MODE {CHANNEL} +ol carol 5")
    line = bob.expect(r"^:\S+ MODE \S+ \+ol carol 5\s*$", timeout=2)
    r.check("M29", line is not None,
            f"multi-flag command produces exactly one consolidated broadcast: {line!r}")
    # No second MODE line should follow immediately after
    extra = bob.expect(r"MODE", timeout=0.4)
    r.check("M29-nodupe", extra is None, "no duplicate/extra MODE broadcast lines for the same command")

    # cleanup for next tests
    alice.send(f"MODE {CHANNEL} -o carol")
    alice.drain(0.3); bob.drain(0.3); carol.drain(0.3)
    alice.send(f"MODE {CHANNEL} -l")
    alice.drain(0.3); bob.drain(0.3); carol.drain(0.3)

    # M30: broadcast preserves input sign/mode/arg order (not reordered/regrouped)
    alice.send(f"MODE {CHANNEL} +k-o key1 bob")
    line = bob.expect(r"MODE .*\+k-o key1 bob", timeout=2)
    r.check("M30", line is not None,
            f"broadcast preserves original sign/flag/arg order for mixed +/- in one command: {line!r}")
    alice.send(f"MODE {CHANNEL} -k")
    alice.drain(0.3); bob.drain(0.3); carol.drain(0.3)
    # make sure bob has op back for further tests if needed
    alice.send(f"MODE {CHANNEL} +o bob")
    alice.drain(0.3); bob.drain(0.3); carol.drain(0.3)

    # ---- M33: query format correctness (RPL_CHANNELMODEIS) ----
    alice.send(f"MODE {CHANNEL} +itk querykey")
    alice.drain(0.3); bob.drain(0.3); carol.drain(0.3)
    alice.send(f"MODE {CHANNEL}")
    line = alice.expect(rf"{RPL_CHANNELMODEIS} \S+ {re.escape(CHANNEL)} \+\S*", timeout=2)
    ok = False
    if line:
        # flags should appear grouped after a single '+', each letter present exactly once
        m = re.search(r"\+(\S+)", line)
        if m:
            flags = m.group(1).split()[0]
            ok = all(c in flags for c in "itk") and flags.count("i") == 1
    r.check("M33", ok, f"RPL_CHANNELMODEIS lists all active flags exactly once, correctly grouped: {line!r}")
    alice.send(f"MODE {CHANNEL} -itk querykey")
    alice.drain(0.3); bob.drain(0.3); carol.drain(0.3)

    # ---- M40/M41/M42: state/broadcast desync on missing required arg ----

    # M40: +ik with missing key -> k should be ignored (continue), i still applied & broadcast
    alice.send(f"MODE {CHANNEL} +ik")
    line = bob.expect(r"MODE", timeout=2)
    applied_only_i = bool(
        line
        and re.search(r"\+i\b", line)
        and "k" not in re.sub(r"\+i", "", line or "")
    )
    r.check("M40", applied_only_i,
            f"+ik with missing key: only 'i' applied/broadcast, 'k' silently ignored (not aborted): {line!r}")
    alice.send(f"MODE {CHANNEL} -i")
    alice.drain(0.3); bob.drain(0.3); carol.drain(0.3)

    # verify no key was actually set as a side effect of the aborted/partial command
    dave2 = Client("dave2", port, password)
    dave2.connect()
    dave2.register()
    dave2.send(f"JOIN {CHANNEL}")
    join_ok = dave2.expect(rf"JOIN :?{re.escape(CHANNEL)}", timeout=2)
    keyerr = dave2.expect(r"47[5-9]", timeout=0.5)  # ERR_BADCHANNELKEY(475) etc, expect NONE
    r.check("M40-state", join_ok is not None and keyerr is None,
            "no stray channel key left set after M40's aborted +ik (state matches broadcast)")
    dave2.close()

    # M41: +ol with missing limit -> l ignored, o still applied & broadcast for carol
    alice.send(f"MODE {CHANNEL} +ol carol")
    line = bob.expect(r"MODE", timeout=2)
    r.check("M41", line is not None and "+o" in (line or "") and "carol" in (line or "")
            and "+ol" not in (line or ""),
            f"+ol with missing limit: 'o carol' applied/broadcast, 'l' silently ignored: {line!r}")
    alice.send(f"MODE {CHANNEL} -o carol")
    alice.drain(0.3); bob.drain(0.3); carol.drain(0.3)

    # verify no limit was actually set as a side effect
    alice.send(f"MODE {CHANNEL}")
    line = alice.expect(rf"{RPL_CHANNELMODEIS}", timeout=2)
    has_l = bool(line and re.search(r"\+\S*l", line))
    r.check("M41-state", not has_l, f"no stray +l left set after M41's aborted +ol command: {line!r}")

    # M42: general invariant — every flag that mutated state appears in the one broadcast line,
    # and nothing that DIDN'T mutate state appears in it either (checked via M40/M41 above +
    # a fresh combined case: +kl with key given but limit missing)
    alice.send(f"MODE {CHANNEL} +kl comboKey")
    line = bob.expect(r"MODE", timeout=2)
    k_applied = bool(line and "comboKey" in line and re.search(r"\+\S*k", line))
    l_absent = bool(line and not re.search(r"\+\S*l\b", line))
    r.check("M42", k_applied and l_absent,
            f"+kl with missing limit arg: k applied & broadcast, l ignored, single consistent broadcast: {line!r}")
    alice.send(f"MODE {CHANNEL} -k")
    alice.drain(0.3); bob.drain(0.3); carol.drain(0.3)

    # ---- Additional MODE edge cases ----

    # M43: unknown mode letter -> ERR_UNKNOWNMODE (472)
    alice.send(f"MODE {CHANNEL} +z")
    line = alice.expect(rf"{ERR_UNKNOWNMODE}", timeout=2)
    r.check("M43", line is not None, "ERR_UNKNOWNMODE (472) for unrecognized mode letter")

    # M44: non-op attempting to change MODE -> ERR_CHANOPRIVSNEEDED (482)
    # bob currently holds op from the M30 cleanup step, so de-op him first —
    # otherwise this test would silently succeed and, worse, leave a stray
    # +i set on the channel that corrupts later "clean state" assertions.
    alice.send(f"MODE {CHANNEL} -o bob")
    alice.drain(0.3); bob.drain(0.3); carol.drain(0.3)
    bob.send(f"MODE {CHANNEL} +s")
    line2 = bob.expect(rf"{ERR_CHANOPRIVSNEEDED}", timeout=2)
    r.check("M44", line2 is not None, "ERR_CHANOPRIVSNEEDED (482) when non-op attempts MODE change")
    alice.drain(0.3)

    # M45: +o targeting a user not in the channel -> ERR_USERNOTINCHANNEL (441)
    alice.send(f"MODE {CHANNEL} +o dave")
    line = alice.expect(rf"{ERR_USERNOTINCHANNEL}", timeout=2)
    r.check("M45", line is not None, "ERR_USERNOTINCHANNEL (441) granting +o to non-member")

    # M46: MODE on nonexistent channel -> ERR_NOSUCHCHANNEL (403)
    alice.send("MODE #ghostchannel +i")
    line = alice.expect(rf"{ERR_NOSUCHCHANNEL}", timeout=2)
    r.check("M46", line is not None, "ERR_NOSUCHCHANNEL (403) for MODE on unknown channel")

    # M47: MODE with no params at all -> ERR_NEEDMOREPARAMS (461)
    alice.send("MODE")
    line = alice.expect(rf"{ERR_NEEDMOREPARAMS}", timeout=2)
    r.check("M47", line is not None, "ERR_NEEDMOREPARAMS (461) when MODE given no args")

    # M48: +l with non-numeric argument — should either ignore flag or reject gracefully,
    # but must NOT crash the server or corrupt state (verified by a follow-up query working)
    alice.send(f"MODE {CHANNEL} +l notanumber")
    alice.drain(0.5)
    alice.send(f"MODE {CHANNEL}")
    line = alice.expect(rf"{RPL_CHANNELMODEIS}", timeout=2)
    r.check("M48", line is not None,
            f"server stays responsive & returns valid RPL_CHANNELMODEIS after +l with non-numeric arg: {line!r}")

    # ---- M50/M51: +o repeated more than 3x in a single command is capped ----
    # dave joins #test just for this segment so we have four valid targets
    dave.send(f"JOIN {CHANNEL}")
    dave.drain(0.3)
    alice.drain(0.3); bob.drain(0.3); carol.drain(0.3)

    # make sure only alice holds op going in, so the cap is unambiguous
    for target in ("bob", "carol", "dave"):
        alice.send(f"MODE {CHANNEL} -o {target}")
        alice.drain(0.3); bob.drain(0.3); carol.drain(0.3); dave.drain(0.3)

    alice.send(f"MODE {CHANNEL} +oooo bob carol dave alice")
    line = bob.expect(r"MODE", timeout=2)
    flags_token = ""
    args_after_flags = []
    if line:
        parts = line.split()
        # expected shape: [":alice!user@host", "MODE", "#test", "+oooo"-ish, "bob", "carol", ...]
        if len(parts) >= 4:
            flags_token = parts[3]
            args_after_flags = parts[4:]
    o_count_in_flags = flags_token.count("o")
    r.check("M50", line is not None and o_count_in_flags <= 3,
            f"+o repeated 4x in one command: at most 3 'o' flags in broadcast: {line!r}")
    r.check("M51", line is not None and len(args_after_flags) <= 3,
            f"+o repeated 4x in one command: at most 3 nick targets in broadcast args: {line!r}")

    # cleanup: strip any op state the above may have granted, part dave from #test
    for target in ("bob", "carol", "dave"):
        alice.send(f"MODE {CHANNEL} -o {target}")
        alice.drain(0.3); bob.drain(0.3); carol.drain(0.3); dave.drain(0.3)
    dave.send(f"PART {CHANNEL}")
    dave.drain(0.3)
    alice.drain(0.3); bob.drain(0.3); carol.drain(0.3)

    # M49: re-op bob and carol back to clean state for future test runs, remove any leftover +l
    alice.send(f"MODE {CHANNEL} -l")
    alice.drain(0.3); bob.drain(0.3); carol.drain(0.3)
    alice.send(f"MODE {CHANNEL}")
    line = alice.expect(rf"{RPL_CHANNELMODEIS} \S+ {re.escape(CHANNEL)} \+?\s*$", timeout=2)
    r.check("M49", line is not None, f"channel returns to clean/no-flags state at end of run: {line!r}")

    # ======================================================================
    for c in (alice, bob, carol, dave):
        c.close()

    ok = r.summary()
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()