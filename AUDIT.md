# ArduiMum — Full Project Audit

**Date:** 2026-07-09
**Scope:** `projetMum/projetMum.ino`, `projetMum/projetMumLib.ino`, `projetMum/config.h` (vendored Adafruit/Time libraries excluded — stock upstream code).
**Context:** Arduino Mega 2560 + Ethernet shield + 64x32 RGB matrix, running unattended on a home LAN. The device is an *appliance*: the top priority is that it never needs a manual power-cycle and never shows wrong data silently.

Findings are ranked from most to least important. Each includes a ready-to-use prompt for an AI assistant to fix the issue.

---

## 1. CRITICAL — Buffer overflow: `strcpy_P` into a 25-byte shared buffer with strings up to 33 bytes

**Where:** `config.h:429` (`char buffer[25];`) + every `strcpy_P(buffer, ...)` call site in `projetMumLib.ino`.

**Problem:** The global scratch buffer is 25 bytes, but several PROGMEM strings copied into it are longer:
- `s_r_44` `"Eteindre l'affichage des bus"` → 28 chars + NUL = **29 bytes**
- `s_r_53` `"Passer en heure d'&eacutet&eacute"` → 33 chars + NUL = **34 bytes**
- `s_r_37` `"Eteindre l'&eacutecran"`, `s_r_85`, and others sit right at or past the boundary.

Every time the `/admin` page is served or those button labels are printed, `strcpy_P` writes past the end of `buffer`, corrupting whatever the linker placed after it in SRAM (other globals — on AVR there is no memory protection). This produces "impossible" bugs: variables changing value spontaneously, random crashes, display glitches. This is the single most likely cause of any long-term instability the device has.

**AI fix prompt:**
> In the ArduiMum Arduino project, `config.h` declares `char buffer[25]` which is used as the destination of `strcpy_P()` for every PROGMEM string in `projetMumLib.ino`. Several PROGMEM strings (e.g. `s_r_44`, `s_r_53` in `config.h`) exceed 24 characters, so `strcpy_P` overflows the buffer and corrupts adjacent SRAM. Fix this by (a) measuring the longest string in `server_response[]` and `rec_str[]` and sizing `buffer` accordingly with a safety margin (e.g. 40 bytes), and (b) preferably eliminating the shared buffer entirely by replacing the PROGMEM string tables with the `F()` macro (e.g. `client.print(F("...HTML..."))` and `msg.replace(F("%20"), F(" ")))` is not supported — for `String::replace` keep small stack-local buffers). Verify no remaining `strcpy_P` destination can overflow. Target: Arduino Mega 2560, avr-gcc.

---

## 2. CRITICAL — Any LAN client can hang the device forever: `busServer()` has no timeout

**Where:** `projetMumLib.ino:128-164` (`busServer`).

