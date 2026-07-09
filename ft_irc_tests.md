# ft_irc — Detailed Test List: PRIVMSG, TOPIC, MODE

> Tests are grouped by command and ordered from basic happy-path → error cases → edge cases.
> Each test lists: the raw `nc` input to send (after registration), the expected server response,
> and the relevant spec reference.
>
> **Setup assumed for every test block:**
> ```
> PASS <password>
> NICK alice
> USER alice 0 * :Alice
> JOIN #test
> ```
> A second client "bob" (same registration + JOIN #test) is used where relay is checked.

---

## 1. PRIVMSG

### 1.1 — Basic channel message
```
PRIVMSG #test :hello world
```
- Bob receives: `:alice!alice@host PRIVMSG #test :hello world`
- Alice receives: nothing (no echo to self)
- Spec: modern.ircdocs.horse §PRIVMSG — "The message is delivered to all users in the channel except the originator"

### 1.2 — Basic direct message (nick-to-nick)
```
PRIVMSG bob :hey there
```
- Bob receives: `:alice!alice@host PRIVMSG bob :hey there`
- Alice receives: nothing
- Note: the `bob` in the relayed message must be Bob's **canonical** nickname as stored by the server, not the sender's capitalisation

### 1.3 — Message with leading colon (trailing parameter)
```
PRIVMSG #test ::this starts with a colon
```
- Bob receives: `:alice!alice@host PRIVMSG #test ::this starts with a colon`
- Verifies that the trailing-parameter colon is parsed correctly and the double-colon body is preserved

### 1.4 — Message with spaces in body
```
PRIVMSG #test :hello   world   spaces
```
- Bob receives full body including all spaces
- Verifies the parser does not split on interior spaces once trailing has started

### 1.5 — Multi-target: channel + nick
```
PRIVMSG #test,bob :multi
```
- #test members (except alice) receive the message once
- Bob receives the message once as a DM
- Alice receives neither copy
- Each target is processed independently; errors on one target do not suppress delivery to the other

### 1.6 — Multi-target: two channels
Join alice and bob to both #test and #dev first.
```
PRIVMSG #test,#dev :broadcast
```
- All members of #test (except alice) receive it once on #test
- All members of #dev (except alice) receive it once on #dev
- No duplicate deliveries

### 1.7 — Case-insensitive nick target
```
PRIVMSG BOB :uppercase nick
```
- Bob receives: `:alice!alice@host PRIVMSG BOB :uppercase nick` (or `bob` depending on implementation)
- Server must match `BOB` to the registered nick `bob` case-insensitively

### 1.8 — Case-insensitive channel target
```
PRIVMSG #TEST :uppercase channel
```
- Delivered to #test members; #TEST and #test are the same channel

### 1.9 — ERR_NOTEXTTOSEND (412) — empty body
```
PRIVMSG #test :
```
- Server replies: `:server 412 alice :No text to send`
- Empty trailing parameter after colon must trigger 412

### 1.10 — ERR_NOTEXTTOSEND (412) — whitespace-only body
```
PRIVMSG #test :   
```
- Server replies: `:server 412 alice :No text to send`
- Body consisting solely of spaces is treated as empty

### 1.11 — ERR_NOTEXTTOSEND (412) — no parameters at all
```
PRIVMSG
```
- Server replies: `:server 411 alice :No recipient given (PRIVMSG)` (ERR_NORECIPIENT)

### 1.12 — ERR_NORECIPIENT (411) — target missing
```
PRIVMSG :just a body
```
- Server replies: `411 ERR_NORECIPIENT`

### 1.13 — ERR_NOSUCHNICK (401) — unknown nick
```
PRIVMSG nobody :hello
```
- Server replies: `:server 401 alice nobody :No such nick/channel`

### 1.14 — ERR_NOSUCHCHANNEL (403) — nonexistent channel
```
PRIVMSG #doesnotexist :hello
```
- Server replies: `403 ERR_NOSUCHCHANNEL`

### 1.15 — ERR_NOTREGISTERED (451) — unregistered client
Send without completing PASS/NICK/USER:
```
PRIVMSG #test :hello
```
- Server replies: `451 ERR_NOTREGISTERED`

### 1.16 — Sender is not in the channel
Alice parts #test, then tries:
```
PRIVMSG #test :i left but i'm back
```
- Expected: either `404 ERR_CANNOTSENDTOCHAN` or `403 ERR_NOSUCHCHANNEL` depending on whether the channel still exists
- Must NOT be delivered to channel members

### 1.17 — Multi-target partial error: one bad nick + one good channel
```
PRIVMSG nobody,#test :hello
```
- `401` is sent for `nobody`
- Message IS delivered to #test (the good target is not suppressed)
- Only one relay, not duplicated

### 1.18 — PRIVMSG to self (nick DM to own nick)
```
PRIVMSG alice :talking to myself
```
- Most IRC servers deliver this back to the sender; acceptable to either deliver or ignore
- Must not crash or hang

### 1.19 — Very long message body
Send a body of ~490 characters (near the 512-byte line limit):
```
PRIVMSG #test :<490-char string>
```
- Must be relayed without truncation or crash
- Body must reach bob intact

---

## 2. TOPIC

### 2.1 — Query topic when none is set
```
TOPIC #test
```
- Server replies: `:server 331 alice #test :No topic is set`

### 2.2 — Set topic (by operator)
Alice is the channel operator (she created #test).
```
TOPIC #test :Welcome to the test channel
```
- Server replies/broadcasts to all channel members: `:alice!alice@host TOPIC #test :Welcome to the test channel`
- Subsequent `TOPIC #test` query returns `332 RPL_TOPIC` with the new text

### 2.3 — Query topic after it is set
```
TOPIC #test
```
- Server replies:
  - `332 alice #test :Welcome to the test channel`
  - `333 alice #test alice <unix_timestamp>` (if implemented)

### 2.4 — Clear topic by sending empty trailing
```
TOPIC #test :
```
- Server broadcasts: `:alice!alice@host TOPIC #test :`
- Subsequent query returns `331 RPL_NOTOPIC`

### 2.5 — Non-operator cannot set topic (default, +t not needed)
Without +t set, a regular channel does not restrict TOPIC. If your implementation enforces +t-only-restriction by default, skip to 2.6.
Bob (non-op) tries:
```
TOPIC #test :bob's topic
```
- If +t is not set: topic changes; all members notified
- If +t is implicitly set: `482 ERR_CHANOPRIVSNEEDED`

### 2.6 — +t mode blocks non-op from changing topic
First set +t:
```
MODE #test +t
TOPIC #test :operator set this
```
Then bob tries:
```
TOPIC #test :bob's attempt
```
- Server replies to bob: `:server 482 bob #test :You're not channel operator`
- Topic does NOT change; alice's topic remains

### 2.7 — Operator can change topic even with +t
After step 2.6, alice changes:
```
TOPIC #test :operator updated
```
- Succeeds; all members get the TOPIC broadcast

### 2.8 — -t lifts the restriction
```
MODE #test -t
```
Now bob tries again:
```
TOPIC #test :bob can now set it
```
- Succeeds; topic changes

### 2.9 — Topic with spaces in body
```
TOPIC #test :this topic has lots of spaces and : colons
```
- Full body preserved in relay and in 332 reply

### 2.10 — ERR_NEEDMOREPARAMS (461) — no parameters
```
TOPIC
```
- Server replies: `461 ERR_NEEDMOREPARAMS`

### 2.11 — ERR_NOTONCHANNEL (442) — client not in channel
Alice parts, then:
```
TOPIC #test
```
- Server replies: `442 ERR_NOTONCHANNEL`

### 2.12 — ERR_NOSUCHCHANNEL (403) — nonexistent channel
```
TOPIC #doesnotexist
```
- Server replies: `403 ERR_NOSUCHCHANNEL`

### 2.13 — TOPIC broadcast reaches all channel members
Three clients: alice (op), bob, carol — all in #test.
Alice sets:
```
TOPIC #test :everyone sees this
```
- alice, bob, and carol all receive `:alice!alice@host TOPIC #test :everyone sees this`

### 2.14 — Topic is shown on JOIN
After alice sets a topic, dave joins:
```
JOIN #test
```
- Dave receives `332 RPL_TOPIC` and `333 RPL_TOPICWHOTIME` (if implemented) as part of the join burst

### 2.15 — Unregistered client cannot set topic
```
TOPIC #test :no auth
```
- Server replies: `451 ERR_NOTREGISTERED`

---

## 3. MODE

### Preliminary: MODE query

#### 3.1 — Query current modes (RPL_CHANNELMODEIS 324)
```
MODE #test
```
- Server replies: `:server 324 alice #test <modestring> [<args>]`
- On a fresh channel with no modes: `324 alice #test +`

### +i / -i — Invite-only

#### 3.2 — Set +i (operator)
```
MODE #test +i
```
- Server broadcasts: `:alice!alice@host MODE #test +i`
- `MODE #test` now returns `+i`

#### 3.3 — Non-member cannot join +i channel (not your handler, but verifies +i state)
Dave (not in #test) tries to join after +i is set without invite:
```
JOIN #test
```
- Dave gets: `473 ERR_INVITEONLYCHAN`

#### 3.4 — Remove -i
```
MODE #test -i
```
- Broadcasts: `:alice!alice@host MODE #test -i`
- Channel is no longer invite-only

#### 3.5 — Non-op cannot set +i
```
MODE #test +i  (sent by bob, a regular user)
```
- Server replies: `:server 482 bob #test :You're not channel operator`
- Mode does NOT change

### +t / -t — Topic restriction

#### 3.6 — Set +t
```
MODE #test +t
```
- Broadcasts: `:alice!alice@host MODE #test +t`

#### 3.7 — Remove -t
```
MODE #test -t
```
- Broadcasts: `:alice!alice@host MODE #test -t`

### +k / -k — Channel key

#### 3.8 — Set +k
```
MODE #test +k secretpass
```
- Broadcasts: `:alice!alice@host MODE #test +k secretpass`
- `MODE #test` query returns `+k secretpass`

#### 3.9 — Join with correct key succeeds
Dave tries:
```
JOIN #test secretpass
```
- Dave joins successfully

#### 3.10 — Join with wrong key fails
Dave tries:
```
JOIN #test wrongpass
```
- Dave gets: `475 ERR_BADCHANNELKEY`

#### 3.11 — Remove -k
```
MODE #test -k
```
OR per RFC convention:
```
MODE #test -k *
```
- Broadcasts removal; subsequent JOINs without key succeed
- `MODE #test` no longer shows `+k`

#### 3.12 — Setting +k without argument
```
MODE #test +k
```
- Server replies: `461 ERR_NEEDMOREPARAMS`

### +o / -o — Channel operator privilege

#### 3.13 — Grant +o to a regular member
```
MODE #test +o bob
```
- Broadcasts: `:alice!alice@host MODE #test +o bob`
- Bob can now perform operator actions

#### 3.14 — Bob (now op) can set modes
```
MODE #test +t  (sent by bob after +o)
```
- Succeeds; broadcasts from bob's prefix

#### 3.15 — Revoke -o
```
MODE #test -o bob
```
- Broadcasts: `:alice!alice@host MODE #test -o bob`
- Bob loses operator status

#### 3.16 — +o on a nick not in the channel
```
MODE #test +o nobody
```
- Server replies: `441 ERR_USERNOTINCHANNEL`

#### 3.17 — +o without argument
```
MODE #test +o
```
- Server replies: `461 ERR_NEEDMOREPARAMS`

#### 3.18 — Non-op cannot grant +o
Bob (regular):
```
MODE #test +o carol
```
- Server replies: `482 ERR_CHANOPRIVSNEEDED`

### +l / -l — User limit

#### 3.19 — Set +l
```
MODE #test +l 3
```
- Broadcasts: `:alice!alice@host MODE #test +l 3`
- `MODE #test` returns `+l 3`

#### 3.20 — Channel rejects join when full (checked by JOIN handler / teammate)
After 3 members are in #test, a 4th tries to join:
```
JOIN #test
```
- Gets: `471 ERR_CHANNELISFULL`

#### 3.21 — Remove -l
```
MODE #test -l
```
- Broadcasts: `:alice!alice@host MODE #test -l`
- No limit; new members can join

#### 3.22 — +l without argument
```
MODE #test +l
```
- Server replies: `461 ERR_NEEDMOREPARAMS`

#### 3.23 — +l with non-numeric or zero argument
```
MODE #test +l abc
MODE #test +l 0
```
- Both should either return `696 ERR_INVALIDMODEPARAM` or be silently ignored; must NOT crash or set a corrupt limit
- Minimum acceptable: the limit is not changed

### Combined / multi-flag

#### 3.24 — Set multiple modes in one command
```
MODE #test +it
```
- Broadcasts: `:alice!alice@host MODE #test +it`
- Both `+i` and `+t` are applied; `MODE #test` confirms both are active

#### 3.25 — Mixed set/unset in one string
```
MODE #test +i-t
```
- Sets +i and removes +t simultaneously
- Broadcast must reflect only what changed

#### 3.26 — Redundant mode (already set)
```
MODE #test +i   (when +i is already active)
```
- Acceptable: no broadcast, or broadcast anyway
- Must NOT crash; mode state must not corrupt

#### 3.27 — Unknown mode character
```
MODE #test +z
```
- Server replies: `:server 472 alice z :is unknown mode char to me`
- The bad char `z` (not channel name, not full modestring) appears in the numeric

#### 3.28 — Mixed valid + unknown modes
```
MODE #test +iz
```
- `+i` is applied and broadcast
- `472` is sent for `z`
- The valid mode is NOT rolled back

### Error / edge cases

#### 3.29 — ERR_NEEDMOREPARAMS — no channel given
```
MODE
```
- Server replies: `461 ERR_NEEDMOREPARAMS`

#### 3.30 — ERR_NOSUCHCHANNEL — channel does not exist
```
MODE #doesnotexist +i
```
- Server replies: `403 ERR_NOSUCHCHANNEL`

#### 3.31 — ERR_NOTREGISTERED — unregistered client
```
MODE #test +i
```
- Server replies: `451 ERR_NOTREGISTERED`

#### 3.32 — Mode broadcast prefix must be sender's full mask
```
MODE #test +t
```
- Broadcast format: `:alice!alice@host MODE #test +t`
- Not `:server MODE #test +t` and not `:alice MODE #test +t`

#### 3.33 — Operator grants themselves op (already op)
```
MODE #test +o alice
```
- Should not crash; may broadcast (some servers do, some don't)

#### 3.34 — +o followed by immediate -o in same command
```
MODE #test +o-o bob
```
- Net result: no change in bob's operator status
- Must not crash

#### 3.35 — arg_idx does not consume argument for non-argument modes
```
MODE #test +oi bob
```
- `+o` consumes `bob` as the argument
- `+i` takes no argument; must not accidentally consume a nonexistent next argument
- Both `+o bob` and `+i` are applied

#### 3.36 — Mode query returns correct modestring after multiple operations
After:
```
MODE #test +i
MODE #test +k pass
MODE #test +l 5
MODE #test -i
```
A `MODE #test` query should return `+kl pass 5` (or equivalent ordering, but accurate).

---

## 4. Cross-command integration tests

### 4.1 — PRIVMSG fails after +k if non-member
Dave never joined (was rejected by +k), then sends:
```
PRIVMSG #test :hello from outside
```
- Gets `404 ERR_CANNOTSENDTOCHAN` or `403 ERR_NOSUCHCHANNEL`

### 4.2 — TOPIC change is visible in PRIVMSG session context
Alice sets `TOPIC #test :new`, then bob sends `PRIVMSG #test :anyone see that?` — both operate on the same channel state without corruption.

### 4.3 — Partial command stress test (per subject §IV.3)
Send the MODE command in three TCP writes using `nc -C` and `ctrl+D`:
```
MOD    (ctrl+D)
E #test    (ctrl+D)
+t\r\n
```
- Server must buffer and process the complete command once `\r\n` arrives
- Other connected clients must not be affected during the partial-send period

### 4.4 — Channel survives operator leaving and rejoining
Alice (only op) leaves #test, re-joins. Depending on implementation:
- She may regain op automatically (if she re-creates the channel) or not
- The mode state (+t, +k, etc.) may reset on empty channel (implementation choice)
- Must not crash

### 4.5 — Memory leak check under mode flood
Using `valgrind ./ircserv <port> <pass>`, connect one client and rapidly fire 50 MODE commands alternating `+i` and `-i`. After server exit, check for `definitely lost` in valgrind output.

