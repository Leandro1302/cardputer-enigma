# USB protocol v1

Native USB Serial/JTAG CDC, nominal baud 115200. One JSON object per line; CRLF is
accepted. Send one request and wait for its response. Maximum request line is 2048
bytes, with a 2-second inactivity timeout for incomplete frames. The host client
uses bounded timeouts, handles fragmented/coalesced input, and never silently
retries a mutating request. IDs correlate requests; they do not provide deduplication.
On Linux the client keeps the native CDC driver's paired DTR/RTS defaults.
Changing the lines one at a time can reset the ESP32-S3. Reopening a serial session
on the tested Cardputer preserves the uptime and the cipher state.

Every request requires `id` (integer 0…2147483647) and `cmd` (uppercase string).
Successful replies include the same `id` and `ok:true`. Errors use
`{"id":1,"ok":false,"error":{"code":"...","message":"..."}}`.
Framing errors that cannot recover an ID return `id:null`. Unknown fields are rejected.

| Command | Additional fields | Result |
|---|---|---|
| PING | none | `reply:"PONG"`, protocol version |
| INFO | none | firmware/version, MAC, chip, flash, detected board, display dimensions/rotation |
| CONFIG | all configuration fields below | validates atomically and resets serial machine |
| STATE | none | current positions, letter count, configuration |
| ENCRYPT | `text`: printable ASCII, maximum 512 bytes | ciphertext and updated state |
| RESET | none | restores configured initial positions and zeroes letter count |
| SELFTEST | none | known-vector and double-step checks using a temporary machine |
| DIAG | none | uptime, reset reason, heap, request/error/frame/key counters, startup animation status and frame count |
| UI_STATE | none | GUI screen, selection, draft editor, messages, rotor state, physical key counter, and Wi-Fi broadcast status |
| UI_KEY | `key`: printable character or named action | injects an abstract action into the real GUI model and renders |
| SCREEN | `row`: integer 0…134 | 240 RGB565 pixels as 960 hexadecimal digits |

```json
{"id":1,"cmd":"CONFIG","rotors":[1,2,3],"rings":"AAA","positions":"AAA","reflector":"B","plugboard":""}
{"id":2,"cmd":"ENCRYPT","text":"AAAAA"}
```

The second response contains `ciphertext:"BDZGO"`, `positions:"AAF"` and
`letters_processed:5`. Rotor IDs run from 1 to 8 in left–middle–right order. Rings
and positions accept exactly three alphabetic letters, normalized to uppercase.
Reflector must be B or C. Plugboard accepts up to ten disjoint pairs separated by
spaces or commas. Empty plugboard is valid. Every CONFIG field is required.

Letters are uppercased and step the machine. Spaces and ASCII punctuation pass
through without stepping. Control characters, NUL, and non-ASCII input are rejected
before any state change. Stream long messages in successive ENCRYPT requests;
there is no cumulative message-length limit in the serial session.

UI_KEY names: ENTER, ESC, RESET, BACKSPACE, UP, DOWN, LEFT, RIGHT. LEFT and RIGHT
move the main menu carousel. UP and DOWN remain aliases there and navigate the
configuration list. Editing uses append and backspace. UI_KEY is for synthetic UI tests
and does not increment the physical keyboard counter. Serial CONFIG/ENCRYPT/RESET
affect the serial session only; UI_KEY affects the on-screen session.

In the message screen, ENTER opens `message_actions`. Its `message_action` index
is 0 for **Broadcast as Wi-Fi** or 1 for **Back**. Selecting the broadcast action
starts an open SoftAP with the ciphertext as its SSID and enters `broadcasting`.
ENTER, ESC, and BACKSPACE stop it and return to Message Actions. The `broadcast`
object reports `active`, `ssid`, and `open` from the firmware's Wi-Fi backend.
When inactive, the SSID is empty and `open` is false.

Errors include BAD_JSON, BAD_REQUEST, UNKNOWN_FIELD, UNKNOWN_COMMAND,
INVALID_CONFIG, INVALID_ROTOR, DUPLICATE_ROTOR, INVALID_RINGS, INVALID_POSITIONS,
INVALID_REFLECTOR, INVALID_PLUGBOARD, INVALID_TEXT, TEXT_TOO_LONG, INVALID_KEY,
INVALID_ROW, LINE_TOO_LONG and FRAME_TIMEOUT. Applications must check `ok`.
