# CAN Upgrade target protocol v5 - RL78/F13 implementation

This document defines the target-side CAN Upgrade v5 contract implemented by
the RL78/F13 bootloader and application. The image and device metadata carry
the MCU and flash layout; the PC tool does not assume a 120-KiB image or an
application start of `0x2000`.

## Current F13 build values

| Item | Current value |
|---|---:|
| MCU family / part | `RL78/F13` / `R5F10BMG` |
| Code flash | `0x20000` bytes |
| Boot reserved region | `0x2000` bytes |
| Application start | `0x2000` |
| Application capacity | `0x1E000` bytes |
| Erase block | `0x400` bytes |
| CAN | Classical CAN 500 kbit/s |
| Request / response ID | `0x700` / `0x701`, standard data frame |
| Integrity | CRC32/ISO-HDLC in the final four image bytes |

These are metadata values for this build, not global PC-tool limits. A future
target may use a different aligned start, capacity, flash size, CAN ID, or MCU
identity while retaining the v5 contract and compatible IRQ ABI.

## Commands

All multi-byte integers are little-endian.

| Command | Request | Response |
|---|---|---|
| `HELLO 0x01` | `[cmd]` | `[cmd,status,protocol,format,target16,capacityKiB16]` |
| `START 0x02` | `[cmd,protocol,target16,imageSize32]` | common ACK |
| `FORCE_START 0x0E` | same fields as START; request-local version override | common ACK; all other validation unchanged |
| `BLOCK_BEGIN 0x0B` | `[cmd,imageOffset24,blockCRC32]` | none |
| raw block segments | ID720..727, up to8 image bytes | none |
| `BLOCK_COMMIT 0x0C` | `[cmd,imageOffset24,blockCRC32]` | common ACK |
| `END 0x04` | `[cmd]` | common ACK |
| `ABORT 0x05` | `[cmd]` | common ACK |
| `ENTER 0x06` | `[cmd]` | `[cmd,status]` |
| `INFO 0x07` | `[cmd,page]` | `[cmd,status,page,state,value32]` |
| `METADATA 0x08` | `[cmd,offset16,00,data4]` | ACK next offset / total256 |
| `SEAL 0x09` | `[cmd,00,00,00,metadataCRC32]` | ACK offset256 / total256 |
| `METADATA_RESET 0x0A` | `[cmd]` | ACK offset0 / total256; no flash access |
| `VERIFY 0x0D` | `[cmd]`, Boot with no active update only | `[0D,status,00,00,calculatedCRC32]`; no erase/write |

INFO page1B is the App health word: byte0 READY(1), byte1 clock/CAN/timer bits0..2,
byte2 Boot-handoff/update bits0..1, byte3 RESF. READY is not merely INFO response.
INFO1C advertises bit0 health, bit1 Boot VERIFY, bit2 classified error status.
Boot reports7; App reports1. VERIFY range is App start + installed size from INFO.
Statuses0A metadata,0B policy/minimum Boot,0C IRQ ABI,0D layout distinguish
pre-Flash failures;09 remains other BAD_IMAGE cases. Wrong/missing metadata and
CRC failure never permit VERIFY to report OK or reach erase/write.
Status0E VERSION_DOWNGRADE rejects an ordinary START before Flash access;
ACK value is the installed App version. Same/newer versions remain allowed.

The common ACK is `[cmd,status,detail16,value32]`.

- `START`: `detail=0`, `value=application start`.
- `BLOCK_COMMIT`: `detail=(image offset / 64) & 0xFFFF`, `value=next absolute address`.
- `END`: `detail=(image size / 4) & 0xFFFF`, `value=calculated CRC32`.
- `ABORT`: reports the current update state.

Blocks use24-bit byte offsets, aligned64. Actual length is min(64,size-offset),
aligned4. Indexed standard IDs720..727 carry8 raw image bytes each, with DLC4
only for the final short segment. No legacy DATA03 fallback exists. BEGIN and
COMMIT carry the same block CRC32; only COMMIT is acknowledged after all data,
CRC, metadata overlap and Flash programming succeed. A repeated last committed
block is checked against Flash CRC and ACKed without erase/write. Missing,
conflicting or corrupt segments cannot reach Flash. Other offsets are rejected.
INFO1A declares the data ID window; it must not overlap control/response IDs.

## INFO metadata pages

Page `0x00` packs `target ID`, `protocol version`, and `image format`. All
other pages return one 32-bit value. String pages contain four little-endian
ASCII bytes and are concatenated by the tool.

| Page | Value |
|---:|---|
| `00` | target / protocol / format |
| `01`, `02` | boot semantic version / build |
| `03`, `04` | installed app semantic version / build, or `0xFFFFFFFF` |
| `05`, `06` | application start / capacity |
| `07`, `08`, `09` | code-flash size / boot reserved size / erase block |
| `0A` | CAN mode, ID format, checksum type, protocol ID |
| `0B`, `0C` | nominal / data bitrate masks |
| `0D`, `0E` | request / response CAN ID |
| `0F..12` | MCU part ASCII |
| `13..15` | MCU family ASCII |
| `16`, `17` | installed app image size / minimum supported image size |
| `18`, `19` | policy version/size/role/live transport mask / IRQ ABI |
| `1A` | high16=raw data base ID; low16=block bytes; current07200040h |
| `1D` | installed version policy:0=no valid App,1=valid/unrestricted,3=valid/same-or-newer |

