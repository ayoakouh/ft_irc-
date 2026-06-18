# ft_irc — Modern IRC Compliance Review

> Compliance analysis of **MODE**, **PRIVMSG**, and **TOPIC** implementations
> against the [Modern IRC Client Protocol Specification](https://modern.ircdocs.horse/).

---

## MODE

### Status: Mostly Compliant — Critical Gaps Present

---

### `RPL_CHANNELMODEIS (324)` — Missing

The spec states: if no `<modestring>` is given, the server **MUST** return numeric `324` with the channel's current modes.

Your current code fires `461 ERR_NEEDMOREPARAMS` when `s.size() < 3`. This is **incorrect** — `MODE #chan` with no modestring is a valid **query**, not a parameter error.

A branch is needed: if `s.size() == 2`, send `324` with the current active modes instead of rejecting the command.

**Format:**
```
:server 324 <nick> <channel> <modestring> [<mode arguments>...]
```

---

### `RPL_CREATIONTIME (329)` — Missing

The spec says servers **SHOULD** send `329` alongside `324` when responding to a mode query.

This is optional, but evaluators using HexChat or irssi may trigger this path and notice the absence.

**Format:**
```
:server 329 <nick> <channel> <unix_timestamp>
```

---

### Multi-flag MODE — Not Handled

Your implementation reads only `s[2][1]` and processes a single flag, with an early `return` in each branch.

Real IRC clients send combined mode strings naturally during normal use, for example:

```
MODE #chan +oi nick
MODE #chan +tki key
MODE #chan -il
```

Only the first flag is processed — the rest are silently ignored. This will be triggered by evaluators through normal client interaction without any special effort.

**The fix:** replace the current `if/else-if` chain with a character-by-character loop over `s[2]`, a dynamic `sign` variable, and a positional `arg_idx` counter starting at `s[3]` that advances only for flags that consume a parameter.

---

### `+k` Unset Argument — Minor Gap

The spec classifies `+k` as a **Type B** mode (always requires a parameter, both when setting and unsetting).

The standard convention when unsetting is to send `*` as a placeholder:
```
MODE #chan -k *
```

Your `-k` path broadcasts no argument, which some clients may misparse. Not a hard failure but worth aligning with spec.

---

### Mode Sign Validation — Wrong Numeric

The check for a missing `+`/`-` sign currently fires `472 ERR_UNKNOWNMODE`. That numeric is semantically for unknown mode *characters*, not for a malformed modestring prefix. In practice `461` or `472` are both seen in real servers here, so this is minor, but worth knowing.

---

### What Is Correct in MODE

- `442 ERR_NOTONCHANNEL` for non-members — correct
- `482 ERR_CHANOPRIVSNEEDED` for non-operators — correct
- Per-flag logic for `+i`, `+t`, `+k`, `+o`, `+l` — correct
- Broadcast to all channel members — correct
- `+l` integer validation and zero/negative rejection — correct
- `-l` sentinel value of `-1` — correct
- `serv_channel.find(ltarget)->second` instead of `operator[]` — correct

---

## PRIVMSG

### Status: Largely Compliant

---

### Numeric Usage — Correct

| Situation | Numeric Used | Spec Numeric | Status |
|---|---|---|---|
| No recipient | `411 ERR_NORECIPIENT` | 411 | ✅ |
| No text to send | `412 ERR_NOTEXTTOSEND` | 412 | ✅ |
| No such nick | `401 ERR_NOSUCHNICK` | 401 | ✅ |
| No such channel | `403 ERR_NOSUCHCHANNEL` | 403 | ✅ |
| Cannot send to channel | `404 ERR_CANNOTSENDTOCHAN` | 404 | ✅ |

---

### Message Format — Correct

```
:<nick>!<user>@ft_irc PRIVMSG <target> :<text>
```

Matches the spec's source prefix format exactly.

---

### Self-exclusion on Channel Messages — Correct

The sender does not receive their own message echoed back. This is standard IRC behaviour.

---

### `RPL_AWAY (301)` — Not Implemented (Acceptable)

The spec says: if the target user is marked as away, the server **MAY** reply with `301`. Since ft_irc does not implement the AWAY command, this is expected and acceptable to omit.

---

### Multi-target PRIVMSG — Not Supported (Low Risk)

The spec signature is `PRIVMSG <target>{,<target>}` — comma-separated multiple targets in one command.

Your implementation handles only a single target. Real clients rarely send multi-target PRIVMSG in normal use, and the ft_irc subject does not require it. This is unlikely to affect evaluation.

---

## TOPIC

### Status: Largely Compliant — One Notable Gap

---

### `RPL_TOPICWHOTIME (333)` — Missing

The spec states: *"If `RPL_TOPIC` is returned to the client sending this command, `RPL_TOPICWHOTIME` SHOULD also be sent."*

**Format:**
```
:server 333 <nick> <channel> <who_set_it> <unix_timestamp>
```

HexChat uses this numeric to render the "Topic set by X at Y" line. If absent, HexChat skips the line silently. irssi may show a blank placeholder. To implement this properly you would need to store the nickname of whoever last set the topic, and the Unix timestamp at the time it was set.

This is a **SHOULD**, not a **MUST** — whether it matters depends on evaluator strictness.

---

### Empty Topic to Clear — Edge Case to Test

The spec says `TOPIC #chan :` (empty trailing parameter) should **clear** the topic.

Your `setTopic(s[2])` with `s[2] = ""` likely works if your parser preserves empty trailing parameters, but many parsers drop empty tokens silently. This edge case is worth explicitly testing.

---

### What Is Correct in TOPIC

- `331 RPL_NOTOPIC` when no topic is set — correct
- `332 RPL_TOPIC` when a topic exists — correct
- `442 ERR_NOTONCHANNEL` for non-members — correct
- `482 ERR_CHANOPRIVSNEEDED` when `+t` is set and user is not op — correct
- Broadcast of `TOPIC` message to all channel members on change — correct

---

## Compliance Summary

| Issue | Command | Severity |
|---|---|---|
| `MODE #chan` (no modestring) must return `324`, not `461` | MODE | Critical |
| Multi-flag MODE not handled | MODE | Critical |
| Missing `RPL_TOPICWHOTIME (333)` alongside `RPL_TOPIC` | TOPIC | Should Fix |
| Missing `RPL_CREATIONTIME (329)` alongside `324` | MODE | Nice to Have |
| `+k` unset should send `*` as argument in broadcast | MODE | Minor |
| Multi-target PRIVMSG not supported | PRIVMSG | Low Risk |

---

## Checklist

### MODE — Critical

- [ ] Add a `s.size() == 2` branch that sends `324 RPL_CHANNELMODEIS` with the channel's current active modes instead of rejecting with `461`
- [ ] Refactor the `if/else-if` flag chain into a character-by-character loop over `s[2]` with a dynamic `sign` variable and a shared `arg_idx` counter (starting at `s[3]`) that only advances for parameter-consuming flags (`+k`, `+o`, `+l`)

### MODE — Should Fix

- [ ] Send `329 RPL_CREATIONTIME` immediately after `324` (requires storing channel creation timestamp)
- [ ] Change `-k` broadcast to include `*` as a trailing argument: `MODE #chan -k *`

### TOPIC — Should Fix

- [ ] Send `333 RPL_TOPICWHOTIME` immediately after `332 RPL_TOPIC` (requires storing who set the topic and when)
- [ ] Test that `TOPIC #chan :` with an empty trailing parameter correctly clears the topic through your parser

### PRIVMSG — Low Priority

- [ ] Test multi-target PRIVMSG (`PRIVMSG nick1,nick2 :hello`) to confirm behaviour (not required by subject but technically out of spec)