**Problem:** `while (busClient.connected())` loops until the client disconnects or a full header line arrives. A client that connects and then sends nothing (a port scanner, a smart-TV probing the network, a half-open TCP connection after Wi-Fi drop on the caller's side) blocks this loop **forever**. The main `loop()` is stuck inside `serverCallback()`, so the clock stops updating, the display freezes, and only a power-cycle recovers it. On a home LAN with phones, TVs and a router that probes devices, this *will* eventually happen.

**AI fix prompt:**
> In `projetMumLib.ino` of the ArduiMum Arduino project, the `busServer(EthernetClient&)` function loops on `while (busClient.connected())` with no timeout, so a silent or half-open TCP client hangs the entire main loop permanently. Add a timeout: record `unsigned long start = millis();` before the loop and break out (then `busClient.stop()`) if `millis() - start > 2000`. Handle `millis()` rollover correctly using subtraction. Keep the existing request-parsing behaviour unchanged for well-behaved clients.

---

## 3. CRITICAL — `nightClub()` is an unauthenticated, unrecoverable denial-of-service (and a strobe hazard)

**Where:** `projetMumLib.ino:1384-1393`, triggered from `busAPI()` when any request contains `nightclub`.

**Problem:** `nightClub()` is `while(1)` with no exit. Any device on the LAN that sends `GET /nightclub` locks the display into a full-brightness 5 Hz strobe **until someone unplugs it**. Beyond the DoS: a full-white 64x32 panel flashing at 5 Hz draws ~3A pulses (your own TODO notes 3A max) and flashing lights in the 3–30 Hz range are a photosensitive-epilepsy risk. An easter egg is fine, but it must be exitable.

**AI fix prompt:**
> In `projetMumLib.ino` of the ArduiMum project, the `nightClub()` easter-egg function runs `while(1)` flashing the whole RGB matrix and can be triggered by any HTTP request containing "nightclub", permanently locking the device. Rework it to run for a fixed duration (e.g. 5 seconds) OR until a new HTTP request arrives (poll `server.available()` inside the loop and break), then restore the previous display state (`matrix.fillScreen(0)` and force redraw by resetting `busOnDisplay`, `TWOnDisplay`, `lastMin`). Also reduce the flash brightness and slow the flash rate to below 3 Hz to avoid photosensitivity risk.

---

## 4. CRITICAL — DHCP failure enters `for(;;)` with no watchdog: device bricks until manual power-cycle

**Where:** `projetMumLib.ino:37` (`for(;;);` after 30 failed DHCP attempts), and no watchdog anywhere in the project.

**Problem:** If the router is rebooting when the Arduino powers up (typical after a power outage — exactly when both devices restart together), DHCP fails 30 times over ~30 s and the sketch parks in an infinite loop. Your mother then has a dead display until someone unplugs/replugs it. For an unattended appliance this is the worst failure mode. There is also no watchdog timer, so *any* hang (findings #2, #3, or a library bug) is permanent.

**AI fix prompt:**
> In the ArduiMum Arduino Mega project (`projetMum.ino` / `projetMumLib.ino`), DHCP failure in `chooseServerNTP()` ends in `for(;;);`, permanently bricking the device until power-cycle, and there is no watchdog. Make two changes: (1) enable the AVR watchdog (`#include <avr/wdt.h>`, `wdt_enable(WDTO_8S)` in `setup()`, `wdt_reset()` at the top of `loop()` and inside the DHCP retry loop and `busServer` loop); (2) replace the `for(;;)` on DHCP failure with: display "Error DHCP" on the matrix, wait 60 s, then trigger a clean restart via watchdog timeout (stop calling `wdt_reset()`), so the device retries DHCP forever instead of dying. Note: on some Mega 2560s with old bootloaders the watchdog can cause a boot loop — mention this and use the Optiboot-safe pattern (`MCUSR = 0; wdt_disable();` first thing in `setup()`).

---

## 5. HIGH — Unsequenced double `strcpy_P` into the *same* buffer: URL decoding is broken/undefined

**Where:** `projetMumLib.ino:269-289` (all `msg.replace(strcpy_P(buffer, A), strcpy_P(buffer, B))` lines).

**Problem:** Both arguments of `msg.replace(...)` copy into the **same** global `buffer` and both return a pointer to it. C++ evaluates function arguments in unspecified order, and nothing forces the first pointer to be converted to a `String` before the second `strcpy_P` overwrites the buffer. Depending on compiler version/optimisation this degenerates into `msg.replace(X, X)` — a no-op — or worse. The URL-decoding of messages (`%20` → space, accents, etc.) is therefore unreliable by construction, even when it happens to work today.

**AI fix prompt:**
> In `projetMumLib.ino` of the ArduiMum project, the message URL-decoding block calls `msg.replace(strcpy_P(buffer, ptrA), strcpy_P(buffer, ptrB))` where both `strcpy_P` calls target the same global `buffer` — argument evaluation order makes this undefined/incorrect. Replace the whole block with a proper in-place URL-percent-decoder: write a small function `void urlDecode(String &s)` that walks the string once, converts `+` to space and `%XX` hex sequences to bytes (handling multi-byte UTF-8 like `%C3%A9` naturally, since decoding bytes individually yields correct UTF-8), and use it instead of the ~20 chained `replace()` calls. This also removes most entries from the `rec_str[]` PROGMEM table in `config.h` — clean those up.

---

## 6. HIGH — The clock never re-syncs: `updateServerTime()` is unreachable from `loop()`

**Where:** `projetMumLib.ino:96-113`; `ntpSyncTime` in `config.h:100` claims "Syncs to NTP server every 1 hour".

**Problem:** `updateServerTime()` is only called when an HTTP request containing `updateTime` arrives. The main `loop()` never calls it. Unless an external cron job polls `/updateTime`, the clock free-runs on the Arduino's ceramic resonator (typically ±0.5%, i.e. **up to ~7 minutes drift per day**) forever. For a device whose main job is showing the time and bus departures in minutes, this silently becomes wrong within days.

**AI fix prompt:**
> In the ArduiMum Arduino project, `updateServerTime()` (in `projetMumLib.ino`) is supposed to re-sync NTP every `ntpSyncTime` seconds but is only invoked via an HTTP endpoint, never from `loop()`. Add a call to `updateServerTime()` in `loop()` in `projetMum.ino` (it already self-gates on `now()-ntpLastUpdate > ntpSyncTime`, so calling it every iteration is safe). Make sure the blocking retry inside it (`while(!getTimeAndDate() && trys<10)` with a 1 s `delay` per try in `getTimeAndDate`) is bounded — reduce retries to 3 per attempt and, on failure, retry on the next hour boundary rather than blocking for 10 s. Also note that after DST adjustment via `adjustTime()`, `now()` shifts — re-derive `ntpLastUpdate` after any `adjustTime()` call so the sync interval stays correct.

---

## 7. HIGH — Unvalidated network input can freeze the display for over a minute per frame

**Where:** `projetMumLib.ino:298-299` (`varSpeedMsg = 100 - ...toInt()`), `:221` (`varSpeedWeather = ...toInt()`), `:250` (`dimmer = HTTP_req.substring(14, 16).toInt()`).

**Problem:**
- `varSpeedMsg` is `unsigned int`. Sending `msg_speed=200` computes `100 - 200 = -100`, which wraps to **65436**, so `displayMsg()` calls `delay(65436)` — the device freezes ~65 s per scrolled frame and ignores the network meanwhile.
- `varSpeedWeather` accepts any value 0–65535 with no bounds check (same 65 s freeze potential). `busSpeed` *is* validated (2–60) — the same guard is simply missing elsewhere.
- `dimmer` is parsed from hard-coded character positions `substring(14, 16)`, which only works for one exact URL shape and silently parses garbage otherwise.

**AI fix prompt:**
> In `projetMumLib.ino` of the ArduiMum project, several HTTP-supplied values lack validation: `varSpeedMsg` (unsigned, computed as `100 - input`, wraps to ~65000 for inputs > 100), `varSpeedWeather` (unbounded, used directly in `delay()`), and `dimmer` (parsed from magic substring offsets 14–16). Add validation mirroring the existing `busSpeed` pattern: clamp `msg_speed` input to 0–100 before the subtraction, clamp `varSpeedWeather` to a sane range (e.g. 100–2000 ms), and parse `dimmer` by locating the `=` separator with `indexOf` instead of fixed offsets, clamping to 0–100. Reject (ignore) out-of-range values.

---

## 8. HIGH — Responses are not valid HTTP: the status line is commented out

**Where:** `projetMumLib.ino:140-143` — `//busClient.println("HTTP/1.1 200 OK"); ... busClient.println();`.

**Problem:** The server replies with a blank line followed by a body — no status line, no `Content-Type`, no `Connection: close`. Modern browsers and HTTP clients (fetch, requests, curl with default settings) either reject this or mis-render it; the `/admin` page and the JSON endpoints work only by luck with lenient clients. This also breaks the page's own `xmlHttp.status == 200` check in the embedded JavaScript — the confirmation "Requête bien reçue" logic can never fire reliably.

**AI fix prompt:**
> In `projetMumLib.ino` of the ArduiMum Arduino project, `busServer()` has the HTTP response status line and headers commented out, so all responses are malformed (body with no `HTTP/1.1 200 OK`, no `Content-Type`). Restore proper minimal headers before dispatching to `busAPI()`: `HTTP/1.1 200 OK`, `Content-Type: text/html` for the `/admin` page and `application/json` for the JSON endpoints, plus `Connection: close` and a blank line. Since content type differs per endpoint, move header emission into `busAPI()` so each branch sends the right `Content-Type` first. Use `F()` macros for the header strings to save SRAM.

---

## 9. HIGH — Heavy `String` usage on 8 KB of SRAM: heap fragmentation will eventually crash the device

**Where:** `config.h:124-141` (`String` globals: `HTTP_req`, `msg`, `temp`, `bus_C3[]`, `bus_9[]`, `busOnDisplay`, `TWOnDisplay`, `weather_cond_txt`), plus per-request concatenations like `"{\"busSpeed\" : "+String(varSpeedBus)+"}"` in `projetMumLib.ino`.

**Problem:** The Mega has 8 KB SRAM. Every HTTP request grows/shrinks `HTTP_req` char by char (`HTTP_req += c` — a realloc per byte), every message runs ~20 `String::replace` passes, and JSON responses build temporary concatenated `String`s. Over weeks of uptime the heap fragments; eventually an allocation fails and Arduino `String` fails *silently* (empty strings, or a crash when free memory runs out between heap and stack). This is the classic slow-death pattern for always-on AVR sketches.

**AI fix prompt:**
> The ArduiMum Arduino Mega sketch (`projetMum/`) uses Arduino `String` extensively for an always-on device: `HTTP_req` accumulates bytes with `+=`, message URL-decoding chains ~20 `String::replace` calls, and JSON responses concatenate temporaries. Refactor to fixed buffers to eliminate heap fragmentation: (1) replace `HTTP_req` with a `char http_req[120]` filled with an index counter; (2) parse with `strstr`/`strtok_r`/`atoi` instead of `indexOf`/`substring`/`toInt`; (3) replace `msg`, `temp`, `bus_C3[]`, `bus_9[]`, `busOnDisplay`, `TWOnDisplay`, `weather_cond_txt` with fixed `char` arrays (msg ~100 bytes, others small) and represent `busOnDisplay`/`TWOnDisplay`/`weather_cond_txt` as small enums instead of strings; (4) emit JSON with several `client.print()` calls instead of `String` concatenation. Do this incrementally and keep behaviour identical; the display logic in `projetMumLib.ino` compares `weather_cond_txt` by name — switch those comparisons to the enum.

---

## 10. MEDIUM — No authentication or safe-method discipline on the control server

**Where:** entire HTTP interface (`busAPI()`), `EthernetServer server(80)` in `config.h`.

**Problem:** Anyone on the LAN can turn the screen off, change the time, display arbitrary messages, or trigger findings #2/#3. On a private home network this is an accepted trade-off, but note: (a) all state changes are plain `GET`s, so any web page your mother visits could flip settings via a hidden `<img src="http://192.168.1.111/screen=off">` (CSRF from the browser *inside* the LAN — no port-forwarding needed); (b) if the router ever enables UPnP port-forwarding or the device is moved, the server is wide open. A shared-secret token is cheap insurance.

**AI fix prompt:**
> The ArduiMum Arduino web server (`busAPI()` in `projetMumLib.ino`) performs all state changes via unauthenticated GET requests, making it vulnerable to cross-site request forgery from any web page visited on the same LAN. Add a lightweight shared-secret check: define `const char API_KEY[] PROGMEM` in `config.h`, require every state-changing request to include `key=<secret>` in the query string, and return `HTTP/1.1 403 Forbidden` otherwise. Keep read-only endpoints (`busSpeed?`, `whatTime`, `screenState`) unauthenticated for convenience. Update the embedded `/admin` page JavaScript to append the key to its `httpGetAsync` URLs, and serve `/admin` itself only with the key. Keep it simple — this is defence for a home LAN, not the internet.

---

## 11. MEDIUM — Contradictory network init: static IP is set, then immediately discarded for DHCP

**Where:** `projetMum.ino:21` (`Ethernet.begin(mac, ip)`) then `projetMumLib.ino:19` (`DHCP = Ethernet.begin(mac)` inside `chooseServerNTP()`).

**Problem:** `setup()` configures the static IP `192.168.1.111`, then `chooseServerNTP()` immediately re-initialises with DHCP, discarding it. Whatever pushes bus/weather data to the device (the external API client) needs a stable address; after a router reboot DHCP may assign a different IP and the display silently stops receiving updates. Pick one strategy — and since a server needs a known address, static (or a DHCP reservation in the router) is the right one. The debug print at `projetMumLib.ino:16-17` also prints `Ethernet.localIP()` *before* `Ethernet.begin()` has run meaningfully.

**AI fix prompt:**
> In the ArduiMum Arduino project, `setup()` in `projetMum.ino` calls `Ethernet.begin(mac, ip)` (static 192.168.1.111) but `chooseServerNTP()` in `projetMumLib.ino` immediately re-runs `Ethernet.begin(mac)` with DHCP, discarding the static address the push-based data API depends on. Refactor: keep static IP as primary (also configure `dns`, `gateway`, `subnet` parameters explicitly), and use DHCP only as a fallback if you can detect link problems. Remove the duplicate initialisation, move all network bring-up into one function, and only fall through to the "Error DHCP" screen if both strategies fail. Also fix the debug print that logs `Ethernet.localIP()` before initialisation completes.

---

## 12. MEDIUM — `Udp.begin()` called on every NTP sync without `Udp.stop()`: potential socket exhaustion

**Where:** `projetMumLib.ino:62` inside `getTimeAndDate()`, which is called in retry loops (up to 10x per sync).

**Problem:** The W5100 Ethernet chip has only 4 sockets, shared between the web server and UDP. `getTimeAndDate()` calls `Udp.begin(localPort)` on every attempt and never `Udp.stop()`. Recent Ethernet library versions close the previous socket inside `begin()`, but older ones (and some clones' libraries) leak a socket per call — after a few syncs the web server can no longer accept connections. Even on new libraries, holding a UDP socket open permanently wastes 1 of 4 sockets.

**AI fix prompt:**
> In `projetMumLib.ino` of the ArduiMum project, `getTimeAndDate()` calls `Udp.begin(localPort)` on every NTP attempt (retried up to 10x) and never calls `Udp.stop()`, risking W5100 socket exhaustion on older Ethernet library versions and permanently occupying one of the four hardware sockets. Fix: call `Udp.begin(localPort)` once, send the NTP packet, wait for the reply, and call `Udp.stop()` before returning (both success and failure paths), so the socket is only held during a sync. Also replace the fixed `delay(1000)` wait with a short polling loop on `Udp.parsePacket()` with a 1500 ms deadline for faster syncs.

---

## 13. MEDIUM — Blocking architecture: animations and delays make the web server deaf for up to a second at a time

**Where:** `projetMum.ino` `loop()` (sprinkled `delay(1)`s, `delay(1000)` when screen off), `regulTempWeath()` (`delay(varSpeedWeather)` = 700 ms default, plus `delay(100)`), `displayMsg()` (`delay(varSpeedMsg)`), `onBoot()` (blocking boot animation).

**Problem:** The W5100 buffers a small backlog, but while the sketch sits in `delay(700)` for the weather animation (or `delay(1000)` with the screen off), incoming API pushes wait and can time out on the sender's side. The whole sketch is a chain of blocking waits; every feature added makes the timing worse. The idiomatic fix is a `millis()`-based scheduler — the code already does this correctly in `busHandler()`/`tempWeathHandler()`, so the pattern just needs to be applied consistently.

**AI fix prompt:**
> Refactor the main loop of the ArduiMum Arduino project (`projetMum.ino` and the display/regulation functions in `projetMumLib.ino`) from blocking `delay()`-based pacing to a non-blocking `millis()` scheduler, following the pattern already used in `busHandler()`/`tempWeathHandler()`. Specifically: (1) remove the scattered `delay(1)` calls in `loop()`; (2) replace `delay(varSpeedWeather)` in `regulTempWeath()` and `delay(varSpeedMsg)` in `displayMsg()` with "run this step when `millis() - last >= interval`" state variables; (3) when the screen is off, keep servicing `serverCallback()` every iteration instead of `delay(1000)`; (4) keep frame state (animation cursor already exists as `thCursor`, scroll position as `textX`) in globals as now. The goal: `serverCallback()` runs every loop iteration with no gap longer than ~50 ms.

---

## 14. MEDIUM — DST logic is wrong at the edges and only runs at boot

**Where:** `projetMumLib.ino:115-124` (`adjustJetLag()`), manual override endpoints `summerTime`/`winterTime`.

**Problem:** European summer time runs from the **last Sunday of March** to the **last Sunday of October**. `adjustJetLag()` applies +1 h for months April–October, so it's wrong for late March (shows winter time during summer time) and for late October (shows summer time after the change) — and since it only runs in `setup()`, a device that stays up across a transition never adjusts; someone has to remember to hit the web page. The clock is the device's core feature; DST should be automatic and exact.

**AI fix prompt:**
> In `projetMumLib.ino` of the ArduiMum project, `adjustJetLag()` approximates EU daylight-saving time as "months 4–10" and only runs at boot. Replace it with an exact EU DST rule evaluated continuously: summer time is from 01:00 UTC on the last Sunday of March to 01:00 UTC on the last Sunday of October. Implement `bool isEUSummerTime(time_t utc)` using the TimeLib `year()/month()/day()/weekday()` helpers (last Sunday of month M = 31 minus ((weekday of 31st + offset) mod 7) — derive carefully and test for 2025–2030), keep NTP time internally as UTC (drop `timeZoneOffset` from the epoch computation in `getTimeAndDate()`), and compute display time as UTC + 1 h + (isEUSummerTime ? 1 h : 0) each time it's shown. Check the transition once per minute in `loop()`. Keep the manual `summerTime`/`winterTime` HTTP endpoints as a temporary override flag. Update `summerJetLag` reporting in the `whatTime` endpoint accordingly.

---

## 15. MEDIUM — Fragile substring routing: any request containing a keyword matches, order-dependently

**Where:** `busAPI()` in `projetMumLib.ino:166-371` — the whole `if/else if` chain of `HTTP_req.indexOf(...)`.

**Problem:** Routing is "does the raw request contain this substring anywhere":
- A displayed message containing the word "screen", "rain", "temp" or "bus" is hijacked by an earlier branch (`screen` is checked *before* `msg`), so `/?msg=Rendez-vous+screen+9h` turns the display off instead of showing the message.
- A browser's automatic `GET /favicon.ico` request falls to the fallback and gets "Go to /admin", fine — but `GET /?msg=nightclub` triggers the easter egg (finding #3) because `nightclub` matches anywhere.
- Matching includes HTTP headers if they arrive within the first 100 chars.

**AI fix prompt:**
> In `projetMumLib.ino` of the ArduiMum Arduino project, `busAPI()` routes requests by `HTTP_req.indexOf("keyword")` over the raw request, so keywords inside a message's text (e.g. `/?msg=bus+screen`) trigger the wrong branch, and match order is load-bearing. Refactor the routing: first extract only the request path+query (the token between `GET ` and ` HTTP`), then match parameters precisely — a parameter matches only as `?name=` / `&name=` / path prefix `/name`. Write small helpers `bool hasParam(const char* req, const char* name)` and `getParamValue(...)` and rewrite each branch to use them, preserving the existing endpoint names so current API clients keep working. Pay attention to `msg` extraction so its value is opaque and never re-inspected by other branches.

---

## 16. LOW — NTP failure leaves the clock at 1970 with no indication; missing return value; 2036 rollover

**Where:** `projetMumLib.ino:52-57` (boot sync gives up after 10 tries), `:60-77` (`getTimeAndDate`), `:80-94` (`sendNTPpacket` declared `unsigned long` but returns nothing — undefined behaviour formally).

**Problem:** If all 10 boot NTP attempts fail, the sketch continues and cheerfully displays 01:00 (epoch zero + offset) as if it were the real time, with no visual hint. `sendNTPpacket` is declared to return `unsigned long` but has no `return` statement. And `epoch = highWord << 16 | lowWord` interprets the NTP era-0 timestamp, which rolls over on **7 Feb 2036** — worth a comment for a device intended to run for years.

**AI fix prompt:**
> In `projetMumLib.ino` of the ArduiMum project: (1) `sendNTPpacket()` is declared `unsigned long` but never returns a value — change its return type to `void`; (2) when the boot-time NTP sync fails after all retries in `chooseServerNTP()`, the device displays epoch-1970 time as if valid — add a `bool timeValid` flag, only render the clock when it's true, otherwise show `--:--` on the matrix and keep retrying every 5 minutes from `loop()`; (3) add a comment noting the NTP era-0 rollover in February 2036 at the `epoch = highWord << 16 | lowWord` computation.

---

## 17. LOW — Malformed JSON responses

**Where:** `projetMumLib.ino:304` (`{"msg" : <unquoted text>}`), plus inconsistent spacing elsewhere.

**Problem:** `get_msg` embeds the message unquoted and unescaped — `{"msg" : hello world}` is not JSON, and a message containing `"` or `}` breaks any parser. The other endpoints are valid but hand-rolled inconsistently.

**AI fix prompt:**
> In `projetMumLib.ino` of the ArduiMum Arduino project, the `get_msg` HTTP endpoint prints `{"msg" : ` + the raw message + `}` — unquoted and unescaped, so it's invalid JSON. Fix it to print the message as a proper JSON string: surround with double quotes and escape `"` and `\` characters (and control chars as `\n` etc.) while streaming with `client.print`. Review the other JSON endpoints (`busSpeed`, `varSpeedWeather`, `dynWeatherState`, `screenState`, `varSpeedMsg`, `jet_lag`) for consistent formatting.

---

## 18. LOW — Maintainability: PROGMEM chunk tables, duplicated display code, magic numbers

**Where:** `config.h:150-347` (179 hand-numbered 20-char HTML fragments), `displayScheduleC3()`/`displaySchedule9()` (identical except the array), `displayClearAO`/`displaySunnyCloudsAO` (hundreds of hand-placed `drawPixel` calls duplicated per animation frame), `for(i; i<37; i++)` no-op expressions, index-based `rec_str[46]` lookups with the meaning only in comments.

**Problem:** Not bugs, but this is where future bugs come from. The `s_r_*` table exists only because `F()` wasn't used — `client.print(F("..."))` stores the string in flash with zero SRAM cost and no shared buffer (which would also eliminate finding #1 for the HTML path). The two bus-schedule functions and the smiley/weather art beg for parameterisation and bitmap tables (`drawBitmap` with PROGMEM bitmaps).

**AI fix prompt:**
> Refactor the ArduiMum Arduino project for maintainability without changing behaviour: (1) in `config.h`/`projetMumLib.ino`, delete the 179-entry `s_r_*`/`server_response[]` PROGMEM fragment table and serve the `/admin` page with direct `client.print(F("..."))` calls (multi-line, readable HTML/JS); (2) merge `displayScheduleC3()` and `displaySchedule9()` into one `displaySchedule(String bus[2])`; (3) convert the pixel-art weather icons (`nuage`, `flocon`, sun frames in `displayClearAO`, `displaySunnyCloudsAO`) into PROGMEM bitmap arrays drawn with `Adafruit_GFX::drawBitmap`, one bitmap per animation frame; (4) replace index-based `rec_str[N]` lookups with named `const char PROGMEM` symbols or `F()`; (5) clean up no-op loop expressions like `for(i; i<37; i++)`. Compile for Arduino Mega 2560 after each step and keep flash/SRAM usage equal or lower.

---

## 19. LOW — Housekeeping: README/library drift, license, dead code

**Where:** repo root and `projetMum/`.

**Problem:**
- README says "install libraries through Arduino IDE" but the repo vendors specific library copies in `libraries/` — installing upstream versions may behave differently (the vendored RGBmatrixPanel matters for pin mapping). Say explicitly which to use.
- "Licence: Copyright" isn't a license; for a public GitHub repo, pick one (or state "all rights reserved" deliberately).
- Dead/commented code: the trial block in `setup()`, `displayTest()`, the commented `hex2rgb` body, old pin-mapping blocks in `config.h`, and a stray non-functional comment (`projetMumLib.ino:31` "does not work - no ref to 'matrix'?" — it does work; the comment is stale).
- `config.h` defines variables (not just declarations) in a header — fine for a single-`.ino` build, but rename to `config.h`-included-once or document it to avoid surprises if the project ever splits into `.cpp` files.

**AI fix prompt:**
> Housekeeping pass on the ArduiMum repo: (1) update `README.md` to state that the vendored libraries in `libraries/` are the tested versions and should be used instead of (or pinned to match) the Arduino IDE library-manager versions; (2) add a proper `LICENSE` file (ask the owner which — otherwise use "All rights reserved" explicitly); (3) delete dead code: the trial block in `projetMum.ino` `setup()`, `displayTest()`, commented-out pin-mapping variants in `config.h`, the commented body of `hex2rgb`, and stale comments; (4) add a note at the top of `config.h` explaining it defines globals and must only be included once from the main sketch.

---

## Summary table

| # | Severity | Issue | Category |
|---|----------|-------|----------|
| 1 | Critical | `strcpy_P` overflow of 25-byte shared buffer | Memory corruption |
| 2 | Critical | No timeout in `busServer` — silent client hangs device | Reliability / DoS |
| 3 | Critical | `nightClub()` infinite strobe loop, network-triggerable | Reliability / DoS / safety |
| 4 | Critical | `for(;;)` on DHCP failure, no watchdog | Reliability |
| 5 | High | Unsequenced double `strcpy_P` in `msg.replace` — URL decode broken | Correctness / UB |
| 6 | High | NTP re-sync never called from `loop()` — clock drifts | Correctness |
| 7 | High | Unvalidated inputs → `delay()` up to 65 s (`msg_speed`, `varSpeedWeather`, `dimmer`) | Input validation |
| 8 | High | HTTP status line commented out — malformed responses | Correctness |
| 9 | High | `String` heap fragmentation on 8 KB SRAM | Memory management |
| 10 | Medium | No auth; GET-based state changes (LAN CSRF) | Security |
| 11 | Medium | Static IP set then discarded for DHCP | Architecture |
| 12 | Medium | `Udp.begin()` per sync, never `Udp.stop()` | Resource leak |
| 13 | Medium | Blocking `delay()` architecture — server deaf up to ~1 s | Architecture / efficiency |
| 14 | Medium | DST rule wrong at edges, boot-only | Correctness |
| 15 | Medium | Substring-anywhere routing, order-dependent | Robustness |
| 16 | Low | 1970 clock on NTP failure; missing return; 2036 rollover | Correctness |
| 17 | Low | Malformed JSON (`get_msg`) | Correctness |
| 18 | Low | PROGMEM chunk tables, duplicated display code | Maintainability |
| 19 | Low | README/library drift, license, dead code | Housekeeping |

**Suggested order of work:** #1–#4 first (they cover every "the display froze and I had to unplug it" scenario), then #6 and #7 (silent wrong time / freezes), then #5 and #8. Findings #9, #13 and #18 pay off best done together as one refactor (fixed buffers + non-blocking loop + `F()` macros), since they touch the same lines.