The application answers INFO and ENTER. The bootloader answers INFO, HELLO,
and the update commands. Reading INFO is non-destructive.

## Image metadata format6 (CAN transport v5)

The metadata region starts at image offset `0x100` and is256 bytes:128-byte
defined core followed by128 reserved0xFF bytes. Code still begins at offset
`0x200`; the8-bit core-length field is128, not256. The actual update
BIN begins at its metadata-defined application start. The last four BIN bytes
store CRC32; `image size` is the exact BIN length, not the application
capacity.

| Offset | Size | Field |
|---:|---:|---|
| `00` | 4 | magic `CAN\0` (`0x004E4143`) |
| `04` | 1 | image format `6` (fixed normal RETI IRQ ABI plus common policy) |
| `05` | 1 | manifest length `128` |
| `06` | 2 | required flags `0x0003` |
| `08` | 2 | protocol ID `1` |
| `0A` | 1 | protocol version `5` |
| `0B` | 1 | checksum type `1` (CRC32) |
| `0C` | 2 | target ID |
| `0E` | 1 | supported CAN-mode mask |
| `0F` | 1 | CAN ID format (`0` standard, `1` extended) |
| `10`, `14` | 4 each | app semantic version / build |
| `18` | 4 | exact image size |
| `1C`, `20` | 4 each | application start / capacity |
| `24`, `28`, `2C` | 4 each | code flash / boot reserve / erase block |
| `30`, `34` | 4 each | nominal / data bitrate masks |
| `38`, `3C` | 4 each | request / response CAN ID |
| `40` | 16 | zero-terminated MCU part ASCII |
| `50` | 12 | zero-terminated MCU family ASCII |
| `5C` | 4 | minimum image size |
| `60..6F` |16 | UPD1:version3,size32,DEVICE_APP,CAN,F1V1 IRQ ABI,minimum Boot |
| `70..73` |4 | CAN profile:zero (device UART baud) |
| `74..77` |4 | version_control:0 any version(default),1 same/newer only |
| `78..7F` |8 | reserved zero |
| `80..FF` |128 | reserved extension, all0xFF |

Before START, the PC verifies the entire manifest, exact BIN length, non-erased
application entry, final CRC32, selected App Start override, and discovered
device metadata. The target independently validates START, manifest fields,
flash bounds, FSL results, received byte count, and CRC32 before booting the
application.

## Update and recovery flow

1. Configure the host from the loaded image metadata, or use the current v5
   bootstrap defaults (`0x700/0x701`, Classical 500K).
2. Read all INFO pages. If the target is running the application, send ENTER
   explicitly and rediscover it after reset.
3. Read HELLO and confirm the same target, protocol, format, and capacity.
4. Compare the loaded image metadata with the discovered device metadata.
5. Reset RAM metadata staging, send64 ordered METADATA chunks, then SEAL the
   complete256-byte region CRC. Device validates identity/layout/policy/ABI
   and minimum boot before START with the exact verified image length.
6. Require host feature bit12 / generic CAN_BURST_WAIT_RX22. Send one BEGIN,
   up to8 indexed raw segments and COMMIT in each host burst; await one ACK.
   Offset advances by the committed block length. Retry a whole block at most
   once for host TIMEOUT or target BAD_SEQUENCE/CRC_ERROR; never retry a Flash
   failure or accept a wrong ACK detail/next address. UART retries use the
   host replay cache; the bridge never interprets target commands/Flash.
7. Send END. Only a matching target-calculated CRC32 completes the update.
8. The target resets and enters the application only when its manifest and
   CRC32 remain valid.

Startup requires non-erased entry, matching header/protocol/target/layout/CAN,
image516..122880 aligned4, non-erased version, reservedFF and whole-image CRC.
Boot now also compares exact ASCII family/part and requires initialized build.
It enforces image minimum Boot. Ordinary START compares App versions only when
the currently valid installed image enables version control. An incoming lower
image cannot bypass that check by disabling its own policy. FORCE_START is a
single transaction, not a persistent unlock, and never bypasses other checks.
No valid App means recovery without a stored version floor.
ENTER is a one-shot RAM marker, not a physical P13.7 input. Only sessions
entered with an already valid app enable the30-second inactivity timer;
matching ID700 or720..727 standard data traffic refreshes it before command validation.
Before accepted START it can return the unchanged valid app. After accepted
START a timeout retains boot; ABORT/fatal errors disable automatic return.
Invalid-app recovery has no inactivity timeout, even after START. END success
resets and repeats startup validation. See the tool guide section2.2 for the
complete condition tables, CAN/FSL failure distinctions and diagrams.

Cancellation, disconnect, flash failure, exhausted block retry, or CRC
failure keeps the bootloader recoverable. The PC performs a bounded best-effort
ABORT and stops CAN. CRC32 plus target/layout identity protects integrity;
the optional version policy and explicit force operation are not authentication.
No WDT, digital signature or persistent version floor is provided.

Initial installation programs matching Boot/App images with a programmer.
Subsequent compatible App updates use CAN. The host must advertise generic
CAN burst capability; its own App maintenance uses Misc independently.
See the PC docs/CAN_BLOCK_TRANSFER_V5_zh-TW.md for the Host Link layout and
physical validation gates. Boot/App release numbers need not be equal.
