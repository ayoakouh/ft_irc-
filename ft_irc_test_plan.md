# ft_irc Test Plan — PRIVMSG / TOPIC / MODE

Scope: your three handlers only. Based on modern.ircdocs.horse (primary,
living spec), cross-checked against RFC 1459 §4.4.1/§4.2.4/§4.2.3 and
RFC 2812 §3.3.1/§3.2.4/§3.2.3.

Setup assumed: 4 `nc` clients — **alice** (op, creates `#test` via JOIN),
**bob** and **carol** (regular members), **dave** (outsider, never joins).
Connect each with:

```
nc 127.0.0.1 <port>
PASS <password>
NICK alice
USER alice 0 * :Alice Test
```

(repeat for bob/carol/dave with different nicks). Then `alice` does
`JOIN #test` to create it, `bob`/`carol` join after.

Legend: ✅ = must pass for eval, ⚠️ = commonly missed edge case.

---

## 1. PRIVMSG

Spec: `PRIVMSG <target>{,<target>} <text to be sent>`. Numeric replies:
`401 ERR_NOSUCHNICK`, `404 ERR_CANNOTSENDTOCHAN`, `411 ERR_NORECIPIENT`,
`412 ERR_NOTEXTTOSEND`.

### 1.1 Basic delivery
| # | Test | Client | Command | Expected |
|---|------|--------|---------|----------|
| P1 ✅ | DM to online user | bob | `PRIVMSG alice :hi` | alice receives `:bob!user@host PRIVMSG alice :hi`; bob gets no echo/error |
| P2 ✅ | Message to channel | alice | `PRIVMSG #test :hello all` | bob, carol receive it; **alice does NOT receive her own message back** |
| P3 ✅ | Message to channel client isn't in | dave | `PRIVMSG #test :hi` | `442`/`404`-style rejection — check what your impl uses (spec allows `404 ERR_CANNOTSENDTOCHAN`) |

### 1.2 Error / edge cases
| # | Test | Command | Expected |
|---|------|---------|----------|
| P4 ✅ | No target | `PRIVMSG` (bare) | `411 ERR_NORECIPIENT` |
| P5 ✅ | Target but no text | `PRIVMSG bob` | `412 ERR_NOTEXTTOSEND` |
| P6 ✅ | Target but empty trailing | `PRIVMSG bob :` | Spec-legal — empty string is valid text (empty final param must use `:`). Should NOT trigger 412; message with empty text should be delivered |
| P7 ✅ | Nonexistent nick | `PRIVMSG ghost :hi` | `401 ERR_NOSUCHNICK` |
| P8 ✅ | Nonexistent channel | `PRIVMSG #ghost :hi` | `401 ERR_NOSUCHNICK` (spec doesn't define a separate channel-not-found code for PRIVMSG — most servers reuse 401; document what you chose) |
| P9 ⚠️ | Multiple targets, mixed valid/invalid | `PRIVMSG bob,ghost,#test :hi` | bob and #test get it; one `401` for ghost. This is your "multi-target loop with continue" — confirm it doesn't abort the whole command on one bad target |
| P10 ⚠️ | Multiple valid targets | `PRIVMSG bob,carol :hi` | Both receive independently formatted messages |
| P11 ⚠️ | Duplicate targets | `PRIVMSG bob,bob :hi` | Spec doesn't forbid; check you don't crash or double-send oddly — either behavior is defensible, just don't crash |
| P12 ⚠️ | Trailing param with colons inside | `PRIVMSG bob :hi: how are you` | Only first `:` after target starts trailing; rest of `:` chars are literal text — full text must arrive intact |
| P13 ⚠️ | Message containing leading space in trailing | `PRIVMSG bob : leading space` | Text = `" leading space"` (space preserved, only the syntactic `:` stripped) |
| P14 ⚠️ | Self-message via DM | `PRIVMSG alice :talking to myself` | Should work like any other DM (server doesn't special-case it per spec) |
| P15 ⚠️ | Message to self via channel exclusion | alice sends to `#test` | Confirm your self-exclusion logic in relay only skips echoing to the *sender*, not to other clients who happen to share nick prefix, etc. |

### 1.3 Format checks
| # | Test | Expected |
|---|------|----------|
| P16 ✅ | Relayed message hostmask | Must be full `nick!user@host`, not bare nick, per your own noted learning |
| P17 ⚠️ | Not registered yet sends PRIVMSG | `451 ERR_NOTREGISTERED` (not in your scope numerically, but confirm dispatcher blocks it before reaching your handler) |

---

## 2. TOPIC

Spec: `TOPIC <channel> [<topic>]`. If no topic param → view (`331`/`332`).
Empty string topic (`TOPIC #chan :`) clears it. `+t` requires ops to set.

### 2.1 Viewing
| # | Test | Client | Command | Expected |
|---|------|--------|---------|----------|
| T1 ✅ | View topic, none set | bob | `TOPIC #test` | `331 RPL_NOTOPIC` |
| T2 ✅ | View topic, one set | bob | `TOPIC #test` | `332 RPL_TOPIC` with current topic |
| T3 ⚠️ | View topic, RPL_TOPICWHOTIME | (after T2) | Spec: "If RPL_TOPIC is returned... RPL_TOPICWHOTIME SHOULD also be sent." You've flagged this as pending (needs Channel class fields) — confirm eval doesn't hard-require it, but note it as a known gap |
| T4 ⚠️ | Non-member views topic | dave | `TOPIC #test` | Spec says server MAY return `442 ERR_NOTONCHANNEL` — decide and document which behavior your impl picked (allow view, or reject) |
| T5 ✅ | View topic of nonexistent channel | any | `TOPIC #ghost` | `403 ERR_NOSUCHCHANNEL` |

### 2.2 Setting
| # | Test | Client | Command | Expected |
|---|------|--------|---------|----------|
| T6 ✅ | Set topic, no +t restriction | bob | `TOPIC #test :New topic` | Topic changes; **all members including bob** receive `TOPIC` broadcast with new value |
| T7 ✅ | Clear topic | alice | `TOPIC #test :` | Topic cleared; broadcast has empty trailing param |
| T8 ⚠️ | Set topic identical to current | alice | `TOPIC #test :New topic` (same as before) | Spec: servers MAY notify anyway — either behavior fine, just don't crash/desync |
| T9 ✅ | +t set, non-op tries to change | (after `MODE #test +t`) bob → `TOPIC #test :hack` | `482 ERR_CHANOPRIVSNEEDED`, topic unchanged |
| T10 ✅ | +t set, op changes topic | alice | `TOPIC #test :ops only` | Succeeds, broadcast to all |
| T11 ✅ | +t NOT set, non-op changes topic | (after `MODE #test -t`) bob → `TOPIC #test :anyone` | Succeeds — this is the classic bug: confirm you're not requiring ops when `+t` is absent |
| T12 ✅ | Set topic, not a member | dave | `TOPIC #test :intrude` | Should fail — likely `442 ERR_NOTONCHANNEL` (not explicitly listed for the "set" case in spec text but implied; RFC2812 lists 442 for TOPIC broadly) |
| T13 ✅ | Set topic on nonexistent channel | any | `TOPIC #ghost :x` | `403 ERR_NOSUCHCHANNEL` |
| T14 ⚠️ | Missing channel param | any | `TOPIC` (bare) | `461 ERR_NEEDMOREPARAMS` |

### 2.3 Edge cases
| # | Test | Command | Expected |
|---|------|---------|----------|
| T15 ⚠️ | Topic with spaces, no leading `:` needed if it's the only trailing form | `TOPIC #test :multi word topic here` | Full string preserved |
| T16 ⚠️ | Topic containing `:` mid-string | `TOPIC #test :time is 12:30` | Entire string after first `:` preserved literally |
| T17 ⚠️ | Very long topic | Send 500+ char topic | Confirm no buffer overflow/truncation crash (RFC caps messages at 512 bytes total — check TOPICLEN isn't silently corrupting rather than truncating cleanly) |
| T18 ⚠️ | Broadcast includes setter's own client | alice sets topic | alice (the setter) must ALSO receive the TOPIC broadcast per spec — "every client in that channel (including the author...)" |

---

## 3. MODE (channel modes only — `+i +t +k +o +l`)

Spec: `MODE <target> [<modestring> [<mode arguments>...]]`. Type
B/C modes (`k`, `o`, `l`) consume args positionally; type D (`i`, `t`)
never do. `472 ERR_UNKNOWNMODE` for unsupported flags,
`482 ERR_CHANOPRIVSNEEDED` if not op, `403 ERR_NOSUCHCHANNEL` for bad
channel, `324 RPL_CHANNELMODEIS` for query.

### 3.1 Query
| # | Test | Client | Command | Expected |
|---|------|--------|---------|----------|
| M1 ✅ | Query modes, no modestring | bob | `MODE #test` | `324 RPL_CHANNELMODEIS #test <current flags>` |
| M2 ⚠️ | Query modes with param-bearing modes active (e.g. +k, +l set) | alice | `MODE #test` (after `+kl` set) | Spec: "Servers MAY choose to hide sensitive information such as channel keys" — decide if you show the key or mask it; be consistent |
| M3 ✅ | Query nonexistent channel | any | `MODE #ghost` | `403 ERR_NOSUCHCHANNEL` |

### 3.2 +i (invite-only) — Type D, no arg
| # | Test | Command | Expected |
|---|------|---------|----------|
| M4 ✅ | Op sets +i | `MODE #test +i` | Applied; broadcast `MODE #test +i` with setter's hostmask, no arg consumed |
| M5 ✅ | Non-op tries +i | bob → `MODE #test +i` | `482 ERR_CHANOPRIVSNEEDED`, mode NOT applied |
| M6 ✅ | Op unsets -i | `MODE #test -i` | Applied, broadcast |
| M7 ⚠️ | Set +i when already +i | `MODE #test +i` twice | No crash, idempotent; broadcast still sent (or not — document choice) |

### 3.3 +t (topic-restricted) — Type D, no arg
| # | Test | Command | Expected |
|---|------|---------|----------|
| M8 ✅ | Op sets +t | `MODE #test +t` | Applied — cross-check with TOPIC tests T9–T11 above |
| M9 ✅ | Non-op tries +t | bob → `MODE #test +t` | `482`, unchanged |

### 3.4 +k (channel key) — Type B, always needs arg
| # | Test | Command | Expected |
|---|------|---------|----------|
| M10 ✅ | Op sets key | `MODE #test +k secret` | Applied; JOIN without key now must fail (cross-cutting with teammate's JOIN handler — worth a joint test) |
| M11 ✅ | Op unsets key | `MODE #test -k secret` (or `-k` alone, check ircd convention) | ⚠️ Spec ambiguity: RFC says type B "always have a parameter" including on unset. Some servers accept `-k` with no arg. **Decide which your server requires and test both** |
| M12 ⚠️ | Set +k with no arg supplied | `MODE #test +k` (no key) | Per spec: "If a type B or C mode does not have a parameter when being set, the server MUST ignore that mode" → mode should be silently skipped, not crash, not consume next flag's arg |
| M13 ✅ | Non-op sets key | bob → `MODE #test +k x` | `482`, unchanged |
| M14 ⚠️ | Key containing spaces | `MODE #test +k "my key"` | Undefined/implementation-specific — at minimum, confirm no crash; likely should reject or take first token only since IRC params can't contain raw spaces without `:` |

### 3.5 +o (operator privilege) — Type B, always needs arg (nick)
| # | Test | Command | Expected |
|---|------|---------|----------|
| M15 ✅ | Op grants op to member | `MODE #test +o bob` | bob becomes op; broadcast to all members |
| M16 ✅ | Op revokes op | `MODE #test -o bob` | bob loses op; broadcast |
| M17 ✅ | Non-op tries +o | carol → `MODE #test +o bob` | `482`, unchanged |
| M18 ⚠️ | +o target not in channel | `MODE #test +o dave` | Spec doesn't give explicit numeric here for ft_irc's stripped scope — common choice is `441 ERR_USERNOTINCHANNEL`. Decide and test |
| M19 ⚠️ | +o nonexistent nick | `MODE #test +o ghost` | Likely `401 ERR_NOSUCHNICK` or `441` — pick one, be consistent |
| M20 ⚠️ | +o with no arg | `MODE #test +o` | Missing param for type B → per spec, ignore that mode entirely (don't apply, don't crash, don't misconsume next arg) |
| M21 ⚠️ | Op removes own op | alice → `MODE #test -o alice` | Should succeed (spec doesn't forbid self-demotion) — worth confirming channel doesn't end up op-less in a broken state |

### 3.6 +l (user limit) — Type C, needs arg on set, not on unset
| # | Test | Command | Expected |
|---|------|---------|----------|
| M22 ✅ | Op sets limit | `MODE #test +l 2` | Applied; further JOINs beyond 2 members should get `471 ERR_CHANNELISFULL` (cross-cutting w/ JOIN) |
| M23 ✅ | Op unsets limit | `MODE #test -l` (no arg, per Type C rule) | Applied — sentinel reset to your `-1` "no limit" |
| M24 ⚠️ | -l WITH an arg supplied anyway | `MODE #test -l 5` | Per spec Type C "MUST NOT have a parameter when being unset" — decide: ignore the stray arg vs error. Just don't let it eat the next mode's argument by mistake |
| M25 ⚠️ | +l with non-numeric arg | `MODE #test +l abc` | Should reject cleanly (your inner-loop digit-validation `valid` flag pattern) — no crash, no partial state |
| M26 ⚠️ | +l with negative number | `MODE #test +l -5` | Note: `-5` looks like a flag switch to your char-by-char parser — confirm parser doesn't misinterpret this as `sign` toggling mid-arg. Likely should be rejected as invalid param, not parsed as a new mode block |
| M27 ⚠️ | +l with 0 | `MODE #test +l 0` | Edge case: does 0 mean "no one can join" or is it treated as invalid? Decide & document |
| M28 ✅ | Non-op sets limit | bob → `MODE #test +l 5` | `482`, unchanged |

### 3.7 Combined / multi-flag strings (this is where your refactor gets stress-tested)
| # | Test | Command | Expected |
|---|------|---------|----------|
| M29 ✅ | Multiple flags, one string, shared sign | `MODE #test +itk secret` | `+i`, `+t` applied (no args), `+k` consumes `secret` — `arg_idx` must advance only for k/o/l |
| M30 ✅ | Sign switch mid-string | `MODE #test +i-t` | `+i` applied, then sign flips to `-`, `t` removed — single command, single broadcast (or one broadcast per net change, whichever you chose — be consistent) |
| M31 ✅ | Multiple sign switches | `MODE #test +i-t+k pass` | i added, t removed, k added w/ "pass" — tests persistent `sign` tracking across multiple switches |
| M32 ⚠️ | Unknown flag mixed with known | `MODE #test +iX` | `i` applied; `X` triggers `472 ERR_UNKNOWNMODE` but doesn't abort processing of flags already handled — confirm partial application + error is your intended behavior (spec allows this for user modes explicitly; for channel modes, decide and document your choice since RFC doesn't spell it out as clearly) |
| M33 ⚠️ | Multiple args for multiple param-modes | `MODE #test +ol bob 5` | `o` consumes `bob`, `l` consumes `5` — `arg_idx` sequencing correctness, this is the main reason `arg_idx` needs to be persistent across the char loop |
| M34 ⚠️ | Args run out before flags do | `MODE #test +ok bob` (missing key) | `o` consumes `bob`; `k` has no arg left → per spec, ignore `k` (don't apply), don't crash reading past arg array |
| M35 ⚠️ | Empty modestring parts / stray `+`/`-` | `MODE #test +` or `MODE #test +-+i` | No crash; a bare `+`/`-` with nothing after it should just be a no-op, not desync the parser |
| M36 ⚠️ | Modestring with only `-` then valid flag | `MODE #test -i` | Straightforward unset, confirm sign defaults correctly when there's no leading `+` block first |

### 3.8 Broadcast correctness
| # | Test | Expected |
|---|------|----------|
| M37 ✅ | MODE broadcast hostmask | Full `nick!user@host` per your noted learning, not bare nick |
| M38 ⚠️ | MODE broadcast reaches all members, not just the target of +o/+k | Every member of `#test` gets the MODE line, not just alice/bob |
| M39 ⚠️ | MODE broadcast on no-op mode changes | If you send `MODE #test +i` and it's already +i (M7), decide whether to still broadcast — but never broadcast an empty/malformed modestring |

---

## 4. Cross-cutting checks (parser / connection level, still worth confirming since they affect your handlers)

| # | Test | Expected |
|---|------|----------|
| X1 ✅ | Blank line `\r\n` sent alone | Server doesn't crash — dispatcher's `s.empty()` guard |
| X2 ⚠️ | Command in lowercase | `privmsg bob :hi` — RFC says commands are compared case-insensitively; confirm your dispatcher uppercases/matches case-insensitively, not just literal `"PRIVMSG"` |
| X3 ⚠️ | Extra trailing whitespace / multiple spaces between params | `PRIVMSG   bob   :hi` | Should still parse (spec: one-or-more spaces are valid separators even though software SHOULD NOT emit >1) |
| X4 ⚠️ | No trailing CRLF, partial message buffered | Split across two `recv()` calls (hard to test via raw `nc` typing, but easy via `printf 'PRIV' ; sleep 1; printf 'MSG bob :hi\r\n'` piped to nc) — server must not process until full `\r\n` arrives |
| X5 ✅ | Message exceeding 512 bytes | Server should not crash; per spec, either `417 ERR_INPUTTOOLONG`, truncate, or drop — pick one and document |

---

## 5. Suggested nc test session (copy/paste script)

```bash
# Terminal 1 - alice (operator, creates channel)
nc 127.0.0.1 6667
PASS pass
NICK alice
USER alice 0 * :Alice
JOIN #test
MODE #test +t
TOPIC #test :welcome
MODE #test +l 3
MODE #test +k secret

# Terminal 2 - bob (member)
nc 127.0.0.1 6667
PASS pass
NICK bob
USER bob 0 * :Bob
JOIN #test secret
TOPIC #test :hack attempt      # expect 482
PRIVMSG #test :hi from bob
PRIVMSG alice :dm test

# Terminal 3 - carol (member)
nc 127.0.0.1 6667
PASS pass
NICK carol
USER carol 0 * :Carol
JOIN #test secret
MODE #test +o carol            # expect 482 (not op)

# Terminal 4 - dave (outsider, never joins)
nc 127.0.0.1 6667
PASS pass
NICK dave
USER dave 0 * :Dave
PRIVMSG #test :intrude          # expect 404/442
TOPIC #test :intrude            # expect 442
MODE #test +i                   # expect 482 (or 442, decide)
```

Watch each terminal for correct numerics and correctly-formatted broadcasts
(`nick!user@host`) landing in the *other* terminals, not just the sender's.

---

## 6. What to explicitly decide and document before eval

A few places above are genuinely spec-ambiguous — evaluators will ask
"why did you choose X", so have an answer ready for each:

1. Non-member viewing/setting TOPIC — reject with 442, or allow view but reject set?
2. `-k`/`-l` sent with a stray argument — ignore it or error?
3. Unknown mode flag mixed into a valid modestring — partial-apply + `472`, or reject the whole modestring?
4. `+o`/`+l` target validation order — which numeric wins when both "not op" and "bad arg" are true simultaneously?

Once you've picked, a one-line comment at each branch citing your own
decision will save you time in the defense.
